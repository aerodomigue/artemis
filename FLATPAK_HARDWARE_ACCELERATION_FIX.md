# Flatpak Hardware Acceleration Fix

## Problem
Flatpak builds of Artemis show the error "No functioning hardware accelerated video decoder was detected by Artemis" due to sandbox restrictions preventing access to GPU drivers and hardware.

## Root Cause
The Flatpak sandbox restricts access to:
- `/dev/dri/*` - GPU device files
- Hardware decoder libraries (VAAPI, VDPAU, NVDEC)
- GPU driver paths and libraries
- System-level video acceleration APIs

## Solution

### 1. Updated Flatpak Manifest Permissions

The Flatpak manifest needs these additional finish-args:

```yaml
finish-args:
  # Existing permissions
  - --share=ipc
  - --socket=x11
  - --socket=wayland
  - --socket=pulseaudio
  - --share=network
  - --filesystem=home
  
  # GPU and hardware acceleration access
  - --device=dri
  - --filesystem=/sys/dev/char:ro
  - --filesystem=/sys/devices/pci*:ro
  
  # Additional GPU driver access
  - --filesystem=/usr/lib/dri:ro
  - --filesystem=/usr/lib/x86_64-linux-gnu/dri:ro
  - --filesystem=/usr/lib64/dri:ro
  - --filesystem=/usr/share/drirc.d:ro
  
  # NVIDIA driver access (if available)
  - --filesystem=/usr/lib/nvidia:ro
  - --filesystem=/usr/lib/x86_64-linux-gnu/nvidia:ro
  - --filesystem=/usr/lib64/nvidia:ro
  
  # Mesa/Intel driver access
  - --filesystem=/usr/lib/mesa:ro
  - --filesystem=/usr/lib/x86_64-linux-gnu/mesa:ro
  
  # VAAPI/VDPAU libraries
  - --filesystem=/usr/lib/libva:ro
  - --filesystem=/usr/lib/x86_64-linux-gnu/libva:ro
  - --filesystem=/usr/lib/vdpau:ro
  - --filesystem=/usr/lib/x86_64-linux-gnu/vdpau:ro
```

### 2. Runtime Dependencies

Add these SDK extensions to the manifest:

```yaml
sdk-extensions:
  - org.freedesktop.Sdk.Extension.llvm16
  - org.freedesktop.Sdk.Extension.rust-stable

add-extensions:
  org.freedesktop.Platform.GL.default:
    directory: lib/GL
    version: "23.08"
    subdirectory: lib
    no-autodownload: true
    autodelete: false
    add-ld-path: lib
  
  org.freedesktop.Platform.GL.Mesa:
    directory: lib/GL
    version: "23.08"
    subdirectory: mesa
    no-autodownload: true
    autodelete: false
    add-ld-path: lib
    
  org.freedesktop.Platform.VAAPI.Intel:
    directory: lib/dri/intel-vaapi-driver
    version: "23.08"
    subdirectory: lib
    no-autodownload: true
    autodelete: false
```

### 3. Environment Variable Override

Users can override hardware detection with:

```bash
flatpak run --env=ARTEMIS_FORCE_HW_ACCEL=1 com.artemisdesktop.ArtemisDesktop
```

### 4. Testing Hardware Acceleration

To test if hardware acceleration is working in Flatpak:

```bash
# Check available GPU devices
flatpak run --command=ls com.artemisdesktop.ArtemisDesktop -la /dev/dri/

# Check VAAPI support
flatpak run --env=LIBVA_DRIVER_NAME=i965 --env=HDR_DEBUG=1 com.artemisdesktop.ArtemisDesktop

# Check decoder capabilities
flatpak run --env=HDR_DEBUG=1 --env=ASPECT_RATIO_DEBUG=1 com.artemisdesktop.ArtemisDesktop
```

## Implementation Status

- [x] Identify root cause (sandbox GPU access restrictions)
- [x] Document required Flatpak permissions
- [x] Create environment variable override option (ARTEMIS_FORCE_HW_ACCEL=1)
- [x] Update Flatpak manifest in CI/CD pipeline
- [x] Update SystemProperties.cpp with override logic
- [x] Version bumped to 0.5.6 for Flatpak hardware acceleration fix
- [ ] Test with different GPU vendors (Intel, NVIDIA, AMD)
- [ ] Add Flatpak-specific troubleshooting to documentation

## GPU Vendor Support

### Intel iGPU
- Requires: `org.freedesktop.Platform.VAAPI.Intel`
- Files: `/usr/lib/dri/i965_drv_video.so`
- Env: `LIBVA_DRIVER_NAME=i965`

### NVIDIA GPU
- Requires: NVIDIA runtime extension
- Files: `/usr/lib/nvidia/*`
- Env: `VDPAU_DRIVER=nvidia`

### AMD GPU  
- Requires: Mesa radeonsi driver
- Files: `/usr/lib/dri/radeonsi_drv_video.so`
- Env: `LIBVA_DRIVER_NAME=radeonsi`

## Testing Commands

```bash
# Intel VAAPI test
flatpak run --env=LIBVA_DRIVER_NAME=i965 --env=HDR_DEBUG=1 com.artemisdesktop.ArtemisDesktop

# NVIDIA VDPAU test  
flatpak run --env=VDPAU_DRIVER=nvidia --env=HDR_DEBUG=1 com.artemisdesktop.ArtemisDesktop

# AMD VAAPI test
flatpak run --env=LIBVA_DRIVER_NAME=radeonsi --env=HDR_DEBUG=1 com.artemisdesktop.ArtemisDesktop

# Force hardware acceleration override
flatpak run --env=ARTEMIS_FORCE_HW_ACCEL=1 com.artemisdesktop.ArtemisDesktop
```
