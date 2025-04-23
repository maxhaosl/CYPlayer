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


#ifndef __CY_VIDEO_RENDER_FILTER_HPP__
#define __CY_VIDEO_RENDER_FILTER_HPP__

#include "ChainFilter/Common/CYBaseFilter.hpp"
#include "Common/CYFFmpegDefine.hpp"

CYPLAYER_NAMESPACE_BEGIN

class CYVideoRenderFilter : public CYBaseFilter
{
public:
    CYVideoRenderFilter(EVideoRenderType eVideoType);
    virtual ~CYVideoRenderFilter();

public:
    virtual int16_t Init(SharePtr<EPlayerParam>& ptrParam) override;
    virtual int16_t UnInit() override;

    virtual int16_t Start(SharePtr<CYMediaContext>& ptrContext) override;
    virtual int16_t Stop(SharePtr<CYMediaContext>& ptrContext) override;

    virtual int16_t Pause() override;
    virtual int16_t Resume() override;

    virtual int16_t ProcessPacket(SharePtr<CYMediaContext>& ptrContext, AVPacketPtr& ptrPacket) override;
    virtual int16_t ProcessFrame(SharePtr<CYMediaContext>& ptrContext, AVFramePtr& ptrFrame) override;

    /**
     * Set Render Window.
     */
    virtual int16_t SetWindow(void* hWnd);

    // Video settings
    virtual int16_t SetVideoScale(EVideoScaleType eScale);
    virtual int16_t SetVideoRotation(ERotationType eRotation);
    virtual int16_t SetVideoMirror(bool bMirror);
    virtual int16_t SetAspectRatio(float fRatio);
    virtual int16_t SetMute(bool bMute);

private:
    void OnEntry();
    void RefreshLoopWaitEvent(SharePtr<CYMediaContext>& ptrContext, SDL_Event* pEvent);
    void VideoRefresh(double* pfRemainingTime);
    int GetMasterSyncType(SharePtr<CYMediaContext>& ptrContext);
    void CheckExternalClockSpeed(SharePtr<CYMediaContext>& ptrContext);
    void VideoDisplay(SharePtr<CYMediaContext>& ptrContext);
    int VideoOpen(SharePtr<CYMediaContext>& ptrContext);
    void VideoAudioDisplay(SharePtr<CYMediaContext>& ptrContext);
    int ComputeMod(int a, int b);
    void FillRectangle(int x, int y, int w, int h);
    int ReallocTexture(SDL_Texture** pTexture, Uint32 nNewFormat, int nNewWidth, int nNewHeight, SDL_BlendMode eBlendMode, int nInitTexture);
    void VideoImageDisplay(SharePtr<CYMediaContext>& ptrContext);
    void CalculateDisplayRect(SDL_Rect* pRect, int nScrXleft, int nScrYtop, int nScrWidth, int nScrHeight, int nPicWidth, int nPicHeight, AVRational objPicSar);
    int UploadTexture(SDL_Texture** pTex, AVFrame* pFrame);
    double VPDuration(SharePtr<CYMediaContext>& ptrContext, CYFrame* pVP, CYFrame* pNextVP);
    double ComputeTargetDelay(double delay, SharePtr<CYMediaContext>& ptrContext);
    void UpdateVideoPTS(SharePtr<CYMediaContext>& ptrContext, double fPts, int nSerial);
    double GetMasterClock(SharePtr<CYMediaContext>& ptrContext);
    void StreamTogglePause(SharePtr<CYMediaContext>& ptrContext);
    void DoExit(SharePtr<CYMediaContext>& ptrContext);
    void StreamCycleChannel(SharePtr<CYMediaContext>& ptrContext, int nCodecType);

private:
    int  m_nSDLFlag = 0;
    bool m_bAlwaysOnTop = false;
    bool m_bBorderLess = true;

    bool  m_bEnableVulkan = false;
    char* m_pszVulkanParams = nullptr;

    char  m_szHWaccel[512] = { 0 };

    bool m_bDisableAudio = false;
    bool m_bDisableVideo = false;
    bool m_bDisableSubTitle = false;

    SDLWindowPtr m_ptrWindow;
    SDLRendererPtr m_ptrRenderer;
    SDL_RendererInfo m_objRendererInfo = { 0 };
#if HAVE_VULKAN_RENDERER
    VkRendererPtr m_ptrVKRenderer;
#endif

    int64_t m_nLastMircoSecond = 0;
    int m_nScreenWidth = 0;
    int m_nScreenHeight = 0;

    std::mutex m_mutex;
    std::thread m_thread;
    SharePtr<EPlayerParam> m_ptrParam;
    SharePtr<CYMediaContext> m_ptrContext;
};

CYPLAYER_NAMESPACE_END

#endif // __CY_VIDEO_RENDER_FILTER_HPP__