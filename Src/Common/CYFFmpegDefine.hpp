/*
 * CYPlayer License
 * -----------
 *
 * CYPlayer is licensed under the terms of the MIT license reproduced below.
 * This means that CYPlayer is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 *
 *
 * ===============================================================================
 *
 * Copyright (C) 2023-2026 ShiLiang.Hao <newhaosl@163.com>, foobra<vipgs99@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * ===============================================================================
 */
 /*
  * AUTHORS:  ShiLiang.Hao <newhaosl@163.com>, foobra<vipgs99@gmail.com>
  * VERSION:  1.0.0
  * PURPOSE:  Cross-platform efficient all-round player SDK.
  * CREATION: 2025.04.23
  * LCHANGE:  2025.04.23
  * LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
  */


#ifndef __CY_FFMPEG_DEFINE_HPP__
#define __CY_FFMPEG_DEFINE_HPP__

#ifdef DEBUG_MEM_CHECK
#include <vld.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#include "libavutil/avstring.h"
#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/mem.h"
#include "libavutil/pixdesc.h"
#include "libavutil/dict.h"
#include "libavutil/fifo.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavutil/bprint.h"
#include "libavutil/error.h"
#include "libavutil/audio_fifo.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavutil/tx.h"
#include "libswresample/swresample.h"

#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"

#include <SDL/SDL.h>

#ifdef __cplusplus
}
#endif

#include <assert.h>
#include <memory>

#if (SDL_VERSION_ATLEAST(2, 0, 6) && CONFIG_LIBPLACEBO)
/* Get PL_API_VER */
#include <libplacebo/config.h>
#define HAVE_VULKAN_RENDERER (PL_API_VER >= 278)
#else
#define HAVE_VULKAN_RENDERER 0
#endif

template<typename T>
void NoFree(T* p)
{
}

template<typename T>
static inline void AVFreeP(T** p)
{
    av_freep(reinterpret_cast<void*>(p));
}

static inline void AVCodecFreeContext(AVCodecContext** pCodecCtx)
{
    AVCodecContext* pCtx = *pCodecCtx;
    if (pCtx->hw_device_ctx)
    {
        av_buffer_unref(&pCtx->hw_device_ctx);
        pCtx->hw_device_ctx = nullptr;
    }
    avcodec_free_context(pCodecCtx);
}

template <typename T, void (*Fn)(T*)>
struct PointerDel
{
    inline void operator()(T* p) const
    {
        if (p)
            Fn(p);
    }
};

template <typename T, void (*Fn)(T**)>
struct PointerDel2
{
    inline void operator()(T* p) const
    {
        if (p)
            Fn(&p);
    }
};

template <typename T, int (*Fn)(T*)>
struct PointerDel3
{
    inline void operator()(T* p) const
    {
        if (p)
            Fn(p);
    }
};

template<typename T = void>
using AVFreePtr = std::unique_ptr<T, PointerDel2<T, AVFreeP<T>>>;

template<typename T = void>
using AVNoFreePtr = std::unique_ptr<T, PointerDel<T, NoFree<T>>>;

using AVStreamPtr = AVNoFreePtr<AVStream>;

using AVFramePtr = std::unique_ptr<AVFrame, PointerDel2<AVFrame, av_frame_free>>;
static inline auto AVFramePtrCreate()
{
    return AVFramePtr(av_frame_alloc());
}

using AVPacketPtr = std::unique_ptr<AVPacket, PointerDel2<AVPacket, av_packet_free>>;
static inline auto AVPacketPtrCreate()
{
    return AVPacketPtr(av_packet_alloc());
}

using SwsContextPtr = std::unique_ptr<SwsContext, PointerDel<SwsContext, sws_freeContext>>;

using SwrContextPtr = std::unique_ptr<SwrContext, PointerDel2<SwrContext, swr_free>>;
static inline auto SwrContextPtrCreate()
{
    return SwrContextPtr(swr_alloc());
}

using AVFormatAllocContextPtr = std::unique_ptr<AVFormatContext, PointerDel<AVFormatContext, avformat_free_context>>;
static inline auto AVFormatAllocContextPtrCreate(const char* pszFileName,
    AVOutputFormat* oformat = nullptr,
    const char* format_name = nullptr)
{
    AVFormatContext* ctx = nullptr;
    avformat_alloc_output_context2(&ctx, oformat, format_name, pszFileName);
    return AVFormatAllocContextPtr(ctx);
}

using AVFormatOpenContextPtr = std::unique_ptr<AVFormatContext, PointerDel2<AVFormatContext, avformat_close_input>>;

static inline auto AVFormatOpenContextPtrCreate(const char* url,
    AVInputFormat* fmt = nullptr,
    AVDictionary** options = nullptr)
{
    AVFormatContext* ifmtCtx = nullptr;
    int result = avformat_open_input(&ifmtCtx, url, fmt, options);
    if (result < 0)
    {
        char* errorMsg = new char[100];
        av_strerror(result, errorMsg, 100);
        //LOG_ERROR("AVFormatOpenContextPtrCreate failed %s,result= %d,errorMsg:%s", url, result, errorMsg);
        return AVFormatOpenContextPtr();
    }
    return AVFormatOpenContextPtr(ifmtCtx);
}

using AVCodecContextPtr = std::unique_ptr<AVCodecContext, PointerDel2<AVCodecContext, AVCodecFreeContext>>;
static inline auto AVCodecContextPtrCreate(AVCodec* codec)
{
    return AVCodecContextPtr(avcodec_alloc_context3(codec));
}

using AVDictionaryPtr = std::unique_ptr<AVDictionary, PointerDel2<AVDictionary, av_dict_free>>;
using AVAudioFifoPtr = std::unique_ptr<AVAudioFifo, PointerDel<AVAudioFifo, av_audio_fifo_free>>;

static inline auto AVAudioFifoPtrCreate(enum AVSampleFormat objSampleFmt, int nChannels, int nNBSamples)
{
    return AVAudioFifoPtr(av_audio_fifo_alloc(objSampleFmt, nChannels, nNBSamples));
}

using AVFilterGraphPtr = std::unique_ptr<AVFilterGraph, PointerDel2<AVFilterGraph, avfilter_graph_free>>;
static inline auto AVFilterGraphPtrCreate()
{
    return AVFilterGraphPtr(avfilter_graph_alloc());
}

using AVFilterContextPtr = AVNoFreePtr<AVFilterContext>;
static inline auto AVFilterContextPtrCreate(const AVFilter* pFilt, const char* pszName, const char* pArgs, void* pOpaque, AVFilterGraph* pGraphCtx)
{
    AVFilterContext* ctx = nullptr;
    const auto       ret = avfilter_graph_create_filter(&ctx, pFilt, pszName, pArgs, pOpaque, pGraphCtx);
    if (ret < 0)
    {
        //LOG_ERROR("AVFilterContextPtrCreate failed name:%s, args %s", name, args);
    }
    return AVFilterContextPtr(ctx);
}

using AVIOContextExPtr = std::unique_ptr<AVIOContext*, PointerDel3<AVIOContext*, avio_closep>>;
using AVBufferRefPtr = std::unique_ptr<AVBufferRef, PointerDel2<AVBufferRef, av_buffer_unref>>;

using AVCodecParametersPtr = std::unique_ptr<AVCodecParameters, PointerDel2<AVCodecParameters, avcodec_parameters_free>>;
static inline auto AVCodecParametersPtrCreate()
{
    return AVCodecParametersPtr(avcodec_parameters_alloc());
}

static inline AVFramePtr AVCreateAudioFrame(enum AVSampleFormat objSampleFmt, AVChannelLayout objChLayout, int nSampleRate, int nNBSamples)
{
    AVFramePtr ptrFrame = AVFramePtrCreate();

    if (!ptrFrame)
    {
        //LOG_ERROR("Error allocating an audio frame");
        assert(0);
    }

    ptrFrame->format = objSampleFmt;
    ptrFrame->ch_layout = objChLayout;
    ptrFrame->sample_rate = nSampleRate;
    ptrFrame->nb_samples = nNBSamples;

    if (nNBSamples)
    {
        const int ret = av_frame_get_buffer(ptrFrame.get(), 0);
        if (ret < 0)
        {
            //LOG_ERROR("Error allocating an audio buffer");
            assert(0);
        }
    }
    return ptrFrame;
}

template <typename T, void (*Fn)(T*)>
struct PointerChannelDel
{
    inline void operator()(T* p) const
    {
        if (p)
        {
            Fn(p);
            delete p;
        }
            
    }
};

using AVChannelLayoutPtr = std::unique_ptr<AVChannelLayout, PointerChannelDel<AVChannelLayout, av_channel_layout_uninit>>;
using AVFormatContextPtr = std::unique_ptr<AVFormatContext, PointerDel2<AVFormatContext, avformat_close_input>>;

static inline auto AVFormatContextPtrCreate()
{
    return AVFormatContextPtr(avformat_alloc_context());
}

struct VkRenderer
{
    const AVClass* cls;

    int (*create)(VkRenderer* renderer, SDL_Window* window, AVDictionary* dict);

    int (*get_hw_dev)(VkRenderer* renderer, AVBufferRef** dev);

    int (*display)(VkRenderer* renderer, AVFrame* frame);

    int (*resize)(VkRenderer* renderer, int width, int height);

    void (*destroy)(VkRenderer* renderer);
};

#if HAVE_VULKAN_RENDERER
using VkRendererPtr = std::unique_ptr<VkRenderer, PointerDel<VkRenderer, vk_renderer_destroy>>;

static inline VkRendererPtr CreateVkRender()
{
    return VkRendererPtr(vk_get_renderer());
}
#endif

using SDLWindowPtr = std::unique_ptr<SDL_Window, PointerDel<SDL_Window, SDL_DestroyWindow>>;
using SDLRendererPtr = std::unique_ptr<SDL_Renderer, PointerDel<SDL_Renderer, SDL_DestroyRenderer>>;
using AVFilterGraphPtr = std::unique_ptr<AVFilterGraph, PointerDel2<AVFilterGraph, avfilter_graph_free>>;

static inline AVFilterGraphPtr CreateAVFilterGraph()
{
    return AVFilterGraphPtr(avfilter_graph_alloc());
}

//////////////////////////////////////////////////////////////////////////
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* Step size for volume control in dB */
#define SDL_VOLUME_STEP (0.75)

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* external clock speed adjustment constants for bRealTime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

#define CURSOR_HIDE_DELAY 1000000

#define USE_ONEPASS_SUBTITLE_RENDER 1

#define FILTER_NB_THREADS  0

#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct CYFrame
{
    AVFrame* pFrame = nullptr;
    AVSubtitle sub = {};
    int serial = 0;
    double pts = 0;           /* presentation timestamp for the frame */
    double duration = 0;      /* estimated duration of the frame */
    int64_t pos = 0;          /* byte position of the frame in the input file */
    int width = 0;
    int height = 0;
    int format = 0;
    AVRational sar = {};
    int uploaded = 0;
    int flip_v = 0;
} CYFrame;

typedef struct CYAudioParams
{
    int freq = 0;
    AVChannelLayout ch_layout = {};
    enum AVSampleFormat fmt = {};
    int frame_size = 0;
    int bytes_per_sec = 0;
} CYAudioParams;

typedef struct FrameData
{
    int64_t pkt_pos = 0;
} FrameData;

#endif // __CY_FFMPEG_DEFINE_HPP__