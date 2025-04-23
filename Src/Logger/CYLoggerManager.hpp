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


#ifndef __CY_LOGGER_MANAGER_HPP__
#define __CY_LOGGER_MANAGER_HPP__

#include "CYPlayerPrivDefine.hpp"
#include "Common/Exception/CYException.hpp"

#include <functional>

CYPLAYER_NAMESPACE_BEGIN

class CYLoggerManager
{
public:
    CYLoggerManager();
    virtual ~CYLoggerManager();

public:
    static SharePtr<CYLoggerManager> GetInstance();
    static void FreeInstance();

    void RegisterLogCallBack(FunLogCallBack callBack);

    /**
     * @brief Write Log.
    */
    bool WriteLog(ELogType eType, const TChar* szFile /*= __TFILE__*/, const TChar* szLocation /*=__TFUNCTION__*/, int nLine /*= __TLINE__*/, const TChar* pszMsg, ...);

private:
    static SharePtr<CYLoggerManager> m_ptrInstance;
    std::function<void(ELogType eType, const char* pszMsg, const char* pszFile, const char* pszLocation, int nLine)> m_funLogCallBack;
};

CYPLAYER_NAMESPACE_END

#define LoggerManager()         CYPLAYER_NAMESPACE::CYLoggerManager::GetInstance()
#define LoggerManager_Free()    CYPLAYER_NAMESPACE::CYLoggerManager::FreeInstance()

#define ExceptionLog(e)		    LoggerManager()->WriteLog(TYPE_LOG_ERROR, __TFILE__, __TFUNCTION__, __TLINE__, CYPLAYER_NAMESPACE::AtoT(e))

#define EXCEPTION_BEGIN 	    { UniquePtr<CYPLAYER_NAMESPACE::CYBaseException> excp;  try

#define EXCEPTION_END		    catch (CYPLAYER_NAMESPACE::CYBaseException* e) {	excp.reset(e); ExceptionLog(excp->what()); } \
							    catch (...) { ExceptionLog("Unknown exception!"); } }

#define EXCEPTION_END_C(fun)	catch (CYPLAYER_NAMESPACE::CYBaseException* e) { fun(); excp.reset(e); ExceptionLog(excp->what()); } \
							    catch (...) { fun(); ExceptionLog("Unknown exception!"); } }

#define CY_LOG_TRACE(szMsg, ...)            LoggerManager()->WriteLog(TYPE_LOG_ERROR,  __TFILE__, __TFUNCTION__, __TLINE__, CYPLAYER_NAMESPACE::AtoT(szMsg), ##__VA_ARGS__)
#define CY_LOG_DEBUG(szMsg, ...)            LoggerManager()->WriteLog(TYPE_LOG_ERROR,  __TFILE__, __TFUNCTION__, __TLINE__, CYPLAYER_NAMESPACE::AtoT(szMsg), ##__VA_ARGS__)
#define CY_LOG_INFO(szMsg, ...)             LoggerManager()->WriteLog(TYPE_LOG_ERROR,  __TFILE__, __TFUNCTION__, __TLINE__, CYPLAYER_NAMESPACE::AtoT(szMsg), ##__VA_ARGS__)
#define CY_LOG_WARN(szMsg, ...)             LoggerManager()->WriteLog(TYPE_LOG_ERROR,  __TFILE__, __TFUNCTION__, __TLINE__, CYPLAYER_NAMESPACE::AtoT(szMsg), ##__VA_ARGS__)
#define CY_LOG_ERROR(szMsg, ...)            LoggerManager()->WriteLog(TYPE_LOG_ERROR,  __TFILE__, __TFUNCTION__, __TLINE__, CYPLAYER_NAMESPACE::AtoT(szMsg), ##__VA_ARGS__)
#define CY_LOG_FATAL(szMsg, ...)            LoggerManager()->WriteLog(TYPE_LOG_ERROR,  __TFILE__, __TFUNCTION__, __TLINE__, CYPLAYER_NAMESPACE::AtoT(szMsg), ##__VA_ARGS__)

#endif // __CY_LOGGER_MANAGER_HPP__