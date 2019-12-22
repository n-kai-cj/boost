/******************************************************************************\
Copyright (c) 2019 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
\**********************************************************************************/

#ifndef __INTELQSVDECODER_H__
#define __INTELQSVDECODER_H__

#ifdef _WIN32
#define INTEL_VIDEO_DECODER_API __declspec(dllexport)
#elif __linux__
#define INTEL_VIDEO_DECODER_API
#endif

#include "intel_video_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif

    // =================================================================
    IntelQsvH264Decoder* dec;

    INTEL_VIDEO_DECODER_API int initialize();
    INTEL_VIDEO_DECODER_API void uninitialize();
    INTEL_VIDEO_DECODER_API int decodeHeader(uint8_t* frame, size_t length);
    INTEL_VIDEO_DECODER_API int decode(uint8_t* in, size_t in_length);
    INTEL_VIDEO_DECODER_API int decode_get(uint8_t* in, size_t in_length, uint8_t* out, int conv_opt);
    INTEL_VIDEO_DECODER_API int getFrame(uint8_t* out, int conv_opt);
    INTEL_VIDEO_DECODER_API int drainFrame(uint8_t* out, int conv_opt);
    INTEL_VIDEO_DECODER_API int getWidth();
    INTEL_VIDEO_DECODER_API int getHeight();
    INTEL_VIDEO_DECODER_API bool isInit();
    // =================================================================


    // =================================================================
    std::vector<IntelQsvH264Decoder*> decodes;
    std::mutex mutex;

    INTEL_VIDEO_DECODER_API int newInstance();
    INTEL_VIDEO_DECODER_API int m_initialize(int i);
    INTEL_VIDEO_DECODER_API void m_uninitialize(int i);
    INTEL_VIDEO_DECODER_API int m_decodeHeader(int i, uint8_t* frame, size_t length);
    INTEL_VIDEO_DECODER_API int m_decode(int i, uint8_t* in, size_t in_length);
    INTEL_VIDEO_DECODER_API int m_decode_get(int i, uint8_t* in, size_t in_length, uint8_t* out, int conv_opt);
    INTEL_VIDEO_DECODER_API int m_getFrame(int i, uint8_t* out, int conv_opt);
    INTEL_VIDEO_DECODER_API int m_drainFrame(int i, uint8_t* out, int conv_opt);
    INTEL_VIDEO_DECODER_API int m_getWidth(int i);
    INTEL_VIDEO_DECODER_API int m_getHeight(int i);
    INTEL_VIDEO_DECODER_API bool m_isInit(int i);
    // =================================================================


#ifdef __cplusplus
}
#endif

#endif /* __INTELQSVDECODER_H__ */
