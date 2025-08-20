# This script requires create-dmg to be installed from https://github.com/sindresorhus/create-dmg
BUILD_CONFIG=$1

fail()
{
	echo "$1" 1>&2
	exit 1
}

if [ "$BUILD_CONFIG" != "Debug" ] && [ "$BUILD_CONFIG" != "Release" ]; then
  fail "Invalid build configuration - expected 'Debug' or 'Release'"
fi

BUILD_ROOT=$PWD/build
SOURCE_ROOT=$PWD
BUILD_FOLDER=$BUILD_ROOT/build-$BUILD_CONFIG
INSTALLER_FOLDER=$BUILD_ROOT/installer-$BUILD_CONFIG
VERSION=`cat $SOURCE_ROOT/app/version.txt`

if [ "$SIGNING_PROVIDER_SHORTNAME" == "" ]; then
  SIGNING_PROVIDER_SHORTNAME=$SIGNING_IDENTITY
fi
if [ "$SIGNING_IDENTITY" == "" ]; then
  SIGNING_IDENTITY=$SIGNING_PROVIDER_SHORTNAME
fi

[ "$SIGNING_IDENTITY" == "" ] || git diff-index --quiet HEAD -- || fail "Signed release builds must not have unstaged changes!"

echo Cleaning output directories
rm -rf $BUILD_FOLDER
rm -rf $INSTALLER_FOLDER
mkdir $BUILD_ROOT
mkdir $BUILD_FOLDER
mkdir $INSTALLER_FOLDER

echo Configuring the project
pushd $BUILD_FOLDER
qmake $SOURCE_ROOT/artemis.pro QMAKE_APPLE_DEVICE_ARCHS="x86_64 arm64" || fail "Qmake failed!"
popd

echo Compiling Moonlight in $BUILD_CONFIG configuration
pushd $BUILD_FOLDER
make -j$(sysctl -n hw.logicalcpu) $(echo "$BUILD_CONFIG" | tr '[:upper:]' '[:lower:]') || fail "Make failed!"
popd

echo Saving dSYM file
pushd $BUILD_FOLDER
dsymutil app/Artemis.app/Contents/MacOS/Artemis -o Artemis-$VERSION.dsym || fail "dSYM creation failed!"
cp -R Artemis-$VERSION.dsym $INSTALLER_FOLDER || fail "dSYM copy failed!"
popd

echo Creating app bundle
EXTRA_ARGS=
if [ "$BUILD_CONFIG" == "Debug" ]; then EXTRA_ARGS="$EXTRA_ARGS -use-debug-libs"; fi
echo Extra deployment arguments: $EXTRA_ARGS
macdeployqt $BUILD_FOLDER/app/Artemis.app $EXTRA_ARGS -qmldir=$SOURCE_ROOT/app/gui -appstore-compliant || fail "macdeployqt failed!"

echo Removing dSYM files from app bundle
find $BUILD_FOLDER/app/Artemis.app/ -name '*.dSYM' | xargs rm -rf

if [ "$SIGNING_IDENTITY" != "" ]; then
  echo Signing app bundle
  codesign --force --deep --options runtime --timestamp --sign "$SIGNING_IDENTITY" $BUILD_FOLDER/app/Artemis.app || fail "Signing failed!"
fi

echo Creating DMG
DMG_NAME="Artemis-$VERSION.dmg"
if [ "$SIGNING_IDENTITY" != "" ]; then
  create-dmg \
    --volname "Artemis $VERSION" \
    --background "$SOURCE_ROOT/scripts/dmg-background.png" \
    --window-pos 200 120 \
    --window-size 660 400 \
    --icon-size 80 \
    --icon \"Artemis.app\" \"$SOURCE_ROOT/scripts/artemis.png\" 180 170 \
    --hide-extension "Artemis.app" \
    --app-drop-link 480 170 \
    "$INSTALLER_FOLDER/$DMG_NAME" \
    "$BUILD_FOLDER/app/Artemis.app" --identity="$SIGNING_IDENTITY" || fail "create-dmg failed!"
else
  create-dmg \
    --volname "Artemis $VERSION" \
    --background "$SOURCE_ROOT/scripts/dmg-background.png" \
    --window-pos 200 120 \
    --window-size 660 400 \
    --icon-size 80 \
    --icon \"Artemis.app\" \"$SOURCE_ROOT/scripts/artemis.png\" 180 170 \
    --app-drop-link 480 170 \
    --hide-extension "Artemis.app" \
    --app-drop-link 480 170 \
    "$INSTALLER_FOLDER/$DMG_NAME" \
    "$BUILD_FOLDER/app/Artemis.app"
  case $? in
    0) ;;
    2) ;;
    *) fail "create-dmg failed!";;
  esac
fi

if [ "$NOTARY_KEYCHAIN_PROFILE" != "" ]; then
  echo Uploading to App Notary service
  xcrun notarytool submit --keychain-profile "$NOTARY_KEYCHAIN_PROFILE" --wait "$INSTALLER_FOLDER/$DMG_NAME" || fail "Notary submission failed"

  echo Stapling notary ticket to DMG
  xcrun stapler staple -v "$INSTALLER_FOLDER/$DMG_NAME" || fail "Notary ticket stapling failed!"
fi

# Create build info file
cat > $INSTALLER_FOLDER/build_info_macos.txt << EOF
Artemis Desktop macOS Universal Development Build
Version: $VERSION
Architecture: Universal (x86_64 + arm64)
Build Configuration: $BUILD_CONFIG
Built: $(date -u '+%Y-%m-%d %H:%M:%S UTC')

Installation Notes:
- This is a universal binary that works on both Intel and Apple Silicon Macs
- If macOS says the app is "damaged", run: xattr -cr Artemis.app
- Or go to System Preferences > Security & Privacy and allow the app
- This is a development build and may trigger Gatekeeper warnings
EOF

echo Build successful