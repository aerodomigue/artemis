# AV1 Hardware Decoder Detection Issue Analysis

## Problem Description
Users are reporting "AV1 isn't supported by your GPU" errors even with AV1-capable hardware:

1. **RTX 3060 + Mac Client**: Should support AV1 decoding but detection fails
2. **Steam Deck (AMD RDNA2)**: AV1 detection fails with Mesa Gallium drivers
3. **General Issue**: AV1 hardware decoder detection is too strict or incorrect

## Current Detection Logic Issues

### From User Logs Analysis:

**Steam Deck (AMD RDNA2) Issues:**
- `SDL Warn (0): HDR Detection: AV1 Main10 hardware decoder FAILED`
- `SDL Error (0): Unable to find working decoder for format: 2000` (AV1_MAIN10)
- Mesa Gallium driver detected but AV1 decode fails

**RTX 3060 Issues:**
- AV1 hardware decoding should work on RTX 30 series
- Detection logic may be failing to find the decoder
- Client-side detection failing even though host supports AV1 encoding

## Root Cause Analysis

### 1. Overly Strict Detection
Current detection tests specifically for:
- `VIDEO_FORMAT_AV1_MAIN10` for HDR detection
- Full hardware acceleration requirement

### 2. Platform-Specific Issues
- **macOS**: VideoToolbox AV1 support limited
- **Linux**: Mesa driver AV1 support varies by GPU generation
- **Windows**: Should work but may have FFmpeg detection issues

### 3. Decoder Priority Issues
The code tries AV1_MAIN10 first, but should fall back to AV1_MAIN8 for broader compatibility.

## Proposed Fixes

### 1. Improved AV1 Fallback Logic
```cpp
// Try AV1 Main10 first (for HDR)
if (!chooseDecoder(..., VIDEO_FORMAT_AV1_MAIN10, ...)) {
    // Fall back to AV1 Main8 for general compatibility
    if (chooseDecoder(..., VIDEO_FORMAT_AV1_MAIN8, ...)) {
        // AV1 supported but only 8-bit (SDR)
        isAV1Supported = true;
        isHdrSupported = false; // Can't do HDR with 8-bit
    }
}
```

### 2. Better GPU-Specific Detection
- RTX 30/40 series: Full AV1 support
- Intel Arc: Full AV1 support  
- AMD RDNA2+: Limited AV1 support (varies by driver)
- AMD GCN: No AV1 support

### 3. Environment Variable Overrides
```bash
# Force AV1 support detection
FORCE_AV1_SUPPORT=1

# Enable AV1 debug logging
AV1_DEBUG=1
```

## Moonlight-Qt Comparison Results

After comparing with moonlight-qt source code, key differences were identified:

### Enhanced Detection in Artemis
1. **AV1 Main8 Fallback**: Artemis adds comprehensive AV1_MAIN8 testing when AV1_MAIN10 fails
2. **Environment Variable Override**: Added `FORCE_AV1_SUPPORT=1` for debugging (not in moonlight-qt)
3. **Improved Logging**: Enhanced AV1 detection logging for troubleshooting

### Potential Issues Found
1. **FPS Parameter Consistency**: HDR detection hardcodes 60fps instead of using configured fps
2. **Enhanced Logic Exposure**: Our improved detection may expose platform-specific AV1 issues that moonlight-qt doesn't encounter

### Root Cause Analysis
The user's "AV1 isn't supported by your GPU" error likely comes from:
- Low-level renderer rejection (d3d11va.cpp: "GPU doesn't support AV1 decoding")
- Enhanced detection logic revealing hardware/driver limitations
- Platform-specific compatibility issues (RTX 3060 + Mac client)

## Implementation Plan

### Phase 1: Immediate Fix
- Add AV1_MAIN8 fallback in HDR detection
- Add environment variable override for AV1 support
- Improve error logging for AV1 detection failures

### Phase 2: GPU-Specific Logic
- Add GPU vendor/model specific AV1 capability detection
- Mesa driver version checking for AMD AV1 support
- Intel/NVIDIA driver version checking

### Phase 3: Comprehensive Testing
- Test matrix across all GPU vendors
- Platform-specific validation
- Fallback behavior verification

## Expected Impact
- RTX 3060 users should get proper AV1 support
- Steam Deck users may get limited AV1 (8-bit only)
- Better error messages for unsupported configurations
- Graceful degradation to H.264/HEVC when AV1 fails

## User Workarounds (Immediate)
1. Force software decoding in advanced settings
2. Disable HDR and use H.264/HEVC instead
3. Use environment variable override when available
