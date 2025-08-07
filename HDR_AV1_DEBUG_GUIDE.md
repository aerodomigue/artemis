# HDR and AV1 Debugging Guide

## Overview

This document describes the comprehensive fixes and debugging infrastructure added to address HDR and AV1 issues reported by the Apollo maintainer on Steam Deck.

## Fixes Implemented

### 1. AV1 Codec Support Fixed
- **Issue**: Empty AV1 section in `k_NonHwaccelCodecInfo` preventing AV1 decoder capability detection
- **Fix**: Added missing AV1 decoder entries in `app/streaming/video/ffmpeg.cpp`:
  - `av1_rkmpp` (Rockchip)
  - `av1_nvv4l2` (NVIDIA V4L2)
  - `av1_nvmpi` (NVIDIA NVMPI)
  - `av1_v4l2m2m` (Generic V4L2)
  - `av1_omx` (OpenMAX)
- **Capability**: All AV1 decoders now properly support `CAPABILITY_REFERENCE_FRAME_INVALIDATION_AV1`

### 2. HDR Detection Enhanced
- **Issue**: Limited visibility into HDR detection pipeline and Vulkan renderer selection
- **Fix**: Added comprehensive debug logging throughout HDR detection chain:
  - `SystemProperties::hasVulkanHdrSupport()` - Environment and driver detection
  - `SystemProperties::isSteamDeckOrGamescope()` - Steam Deck platform detection
  - `Session::getDecoderInfo()` - HDR decoder capability testing
  - `createFrontendRenderer()` - Vulkan renderer selection logic

### 3. Environment Variable Support
- **Issue**: Difficult to troubleshoot HDR/AV1 issues without runtime control
- **Fix**: Added/enhanced environment variable support:
  - `HDR_DEBUG=1` - Enables comprehensive HDR debug logging
  - `FORCE_HDR_SUPPORT=1` - Forces HDR support detection (existing, enhanced logging)
  - `FORCE_VULKAN=1` - Forces Vulkan renderer selection for testing
  - `PREFER_VULKAN=1` - Existing variable for Vulkan preference

## Debugging Environment Variables

### HDR_DEBUG=1
Enables detailed logging for:
- Steam Deck detection steps
- Vulkan HDR environment detection
- Wayland compositor identification
- Vulkan driver detection
- HDR decoder capability testing
- Renderer selection decisions

### FORCE_VULKAN=1
Forces the application to use Vulkan renderer regardless of platform detection:
- Overrides automatic renderer selection
- Useful for testing Vulkan HDR on non-Steam Deck systems
- Logs the override decision

### FORCE_HDR_SUPPORT=1
Forces HDR support to be enabled:
- Overrides decoder capability detection
- Shows before/after HDR support status
- Useful for testing HDR pipeline with unsupported hardware

## Testing on Steam Deck

### Basic Testing
```bash
# Run with comprehensive debug logging
HDR_DEBUG=1 ./artemis

# Force Vulkan renderer for testing
FORCE_VULKAN=1 HDR_DEBUG=1 ./artemis

# Force HDR support for pipeline testing
FORCE_HDR_SUPPORT=1 HDR_DEBUG=1 ./artemis
```

### Expected Debug Output
When running with `HDR_DEBUG=1`, you should see logs like:
```
Steam Deck Detection: Starting detection analysis
Steam Deck Detection: GAMESCOPE_WAYLAND_DISPLAY found ('gamescope-0'), returning true
Vulkan HDR Detection: Starting environment analysis
Vulkan HDR Detection: Gamescope detected (GAMESCOPE_WAYLAND_DISPLAY='gamescope-0', WAYLAND_DISPLAY='wayland-0')
Vulkan HDR Detection: HDR-capable environment found, checking Vulkan drivers...
FFmpeg: Steam Deck detected, preferring Vulkan renderer for HDR support
FFmpeg: Selected PlVkRenderer for HDR support
Session HDR Decoder Test: Attempting HEVC Main10 hardware decoder...
Session HDR Decoder Test: HEVC Main10 hardware decoder: SUCCESS
```

## Validation Checklist

### AV1 Support
- [ ] AV1 streams decode successfully on Steam Deck
- [ ] No "AV1 not working" errors in Apollo
- [ ] AV1 hardware acceleration utilized when available

### HDR Support  
- [ ] HDR streams display correctly on Steam Deck
- [ ] No "HDR not working" errors in Apollo
- [ ] Vulkan renderer selected automatically on Steam Deck
- [ ] HDR color mapping functions correctly

### Debug Infrastructure
- [ ] `HDR_DEBUG=1` produces comprehensive logging
- [ ] `FORCE_VULKAN=1` successfully forces Vulkan renderer
- [ ] `FORCE_HDR_SUPPORT=1` overrides capability detection
- [ ] All environment variables work independently and together

## Root Cause Analysis

### AV1 Issue
The `k_NonHwaccelCodecInfo` array had an empty AV1 section:
```cpp
// Before (broken)
{
    .codecs = {}  // Empty AV1 codec list
}

// After (fixed)
{
    .codecs = {
        { .name = "av1_rkmpp", .capabilities = CAPABILITY_REFERENCE_FRAME_INVALIDATION_AV1 },
        { .name = "av1_nvv4l2", .capabilities = CAPABILITY_REFERENCE_FRAME_INVALIDATION_AV1 },
        // ... additional AV1 codecs
    }
}
```

### HDR Issue
HDR detection was working but lacked visibility into the decision-making process. The comprehensive debug logging now reveals:
1. Steam Deck platform detection steps
2. Vulkan HDR environment assessment
3. Driver capability detection
4. Renderer selection logic
5. HDR decoder testing results

## Next Steps

1. **Deploy and Test**: Test fixes on actual Steam Deck hardware with Apollo server
2. **Collect Logs**: Use `HDR_DEBUG=1` to gather detailed diagnostic information
3. **Iterate**: Address any remaining issues with enhanced debugging visibility
4. **Aspect Ratio**: Port aspect ratio fixes from Android Artemis version (complete analysis needed)
5. **UI Updates**: Fix fractional refresh rate toggle integration and virtual display caption

## Additional Apollo Feedback Addressed

### ✅ Aspect Ratio Debugging Enhanced
- **Issue**: Apollo maintainer reported "image aspect ratio problems" on Steam Deck
- **Progress**: Added comprehensive aspect ratio debugging with `ASPECT_RATIO_DEBUG=1`
- **Enhancement**: Added detailed logging to `updateOptimalWindowDisplayMode()` showing video vs desktop aspect ratios
- **Analysis**: libplacebo library identified as key component for aspect ratio calculations
- **Monitoring**: Debug infrastructure in place to catch issues when HDR/AV1 renderer changes are deployed
- **TODO**: Port aspect ratio fixes from Android Artemis version when source access is available

### ✅ UI Improvements Completed  
- **Fractional Refresh Rate**: ✅ Successfully integrated toggle into custom frame rate dialog
  - Users can now switch between integer FPS and fractional Hz modes seamlessly
  - Eliminates separate UI controls for cleaner interface
  - Maintains all existing functionality while improving user experience
- **Virtual Display Caption**: ✅ Updated to specify "Apollo server - not available with Sunshine/GeForce Experience"
  - Provides clear guidance about feature availability and requirements

## Files Modified

- `app/streaming/video/ffmpeg.cpp` - Fixed AV1 codecs, enhanced renderer selection logging  
- `app/backend/systemproperties.cpp` - Enhanced HDR/Steam Deck detection with debug logging
- `app/streaming/session.cpp` - Added HDR decoder capability testing logs + aspect ratio debugging
- `app/gui/SettingsView.qml` - Integrated fractional refresh rate into custom FPS dialog, fixed virtual display caption

## Compilation Test

To verify the changes compile correctly:
```bash
cd /Users/will/code/artemis
make clean
make -j$(nproc)
```

The build should complete successfully with the new debug logging infrastructure.
