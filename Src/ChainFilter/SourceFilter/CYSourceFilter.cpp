#include "ChainFilter/SourceFilter/CYSourceFilter.hpp"
#include "Logger/CYLoggerManager.hpp"

CYPLAYER_NAMESPACE_BEGIN

void StreamClose(SharePtr<CYMediaContext>& ptrContext);

CYSourceFilter::CYSourceFilter(EPlayerRefClockType eClockType)
    : CYBaseFilter()
    , m_eClockType(eClockType)
{
}

CYSourceFilter::~CYSourceFilter()
{
}

int16_t CYSourceFilter::Init(SharePtr<EPlayerParam>& ptrParam)
{
    m_ptrMediaParam = MakeShared<EPlayerMediaParam>();
    return CYBaseFilter::Init(ptrParam);
}

int16_t CYSourceFilter::UnInit()
{
    if (m_ptrContext)
    {
        StreamClose(m_ptrContext);
    }
    return CYBaseFilter::UnInit();
}

int16_t CYSourceFilter::Start(SharePtr<CYMediaContext>& ptrContext)
{
    m_ptrContext = ptrContext;
    for (int nIndex = 0; nIndex < AVMEDIA_TYPE_NUMBER; nIndex++)
    {
        if (m_ptrMediaParam->szStreamSpec[nIndex])
        {
            strcpy(&ptrContext->szStreamSpec[nIndex][0], m_ptrMediaParam->szStreamSpec[nIndex]);
        }
    }
    return CYBaseFilter::Start(ptrContext);
}

int16_t CYSourceFilter::Stop(SharePtr<CYMediaContext>& ptrContext)
{
    int16_t nRet = CYBaseFilter::Stop(ptrContext);
    StreamClose(ptrContext);
    return nRet;
}

int16_t CYSourceFilter::Pause()
{
    return CYBaseFilter::Pause();
}

int16_t CYSourceFilter::Resume()
{
    return CYBaseFilter::Resume();
}

int16_t CYSourceFilter::ProcessPacket(SharePtr<CYMediaContext>& ptrContext, AVPacketPtr& ptrPacket)
{
    return CYBaseFilter::ProcessPacket(ptrContext, ptrPacket);
}

int16_t CYSourceFilter::ProcessFrame(SharePtr<CYMediaContext>& ptrContext, AVFramePtr& ptrFrame)
{
    return CYBaseFilter::ProcessFrame(ptrContext, ptrFrame);
}

/**
 * Open Media File.
 */
int16_t CYSourceFilter::Open(SharePtr<CYMediaContext>& ptrContext, const char* pszURL, const AVInputFormat* pInputFormat, EPlayerMediaParam* pParam)
{
    int nVolume = 0;
    IfTrueThrow(!ptrContext, "ptrContext is nullptr");
    if (pParam)
        memcpy(m_ptrMediaParam.get(), pParam, sizeof(EPlayerMediaParam));

    const AVInputFormat* iformat = pInputFormat;

    ptrContext->nLastVideoStream = ptrContext->nVideoStreamIndex = -1;
    ptrContext->nLastAudioStream = ptrContext->nAudioStreamIndex = -1;
    ptrContext->nLastSubTitleStream = ptrContext->nSubtitleStreamIndex = -1;
    ptrContext->pszFileName = av_strdup(pszURL);
    if (!ptrContext->pszFileName)
        goto fail;
    ptrContext->iformat = iformat;
    ptrContext->nShowYTop = 0;
    ptrContext->nShowXLeft = 0;

    ptrContext->ptrVideoQueue = std::make_shared<CYPLAYER_NAMESPACE::CYPacketQueue>();
    ptrContext->ptrAudioQueue = std::make_shared<CYPLAYER_NAMESPACE::CYPacketQueue>();
    ptrContext->ptrSubTitleQueue = std::make_shared<CYPLAYER_NAMESPACE::CYPacketQueue>();

    if (ptrContext->pictq.Init(ptrContext->ptrVideoQueue, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
        goto fail;

    if (ptrContext->subpq.Init(ptrContext->ptrSubTitleQueue, SUBPICTURE_QUEUE_SIZE, 0) < 0)
        goto fail;

    if (ptrContext->sampq.Init(ptrContext->ptrAudioQueue, SAMPLE_QUEUE_SIZE, 1) < 0)
        goto fail;

    if (ptrContext->ptrVideoQueue->Init() < 0 || ptrContext->ptrAudioQueue->Init() < 0 || ptrContext->ptrSubTitleQueue->Init() < 0)
        goto fail;

    ptrContext->ptrReadCond = std::make_shared<CYPLAYER_NAMESPACE::CYCondition>();

    ptrContext->vidclk.InitClock(&ptrContext->ptrVideoQueue->serial);
    ptrContext->audclk.InitClock(&ptrContext->ptrAudioQueue->serial);
    ptrContext->extclk.InitClock(&ptrContext->extclk.m_fSerial);
    ptrContext->nAudioClockSerial = -1;

    nVolume = pParam ? pParam->nStartVolume : 100;
    if (nVolume < 0)
        CY_LOG_WARN("-volume=%d < 0, setting to 0\n", nVolume);

    if (nVolume > 100)
        CY_LOG_WARN("-volume=%d > 100, setting to 100\n", nVolume);

    nVolume = av_clip(nVolume, 0, 100);
    nVolume = av_clip(SDL_MIX_MAXVOLUME * nVolume / 100, 0, SDL_MIX_MAXVOLUME);

    ptrContext->nAudioVolume = nVolume;
    ptrContext->bMuted = false;

    ptrContext->nAVSyncType = m_eClockType;

    return ERR_SUCESS;

fail:
    StreamClose(ptrContext);
    return ERR_OPEN_MEDIA_ERROR;
}

CYPLAYER_NAMESPACE_END