@echo off
setlocal enableDelayedExpansion

rem Run from Qt command prompt with working directory set to root of repo

set BUILD_CONFIG=%1

rem Convert to lower case for windeployqt
if /I "%BUILD_CONFIG%"=="debug" (
    set BUILD_CONFIG=debug
    set WIX_MUMS=10
) else (
    if /I "%BUILD_CONFIG%"=="release" (
        set BUILD_CONFIG=release
        set WIX_MUMS=10
    ) else (
        if /I "%BUILD_CONFIG%"=="signed-release" (
            set BUILD_CONFIG=release
            set SIGN=1
            set MUST_DEPLOY_SYMBOLS=1

            rem Fail if there are unstaged changes
            git diff-index --quiet HEAD --
            if !ERRORLEVEL! NEQ 0 (
                echo Signed release builds must not have unstaged changes!
                exit /b 1
            )
        ) else (
            echo Invalid build configuration - expected 'debug' or 'release'
            echo Usage: scripts\build-artemis-arch.bat ^(release^|debug^)
            exit /b 1
        )
    )
)


rem Locate qmake and determine if we're using qmake.exe, qmake.bat, or qmake6.bat
set QMAKE_CMD=
for %%Q in (qmake.bat qmake6.bat qmake.exe qmake6.exe) do (
    where %%Q >nul 2>&1
    if !ERRORLEVEL! EQU 0 if not defined QMAKE_CMD set QMAKE_CMD=%%Q
)
if not defined QMAKE_CMD (
    echo Unable to find QMake. Did you add Qt bins to your PATH?
    goto Error
)

rem Find Qt path to determine our architecture
set QT_PATH=
for %%Q in (qmake.bat qmake6.bat qmake.exe qmake6.exe) do (
    for /F %%i in ('where %%Q 2^>nul') do if not defined QT_PATH set QT_PATH=%%i
)

rem Strip the qmake filename off the end to get the Qt bin directory itself
set QT_PATH=%QT_PATH:\qmake.exe=%
set QT_PATH=%QT_PATH:\qmake.bat=%
set QT_PATH=%QT_PATH:\qmake.cmd=%
set QT_PATH=%QT_PATH:\qmake6.exe=%
set QT_PATH=%QT_PATH:\qmake6.bat=%

echo QT_PATH=%QT_PATH%
if not x%QT_PATH:_arm64=%==x%QT_PATH% (
    set ARCH=arm64

    rem Replace the _arm64 suffix with _64 to get the x64 bin path
    set HOSTBIN_PATH=%QT_PATH:_arm64=_64%
    echo HOSTBIN_PATH=!HOSTBIN_PATH!

    if exist %QT_PATH%\windeployqt.exe (
        echo Using windeployqt.exe from QT_PATH
        set WINDEPLOYQT_CMD=windeployqt.exe
    ) else (
        echo Using windeployqt.exe from HOSTBIN_PATH
        set WINDEPLOYQT_CMD=!HOSTBIN_PATH!\windeployqt.exe --qtpaths %QT_PATH%\qtpaths.bat
    )
) else (
    if not x%QT_PATH:_64=%==x%QT_PATH% (
        set ARCH=x64
        set WINDEPLOYQT_CMD=windeployqt.exe
    ) else (
        if not x%QT_PATH:msvc=%==x%QT_PATH% (
            set ARCH=x86
            set WINDEPLOYQT_CMD=windeployqt.exe
        ) else (
            echo Unable to determine Qt architecture
            goto Error
        )
    )
)

echo Detected target architecture: %ARCH%

set SIGNTOOL_PARAMS=sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /sha1 8b9d0d682ad9459e54f05a79694bc10f9876e297 /v

set BUILD_ROOT=%cd%\build
set SOURCE_ROOT=%cd%
set BUILD_FOLDER=%BUILD_ROOT%\build-%ARCH%-%BUILD_CONFIG%
set DEPLOY_FOLDER=%BUILD_ROOT%\deploy-%ARCH%-%BUILD_CONFIG%
set INSTALLER_FOLDER=%BUILD_ROOT%\installer-%ARCH%-%BUILD_CONFIG%
set SYMBOLS_FOLDER=%BUILD_ROOT%\symbols-%ARCH%-%BUILD_CONFIG%
set /p VERSION=<%SOURCE_ROOT%\app\version.txt

rem Use the correct VC tools for the specified architecture
if /I "%ARCH%" EQU "x64" (
    rem x64 is a special case that doesn't match %PROCESSOR_ARCHITECTURE%
    set VC_ARCH=AMD64
) else (
    set VC_ARCH=%ARCH%
)

rem If we're not building for the current platform, use the cross compiling toolchain
if /I "%VC_ARCH%" NEQ "%PROCESSOR_ARCHITECTURE%" (
    set VC_ARCH=%PROCESSOR_ARCHITECTURE%_%VC_ARCH%
)

rem Find Visual Studio and run vcvarsall.bat
set VSWHERE="%SOURCE_ROOT%\scripts\vswhere.exe"
for /f "usebackq delims=" %%i in (`%VSWHERE% -latest -property installationPath`) do (
    rem Use a simplified environment call to avoid "input line too long" errors
    set VCVARS_PATH=%%i\VC\Auxiliary\Build\vcvarsall.bat
)

rem Call vcvarsall with minimal environment to avoid command line length issues
echo Setting up MSVC environment for %VC_ARCH%
call "%VCVARS_PATH%" %VC_ARCH% >nul 2>&1
if !ERRORLEVEL! NEQ 0 (
    echo Warning: vcvarsall.bat failed, trying alternative approach
    rem Try to set up minimal MSVC environment manually
    for /f "usebackq delims=" %%i in (`%VSWHERE% -latest -property installationPath`) do (
        set MSVC_PATH=%%i\VC\Tools\MSVC
        for /f "delims=" %%j in ('dir /b "%%i\VC\Tools\MSVC"') do set MSVC_VERSION=%%j
        set "INCLUDE=%%i\VC\Tools\MSVC\!MSVC_VERSION!\include;%%i\VC\Tools\MSVC\!MSVC_VERSION!\atlmfc\include"
        if /I "%VC_ARCH%" EQU "AMD64_ARM64" (
            set "PATH=%%i\VC\Tools\MSVC\!MSVC_VERSION!\bin\Hostx64\arm64;%%i\VC\Tools\MSVC\!MSVC_VERSION!\bin\Hostx64\x64;!PATH!"
            set "LIB=%%i\VC\Tools\MSVC\!MSVC_VERSION!\lib\arm64;%%i\VC\Tools\MSVC\!MSVC_VERSION!\atlmfc\lib\arm64"
        ) else (
            set "PATH=%%i\VC\Tools\MSVC\!MSVC_VERSION!\bin\Hostx64\x64;!PATH!"
            set "LIB=%%i\VC\Tools\MSVC\!MSVC_VERSION!\lib\x64;%%i\VC\Tools\MSVC\!MSVC_VERSION!\atlmfc\lib\x64"
        )
    )
)

rem Find VC redistributable DLLs
for /f "usebackq delims=" %%i in (`%VSWHERE% -latest -find VC\Redist\MSVC\*\%ARCH%\Microsoft.VC*.CRT`) do set VC_REDIST_DLL_PATH=%%i

echo Cleaning output directories
rmdir /s /q %DEPLOY_FOLDER%
rmdir /s /q %BUILD_FOLDER%
rmdir /s /q %INSTALLER_FOLDER%
rmdir /s /q %SYMBOLS_FOLDER%
mkdir %BUILD_ROOT%
mkdir %DEPLOY_FOLDER%
mkdir %BUILD_FOLDER%
mkdir %INSTALLER_FOLDER%
mkdir %SYMBOLS_FOLDER%

echo Configuring the project
pushd %BUILD_FOLDER%
echo Running qmake command: %QMAKE_CMD% %SOURCE_ROOT%\artemis.pro
echo Current directory: %CD%
echo Target architecture: %ARCH%
echo Qt path: %QT_PATH%

rem For ARM64 builds, we need to specify the target platform explicitly
if /I "%ARCH%" EQU "arm64" (
    echo Configuring for ARM64 cross-compilation
    %QMAKE_CMD% %SOURCE_ROOT%\artemis.pro "CONFIG+=arm64"
) else (
    %QMAKE_CMD% %SOURCE_ROOT%\artemis.pro
)
if !ERRORLEVEL! NEQ 0 goto Error

rem Verify the generated Makefile has the correct architecture
if exist "Makefile" (
    echo Checking Makefile for architecture settings:
    findstr /C:"arm64" Makefile || echo "Warning: arm64 not found in Makefile"
    findstr /C:"ARCH" Makefile || echo "Warning: ARCH not found in Makefile"
)
popd

echo Compiling Artemis in %BUILD_CONFIG% configuration
pushd %BUILD_FOLDER%
if exist "%SOURCE_ROOT%\scripts\jom.exe" (
    %SOURCE_ROOT%\scripts\jom.exe %BUILD_CONFIG%
) else (
    nmake %BUILD_CONFIG%
)
if !ERRORLEVEL! NEQ 0 goto Error

rem Debug: Check what was actually built
echo Checking build output:
dir app\%BUILD_CONFIG%\*.exe 2>nul || echo "No exe files found in app\%BUILD_CONFIG%"
if exist "app\%BUILD_CONFIG%\Artemis.exe" (
    echo Artemis.exe found, checking architecture...
    file app\%BUILD_CONFIG%\Artemis.exe 2>nul || echo "file command not available"
    dumpbin /headers app\%BUILD_CONFIG%\Artemis.exe | findstr "machine" 2>nul || echo "dumpbin not available"
) else (
    echo ERROR: Artemis.exe was not built!
    dir app\* /s 2>nul || echo "No files in app directory"
)
popd

echo Saving PDBs
for /r "%BUILD_FOLDER%" %%f in (*.pdb) do (
    copy "%%f" %SYMBOLS_FOLDER%
    if !ERRORLEVEL! NEQ 0 goto Error
)
copy %SOURCE_ROOT%\libs\windows\lib\%ARCH%\*.pdb %SYMBOLS_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error
7z a %SYMBOLS_FOLDER%\ArtemisDebuggingSymbols-%ARCH%-%VERSION%.zip %SYMBOLS_FOLDER%\*.pdb
if !ERRORLEVEL! NEQ 0 goto Error

echo Copying DLL dependencies
copy %SOURCE_ROOT%\libs\windows\lib\%ARCH%\*.dll %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

echo Copying AntiHooking.dll
copy %BUILD_FOLDER%\AntiHooking\%BUILD_CONFIG%\AntiHooking.dll %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

echo Copying GC mapping list
copy %SOURCE_ROOT%\app\SDL_GameControllerDB\gamecontrollerdb.txt %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

if not x%QT_PATH:\5.=%==x%QT_PATH% (
    echo Copying qt.conf for Qt 5
    copy %SOURCE_ROOT%\app\qt_qt5.conf %DEPLOY_FOLDER%\qt.conf
    if !ERRORLEVEL! NEQ 0 goto Error

    rem Qt 5.15
    set WINDEPLOYQT_ARGS=--no-qmltooling --no-virtualkeyboard
) else (
    rem Qt 6.5+
    set WINDEPLOYQT_ARGS=--no-system-d3d-compiler --no-system-dxc-compiler --skip-plugin-types qmltooling,generic --no-ffmpeg
    set WINDEPLOYQT_ARGS=!WINDEPLOYQT_ARGS! --no-quickcontrols2fusion --no-quickcontrols2imagine --no-quickcontrols2universal
    set WINDEPLOYQT_ARGS=!WINDEPLOYQT_ARGS! --no-quickcontrols2fusionstyleimpl --no-quickcontrols2imaginestyleimpl --no-quickcontrols2universalstyleimpl --no-quickcontrols2windowsstyleimpl
)

echo Deploying Qt dependencies
%WINDEPLOYQT_CMD% --dir %DEPLOY_FOLDER% --%BUILD_CONFIG% --qmldir %SOURCE_ROOT%\app\gui --no-opengl-sw --no-compiler-runtime --no-sql %WINDEPLOYQT_ARGS% %BUILD_FOLDER%\app\%BUILD_CONFIG%\Artemis.exe
if !ERRORLEVEL! NEQ 0 goto Error

echo Deleting unused styles
rem Qt 5.x directories
rmdir /s /q %DEPLOY_FOLDER%\QtQuick\Controls.2\Fusion
rmdir /s /q %DEPLOY_FOLDER%\QtQuick\Controls.2\Imagine
rmdir /s /q %DEPLOY_FOLDER%\QtQuick\Controls.2\Universal
rem Qt 6.5+ directories
rmdir /s /q %DEPLOY_FOLDER%\qml\QtQuick\Controls\Fusion
rmdir /s /q %DEPLOY_FOLDER%\qml\QtQuick\Controls\Imagine
rmdir /s /q %DEPLOY_FOLDER%\qml\QtQuick\Controls\Universal
rmdir /s /q %DEPLOY_FOLDER%\qml\QtQuick\Controls\Windows
rmdir /s /q %DEPLOY_FOLDER%\qml\QtQuick\NativeStyle

if "%SIGN%"=="1" (
    echo Signing deployed binaries
    set FILES_TO_SIGN=%BUILD_FOLDER%\app\%BUILD_CONFIG%\Artemis.exe
    for /r "%DEPLOY_FOLDER%" %%f in (*.dll *.exe) do (
        set FILES_TO_SIGN=!FILES_TO_SIGN! %%f
    )
    signtool %SIGNTOOL_PARAMS% !FILES_TO_SIGN!
    if !ERRORLEVEL! NEQ 0 goto Error
)

echo Building MSI
set DEPLOY_FOLDER=%DEPLOY_FOLDER%
set BUILD_FOLDER=%BUILD_FOLDER%
msbuild %SOURCE_ROOT%\wix\Artemis\Artemis.wixproj -Restore /p:Configuration=%BUILD_CONFIG% /p:Platform=%ARCH% /p:OutputPath=%INSTALLER_FOLDER%\ /p:DEPLOY_FOLDER=%DEPLOY_FOLDER% /p:BUILD_FOLDER=%BUILD_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

echo Copying application binary to deployment directory
copy %BUILD_FOLDER%\app\%BUILD_CONFIG%\Artemis.exe %DEPLOY_FOLDER%\Artemis.exe
if !ERRORLEVEL! NEQ 0 goto Error

echo Building portable package
rem This must be done after WiX harvesting and signing, since the VCRT dlls are MS signed
rem and should not be harvested for inclusion in the full installer
copy "%VC_REDIST_DLL_PATH%\*.dll" %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error
rem This file tells Artemis that it's a portable installation
echo. > %DEPLOY_FOLDER%\portable.dat
if !ERRORLEVEL! NEQ 0 goto Error
7z a %INSTALLER_FOLDER%\ArtemisPortable-%ARCH%-%VERSION%.zip %DEPLOY_FOLDER%\*
if !ERRORLEVEL! NEQ 0 goto Error

echo Build successful for Artemis v%VERSION% %ARCH% binaries!
exit /b 0

:Error
echo Build failed!
exit /b !ERRORLEVEL!
