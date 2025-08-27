#ifndef PL_LIBAV_SHIM_H
#define PL_LIBAV_SHIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <libplacebo/renderer.h>
#include <libavutil/frame.h>

// Simple C-callable wrappers to avoid including libplacebo's C99-heavy
// libav headers in C++ translation units.
bool pl_map_avframe_simple(pl_gpu gpu, pl_frame *out_frame, const AVFrame *frame, const pl_tex *tex);
void pl_unmap_avframe_simple(pl_gpu gpu, pl_frame *frame);

#ifdef __cplusplus
}
#endif

#endif // PL_LIBAV_SHIM_H

