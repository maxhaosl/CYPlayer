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


#ifndef __CY_VIDEO_DECODE_FILTER_HPP__
#define __CY_VIDEO_DECODE_FILTER_HPP__

#include "ChainFilter/Common/CYBaseFilter.hpp"

CYPLAYER_NAMESPACE_BEGIN

class CYVideoDecodeFilter : public CYBaseFilter
{
public:
    CYVideoDecodeFilter();
    virtual ~CYVideoDecodeFilter();

public:
    virtual int16_t Init(SharePtr<EPlayerParam>& ptrParam) override;
    virtual int16_t UnInit() override;

    virtual int16_t Start(SharePtr<CYMediaContext>& ptrContext) override;
    virtual int16_t Stop(SharePtr<CYMediaContext>& ptrContext) override;

    virtual int16_t Pause() override;
    virtual int16_t Resume() override;

    virtual int16_t ProcessPacket(SharePtr<CYMediaContext>& ptrContext, AVPacketPtr& ptrPacket) override;
    virtual int16_t ProcessFrame(SharePtr<CYMediaContext>& ptrContext, AVFramePtr& ptrFrame) override;
    
private:
    void OnEntry();
    int GetVideoFrame(AVFrame* frame);
    int QueuePicture(AVFrame* pSrcFrame, double pts, double duration, int64_t pos, int serial);
    int GetMasterSyncType(SharePtr<CYMediaContext>& ptrContext);

private:
    std::thread m_thread;
    SharePtr<EPlayerParam> m_ptrParam;
    SharePtr<CYMediaContext> m_ptrContext;
};

CYPLAYER_NAMESPACE_END

#endif // __CY_VIDEO_DECODE_FILTER_HPP__