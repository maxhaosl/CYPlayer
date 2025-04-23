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


#ifndef __CY_TIMESTAMPS_HPP__
#define __CY_TIMESTAMPS_HPP__

#include "CYPlayerPrivDefine.hpp"

CYPLAYER_NAMESPACE_BEGIN

class CYTimeStamps final
{
public:
    /**
    * Creates using a current time
    */
    CYTimeStamps();

    /**
     * @brief Destructor.
    */
    virtual ~CYTimeStamps();

    /**
    * Sets itself to be a current time
    */
    void SetTime();

    /**
     * @brief Set Offset second.
    */
    void SetOffsetTime(int nOffsetSec);

    /**
     * @brief Get Time
    */
    const int64_t GetTime() const;

    /**
     * @brief Get Time
    */
    const TString GetTimeStr() const;

    /**
    * Formats the timestamp - YYYYMMDD hh:mm:ss:nnnnnn where nnnnnn is microseconds.
    * @returns reference to a formatted time stamp
    */
    const TString ToString() const;

    /**
     * @brief Get Date Value.
    */
    int GetYY()
    {
        return m_nYY;
    }
    int GetMM()
    {
        return m_nMM;
    }
    int GetDD()
    {
        return m_nDD;
    }
    int GetHR()
    {
        return m_nHR;
    }
    int GetMN()
    {
        return m_nMN;
    }
    int GetSC()
    {
        return m_nSC;
    }
    int GetMMN()
    {
        return m_nMMN;
    }

private:
    /**
     * @brief Set Local Time.
    */
    inline void SetLocalTimeData();

private:
    int				m_nYY;		// Year
    int				m_nMM;		// Month
    int				m_nDD;		// Day
    int				m_nHR;		// Hour
    int				m_nMN;		// Minute
    int				m_nSC;		// Second
    int				m_nMMN;		// MicroSecond

    mutable TString m_strTime;
};

CYPLAYER_NAMESPACE_END

#endif // __CY_TIMESTAMPS_HPP__