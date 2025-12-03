#include "CYPlayerImpl.hpp"
#include "ChainFilter/ChainFilterManager.hpp"

#include "Logger/CYLoggerManager.hpp"

CYPLAYER_NAMESPACE_BEGIN

CYPlayerImpl::CYPlayerImpl()
    : ICYPlayer()
{
}

CYPlayerImpl::~CYPlayerImpl()
{
}

/**
 * Initialize.
 */
int16_t CYPlayerImpl::Init(EPlayerParam* pParam)
{
    m_ptrChainFilterManager = MakeUnique<CChainFilterManager>();
    return m_ptrChainFilterManager->Init(pParam);
}

int16_t CYPlayerImpl::UnInit()
{
    int16_t nRet = m_ptrChainFilterManager->UnInit();
    m_ptrChainFilterManager.reset();
    return nRet;
}

/**
* Set Render Window.
*/
int16_t CYPlayerImpl::SetWindow(void* hWnd)
{
    return m_ptrChainFilterManager->SetWindow(hWnd);
}

/**
* Open Media File.
*/
int16_t CYPlayerImpl::Open(const char* pszURL, EPlayerMediaParam* pParam)
{
    return m_ptrChainFilterManager->Open(pszURL, pParam);;
}

/**
* Playback control.
*/
int16_t CYPlayerImpl::Play()
{
    return m_ptrChainFilterManager->Play();
}

int16_t CYPlayerImpl::Pause(bool* bPaused)
{
    return m_ptrChainFilterManager->Pause(bPaused);
}

int16_t CYPlayerImpl::Stop()
{
   return m_ptrChainFilterManager->Stop();
}

/**
 * Set Display Size.
 */
int16_t CYPlayerImpl::SetDisplaySize(int32_t nWidth, int32_t nHeight)
{
    return m_ptrChainFilterManager->SetDisplaySize(nWidth, nHeight);
}

/**
* Player State
*/
EStateType CYPlayerImpl::GetState() const
{
    return m_ptrChainFilterManager->GetState();
}

int64_t CYPlayerImpl::GetDuration() const
{
    return m_ptrChainFilterManager->GetDuration();
}

int64_t CYPlayerImpl::GetPosition() const
{
    return m_ptrChainFilterManager->GetPosition();
}

/**
* Jump to the specified position (seconds).
*/
int16_t CYPlayerImpl::Seek(int64_t nTimestamp)
{
    return m_ptrChainFilterManager->Seek(nTimestamp);
}

int16_t CYPlayerImpl::SetMute(bool bMute)
{
    return m_ptrChainFilterManager->SetMute(bMute);
}

int16_t CYPlayerImpl::SetLoop(bool bLoop)
{
    return m_ptrChainFilterManager->SetLoop(bLoop);
}

int16_t CYPlayerImpl::SetSpeed(float fSpeed)
{
    return m_ptrChainFilterManager->SetSpeed(fSpeed);
}

/**
* Volume control.0.0 ~ 1.0
*/
int16_t CYPlayerImpl::SetVolume(float fVolume)
{
    return m_ptrChainFilterManager->SetVolume(fVolume);
}

float CYPlayerImpl::GetVolume()
{
    return m_ptrChainFilterManager->GetVolume();
}

// Video settings
int16_t CYPlayerImpl::SetVideoScale(EVideoScaleType eScale)
{
    return m_ptrChainFilterManager->SetVideoScale(eScale);
}

int16_t CYPlayerImpl::SetVideoRotation(ERotationType eRotation)
{
    return m_ptrChainFilterManager->SetVideoRotation(eRotation);
}

int16_t CYPlayerImpl::SetVideoMirror(bool bMirror)
{
    return m_ptrChainFilterManager->SetVideoMirror(bMirror);
}

int16_t CYPlayerImpl::SetAspectRatio(float fRatio)
{
    return m_ptrChainFilterManager->SetAspectRatio(fRatio);
}

/**
* Event callback settings.
*/
int16_t CYPlayerImpl::SetEventCallback(FunEventCallback callback)
{
    return m_ptrChainFilterManager->SetEventCallback(callback);
}

int16_t CYPlayerImpl::SetStateCallback(FunStateCallBack callback)
{
    return m_ptrChainFilterManager->SetStateCallback(callback);
}

int16_t CYPlayerImpl::SetPositionCallback(FunPositionCallBack callback)
{
    return m_ptrChainFilterManager->SetPositionCallback(callback);
}

int16_t CYPlayerImpl::SetLogCallBack(FunLogCallBack callback)
{
    LoggerManager()->RegisterLogCallBack(callback);
    return ERR_SUCESS;
}

CYPLAYER_NAMESPACE_END

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#endif