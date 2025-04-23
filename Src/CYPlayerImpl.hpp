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
 * Copyright (C) 2023-2026 ShiLiang.Hao <newhaosl@163.com>
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
  * AUTHORS:  ShiLiang.Hao <newhaosl@163.com>
  * VERSION:  1.0.0
  * PURPOSE:  Cross-platform efficient all-round player SDK.
  * CREATION: 2025.04.23
  * LCHANGE:  2025.04.23
  * LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
  */


#ifndef __I_CYPLAYER_IMPL_HPP__
#define __I_CYPLAYER_IMPL_HPP__

#include "CYPlayer/ICYPlayer.hpp"
#include "CYPlayerPrivDefine.hpp"

CYPLAYER_NAMESPACE_BEGIN

class CChainFilterManager;
class CYPlayerImpl : public ICYPlayer
{
public:
    CYPlayerImpl();
    virtual ~CYPlayerImpl();

public:
    /**
     * Initialize.
     */
    virtual int16_t Init(EPlayerParam* pParam) override;
    virtual int16_t UnInit() override;

    /**
     * Set Render Window.
     */
    virtual int16_t SetWindow(void* hWnd) override;

    /**
     * Open Media File.
     */
    virtual int16_t Open(const char* pszURL, EPlayerMediaParam* pParam) override;

    /**
     * Playback control.
     */
    virtual int16_t  Play() override;
    virtual int16_t  Pause(bool* bPaused) override;
    virtual int16_t  Stop() override;

    /**
     * Player State
     */
    virtual EStateType GetState() const override;
    virtual int64_t GetDuration() const override;
    virtual int64_t GetPosition() const override;

    /**
     * Jump to the specified position (seconds).
     */
    virtual int16_t Seek(int64_t nTimestamp) override;
    virtual int16_t SetMute(bool bMute) override;
    virtual int16_t SetLoop(bool bLoop) override;
    virtual int16_t SetSpeed(float fSpeed) override;

    /**
     * Volume control.0.0 ~ 1.0
     */
    virtual int16_t SetVolume(float fVolume) override;
    virtual float GetVolume() override;

    // Video settings
    virtual int16_t SetVideoScale(EVideoScaleType eScale) override;
    virtual int16_t SetVideoRotation(ERotationType eRotation) override;
    virtual int16_t SetVideoMirror(bool bMirror) override;
    virtual int16_t SetAspectRatio(float fRatio) override;

    /**
     * Event callback settings.
     */
    virtual int16_t SetEventCallback(FunEventCallback callback) override;
    virtual int16_t SetStateCallback(FunStateCallBack callback) override;
    virtual int16_t SetPositionCallback(FunPositionCallBack callback) override;
    virtual int16_t SetLogCallBack(FunLogCallBack callback) override;

private:
    UniquePtr<CChainFilterManager> m_ptrChainFilterManager;
};

CYPLAYER_NAMESPACE_END

#endif // __I_CYPLAYER_IMPL_HPP__