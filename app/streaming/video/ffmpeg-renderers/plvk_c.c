// This compilation unit contains the implementations of libplacebo header-only libraries.
// These must be compiled as C code, so they cannot be placed inside plvk.cpp.

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4068) // unknown pragma
#pragma warning(disable: 4244) // double -> float truncation warning
#pragma warning(disable: 4267) // size_t -> int truncation warning
#endif

#define PL_LIBAV_IMPLEMENTATION 1
#include <libplacebo/utils/libav.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Provide a dummy implementation of av_stream_get_side_data() to avoid having to link with libavformat
// Use size_t for compatibility with newer FFmpeg versions on some platforms
#if defined(LIBAVFORMAT_VERSION_INT) && LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(61, 0, 0)
uint8_t *av_stream_get_side_data(const AVStream *stream, enum AVPacketSideDataType type, size_t *size)
#else
uint8_t *av_stream_get_side_data(const AVStream *stream, enum AVPacketSideDataType type, int *size)
#endif
{
    (void)stream;
    (void)type;
    if (size) *size = 0;
    return NULL;
}
