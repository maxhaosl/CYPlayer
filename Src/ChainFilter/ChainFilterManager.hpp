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


#ifndef __CHAIN_FILTER_MANAGER_HPP__
#define __CHAIN_FILTER_MANAGER_HPP__

#include "CYPlayerPrivDefine.hpp"
#include "Common/CYFFmpegDefine.hpp"

CYPLAYER_NAMESPACE_BEGIN

class CYBaseFilter;
class CYMediaContext;
class CYFilterController;
class CChainFilterManager
{
public:
    CChainFilterManager();
    virtual ~CChainFilterManager();

public:
    /**
     * Initialize.
     */
    virtual int16_t Init(EPlayerParam* pParam);
    virtual int16_t UnInit();

    /**
      * Set Render Window.
      */
    virtual int16_t SetWindow(void* hWnd);

    /**
     * Open Media File.
     */
    virtual int16_t Open(const char* pszURL, EPlayerMediaParam* pParam);

    /**
     * Playback control.
     */
    virtual int16_t  Play();
    virtual int16_t  Pause(bool* bPaused);
    virtual int16_t  Stop();

    /**
     * Set Display Size.
     */
    virtual int16_t SetDisplaySize(int32_t nWidth, int32_t nHeight);

    /**
     * Player State
     */
    virtual EStateType GetState() const;
    virtual int64_t GetDuration() const;
    virtual int64_t GetPosition() const;

    /**
     * Jump to the specified position (seconds).
     */
    virtual int16_t Seek(int64_t nTimestamp);
    virtual int16_t SetMute(bool bMute);
    virtual int16_t SetLoop(bool bLoop);
    virtual int16_t SetSpeed(float fSpeed);

    /**
     * Volume control.0.0 ~ 1.0
     */
    virtual int16_t SetVolume(float fVolume);
    virtual float GetVolume();

    // Video settings
    virtual int16_t SetVideoScale(EVideoScaleType eScale);
    virtual int16_t SetVideoRotation(ERotationType eRotation);
    virtual int16_t SetVideoMirror(bool bMirror);
    virtual int16_t SetAspectRatio(float fRatio);

    /**
     * Event callback settings.
     */
    virtual int16_t SetEventCallback(FunEventCallback callback);
    virtual int16_t SetStateCallback(FunStateCallBack callback);
    virtual int16_t SetPositionCallback(FunPositionCallBack callback);

private:
    int16_t InitFFmpeg();
    int16_t UnInitFFmpeg();

    int16_t SetInputFormat(const char* pszFormat);
    int16_t ResetContext(SharePtr<CYMediaContext>& ptrContext);
private:
    /**
     * Chain Filter.
     */
    SharePtr<CYBaseFilter> m_ptrSourceFilter;
    SharePtr<CYBaseFilter> m_ptrDemuxFilter;
    SharePtr<CYBaseFilter> m_ptrAudioDecodeFilter;
    SharePtr<CYBaseFilter> m_ptrVideoDecodeFilter;
    SharePtr<CYBaseFilter> m_ptrSubTitleDecodeFilter;
    SharePtr<CYBaseFilter> m_ptrProcessFilter;
    SharePtr<CYBaseFilter> m_ptrAudioRenderFilter;
    SharePtr<CYBaseFilter> m_ptrVideoRenderFilter;

    SharePtr<CYBaseFilter> m_ptrFirstFilter;
    SharePtr<CYBaseFilter> m_ptrLastFilter;

    SharePtr<EPlayerParam> m_ptrParam;
    SharePtr<CYMediaContext> m_ptrContext;
    const AVInputFormat* m_pFileInputFormat = nullptr;

    FunEventCallback m_funEventCallBack;
    FunStateCallBack m_funStateCallBack;
    FunPositionCallBack m_funPositionCallBack;
    EStateType m_eStateType = TYPE_STATUS_IDLE;
};

CYPLAYER_NAMESPACE_END

#endif //__CHAIN_FILTER_MANAGER_HPP__