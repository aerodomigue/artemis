# HDR Detection Comparison: Moonlight-Qt vs Artemis

## Problem Statement
- HDR detection works perfectly in moonlight-qt (HDR checkbox enabled, HDR works on host PC)
- In Artemis, HDR checkbox is no longer greyed out but HDR isn't available in client logs
- Need to identify differences in HDR detection implementation

## Key Differences Found

### 1. Software Decoder Fallback Logic ✅ FIXED

**Moonlight-Qt (Original)**:
```cpp
else {
    // If we found no hardware decoders with HDR, check for a renderer
    // that supports HDR rendering with software decoded frames.
    if (chooseDecoder(StreamingPreferences::VDS_FORCE_SOFTWARE,
                      window, VIDEO_FORMAT_H265_MAIN10, 1920, 1080, 60,
                      false, false, true, decoder) ||
        chooseDecoder(StreamingPreferences::VDS_FORCE_SOFTWARE,
                      window, VIDEO_FORMAT_AV1_MAIN10, 1920, 1080, 60,
                      false, false, true, decoder)) {
        isHdrSupported = decoder->isHdrSupported();
        delete decoder;
    }
    else {
        isHdrSupported = false;
    }
}
```

**Artemis (Previous)**:
- Software decoder testing was separated into if-else chain
- Only tried software decoders if `!isHdrSupported` later in function
- This could miss cases where software HDR decoding works

**Fixed**: Updated Artemis to match moonlight-qt's compound condition approach

### 2. Enhanced AV1 Testing (Artemis Addition)

**Artemis Enhancement**:
- Added AV1 Main8 fallback testing in HDR detection
- This provides better AV1 compatibility detection
- Moonlight-qt lacks this comprehensive AV1 testing

### 3. Logging Differences

**Moonlight-Qt**: Minimal HDR detection logging
**Artemis**: Comprehensive logging for HDR detection steps

## Remaining Investigation Areas

### 1. Renderer Capability Differences
- Check if HDR-capable renderers are available on different platforms
- Verify HDR renderer compilation/availability in Artemis vs moonlight-qt

### 2. Platform-Specific HDR Support
- macOS: VideoToolbox HDR capabilities
- Linux: Vulkan/OpenGL HDR support
- Windows: D3D11VA/DXVA2 HDR support

### 3. Decoder Implementation Differences
- Check if `decoder->isHdrSupported()` behaves differently
- Verify HDR capability detection in underlying decoders

## Testing Plan

### 1. Test HDR Detection Logic ✅ COMPLETED
- Fixed software decoder fallback to match moonlight-qt
- Test with RTX 3060 + Mac client scenario

### 2. Compare Renderer Capabilities
- Check available renderers in both codebases
- Verify HDR-capable renderer compilation

### 3. Decoder Backend Comparison
- Compare FFmpeg decoder HDR support
- Check for differences in HDR capability reporting

## Expected Impact
- HDR detection should now work more reliably in Artemis
- Better compatibility with moonlight-qt's proven HDR detection
- Maintain enhanced AV1 detection while fixing HDR issues
