#include <string.h>
#include <libplacebo/utils/libav.h>
#include "pl_libav_shim.h"

bool pl_map_avframe_simple(pl_gpu gpu, pl_frame *out_frame, const AVFrame *frame, const pl_tex *tex)
{
    struct pl_avframe_params params;
    memset(&params, 0, sizeof(params));
    params.frame = frame;
    params.tex = tex;
    return pl_map_avframe_ex(gpu, out_frame, &params);
}

void pl_unmap_avframe_simple(pl_gpu gpu, pl_frame *frame)
{
    pl_unmap_avframe(gpu, frame);
}

