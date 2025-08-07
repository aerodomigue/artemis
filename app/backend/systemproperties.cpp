#include "systemproperties.h"
#include "utils.h"

#include <QGuiApplication>
#include <QLibraryInfo>
#include <QFile>
#include <QProcess>

#include "streaming/session.h"
#include "streaming/streamutils.h"

#ifdef Q_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

SystemProperties::SystemProperties()
{
    versionString = QString(VERSION_STR);
    hasDesktopEnvironment = WMUtils::isRunningDesktopEnvironment();
    isRunningWayland = WMUtils::isRunningWayland();
    isRunningXWayland = isRunningWayland && QGuiApplication::platformName() == "xcb";
    usesMaterial3Theme = QLibraryInfo::version() >= QVersionNumber(6, 5, 0);
    QString nativeArch = QSysInfo::currentCpuArchitecture();

#ifdef Q_OS_WIN32
    {
        USHORT processArch, machineArch;

        // Use IsWow64Process2 on TH2 and later, because it supports ARM64
        auto fnIsWow64Process2 = (decltype(IsWow64Process2)*)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process2");
        if (fnIsWow64Process2 != nullptr && fnIsWow64Process2(GetCurrentProcess(), &processArch, &machineArch)) {
            switch (machineArch) {
            case IMAGE_FILE_MACHINE_I386:
                nativeArch = "i386";
                break;
            case IMAGE_FILE_MACHINE_AMD64:
                nativeArch = "x86_64";
                break;
            case IMAGE_FILE_MACHINE_ARM64:
                nativeArch = "arm64";
                break;
            }
        }

        isWow64 = nativeArch != QSysInfo::buildCpuArchitecture();
    }
#else
    isWow64 = false;
#endif

    if (nativeArch == "i386") {
        friendlyNativeArchName = "x86";
    }
    else if (nativeArch == "x86_64") {
        friendlyNativeArchName = "x64";
    }
    else {
        friendlyNativeArchName = nativeArch.toUpper();
    }

    // Assume we can probably launch a browser if we're in a GUI environment
    hasBrowser = hasDesktopEnvironment;

#ifdef HAVE_DISCORD
    hasDiscordIntegration = true;
#else
    hasDiscordIntegration = false;
#endif

    unmappedGamepads = SdlInputHandler::getUnmappedGamepads();

    // Detect Steam Deck platform for automatic Vulkan renderer selection
    isSteamDeck = isSteamDeckOrGamescope();
    
    // Detect broader Vulkan HDR support for automatic renderer selection
    hasVulkanHdr = hasVulkanHdrSupport();

    // Populate data that requires talking to SDL. We do it all in one shot
    // and cache the results to speed up future queries on this data.
    querySdlVideoInfo();

    Q_ASSERT(!monitorRefreshRates.isEmpty());
    Q_ASSERT(!monitorNativeResolutions.isEmpty());
    Q_ASSERT(!monitorSafeAreaResolutions.isEmpty());
}

QRect SystemProperties::getNativeResolution(int displayIndex)
{
    // Returns default constructed QRect if out of bounds
    return monitorNativeResolutions.value(displayIndex);
}

QRect SystemProperties::getSafeAreaResolution(int displayIndex)
{
    // Returns default constructed QRect if out of bounds
    return monitorSafeAreaResolutions.value(displayIndex);
}

int SystemProperties::getRefreshRate(int displayIndex)
{
    // Returns 0 if out of bounds
    return monitorRefreshRates.value(displayIndex);
}

class QuerySdlVideoThread : public QThread
{
public:
    QuerySdlVideoThread(SystemProperties* me) :
        QThread(nullptr),
        m_Me(me) {}

    void run() override
    {
        m_Me->querySdlVideoInfoInternal();
    }

    SystemProperties* m_Me;
};

void SystemProperties::querySdlVideoInfo()
{
    if (WMUtils::isRunningX11() || WMUtils::isRunningWayland()) {
        // Use a separate thread to temporarily initialize SDL
        // video to avoid stomping on Qt's X11 and OGL state.
        QuerySdlVideoThread thread(this);
        thread.start();
        thread.wait();
    }
    else {
        querySdlVideoInfoInternal();
    }
}

void SystemProperties::querySdlVideoInfoInternal()
{
    hasHardwareAcceleration = false;

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SDL_InitSubSystem(SDL_INIT_VIDEO) failed: %s",
                     SDL_GetError());
        return;
    }

    // Update display related attributes (max FPS, native resolution, etc).
    // We call the internal variant because we're already in a safe thread context.
    refreshDisplaysInternal();

    SDL_Window* testWindow = SDL_CreateWindow("", 0, 0, 1280, 720,
                                              SDL_WINDOW_HIDDEN | StreamUtils::getPlatformWindowFlags());
    if (!testWindow) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Failed to create test window with platform flags: %s",
                    SDL_GetError());

        testWindow = SDL_CreateWindow("", 0, 0, 1280, 720, SDL_WINDOW_HIDDEN);
        if (!testWindow) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Failed to create window for hardware decode test: %s",
                         SDL_GetError());
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            return;
        }
    }

    Session::getDecoderInfo(testWindow, hasHardwareAcceleration, rendererAlwaysFullScreen, supportsHdr, maximumResolution);

    // Allow environment variable override for HDR support (for testing and debugging)
    if (qgetenv("FORCE_HDR_SUPPORT") == "1") {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "SystemProperties: Forcing HDR support via FORCE_HDR_SUPPORT environment variable (was: %s)",
                    supportsHdr ? "ENABLED" : "DISABLED");
        supportsHdr = true;
    }
    
    // Allow environment variable override for hardware acceleration (for Flatpak and testing)
    if (qgetenv("ARTEMIS_FORCE_HW_ACCEL") == "1") {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "SystemProperties: Forcing hardware acceleration via ARTEMIS_FORCE_HW_ACCEL environment variable (was: %s)",
                    hasHardwareAcceleration ? "ENABLED" : "DISABLED");
        hasHardwareAcceleration = true;
    }
    
    // Additional debug logging for HDR capability investigation
    if (qgetenv("HDR_DEBUG") == "1") {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "SystemProperties HDR Debug: Hardware acceleration=%s, Renderer fullscreen=%s, HDR support=%s",
                    hasHardwareAcceleration ? "YES" : "NO",
                    rendererAlwaysFullScreen ? "YES" : "NO", 
                    supportsHdr ? "YES" : "NO");
        
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "SystemProperties HDR Debug: Steam Deck detection=%s, Vulkan HDR support=%s",
                    SystemProperties::isSteamDeckOrGamescope() ? "YES" : "NO",
                    SystemProperties::hasVulkanHdrSupport() ? "YES" : "NO");
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "SystemProperties: Final HDR support status: %s", 
                supportsHdr ? "ENABLED" : "DISABLED");

    SDL_DestroyWindow(testWindow);

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

class RefreshDisplaysThread : public QThread
{
public:
    RefreshDisplaysThread(SystemProperties* me) :
        QThread(nullptr),
        m_Me(me) {}

    void run() override
    {
        m_Me->refreshDisplaysInternal();
    }

    SystemProperties* m_Me;
};

void SystemProperties::refreshDisplays()
{
    if (WMUtils::isRunningX11() || WMUtils::isRunningWayland()) {
        // Use a separate thread to temporarily initialize SDL
        // video to avoid stomping on Qt's X11 and OGL state.
        RefreshDisplaysThread thread(this);
        thread.start();
        thread.wait();
    }
    else {
        refreshDisplaysInternal();
    }
}

void SystemProperties::refreshDisplaysInternal()
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "SDL_InitSubSystem(SDL_INIT_VIDEO) failed: %s",
                     SDL_GetError());
        return;
    }

    monitorNativeResolutions.clear();

    SDL_DisplayMode bestMode;
    for (int displayIndex = 0; displayIndex < SDL_GetNumVideoDisplays(); displayIndex++) {
        SDL_DisplayMode desktopMode;
        SDL_Rect safeArea;

        if (StreamUtils::getNativeDesktopMode(displayIndex, &desktopMode, &safeArea)) {
            if (desktopMode.w <= 8192 && desktopMode.h <= 8192) {
                monitorNativeResolutions.insert(displayIndex, QRect(0, 0, desktopMode.w, desktopMode.h));
                monitorSafeAreaResolutions.insert(displayIndex, QRect(0, 0, safeArea.w, safeArea.h));
            }
            else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Skipping resolution over 8K: %dx%d",
                            desktopMode.w, desktopMode.h);
            }

            // Start at desktop mode and work our way up
            bestMode = desktopMode;
            for (int i = 0; i < SDL_GetNumDisplayModes(displayIndex); i++) {
                SDL_DisplayMode mode;
                if (SDL_GetDisplayMode(displayIndex, i, &mode) == 0) {
                    if (mode.w == desktopMode.w && mode.h == desktopMode.h) {
                        if (mode.refresh_rate > bestMode.refresh_rate) {
                            bestMode = mode;
                        }
                    }
                }
            }

            // Try to normalize values around our our standard refresh rates.
            // Some displays/OSes report values that are slightly off.
            if (bestMode.refresh_rate >= 58 && bestMode.refresh_rate <= 62) {
                monitorRefreshRates.append(60);
            }
            else if (bestMode.refresh_rate >= 28 && bestMode.refresh_rate <= 32) {
                monitorRefreshRates.append(30);
            }
            else {
                monitorRefreshRates.append(bestMode.refresh_rate);
            }
        }
    }
}

bool SystemProperties::isSteamDeckOrGamescope()
{
    bool hdrDebug = qgetenv("HDR_DEBUG") == "1";
    
    if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Steam Deck Detection: Starting detection analysis");
    }
    
    // Check for Gamescope environment (Steam Deck's compositor)
    QString gamescapeDisplay = qgetenv("GAMESCOPE_WAYLAND_DISPLAY");
    if (!gamescapeDisplay.isEmpty()) {
        if (hdrDebug) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Steam Deck Detection: GAMESCOPE_WAYLAND_DISPLAY found ('%s'), returning true",
                        gamescapeDisplay.toUtf8().constData());
        }
        return true;
    }
    
    // Check for explicit Steam Deck environment variable
    QString underSteamDeck = qgetenv("UNDER_STEAM_DECK");
    if (!underSteamDeck.isEmpty()) {
        if (hdrDebug) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Steam Deck Detection: UNDER_STEAM_DECK found ('%s'), returning true",
                        underSteamDeck.toUtf8().constData());
        }
        return true;
    }
    
    // Check for Gamescope session type
    QString sessionType = qgetenv("XDG_SESSION_TYPE");
    QString waylandDisplay = qgetenv("WAYLAND_DISPLAY");
    if (sessionType == "wayland" && !waylandDisplay.isEmpty()) {
        bool isGamescope = waylandDisplay.contains("gamescope", Qt::CaseInsensitive);
        if (hdrDebug) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Steam Deck Detection: Wayland session check (XDG_SESSION_TYPE='%s', WAYLAND_DISPLAY='%s', contains gamescope=%s)",
                        sessionType.toUtf8().constData(),
                        waylandDisplay.toUtf8().constData(),
                        isGamescope ? "YES" : "NO");
        }
        if (isGamescope) {
            return true;
        }
    } else if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Steam Deck Detection: Not Wayland or no display (XDG_SESSION_TYPE='%s', WAYLAND_DISPLAY='%s')",
                    sessionType.toUtf8().constData(),
                    waylandDisplay.toUtf8().constData());
    }
    
#ifdef Q_OS_LINUX
    // Check hardware detection - Steam Deck product name
    QFile productFile("/sys/devices/virtual/dmi/id/product_name");
    if (productFile.exists() && productFile.open(QIODevice::ReadOnly)) {
        QString product = productFile.readAll().trimmed();
        bool isSteamDeckProduct = product.contains("Steam Deck", Qt::CaseInsensitive);
        
        if (hdrDebug) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Steam Deck Detection: DMI product name check ('%s', contains 'Steam Deck'=%s)",
                        product.toUtf8().constData(),
                        isSteamDeckProduct ? "YES" : "NO");
        }
        
        if (isSteamDeckProduct) {
            productFile.close();
            return true;
        }
        productFile.close();
    } else if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Steam Deck Detection: DMI product name file not accessible (/sys/devices/virtual/dmi/id/product_name)");
    }
    
    // Check for Steam Deck CPU model
    QFile cpuFile("/proc/cpuinfo");
    if (cpuFile.exists() && cpuFile.open(QIODevice::ReadOnly)) {
        QString cpuInfo = cpuFile.readAll();
        cpuFile.close();
        
        // Steam Deck uses AMD Custom APU 0932 (Vangogh/Aerith)
        bool hasCustomApu = cpuInfo.contains("Custom APU 0932", Qt::CaseInsensitive);
        bool hasVangogh = cpuInfo.contains("Vangogh", Qt::CaseInsensitive);
        bool hasAerith = cpuInfo.contains("Aerith", Qt::CaseInsensitive);
        
        bool isSteamDeckCpu = hasCustomApu || hasVangogh || hasAerith;
        
        if (hdrDebug) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Steam Deck Detection: CPU model check (Custom APU 0932=%s, Vangogh=%s, Aerith=%s, Steam Deck CPU=%s)",
                        hasCustomApu ? "YES" : "NO",
                        hasVangogh ? "YES" : "NO", 
                        hasAerith ? "YES" : "NO",
                        isSteamDeckCpu ? "YES" : "NO");
        }
        
        if (isSteamDeckCpu) {
            return true;
        }
    } else if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Steam Deck Detection: CPU info file not accessible (/proc/cpuinfo)");
    }
#else
    if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Steam Deck Detection: Non-Linux platform, skipping hardware checks");
    }
#endif
    
    if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Steam Deck Detection: No Steam Deck indicators found, returning false");
    }
    
    return false;
}

bool SystemProperties::hasVulkanHdrSupport()
{
    bool hdrDebug = qgetenv("HDR_DEBUG") == "1";
    
    if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Vulkan HDR Detection: Starting environment analysis");
    }
    
    // First check if we're in an HDR-capable environment
    bool hasHdrEnvironment = false;
    
    // Check for Gamescope (Steam Deck, other handheld devices)
    QString gamescapeDisplay = qgetenv("GAMESCOPE_WAYLAND_DISPLAY");
    QString waylandDisplay = qgetenv("WAYLAND_DISPLAY");
    bool isGamescope = !gamescapeDisplay.isEmpty() || 
                      (!waylandDisplay.isEmpty() && 
                       waylandDisplay.contains("gamescope", Qt::CaseInsensitive));
    
    if (isGamescope) {
        hasHdrEnvironment = true;
        if (hdrDebug) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Vulkan HDR Detection: Gamescope detected (GAMESCOPE_WAYLAND_DISPLAY='%s', WAYLAND_DISPLAY='%s')",
                        gamescapeDisplay.toUtf8().constData(),
                        waylandDisplay.toUtf8().constData());
        }
    }
    
    // Check for HDR-capable Wayland compositors
    QString sessionType = qgetenv("XDG_SESSION_TYPE");
    if (sessionType == "wayland") {
        if (hdrDebug) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Vulkan HDR Detection: Wayland session detected (XDG_SESSION_TYPE='%s')",
                        sessionType.toUtf8().constData());
        }
        
        // Check for KDE Plasma Wayland HDR support (6.1+)
        QString kdeSession = qgetenv("KDE_SESSION_VERSION");
        QString plasma = qgetenv("PLASMA_DESKTOP_SESSION");
        if (!kdeSession.isEmpty() || !plasma.isEmpty()) {
            hasHdrEnvironment = true; // Assume recent KDE has HDR support
            if (hdrDebug) {
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                            "Vulkan HDR Detection: KDE Plasma detected (KDE_SESSION_VERSION='%s', PLASMA_DESKTOP_SESSION='%s')",
                            kdeSession.toUtf8().constData(),
                            plasma.toUtf8().constData());
            }
        }
        
        // Check for GNOME/Mutter HDR support
        QString gnomeSession = qgetenv("GNOME_DESKTOP_SESSION_ID");
        QString gdmSession = qgetenv("GDMSESSION");
        if (!gnomeSession.isEmpty() || gdmSession == "gnome" || gdmSession == "gnome-wayland") {
            hasHdrEnvironment = true; // Recent GNOME versions support HDR
            if (hdrDebug) {
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                            "Vulkan HDR Detection: GNOME detected (GNOME_DESKTOP_SESSION_ID='%s', GDMSESSION='%s')",
                            gnomeSession.toUtf8().constData(),
                            gdmSession.toUtf8().constData());
            }
        }
        
        // Check for wlroots-based compositors with HDR
        QString xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP");
        QString swaysock = qgetenv("SWAYSOCK");
        if (xdgCurrentDesktop.contains("sway", Qt::CaseInsensitive) ||
            xdgCurrentDesktop.contains("hyprland", Qt::CaseInsensitive) ||
            !swaysock.isEmpty()) {
            hasHdrEnvironment = true;
            if (hdrDebug) {
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                            "Vulkan HDR Detection: wlroots compositor detected (XDG_CURRENT_DESKTOP='%s', SWAYSOCK='%s')",
                            xdgCurrentDesktop.toUtf8().constData(),
                            swaysock.isEmpty() ? "empty" : "set");
            }
        }
    }
    else if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Vulkan HDR Detection: Not a Wayland session (XDG_SESSION_TYPE='%s')",
                    sessionType.toUtf8().constData());
    }
    
    // If no HDR environment detected, don't prefer Vulkan
    if (!hasHdrEnvironment) {
        if (hdrDebug) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Vulkan HDR Detection: No HDR-capable environment detected, returning false");
        }
        return false;
    }

    if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Vulkan HDR Detection: HDR-capable environment found, checking Vulkan drivers...");
    }

#ifdef Q_OS_LINUX
    // Check if Vulkan is available on the system
    // Try to find vulkan loader library
    QStringList vulkanPaths = {
        "/usr/lib/x86_64-linux-gnu/libvulkan.so.1",
        "/usr/lib64/libvulkan.so.1", 
        "/usr/lib/libvulkan.so.1",
        "/lib/x86_64-linux-gnu/libvulkan.so.1",
        "/lib64/libvulkan.so.1"
    };
    
    bool hasVulkanLibrary = false;
    QString foundVulkanPath;
    for (const QString& path : vulkanPaths) {
        if (QFile::exists(path)) {
            hasVulkanLibrary = true;
            foundVulkanPath = path;
            break;
        }
    }
    
    if (hdrDebug) {
        if (hasVulkanLibrary) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Vulkan HDR Detection: Vulkan library found at '%s'",
                        foundVulkanPath.toUtf8().constData());
        } else {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Vulkan HDR Detection: No Vulkan library found in standard paths");
            for (const QString& path : vulkanPaths) {
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                            "  Checked: %s", path.toUtf8().constData());
            }
        }
    }
    
    if (!hasVulkanLibrary) {
        if (hdrDebug) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Vulkan HDR Detection: No Vulkan library found, returning false");
        }
        return false;
    }
    
    // Check for Mesa drivers that support Vulkan HDR
    QFile gpuFile("/proc/driver/nvidia/version");
    bool hasNvidia = gpuFile.exists();
    
    if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Vulkan HDR Detection: NVIDIA driver detection: %s (/proc/driver/nvidia/version %s)",
                    hasNvidia ? "YES" : "NO",
                    hasNvidia ? "exists" : "not found");
    }
    
    // For NVIDIA, check if we have recent enough drivers (545.29.06+)
    if (hasNvidia) {
        if (hdrDebug) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Vulkan HDR Detection: NVIDIA detected, assuming Vulkan HDR support, returning true");
        }
        return true;
    }
    
    // For AMD/Intel, check for Mesa and radv/anv drivers
    if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Vulkan HDR Detection: Running vulkaninfo to detect GPU drivers...");
    }
    
    QProcess vulkanInfo;
    vulkanInfo.start("vulkaninfo", QStringList() << "--summary");
    bool vulkanInfoFinished = vulkanInfo.waitForFinished(3000);
    
    if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Vulkan HDR Detection: vulkaninfo process finished=%s, exit code=%d",
                    vulkanInfoFinished ? "YES" : "NO",
                    vulkanInfo.exitCode());
    }
    
    if (vulkanInfoFinished && vulkanInfo.exitCode() == 0) {
        QString output = vulkanInfo.readAllStandardOutput();
        
        if (hdrDebug) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Vulkan HDR Detection: vulkaninfo output length: %d characters",
                        output.length());
        }
        
        // Check for modern GPU drivers that support HDR
        bool hasRadv = output.contains("radv", Qt::CaseInsensitive);        // AMD RADV
        bool hasAnv = output.contains("anv", Qt::CaseInsensitive);          // Intel ANV 
        bool hasNvidiaVk = output.contains("nvidia", Qt::CaseInsensitive);  // NVIDIA
        bool hasAdreno = output.contains("adreno", Qt::CaseInsensitive);    // Qualcomm Adreno
        
        bool hasHdrCapableDriver = hasRadv || hasAnv || hasNvidiaVk || hasAdreno;
        
        if (hdrDebug) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Vulkan HDR Detection: Driver detection results:");
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "  RADV (AMD): %s", hasRadv ? "YES" : "NO");
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "  ANV (Intel): %s", hasAnv ? "YES" : "NO");
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "  NVIDIA: %s", hasNvidiaVk ? "YES" : "NO");
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "  Adreno: %s", hasAdreno ? "YES" : "NO");
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "  HDR-capable driver found: %s", hasHdrCapableDriver ? "YES" : "NO");
        }
        
        return hasHdrCapableDriver;
    }
    
    if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Vulkan HDR Detection: vulkaninfo failed or timed out, returning false");
    }
#else
    if (hdrDebug) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Vulkan HDR Detection: Non-Linux platform, assuming Vulkan available, returning true");
    }
    return true; // Assume other platforms have Vulkan support
#endif
    
    return false;
}
