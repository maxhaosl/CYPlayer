#include "Logger/CYLoggerManager.hpp"
#include "Logger/CYDebugString.hpp"

#include "Common/CYCommonDefine.hpp"

#include <cstdarg>

CYPLAYER_NAMESPACE_BEGIN

SharePtr<CYLoggerManager> CYLoggerManager::m_ptrInstance;

CYLoggerManager::CYLoggerManager()
{
}

CYLoggerManager::~CYLoggerManager()
{
}

SharePtr<CYLoggerManager> CYLoggerManager::GetInstance()
{
    if (!m_ptrInstance)
    {
        m_ptrInstance = MakeShared<CYLoggerManager>();
    }
    return m_ptrInstance;
}

void CYLoggerManager::FreeInstance()
{
    m_ptrInstance.reset();
}

void CYLoggerManager::RegisterLogCallBack(FunLogCallBack callBack)
{
    m_funLogCallBack = callBack;
}

/**
 * @brief Write Log.
*/
bool CYLoggerManager::WriteLog(ELogType eType, const TChar* szFile /*= __TFILE__*/, const TChar* szLocation/* = TEXT("")*//*__TFUNCTION__*/, int nLine/* = __TLINE__*/, const TChar* pszMsg, ...)
{
    va_list args;
    va_start(args, pszMsg);
#ifdef _WIN32
    int iLen = cy_vscprintf(pszMsg, args) + 1;
#else
    int iLen = vsnprintf(nullptr, 0, pszMsg, args) + 1;
#endif
    int count = 0;

    TChar* pLogBuffer;
    pLogBuffer = reinterpret_cast<TChar*>(malloc(iLen * 2 + 2));
    while (!pLogBuffer && count++ < 10)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        pLogBuffer = reinterpret_cast<TChar*>(malloc(iLen * 2 + 2));
    }
    if (!pLogBuffer)
    {
        return false;
    }
    pLogBuffer[0] = '\0';
#ifdef _WIN32
    cy_vsnprintf_s(pLogBuffer, iLen+1, iLen+1, pszMsg, args);
#else
    va_start(args, pszMsg);
    vsnprintf(pLogBuffer, iLen + 1, pszMsg, args);
#endif
    va_end(args);

    std::shared_ptr<TChar> ptrLogString(pLogBuffer, [](TChar* p) { if (p)
    {
        free(p);
    } });


    if (m_funLogCallBack)
    {
        m_funLogCallBack(eType, ptrLogString.get(), szFile, szLocation, nLine);
    }
    else
    {
        DebugString(AtoT(ptrLogString.get()));
    }
    return true;
}

CYPLAYER_NAMESPACE_END