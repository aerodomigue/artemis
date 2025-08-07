# Flatpak Hardware Acceleration Troubleshooting

## Problem: "No functioning hardware accelerated video decoder was detected by Artemis"

If you're seeing this error when running the Flatpak version of Artemis, it's likely due to the Flatpak sandbox restricting access to your GPU hardware acceleration drivers.

## Quick Fix

Try running Artemis with hardware acceleration override:

```bash
flatpak run --env=ARTEMIS_FORCE_HW_ACCEL=1 com.artemisdesktop.ArtemisDesktop
```

This will bypass the hardware detection and assume hardware acceleration is available.

## GPU-Specific Solutions

### Intel iGPU Users
```bash
# Try with Intel VAAPI driver
flatpak run --env=LIBVA_DRIVER_NAME=i965 --env=ARTEMIS_FORCE_HW_ACCEL=1 com.artemisdesktop.ArtemisDesktop

# For newer Intel GPUs (Gen 9+), try:
flatpak run --env=LIBVA_DRIVER_NAME=iHD --env=ARTEMIS_FORCE_HW_ACCEL=1 com.artemisdesktop.ArtemisDesktop
```

### NVIDIA GPU Users
```bash
# Try with NVIDIA VDPAU driver
flatpak run --env=VDPAU_DRIVER=nvidia --env=ARTEMIS_FORCE_HW_ACCEL=1 com.artemisdesktop.ArtemisDesktop

# If you have NVIDIA proprietary drivers:
flatpak run --env=__NV_PRIME_RENDER_OFFLOAD=1 --env=__GLX_VENDOR_LIBRARY_NAME=nvidia --env=ARTEMIS_FORCE_HW_ACCEL=1 com.artemisdesktop.ArtemisDesktop
```

### AMD GPU Users
```bash
# Try with AMD VAAPI driver
flatpak run --env=LIBVA_DRIVER_NAME=radeonsi --env=ARTEMIS_FORCE_HW_ACCEL=1 com.artemisdesktop.ArtemisDesktop

# For older AMD cards:
flatpak run --env=LIBVA_DRIVER_NAME=r600 --env=ARTEMIS_FORCE_HW_ACCEL=1 com.artemisdesktop.ArtemisDesktop
```

## Debugging Steps

### 1. Check GPU Device Access
```bash
# Verify GPU devices are accessible
flatpak run --command=ls com.artemisdesktop.ArtemisDesktop -la /dev/dri/
```

You should see files like:
- `card0`, `card1` (GPU cards)
- `renderD128`, `renderD129` (render nodes)

### 2. Check Video Acceleration Libraries
```bash
# Check for VAAPI libraries (Intel/AMD)
flatpak run --command=ls com.artemisdesktop.ArtemisDesktop -la /usr/lib/dri/ | grep vaapi

# Check for VDPAU libraries (NVIDIA)
flatpak run --command=ls com.artemisdesktop.ArtemisDesktop -la /usr/lib/vdpau/ | grep nvidia
```

### 3. Enable Debug Logging
```bash
# Run with comprehensive debug output
flatpak run --env=HDR_DEBUG=1 --env=ASPECT_RATIO_DEBUG=1 --env=ARTEMIS_FORCE_HW_ACCEL=1 com.artemisdesktop.ArtemisDesktop
```

Look for debug messages about hardware acceleration and decoder detection.

### 4. Test Hardware Acceleration Outside Artemis
```bash
# Test VAAPI with vainfo (if available)
flatpak run --command=vainfo com.artemisdesktop.ArtemisDesktop

# Test VDPAU with vdpauinfo (if available)  
flatpak run --command=vdpauinfo com.artemisdesktop.ArtemisDesktop
```

## Alternative Solutions

### 1. Use Native Package Instead
If available for your distribution, consider using the native `.deb`, `.rpm`, or AppImage version instead of Flatpak for better hardware access.

### 2. Force Software Decoding
If hardware acceleration continues to fail, you can force software decoding:

1. Start Artemis with the override: `flatpak run --env=ARTEMIS_FORCE_HW_ACCEL=1 com.artemisdesktop.ArtemisDesktop`
2. In Settings → Video → Advanced Settings
3. Set "Video decoder" to "Force software decoding"
4. This will work but may impact performance

### 3. Contact Support
If none of these solutions work, please report the issue with:

1. Your GPU model: `lspci | grep VGA`
2. Your driver version: `glxinfo | grep "OpenGL version"`
3. Debug output from: `flatpak run --env=HDR_DEBUG=1 com.artemisdesktop.ArtemisDesktop`

## Technical Background

The Flatpak sandbox restricts access to system resources for security. This can prevent Artemis from detecting and using your GPU's hardware video decoders. The override (`ARTEMIS_FORCE_HW_ACCEL=1`) tells Artemis to assume hardware acceleration is available and skip the detection check.

This fix is included in Artemis version 0.5.5 and later.
