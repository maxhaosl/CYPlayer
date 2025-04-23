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


#ifndef __CY_MEDIA_CONTEXT_HPP__
#define __CY_MEDIA_CONTEXT_HPP__

#ifdef DEBUG_MEM_CHECK
#include <vld.h>
#endif

#include "CYPlayerPrivDefine.hpp"
#include "Common/CYFFmpegDefine.hpp"
#include "Common/Thread/CYCondition.hpp"
#include "Common/Queue/CYFrameQueue.hpp"
#include "Common/Queue/CYPacketQueue.hpp"
#include "ChainFilter/Common/CYMediaClock.hpp"
#include "ChainFilter/Common/CYDecoder.hpp"

CYPLAYER_NAMESPACE_BEGIN

enum ShowMode
{
    SHOW_MODE_NONE = -1,
    SHOW_MODE_VIDEO = 0,
    SHOW_MODE_WAVES,
    SHOW_MODE_RDFT,
    SHOW_MODE_NB
};

class CYMediaContext
{
public:
    CYMediaContext();
    virtual ~CYMediaContext();

public:
    const AVInputFormat* iformat = nullptr;

    std::mutex AbortMutex;
    bool bAbortRequest = false;
    bool bForceRefresh = false;
    bool bPaused = false;
    int nLastPaused = 0;
    int nQueueAttachmentsReq = 0;
    bool bSeekReq = false;
    bool bAccurate = false;
    int nSeekFlags = 0;
    int64_t nSeekPos = 0;
    int64_t nSeekRel = 0;
    int nReadPauseReturn = 0;
    AVFormatContextPtr ptrIC;

    bool bRealTime = false;

    CYMediaClock audclk;
    CYMediaClock vidclk;
    CYMediaClock extclk;

    CYFrameQueue pictq;
    CYFrameQueue subpq;
    CYFrameQueue sampq;

    CYDecoder auddec;
    CYDecoder viddec;
    CYDecoder subdec;

    int nAVSyncType = 0;

    double fAudioClock = 0;
    int nAudioClockSerial = 0;
    double fAudioDiffCum = 0; /* used for AV difference average computation */
    double fAudioDiffAvgCoef = 0;
    double fAudioDiffThreshold = 0;
    int nAudioDiffAvgCount = 0;

    int nAudioStreamIndex = 0;
    AVStream* pAudioStream = nullptr;
    SharePtr<CYPacketQueue> ptrAudioQueue;

    int nAudioHWBufSize = 0;
    AVNoFreePtr<uint8_t> ptrAudioBuffer;
    unsigned int nAudioBufSize = 0; /* in bytes */

    AVFreePtr<uint8_t> ptrAudioBuffer1;
    unsigned int nAudioBuf1Size = 0;
    int nAudioBufIndex = 0; /* in bytes */
    int nAudioWriteBufSize = 0;
    int nAudioVolume = 100;
    bool bMuted = false;
    struct CYAudioParams objAudioSrc = {};
    struct CYAudioParams objAudioFilterSrc = {};
    struct CYAudioParams objAudioTgt = {};
    SwrContextPtr ptrSwrCtx;
    int nFrameDropsEarly = 0;
    int nFrameDropsLate = 0;

    enum ShowMode eShowMode = SHOW_MODE_NONE;
    int16_t arraySample[SAMPLE_ARRAY_SIZE] = { 0 };
    int nSampleArrayIndex = 0;
    int nLastIStart = 0;
    AVTXContext* pRdftContext = nullptr;
    av_tx_fn funRdft = nullptr;
    int nRdftBits = 0;
    float* pfRealData = nullptr;
    AVComplexFloat* pRdftData = nullptr;
    int xpos = 0;
    double fLastVisTime = 0;
    SDL_Texture* vis_texture = nullptr;
    SDL_Texture* sub_texture = nullptr;
    SDL_Texture* vid_texture = nullptr;

    int nSubtitleStreamIndex = 0;
    AVStream* pSubTitleStream = nullptr;
    std::shared_ptr<CYPacketQueue> ptrSubTitleQueue;

    double fFrameTimer = 0;
    double fFrameLastReturnedTime = 0;
    double fFrameLastFilterDelay = 0;

    int nVideoStreamIndex = 0;
    AVStream* pVideoStream = nullptr;
    std::shared_ptr<CYPacketQueue> ptrVideoQueue;

    double fMaxFrameDuration = 0;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
    SwsContextPtr ptrSubConvertCtx;
    bool bEof = false;

    char* pszFileName = nullptr;
    int nShowWidth = 0;
    int nShowHeight = 0;
    int nShowXLeft = 0;
    int nShowYTop = 0;
    bool bStep = 0;

    int nVFilterIndex = 0;
    AVFilterContext* pInVideoFilter = nullptr;   // the first filter in the video chain
    AVFilterContext* pOutVideoFilter = nullptr;  // the last filter in the video chain

    AVFilterContext* pInAudioFilter = nullptr;   // the first filter in the audio chain
    AVFilterContext* pOutAudioFilter = nullptr;  // the last filter in the audio chain
    AVFilterGraphPtr ptrAgraph;        // audio filter graph

    int nLastVideoStream = 0;
    int nLastAudioStream = 0;
    int nLastSubTitleStream = 0;

    SharePtr<CYCondition> ptrReadCond;

    SDL_AudioDeviceID hAudioDev = 0;

    SDLWindowPtr ptrWindow;
    SDLRendererPtr ptrRenderer;
    SDL_RendererInfo objRendererInfo = { 0 };
#if HAVE_VULKAN_RENDERER
    VkRendererPtr ptrVKRenderer;
#endif

    int isFullScreen = 0;
    double fRdftSpeed = 0.02;
    int nShowStatus = -1;

    const char** pVfiltersList = nullptr;
    int nNBVFilters = 0;
    char* pAFilters = nullptr;

    int nDecoderReorderPTS = -1;
    int64_t nAudioCallBackTime = 0;
    //////////////////////////////////////////////////////////////////////////
    int nLowRes = 0;
    bool bAutoExit = false;
    int nLoop = 1;
    bool bLoop = false;
    int64_t nStartTime = AV_NOPTS_VALUE;
    int64_t nDuration = AV_NOPTS_VALUE;
    char szStreamSpec[AVMEDIA_TYPE_NB][256] = {0};

    bool bFastDecode = false;
    char szForceAudioCodecName[256] = { 0 };
    char szForceSubtitleCodecName[256] = { 0 };
    char szForceVideoCodecName[256] = { 0 };
    char szHWAccel[256] = { 0 };

    int nSampleRate = 0;
    int64_t nAudioCallbackTime = 0;
    AVChannelLayoutPtr ptrChLayout;

    int64_t nCursorLastShown = 0;
    bool bCursorHidden = false;

    bool bAutoRotate = true;
    int nScreenWidth = 0;
    int nScreenHeight = 0;
    int nDefaultWidth = 640;
    int nDefaultHeight = 480;

    int64_t nFileDuration = 0;
    FunEventCallback funEventCallBack = nullptr;
    FunStateCallBack funStateCallBack = nullptr;
    FunPositionCallBack funPositionCallBack = nullptr;
};

CYPLAYER_NAMESPACE_END

#endif // __CY_MEDIA_CONTEXT_HPP__