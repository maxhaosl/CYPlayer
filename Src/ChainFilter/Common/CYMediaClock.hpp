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


#ifndef __CY_MEDIA_CLOCK_HPP__
#define __CY_MEDIA_CLOCK_HPP__

#include "CYPlayerPrivDefine.hpp"

CYPLAYER_NAMESPACE_BEGIN

class CYMediaClock
{
public:
    CYMediaClock();
    virtual ~CYMediaClock();

public:
    void InitClock(int* nQueueSerial);
    double GetClock();
    void SetClockAt(double fPts, int nSerial, double fTime);
    void SetClock(double fPts, int nSerial);
    void SetClockSpeed(double fSpeed);
    void SyncClockToSlave(CYMediaClock& objSlave);

public:
    double m_fPTS = 0;           /* clock base */
    double m_fPTSDrift = 0;     /* clock base minus time at which we updated the clock */
    double m_fLastUpdated = 0;
    double m_fSpeed = 0;
    int m_fSerial = 0;           /* clock is based on a packet with this serial */
    bool m_bPaused = 0;
    int* m_pQueueSerial = nullptr;    /* pointer to the current packet queue serial, used for obsolete clock detection */
};

CYPLAYER_NAMESPACE_END

#endif // !__CY_MEDIA_CLOCK_HPP__
