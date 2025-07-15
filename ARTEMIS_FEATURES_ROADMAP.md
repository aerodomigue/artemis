# 🎯 Artemis Qt Features Implementation Roadmap

## 📋 Feature Status Overview

| Feature | Status | Priority | Complexity | Notes |
|---------|--------|----------|------------|-------|
| **Clipboard Sync** | ✅ **COMPLETED** | High | Medium | Fully implemented and working |
| **Server Commands** | 🔄 **IN PROGRESS** | High | Medium | Backend done, needs streaming UI |
| **Fractional Refresh Rate** | ❌ **TODO** | High | Low | Client-side setting |
| **Resolution Scaling** | ❌ **TODO** | High | Low | Client-side setting |
| **Virtual Display Control** | ❌ **TODO** | Medium | Medium | Client-side setting |
| **App Ordering** | ❌ **TODO** | Medium | High | Protocol extension needed |
| **OTP Pairing** | ✅ **COMPLETED** | Medium | Medium | Backend done, needs UI |
| **Permission Viewing** | ❌ **TODO** | Low | Low | UI component needed |
| **Input Only Mode** | ❌ **TODO** | Low | High | Not yet in Android Artemis |

## 🚀 Implementation Plan

### Phase 3A: Complete Current Features (Week 1-2)

#### 1. Server Commands Streaming Integration ⚡ **HIGH PRIORITY**
**Status**: Backend complete, needs streaming UI integration

**Implementation Steps**:
```cpp
// 1. Extend OverlayManager for interactive overlays
enum OverlayType {
    OverlayDebug,
    OverlayStatusUpdate,
    OverlayServerCommands,  // Add this
    OverlayMax
};

// 2. Add key binding in input handler
if (key == SDLK_m && (modifiers & KMOD_CTRL) && (modifiers & KMOD_ALT)) {
    session->toggleServerCommandsOverlay();
}

// 3. Integrate ServerCommands.qml as streaming overlay
```

**Files to Modify**:
- `app/streaming/video/overlaymanager.h` - Add new overlay type
- `app/streaming/video/overlaymanager.cpp` - Implement interactive overlay
- `app/streaming/input/input.cpp` - Add key binding
- `app/streaming/session.cpp` - Add overlay toggle method
- Create `app/gui/StreamingOverlay.qml` - Container for streaming overlays

#### 2. OTP Pairing UI Integration ⚡ **HIGH PRIORITY**
**Status**: Backend complete, needs UI integration

**Implementation Steps**:
```qml
// 1. Create OTP pairing dialog
OTPPairingDialog {
    id: otpDialog
    visible: computer.isApolloServer && !computer.paired
    onOtpEntered: NvPairingManager.pairWithOTP(computer, otp)
}

// 2. Integrate with existing pairing flow
```

**Files to Modify**:
- Create `app/gui/OTPPairingDialog.qml` - OTP input dialog
- `app/gui/PcView.qml` - Add OTP pairing option
- `app/backend/nvpairingmanager.cpp` - Integrate OTPPairingManager

### Phase 3B: Client-Side Settings (Week 3-4)

#### 3. Fractional Refresh Rate Settings ⚡ **HIGH PRIORITY**
**Status**: New feature, high impact

**Implementation**:
```cpp
// StreamingPreferences extension
class StreamingPreferences {
    Q_PROPERTY(bool enableFractionalRefreshRate READ enableFractionalRefreshRate WRITE setEnableFractionalRefreshRate NOTIFY enableFractionalRefreshRateChanged)
    Q_PROPERTY(double customRefreshRate READ customRefreshRate WRITE setCustomRefreshRate NOTIFY customRefreshRateChanged)
    
private:
    bool m_enableFractionalRefreshRate = false;
    double m_customRefreshRate = 59.94; // Common fractional rate
};
```

**UI Component**:
```qml
GroupBox {
    title: "Refresh Rate Settings"
    
    CheckBox {
        text: "Enable Fractional Refresh Rate"
        checked: StreamingPreferences.enableFractionalRefreshRate
        onCheckedChanged: StreamingPreferences.enableFractionalRefreshRate = checked
    }
    
    SpinBox {
        enabled: StreamingPreferences.enableFractionalRefreshRate
        from: 3000  // 30.00 Hz
        to: 24000   // 240.00 Hz
        stepSize: 1
        value: StreamingPreferences.customRefreshRate * 100
        onValueChanged: StreamingPreferences.customRefreshRate = value / 100.0
        
        textFromValue: function(value) {
            return (value / 100.0).toFixed(2) + " Hz"
        }
    }
}
```

#### 4. Resolution Scaling Settings ⚡ **HIGH PRIORITY**
**Status**: New feature, high impact

**Implementation**:
```cpp
// StreamingPreferences extension
class StreamingPreferences {
    Q_PROPERTY(bool enableResolutionScaling READ enableResolutionScaling WRITE setEnableResolutionScaling NOTIFY enableResolutionScalingChanged)
    Q_PROPERTY(double resolutionScale READ resolutionScale WRITE setResolutionScale NOTIFY resolutionScaleChanged)
    
private:
    bool m_enableResolutionScaling = false;
    double m_resolutionScale = 1.0; // 100% = no scaling
};
```

**UI Component**:
```qml
GroupBox {
    title: "Resolution Scaling"
    
    CheckBox {
        text: "Enable Custom Resolution Scaling"
        checked: StreamingPreferences.enableResolutionScaling
        onCheckedChanged: StreamingPreferences.enableResolutionScaling = checked
    }
    
    Slider {
        enabled: StreamingPreferences.enableResolutionScaling
        from: 0.5   // 50%
        to: 2.0     // 200%
        stepSize: 0.1
        value: StreamingPreferences.resolutionScale
        onValueChanged: StreamingPreferences.resolutionScale = value
        
        Label {
            text: Math.round(parent.value * 100) + "%"
        }
    }
}
```

#### 5. Virtual Display Control 🔧 **MEDIUM PRIORITY**
**Status**: New feature, platform-specific

**Implementation**:
```cpp
// StreamingPreferences extension
class StreamingPreferences {
    Q_PROPERTY(bool forceVirtualDisplay READ forceVirtualDisplay WRITE setForceVirtualDisplay NOTIFY forceVirtualDisplayChanged)
    Q_PROPERTY(bool disableVirtualDisplay READ disableVirtualDisplay WRITE setDisableVirtualDisplay NOTIFY disableVirtualDisplayChanged)
    
private:
    bool m_forceVirtualDisplay = false;
    bool m_disableVirtualDisplay = false;
};
```

### Phase 4: Advanced Features (Week 5-8)

#### 6. App Ordering Without Compatibility Mode 🔧 **MEDIUM PRIORITY**
**Status**: Complex, requires protocol extension

**Analysis**: This requires extending the app list protocol to support custom ordering
```cpp
// Extend NvApp class
class NvApp {
    Q_PROPERTY(int customOrder READ customOrder WRITE setCustomOrder NOTIFY customOrderChanged)
    Q_PROPERTY(bool useCustomOrder READ useCustomOrder WRITE setUseCustomOrder NOTIFY useCustomOrderChanged)
    
private:
    int m_customOrder = -1;  // -1 = use default order
    bool m_useCustomOrder = false;
};

// Extend AppModel for custom sorting
class AppModel {
    Q_INVOKABLE void moveApp(int fromIndex, int toIndex);
    Q_INVOKABLE void resetAppOrder();
    Q_INVOKABLE void saveCustomOrder();
};
```

#### 7. Client-Side Permission Viewing 📊 **LOW PRIORITY**
**Status**: Simple UI feature

**Implementation**:
```qml
// Add to computer details or settings
GroupBox {
    title: "Server Permissions"
    visible: computer.isApolloServer
    
    Column {
        Label { text: "Clipboard Read: " + (computer.hasClipboardRead ? "✅" : "❌") }
        Label { text: "Clipboard Write: " + (computer.hasClipboardWrite ? "✅" : "❌") }
        Label { text: "Server Commands: " + (computer.hasServerCommands ? "✅" : "❌") }
        Label { text: "File Upload: " + (computer.hasFileUpload ? "✅" : "❌") }
        Label { text: "File Download: " + (computer.hasFileDownload ? "✅" : "❌") }
    }
}
```

#### 8. Input Only Mode 🚧 **LOW PRIORITY**
**Status**: Not yet implemented in Android Artemis

**Note**: Wait for Android implementation first, then port

## 📅 Detailed Implementation Timeline

### Week 1: Server Commands Streaming Integration
- **Day 1-2**: Extend OverlayManager for interactive overlays
- **Day 3-4**: Add key bindings and session integration
- **Day 5-7**: Create streaming overlay UI and test

### Week 2: OTP Pairing UI
- **Day 1-3**: Create OTP pairing dialog component
- **Day 4-5**: Integrate with existing pairing flow
- **Day 6-7**: Test with Apollo server

### Week 3: Client-Side Settings (Part 1)
- **Day 1-3**: Implement fractional refresh rate settings
- **Day 4-7**: Implement resolution scaling settings

### Week 4: Client-Side Settings (Part 2)
- **Day 1-4**: Implement virtual display control
- **Day 5-7**: Test all client-side settings

### Week 5-6: App Ordering
- **Day 1-5**: Extend app model for custom ordering
- **Day 6-10**: Implement drag-and-drop UI

### Week 7: Permission Viewing
- **Day 1-4**: Create permission viewing UI
- **Day 5-7**: Integrate with computer details

### Week 8: Testing & Polish
- **Day 1-7**: Comprehensive testing, bug fixes, documentation

## 🛠️ Technical Implementation Details

### File Structure for New Features
```
app/
├── backend/
│   ├── streamingpreferences.h/cpp     # Extended for new settings
│   ├── nvapp.h/cpp                    # Extended for app ordering
│   └── nvcomputer.h/cpp               # Extended for permissions
├── gui/
│   ├── OTPPairingDialog.qml           # New: OTP pairing
│   ├── StreamingOverlay.qml           # New: Streaming overlays
│   ├── RefreshRateSettings.qml        # New: Fractional refresh rate
│   ├── ResolutionSettings.qml         # New: Resolution scaling
│   ├── VirtualDisplaySettings.qml     # New: Virtual display
│   ├── AppOrderingView.qml            # New: App ordering
│   └── PermissionView.qml             # New: Permission viewing
└── streaming/
    ├── video/overlaymanager.h/cpp     # Extended for interactive overlays
    ├── input/input.cpp                # Extended for overlay key bindings
    └── session.h/cpp                  # Extended for overlay management
```

### Integration Points
1. **StreamingPreferences**: Extend existing class for new client-side settings
2. **OverlayManager**: Extend for interactive streaming overlays
3. **NvComputer**: Extend for Apollo permission tracking
4. **AppModel**: Extend for custom app ordering
5. **Session**: Extend for overlay management during streaming

## 🎯 Success Metrics

### Phase 3A Completion
- ✅ Server Commands accessible during streaming (Ctrl+Alt+M)
- ✅ OTP pairing works with Apollo servers
- ✅ No QML errors or crashes

### Phase 3B Completion
- ✅ Fractional refresh rates work (e.g., 59.94 Hz)
- ✅ Resolution scaling works (50%-200%)
- ✅ Virtual display control functions properly

### Phase 4 Completion
- ✅ Custom app ordering persists between sessions
- ✅ Permission viewing shows accurate server capabilities
- ✅ All features work seamlessly together

## 🚀 Quick Start for Next Implementation

**Immediate Next Steps**:
1. **Start with Server Commands streaming integration** (highest impact)
2. **Implement fractional refresh rate** (easy win, high user value)
3. **Add OTP pairing UI** (complete existing backend work)

This roadmap will give you a fully-featured Artemis Qt client that matches and exceeds the Android version's capabilities!