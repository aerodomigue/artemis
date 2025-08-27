#include <string.h>
#include <libplacebo/utils/libav.h>
#include "pl_libav_shim.h"

bool pl_map_avframe_simple(void *gpu, void *out_frame, const AVFrame *frame, const void *tex)
{
    struct pl_avframe_params params;
    memset(&params, 0, sizeof(params));
    params.frame = frame;
    params.tex = (const pl_tex *)tex;
    return pl_map_avframe_ex((pl_gpu)gpu, (pl_frame *)out_frame, &params);
}

void pl_unmap_avframe_simple(void *gpu, void *frame)
{
    pl_unmap_avframe((pl_gpu)gpu, (pl_frame *)frame);
}

