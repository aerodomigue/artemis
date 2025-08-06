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

    // Allow environment variable override for HDR support (for testing)
    if (qgetenv("FORCE_HDR_SUPPORT") == "1") {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Forcing HDR support via FORCE_HDR_SUPPORT environment variable");
        supportsHdr = true;
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
    // Check for Gamescope environment (Steam Deck's compositor)
    if (!qgetenv("GAMESCOPE_WAYLAND_DISPLAY").isEmpty()) {
        return true;
    }
    
    // Check for explicit Steam Deck environment variable
    if (!qgetenv("UNDER_STEAM_DECK").isEmpty()) {
        return true;
    }
    
    // Check for Gamescope session type
    if (qgetenv("XDG_SESSION_TYPE") == "wayland" && !qgetenv("WAYLAND_DISPLAY").isEmpty()) {
        QString waylandDisplay = qgetenv("WAYLAND_DISPLAY");
        if (waylandDisplay.contains("gamescope", Qt::CaseInsensitive)) {
            return true;
        }
    }
    
#ifdef Q_OS_LINUX
    // Check hardware detection - Steam Deck product name
    QFile productFile("/sys/devices/virtual/dmi/id/product_name");
    if (productFile.exists() && productFile.open(QIODevice::ReadOnly)) {
        QString product = productFile.readAll().trimmed();
        if (product.contains("Steam Deck", Qt::CaseInsensitive)) {
            productFile.close();
            return true;
        }
        productFile.close();
    }
    
    // Check for Steam Deck CPU model
    QFile cpuFile("/proc/cpuinfo");
    if (cpuFile.exists() && cpuFile.open(QIODevice::ReadOnly)) {
        QString cpuInfo = cpuFile.readAll();
        cpuFile.close();
        
        // Steam Deck uses AMD Custom APU 0932 (Vangogh/Aerith)
        if (cpuInfo.contains("Custom APU 0932", Qt::CaseInsensitive) ||
            cpuInfo.contains("Vangogh", Qt::CaseInsensitive) ||
            cpuInfo.contains("Aerith", Qt::CaseInsensitive)) {
            return true;
        }
    }
#endif
    
    return false;
}

bool SystemProperties::hasVulkanHdrSupport()
{
    // First check if we're in an HDR-capable environment
    bool hasHdrEnvironment = false;
    
    // Check for Gamescope (Steam Deck, other handheld devices)
    if (!qgetenv("GAMESCOPE_WAYLAND_DISPLAY").isEmpty() || 
        (!qgetenv("WAYLAND_DISPLAY").isEmpty() && 
         QString(qgetenv("WAYLAND_DISPLAY")).contains("gamescope", Qt::CaseInsensitive))) {
        hasHdrEnvironment = true;
    }
    
    // Check for HDR-capable Wayland compositors
    if (qgetenv("XDG_SESSION_TYPE") == "wayland") {
        // Check for KDE Plasma Wayland HDR support (6.1+)
        QString kdeSession = qgetenv("KDE_SESSION_VERSION");
        QString plasma = qgetenv("PLASMA_DESKTOP_SESSION");
        if (!kdeSession.isEmpty() || !plasma.isEmpty()) {
            hasHdrEnvironment = true; // Assume recent KDE has HDR support
        }
        
        // Check for GNOME/Mutter HDR support
        QString gnomeSession = qgetenv("GNOME_DESKTOP_SESSION_ID");
        QString gdmSession = qgetenv("GDMSESSION");
        if (!gnomeSession.isEmpty() || gdmSession == "gnome" || gdmSession == "gnome-wayland") {
            hasHdrEnvironment = true; // Recent GNOME versions support HDR
        }
        
        // Check for wlroots-based compositors with HDR
        QString waylandDisplay = qgetenv("WAYLAND_DISPLAY");
        QString xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP");
        if (xdgCurrentDesktop.contains("sway", Qt::CaseInsensitive) ||
            xdgCurrentDesktop.contains("hyprland", Qt::CaseInsensitive) ||
            !qgetenv("SWAYSOCK").isEmpty()) {
            hasHdrEnvironment = true;
        }
    }
    
    // If no HDR environment detected, don't prefer Vulkan
    if (!hasHdrEnvironment) {
        return false;
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
    for (const QString& path : vulkanPaths) {
        if (QFile::exists(path)) {
            hasVulkanLibrary = true;
            break;
        }
    }
    
    if (!hasVulkanLibrary) {
        return false;
    }
    
    // Check for Mesa drivers that support Vulkan HDR
    QFile gpuFile("/proc/driver/nvidia/version");
    bool hasNvidia = gpuFile.exists();
    
    // For NVIDIA, check if we have recent enough drivers (545.29.06+)
    if (hasNvidia) {
        // NVIDIA generally supports Vulkan HDR with recent drivers
        return true;
    }
    
    // For AMD/Intel, check for Mesa and radv/anv drivers
    QProcess vulkanInfo;
    vulkanInfo.start("vulkaninfo", QStringList() << "--summary");
    vulkanInfo.waitForFinished(3000);
    
    if (vulkanInfo.exitCode() == 0) {
        QString output = vulkanInfo.readAllStandardOutput();
        
        // Check for modern GPU drivers that support HDR
        if (output.contains("radv", Qt::CaseInsensitive) ||        // AMD RADV
            output.contains("anv", Qt::CaseInsensitive) ||         // Intel ANV 
            output.contains("nvidia", Qt::CaseInsensitive) ||      // NVIDIA
            output.contains("adreno", Qt::CaseInsensitive)) {      // Qualcomm Adreno
            return true;
        }
    }
#endif
    
    return false;
}
