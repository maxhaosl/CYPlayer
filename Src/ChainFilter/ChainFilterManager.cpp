#include "ChainFilter/ChainFilterManager.hpp"
#include "ChainFilter/SourceFilter/CYSourceFilter.hpp"
#include "ChainFilter/DemuxFilter/CYDemuxFilter.hpp"
#include "ChainFilter/DecodeFilter/CYAudioDecodeFilter.hpp"
#include "ChainFilter/DecodeFilter/CYVideoDecodeFilter.hpp"
#include "ChainFilter/DecodeFilter/CYSubTitleDecodeFilter.hpp"
#include "ChainFilter/ProcessFilter/CYProcessFilter.hpp"
#include "ChainFilter/RenderFilter/CYAudioRenderFilter.hpp"
#include "ChainFilter/RenderFilter/CYVideoRenderFilter.hpp"

#include "ChainFilter/Common/CYDecoder.hpp"
#include "Logger/CYLoggerManager.hpp"

#include <memory>

CYPLAYER_NAMESPACE_BEGIN

CChainFilterManager::CChainFilterManager()
{
}

CChainFilterManager::~CChainFilterManager()
{
}

int16_t CChainFilterManager::InitFFmpeg()
{
#if HAVE_SETDLLDIRECTORY && defined(_WIN32)
    /* Calling SetDllDirectory with the empty string (but not nullptr) removes the
     * current working directory from the DLL search path as a security pre-caution. */
    SetDllDirectory(TEXT(""));
#endif

    av_log_set_flags(AV_LOG_SKIP_REPEATED);
    av_log_set_level(AV_LOG_INFO);

#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
    avformat_network_init();

    return ERR_SUCESS;
}

int16_t CChainFilterManager::UnInitFFmpeg()
{
    avformat_network_deinit();
    return ERR_SUCESS;
}

/**
 * Initialize.
 */
int16_t CChainFilterManager::Init(EPlayerParam* pParam)
{
    if (!pParam) return ERR_PLAYER_PARAM_NOT_VARIABLE;
    m_eStateType = TYPE_STATUS_IDLE;

    EXCEPTION_BEGIN
    {
        InitFFmpeg();
    }
    EXCEPTION_END;

    EXCEPTION_BEGIN
    {
        m_ptrSourceFilter = MakeShared<CYSourceFilter>(pParam->eClockType);
        m_ptrDemuxFilter = MakeShared<CYDemuxFilter>();
        m_ptrAudioDecodeFilter = MakeShared<CYAudioDecodeFilter>();
        m_ptrVideoDecodeFilter = MakeShared<CYVideoDecodeFilter>();
        m_ptrSubTitleDecodeFilter = MakeShared<CYSubTitleDecodeFilter>();
        m_ptrProcessFilter = MakeShared<CYProcessFilter>();
        m_ptrAudioRenderFilter = MakeShared<CYAudioRenderFilter>(pParam->eAudioRenderType);
        m_ptrVideoRenderFilter = MakeShared<CYVideoRenderFilter>(pParam->eVideoRenderType);

        m_ptrFirstFilter = m_ptrLastFilter = m_ptrSourceFilter;
        m_ptrLastFilter = m_ptrLastFilter->SetNext(m_ptrDemuxFilter);
        m_ptrLastFilter = m_ptrLastFilter->SetNext(m_ptrAudioDecodeFilter);
        m_ptrLastFilter = m_ptrLastFilter->SetNext(m_ptrVideoDecodeFilter);
        m_ptrLastFilter = m_ptrLastFilter->SetNext(m_ptrSubTitleDecodeFilter);
        m_ptrLastFilter = m_ptrLastFilter->SetNext(m_ptrProcessFilter);
        m_ptrLastFilter = m_ptrLastFilter->SetNext(m_ptrAudioRenderFilter);
        m_ptrLastFilter = m_ptrLastFilter->SetNext(m_ptrVideoRenderFilter);

        m_ptrContext = MakeShared<CYMediaContext>();
    }
    EXCEPTION_END;

    EXCEPTION_BEGIN
    {
        m_ptrParam = MakeShared<EPlayerParam>();
        memcpy(m_ptrParam.get(), pParam, sizeof(EPlayerParam));
        m_ptrFirstFilter->Init(m_ptrParam);
        m_eStateType = TYPE_STATUS_INITIALIZED;
    }
    EXCEPTION_END([&] {m_eStateType = TYPE_STATUS_ERROR; });

    return ERR_SUCESS;
}

int16_t CChainFilterManager::UnInit()
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(!m_ptrFirstFilter, "m_ptrFirstFilter is nullptr.");
        IfTrueThrow(!m_ptrLastFilter, "m_ptrLastFilter is nullptr.");

        m_ptrFirstFilter->Stop(m_ptrContext);
        m_ptrFirstFilter->UnInit();
        m_ptrFirstFilter.reset();
        m_ptrLastFilter.reset();
        m_ptrContext.reset();
    }
    EXCEPTION_END;

    EXCEPTION_BEGIN
    {
        m_eStateType = TYPE_STATUS_IDLE;
        return UnInitFFmpeg();
    }
    EXCEPTION_END;

    return ERR_UINIT_ERROR;
}

/**
 * Set Render Window.
 */
int16_t CChainFilterManager::SetWindow(void* hwnd)
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType == TYPE_STATUS_IDLE, "Player State is TYPE_STATUS_IDLE.");

        if (m_ptrVideoRenderFilter)
        {
            auto ptrVideoRenderFilter = std::dynamic_pointer_cast<CYVideoRenderFilter>(m_ptrVideoRenderFilter);
            if (ptrVideoRenderFilter)
            {
               return ptrVideoRenderFilter->SetWindow(hwnd);
            }
            return ERR_VARIABLE_CONVER_FAILED;
        }
        return ERR_NOT_INIT;
    }
    EXCEPTION_END;

    return ERR_SETWINDOW_ERROR;
}

/**
 * Open Media File.
 */
int16_t CChainFilterManager::Open(const char* pszURL, EPlayerMediaParam* pParam)
{
    if (m_eStateType >= TYPE_STATUS_PLAYING)
    {
        Stop();
    }

    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType == TYPE_STATUS_IDLE, "Player State is TYPE_STATUS_IDLE.");
        IfTrueThrow(pParam->nStartTime < -1, "Playback start time is less than -1.");

        if (pParam && strlen(pParam->szInputFormat) > 0)
        {
            SetInputFormat(pParam->szInputFormat);
        }
        ResetContext(m_ptrContext);
        m_ptrContext->bAutoExit = pParam->bAutoExit;
        m_ptrContext->nLoop = pParam->nLoop;
        m_ptrContext->nLowRes = pParam->nLowRes;
        m_ptrContext->nStartTime = pParam->nStartTime == -1 ? AV_NOPTS_VALUE : pParam->nStartTime;
        m_ptrContext->nDuration = pParam->nDuration == -1 ? AV_NOPTS_VALUE : pParam->nDuration;

        m_ptrContext->bFastDecode = pParam->bFastDecode;
        strcpy(m_ptrContext->szForceAudioCodecName, pParam->szForceAudioCodecName);
        strcpy(m_ptrContext->szForceSubtitleCodecName, pParam->szForceSubtitleCodecName);
        strcpy(m_ptrContext->szForceVideoCodecName, pParam->szForceVideoCodecName);
        strcpy(m_ptrContext->szHWAccel, pParam->szHWAccel);

        if (m_ptrSourceFilter)
        {
            auto ptrSourceFilter = std::dynamic_pointer_cast<CYSourceFilter>(m_ptrSourceFilter);
            if (ptrSourceFilter)
            {
               int16_t nRet = ptrSourceFilter->Open(m_ptrContext, pszURL, m_pFileInputFormat, pParam);
               if (nRet == ERR_SUCESS)
               {
                   m_eStateType = TYPE_STATUS_PREPARED;
               }
               return nRet;
            }
            return ERR_VARIABLE_CONVER_FAILED;
        }
        return ERR_NOT_INIT;
    }
    EXCEPTION_END;

    return ERR_OPEN_ERROR;
}

/**
 * Playback control.
 */
int16_t  CChainFilterManager::Play()
{
    if (m_eStateType == TYPE_STATUS_PAUSED)
    {
        bool bPaused = false;
        return Pause(&bPaused);
    }

    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType != TYPE_STATUS_PREPARED, "Player State is not TYPE_STATUS_PREPARED.");
        IfTrueThrow(m_eStateType == TYPE_STATUS_PLAYING, "Player Is Playing.");
        m_ptrFirstFilter->Start(m_ptrContext);
        m_eStateType = TYPE_STATUS_PLAYING;
        return ERR_SUCESS;
    }
    EXCEPTION_END;

    return ERR_PLAY_ERROR;
}

int16_t  CChainFilterManager::Pause(bool* bPaused)
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow((m_eStateType != TYPE_STATUS_PLAYING && m_eStateType != TYPE_STATUS_PAUSED), "Player Is Not Playing Or Paused State.");

        if (m_eStateType == TYPE_STATUS_PLAYING)
        {
            m_ptrFirstFilter->Pause();
            m_eStateType = TYPE_STATUS_PAUSED;
            *bPaused = true;
        }
        else
        {
            m_ptrFirstFilter->Resume();
            m_eStateType = TYPE_STATUS_PLAYING;
            *bPaused = false;
        }
        return ERR_SUCESS;
    }
    EXCEPTION_END;

    return ERR_SUCESS;
}

int16_t  CChainFilterManager::Stop()
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType == TYPE_STATUS_IDLE, "Player State is TYPE_STATUS_IDLE.");
        m_ptrFirstFilter->Stop(m_ptrContext);
        m_eStateType = TYPE_STATUS_STOPPED;
    }
    EXCEPTION_END;

    return ERR_SUCESS;
}

/**
 * Player State
 */
EStateType CChainFilterManager::GetState() const
{
    return m_eStateType;
}

int64_t CChainFilterManager::GetDuration() const
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType < TYPE_STATUS_PREPARED, "Player State is not TYPE_STATUS_PREPARED.");

        if (m_ptrSourceFilter)
        {
            auto ptrDemuxFilter = std::dynamic_pointer_cast<CYDemuxFilter>(m_ptrDemuxFilter);
            if (ptrDemuxFilter)
            {
               return ptrDemuxFilter->GetDuration();
            }
            IfTrueThrow(true, "variable is conver failed.");
        }
        IfTrueThrow(true, "Player is not Init.");
    }
    EXCEPTION_END;

    return ERR_SUCESS;
}

int64_t CChainFilterManager::GetPosition() const
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType < TYPE_STATUS_PREPARED, "Player State is not TYPE_STATUS_PREPARED.");

        if (m_ptrSourceFilter)
        {
            auto ptrDemuxFilter = std::dynamic_pointer_cast<CYDemuxFilter>(m_ptrDemuxFilter);
            if (ptrDemuxFilter)
            {
               return ptrDemuxFilter->GetPosition();
            }
            IfTrueThrow(true, "variable is conver failed.");
        }
        IfTrueThrow(true, "Player is not Init.");
    }
    EXCEPTION_END;

    return ERR_SUCESS;
}

/**
 * Jump to the specified position (seconds).
 */
int16_t CChainFilterManager::Seek(int64_t nTimestamp)
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType < TYPE_STATUS_PREPARED, "Player State is not TYPE_STATUS_PREPARED.");

        if (m_ptrSourceFilter)
        {
            auto ptrDemuxFilter = std::dynamic_pointer_cast<CYDemuxFilter>(m_ptrDemuxFilter);
            if (ptrDemuxFilter)
            {
               return ptrDemuxFilter->Seek(nTimestamp);
            }
            IfTrueThrow(true, "variable is conver failed.");
        }
        IfTrueThrow(true, "Player is not Init.");
    }
    EXCEPTION_END;

    return ERR_SEEK_FAILED;
}

int16_t CChainFilterManager::SetMute(bool bMute)
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType < TYPE_STATUS_PREPARED, "Player State is not TYPE_STATUS_PREPARED.");

        if (m_ptrSourceFilter)
        {
            auto ptrVideoRenderFilter = std::dynamic_pointer_cast<CYVideoRenderFilter>(m_ptrVideoRenderFilter);
            if (ptrVideoRenderFilter)
            {
                return ptrVideoRenderFilter->SetMute(bMute);
            }
            IfTrueThrow(true, "variable is conver failed.");
        }
        IfTrueThrow(true, "Player is not Init.");
    }
    EXCEPTION_END;

    return ERR_SETMUTE_FAILED;
}

int16_t CChainFilterManager::SetLoop(bool bLoop)
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType < TYPE_STATUS_PREPARED, "Player State is not TYPE_STATUS_PREPARED.");

        if (m_ptrSourceFilter)
        {
            auto ptrDemuxFilter = std::dynamic_pointer_cast<CYDemuxFilter>(m_ptrDemuxFilter);
            if (ptrDemuxFilter)
            {
               return ptrDemuxFilter->SetLoop(bLoop);
            }
            IfTrueThrow(true, "variable is conver failed.");
        }
        IfTrueThrow(true, "Player is not Init.");
    }
    EXCEPTION_END;

    return ERR_SETLOOP_FAILED;
}

int16_t CChainFilterManager::SetSpeed(float fSpeed)
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType == TYPE_STATUS_IDLE, "Player State is TYPE_STATUS_IDLE.");

        if (m_ptrAudioRenderFilter)
        {
            auto ptrAudioRenderFilter = std::dynamic_pointer_cast<CYAudioRenderFilter>(m_ptrAudioRenderFilter);
            if (ptrAudioRenderFilter)
            {
               return ptrAudioRenderFilter->SetSpeed(fSpeed);
            }
            return ERR_VARIABLE_CONVER_FAILED;
        }
        return ERR_NOT_INIT;
    }
    EXCEPTION_END;

    return ERR_SETSPEED_FAILED;
}

/**
 * Volume control.0.0 ~ 1.0
 */
int16_t CChainFilterManager::SetVolume(float fVolume)
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType == TYPE_STATUS_IDLE, "Player State is TYPE_STATUS_IDLE.");

        if (m_ptrAudioRenderFilter)
        {
            auto ptrAudioRenderFilter = std::dynamic_pointer_cast<CYAudioRenderFilter>(m_ptrAudioRenderFilter);
            if (ptrAudioRenderFilter)
            {
               return ptrAudioRenderFilter->SetVolume(fVolume);
            }
            return ERR_VARIABLE_CONVER_FAILED;
        }
        return ERR_NOT_INIT;
    }
    EXCEPTION_END;

    return ERR_SETVOLUME_ERROR;
}

float CChainFilterManager::GetVolume()
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType == TYPE_STATUS_IDLE, "Player State is TYPE_STATUS_IDLE.");

        if (m_ptrAudioRenderFilter)
        {
            auto ptrAudioRenderFilter = std::dynamic_pointer_cast<CYAudioRenderFilter>(m_ptrAudioRenderFilter);
            if (ptrAudioRenderFilter)
            {
                return ptrAudioRenderFilter->GetVolume();
            }
            IfTrueThrow(true, "variable is conver failed.");
        }
        IfTrueThrow(true, "Player is not Init.");
    }
    EXCEPTION_END;

    return 0.0f;
}

// Video settings
int16_t CChainFilterManager::SetVideoScale(EVideoScaleType eScale)
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType == TYPE_STATUS_IDLE, "Player State is TYPE_STATUS_IDLE.");

        if (m_ptrVideoRenderFilter)
        {
            auto ptrVideoRenderFilter = std::dynamic_pointer_cast<CYVideoRenderFilter>(m_ptrVideoRenderFilter);
            if (ptrVideoRenderFilter)
            {
               return ptrVideoRenderFilter->SetVideoScale(eScale);
            }
            return ERR_VARIABLE_CONVER_FAILED;
        }
        return ERR_NOT_INIT;
    }
    EXCEPTION_END;

    return ERR_SETVIDEOSCALE_ERROR;
}

int16_t CChainFilterManager::SetVideoRotation(ERotationType eRotation)
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType == TYPE_STATUS_IDLE, "Player State is TYPE_STATUS_IDLE.");

        if (m_ptrVideoRenderFilter)
        {
            auto ptrVideoRenderFilter = std::dynamic_pointer_cast<CYVideoRenderFilter>(m_ptrVideoRenderFilter);
            if (ptrVideoRenderFilter)
            {
               return ptrVideoRenderFilter->SetVideoRotation(eRotation);
            }
            return ERR_VARIABLE_CONVER_FAILED;
        }
        return ERR_NOT_INIT;
    }
    EXCEPTION_END;

    return ERR_SETVIDEO_ROTATION_ERROR;
}

int16_t CChainFilterManager::SetVideoMirror(bool bMirror)
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType == TYPE_STATUS_IDLE, "Player State is TYPE_STATUS_IDLE.");

        if (m_ptrVideoRenderFilter)
        {
            auto ptrVideoRenderFilter = std::dynamic_pointer_cast<CYVideoRenderFilter>(m_ptrVideoRenderFilter);
            if (ptrVideoRenderFilter)
            {
               return ptrVideoRenderFilter->SetVideoMirror(bMirror);
            }
            return ERR_VARIABLE_CONVER_FAILED;
        }
        return ERR_NOT_INIT;
    }
    EXCEPTION_END;

    return ERR_SETVIDEO_MIRROR_ERROR;
}

int16_t CChainFilterManager::SetAspectRatio(float fRatio)
{
    EXCEPTION_BEGIN
    {
        IfTrueThrow(m_eStateType == TYPE_STATUS_IDLE, "Player State is TYPE_STATUS_IDLE.");

        if (m_ptrVideoRenderFilter)
        {
            auto ptrVideoRenderFilter = std::dynamic_pointer_cast<CYVideoRenderFilter>(m_ptrVideoRenderFilter);
            if (ptrVideoRenderFilter)
            {
               return ptrVideoRenderFilter->SetAspectRatio(fRatio);
            }
            return ERR_VARIABLE_CONVER_FAILED;
        }
        return ERR_NOT_INIT;
    }
    EXCEPTION_END;

    return ERR_SETASPECT_RATIO_ERROR;
}

/**
 * Event callback settings.
 */
int16_t CChainFilterManager::SetEventCallback(FunEventCallback callback)
{
    m_funEventCallBack = callback;
    m_ptrContext->funEventCallBack = callback;
    return ERR_SUCESS;
}

int16_t CChainFilterManager::SetStateCallback(FunStateCallBack callback)
{
    m_funStateCallBack = callback;
    m_ptrContext->funStateCallBack = callback;
    return ERR_SUCESS;
}

int16_t CChainFilterManager::SetPositionCallback(FunPositionCallBack callback)
{
    m_funPositionCallBack = callback;
    m_ptrContext->funPositionCallBack = callback;
    return ERR_SUCESS;
}

int16_t CChainFilterManager::SetInputFormat(const char* pszFormat)
{
    m_pFileInputFormat = av_find_input_format(pszFormat);
    if (!m_pFileInputFormat)
    {
        CY_LOG_FATAL("Unknown input format: %s\n", pszFormat);
        return ERR_FIND_INPUT_FORMAT_ERROR;
    }
    return ERR_SUCESS;
}

int16_t CChainFilterManager::ResetContext(SharePtr<CYMediaContext>& ptrContext)
{
    ptrContext->iformat = nullptr;
    ptrContext->bAbortRequest = false;
    ptrContext->bForceRefresh = false;
    ptrContext->bPaused = false;
    ptrContext->nLastPaused = 0;
    ptrContext->nQueueAttachmentsReq = 0;
    ptrContext->bSeekReq = false;
    ptrContext->nSeekFlags = 0;
    ptrContext->nSeekPos = 0;
    ptrContext->nSeekRel = 0;
    ptrContext->nReadPauseReturn = 0;
    ptrContext->ptrIC.reset();
    ptrContext->bRealTime = false;
    ptrContext->audclk = {};
    ptrContext->vidclk = {};
    ptrContext->extclk = {};

    //     CYFrameQueue pictq;
    //     CYFrameQueue subpq;
    //     CYFrameQueue sampq;
    //     CYDecoder auddec;
    //     CYDecoder viddec;
    //     CYDecoder subdec;

    ptrContext->nAVSyncType = 0;

    ptrContext->fAudioClock = 0;
    ptrContext->nAudioClockSerial = 0;
    ptrContext->fAudioDiffCum = 0; /* used for AV difference average computation */
    ptrContext->fAudioDiffAvgCoef = 0;
    ptrContext->fAudioDiffThreshold = 0;
    ptrContext->nAudioDiffAvgCount = 0;

    ptrContext->nAudioStreamIndex = 0;
    ptrContext->pAudioStream = nullptr;
    ptrContext->ptrAudioQueue;

    ptrContext->nAudioHWBufSize = 0;
    ptrContext->ptrAudioBuffer.reset();
    ptrContext->nAudioBufSize = 0; /* in bytes */

    ptrContext->ptrAudioBuffer1.reset();
    ptrContext->nAudioBuf1Size = 0;
    ptrContext->nAudioBufIndex = 0; /* in bytes */
    ptrContext->nAudioWriteBufSize = 0;
    ptrContext->nAudioVolume = 100;
    ptrContext->bMuted = false;
    ptrContext->objAudioSrc = {};
    ptrContext->objAudioFilterSrc = {};
    ptrContext->objAudioTgt = {};
    ptrContext->ptrSwrCtx.reset();
    ptrContext->nFrameDropsEarly = 0;
    ptrContext->nFrameDropsLate = 0;

    ptrContext->eShowMode = SHOW_MODE_NONE;
    ptrContext->arraySample[SAMPLE_ARRAY_SIZE] = { 0 };
    ptrContext->nSampleArrayIndex = 0;
    ptrContext->nLastIStart = 0;
    ptrContext->pRdftContext = nullptr;
    ptrContext->funRdft = nullptr;
    ptrContext->nRdftBits = 0;
    ptrContext->pfRealData = nullptr;
    ptrContext->pRdftData = nullptr;
    ptrContext->xpos = 0;
    ptrContext->fLastVisTime = 0;
    ptrContext->vis_texture = nullptr;
    ptrContext->sub_texture = nullptr;
    ptrContext->vid_texture = nullptr;

    ptrContext->nSubtitleStreamIndex = 0;
    ptrContext->pSubTitleStream = nullptr;
    ptrContext->ptrSubTitleQueue.reset();

    ptrContext->fFrameTimer = 0;
    ptrContext->fFrameLastReturnedTime = 0;
    ptrContext->fFrameLastFilterDelay = 0;

    ptrContext->nVideoStreamIndex = 0;
    ptrContext->pVideoStream = nullptr;
    ptrContext->ptrVideoQueue.reset();

    ptrContext->fMaxFrameDuration = 0;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
    ptrContext->ptrSubConvertCtx.reset();
    ptrContext->bEof = false;

    ptrContext->pszFileName = nullptr;
    ptrContext->nShowWidth = 0;
    ptrContext->nShowHeight = 0;
    ptrContext->nShowXLeft = 0;
    ptrContext->nShowYTop = 0;
    ptrContext->bStep = 0;

    ptrContext->nVFilterIndex = 0;
    ptrContext->pInVideoFilter = nullptr;   // the first filter in the video chain
    ptrContext->pOutVideoFilter = nullptr;  // the last filter in the video chain

    ptrContext->pInAudioFilter = nullptr;   // the first filter in the audio chain
    ptrContext->pOutAudioFilter = nullptr;  // the last filter in the audio chain
    ptrContext->ptrAgraph.reset();        // audio filter graph

    ptrContext->nLastVideoStream = 0;
    ptrContext->nLastAudioStream = 0;
    ptrContext->nLastSubTitleStream = 0;

    ptrContext->ptrReadCond.reset();

    ptrContext->hAudioDev = 0;

    //     SDLWindowPtr ptrWindow;
    //     SDLRendererPtr ptrRenderer;
    //     SDL_RendererInfo objRendererInfo = { 0 };
    // #if HAVE_VULKAN_RENDERER
    //     VkRendererPtr ptrVKRenderer;
    // #endif

    ptrContext->isFullScreen = 0;
    ptrContext->fRdftSpeed = 0.02;
    ptrContext->nShowStatus = -1;

    ptrContext->pVfiltersList = nullptr;
    ptrContext->nNBVFilters = 0;
    ptrContext->pAFilters = nullptr;

    ptrContext->nDecoderReorderPTS = -1;
    ptrContext->nAudioCallBackTime = 0;
    //     //////////////////////////////////////////////////////////////////////////
    ptrContext->nLowRes = 0;
    ptrContext->bAutoExit = false;
    ptrContext->nLoop = 1;
    ptrContext->nStartTime = AV_NOPTS_VALUE;
    ptrContext->nDuration = AV_NOPTS_VALUE;
    ptrContext->szStreamSpec[AVMEDIA_TYPE_NB][256] = {0};

    ptrContext->bFastDecode = false;
    ptrContext->szForceAudioCodecName[256] = { 0 };
    ptrContext->szForceSubtitleCodecName[256] = { 0 };
    ptrContext->szForceVideoCodecName[256] = { 0 };
    ptrContext->szHWAccel[256] = { 0 };

    ptrContext->nSampleRate = 0;
    ptrContext->nAudioCallbackTime = 0;
    ptrContext->ptrChLayout.reset();

    ptrContext->nCursorLastShown = 0;
    ptrContext->bCursorHidden = false;

    return ERR_SUCESS;
}

CYPLAYER_NAMESPACE_END