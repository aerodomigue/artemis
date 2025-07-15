#!/bin/bash

# Artemis Qt Build Verification Script
# This script helps verify that the recent changes are working correctly

echo "🚀 Artemis Qt Build Verification"
echo "================================="

# Check if we're in the right directory
if [ ! -f "moonlight-qt.pro" ]; then
    echo "❌ Error: moonlight-qt.pro not found. Please run this script from the project root."
    exit 1
fi

echo "✅ Project structure verified"

# Check if the modified files exist and have our changes
echo ""
echo "🔍 Checking modified files..."

# Check main.qml for ToolTip fix
if grep -q "id: tooltipHelper" app/gui/main.qml; then
    echo "✅ main.qml: ToolTip fix applied"
else
    echo "❌ main.qml: ToolTip fix not found"
fi

# Check ClipboardSettings.qml for property binding fixes
if grep -q "ClipboardManager.isEnabled = checked" app/gui/ClipboardSettings.qml; then
    echo "✅ ClipboardSettings.qml: Property binding fixes applied"
else
    echo "❌ ClipboardSettings.qml: Property binding fixes not found"
fi

# Check ServerCommands.qml for layout fixes
if ! grep -q "clip: true" app/gui/ServerCommands.qml; then
    echo "✅ ServerCommands.qml: Clipping removed"
else
    echo "❌ ServerCommands.qml: Clipping still present"
fi

# Check SettingsView.qml for layout improvements
if grep -q "bottomPadding: 30" app/gui/SettingsView.qml; then
    echo "✅ SettingsView.qml: Layout improvements applied"
else
    echo "❌ SettingsView.qml: Layout improvements not found"
fi

echo ""
echo "🔧 Build Instructions:"
echo "1. Clean any previous builds: make clean"
echo "2. Run qmake: qmake moonlight-qt.pro"
echo "3. Build: make -j$(nproc)"
echo "4. Run and test the settings page"

echo ""
echo "🧪 Testing Checklist:"
echo "□ No ToolTip warnings in logs"
echo "□ No ClipboardManager property errors"
echo "□ Clipboard settings are functional"
echo "□ ServerCommands section is visible"
echo "□ Settings page scrolls properly"
echo "□ All Artemis features are accessible"

echo ""
echo "📝 If issues persist:"
echo "1. Check the logs for any remaining errors"
echo "2. Verify QML component registration in main.cpp"
echo "3. Test with different window sizes"
echo "4. Consider further layout optimizations"

echo ""
echo "✨ Verification complete!"