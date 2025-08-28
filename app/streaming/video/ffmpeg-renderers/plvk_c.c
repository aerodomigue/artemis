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
// Ensure we get FFmpeg version macros for accurate prototype matching
#include <libavformat/avformat.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Provide a dummy implementation of av_stream_get_side_data() to avoid having to link with libavformat
// Match the header's declaration exactly based on libavformat major version
#if defined(LIBAVFORMAT_VERSION_MAJOR) && (LIBAVFORMAT_VERSION_MAJOR >= 61)
uint8_t *av_stream_get_side_data(const AVStream *stream, enum AVPacketSideDataType type, size_t *size)
#else
uint8_t *av_stream_get_side_data(const AVStream *stream, enum AVPacketSideDataType type, int *size)
#endif
{
    (void)stream;
    (void)type;
#if defined(LIBAVFORMAT_VERSION_MAJOR) && (LIBAVFORMAT_VERSION_MAJOR >= 61)
    if (size) *size = 0;
#else
    if (size) *size = 0;
#endif
    return NULL;
}
