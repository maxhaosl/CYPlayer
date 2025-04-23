#include "ChainFilter/RenderFilter/CYAudioRenderFilter.hpp"

CYPLAYER_NAMESPACE_BEGIN

struct SDLAudioContext
{
    std::weak_ptr<CYMediaContext> ptrContext;
};

SDLAudioContext objContext;

CYAudioRenderFilter::CYAudioRenderFilter(EAudioRenderType eAudioType)
    : CYBaseFilter()
{
}

CYAudioRenderFilter::~CYAudioRenderFilter()
{
}

int16_t CYAudioRenderFilter::Init(SharePtr<EPlayerParam>& ptrParam)
{
    m_ptrParam = ptrParam;
    return CYBaseFilter::Init(ptrParam);
}

int16_t CYAudioRenderFilter::UnInit()
{
    return CYBaseFilter::UnInit();
}

int16_t CYAudioRenderFilter::Start(SharePtr<CYMediaContext>& ptrContext)
{
    return CYBaseFilter::Start(ptrContext);
}

int16_t CYAudioRenderFilter::Stop(SharePtr<CYMediaContext>& ptrContext)
{
    return CYBaseFilter::Stop(ptrContext);
}

int16_t CYAudioRenderFilter::Pause()
{
    return CYBaseFilter::Pause();
}

int16_t CYAudioRenderFilter::Resume()
{
    return CYBaseFilter::Resume();
}

int16_t CYAudioRenderFilter::ProcessPacket(SharePtr<CYMediaContext>& ptrContext, AVPacketPtr& ptrPacket)
{
    return CYBaseFilter::ProcessPacket(ptrContext, ptrPacket);
}

int16_t CYAudioRenderFilter::ProcessFrame(SharePtr<CYMediaContext>& ptrContext, AVFramePtr& ptrFrame)
{
    return CYBaseFilter::ProcessFrame(ptrContext, ptrFrame);
}

int16_t CYAudioRenderFilter::SetSpeed(float fSpeed)
{
    return ERR_SUCESS;
}

int16_t CYAudioRenderFilter::SetVolume(float fVolume)
{
    m_ptrContext->nAudioVolume = av_clip(fVolume * SDL_MIX_MAXVOLUME, 0, SDL_MIX_MAXVOLUME);
    return ERR_SUCESS;
}

float CYAudioRenderFilter::GetVolume()
{
    return m_ptrContext->nAudioVolume * 1.0 / SDL_MIX_MAXVOLUME;
}

//////////////////////////////////////////////////////////////////////////
/* copy samples for viewing in editor window */
static void UpdateSampleDisplay(SharePtr<CYMediaContext>& ptrContext, short* samples, int samples_size)
{
    int nSize = 0, nLen = 0;

    nSize = samples_size / sizeof(short);
    while (nSize > 0)
    {
        nLen = SAMPLE_ARRAY_SIZE - ptrContext->nSampleArrayIndex;
        if (nLen > nSize)
            nLen = nSize;
        memcpy(ptrContext->arraySample + ptrContext->nSampleArrayIndex, samples, nLen * sizeof(short));
        samples += nLen;
        ptrContext->nSampleArrayIndex += nLen;
        if (ptrContext->nSampleArrayIndex >= SAMPLE_ARRAY_SIZE)
            ptrContext->nSampleArrayIndex = 0;
        nSize -= nLen;
    }
}

int AudioDecodeFrame(SharePtr<CYMediaContext>& ptrContext);
/* prepare a new audio buffer */
void SDLAudioCallback(void* pOpaque, Uint8* stream, int nLen)
{
    SDLAudioContext* is = (SDLAudioContext*)pOpaque;
    SharePtr<CYMediaContext> ptrContext = is->ptrContext.lock();
    if (!ptrContext) return;
    int nAudioSize = 0, nLen1 = 0;

    ptrContext->nAudioCallbackTime = av_gettime_relative();

    while (nLen > 0)
    {
        if (ptrContext->bAbortRequest) break;
        if (ptrContext->nAudioBufIndex >= ptrContext->nAudioBufSize)
        {
            nAudioSize = AudioDecodeFrame(ptrContext);
            if (nAudioSize < 0)
            {
                /* if error, just output silence */
                ptrContext->ptrAudioBuffer.reset();
                ptrContext->nAudioBufSize = SDL_AUDIO_MIN_BUFFER_SIZE / ptrContext->objAudioTgt.frame_size * ptrContext->objAudioTgt.frame_size;
            }
            else
            {
                if (ptrContext->eShowMode != SHOW_MODE_VIDEO)
                    UpdateSampleDisplay(ptrContext, (int16_t*)ptrContext->ptrAudioBuffer.get(), nAudioSize);
                ptrContext->nAudioBufSize = nAudioSize;
            }
            ptrContext->nAudioBufIndex = 0;
        }
        nLen1 = ptrContext->nAudioBufSize - ptrContext->nAudioBufIndex;
        if (nLen1 > nLen)
            nLen1 = nLen;
        if (!ptrContext->bMuted && ptrContext->ptrAudioBuffer && ptrContext->nAudioVolume == SDL_MIX_MAXVOLUME)
            memcpy(stream, (uint8_t*)ptrContext->ptrAudioBuffer.get() + ptrContext->nAudioBufIndex, nLen1);
        else
        {
            memset(stream, 0, nLen1);
            if (!ptrContext->bMuted && ptrContext->ptrAudioBuffer)
                SDL_MixAudioFormat(stream, (uint8_t*)ptrContext->ptrAudioBuffer.get() + ptrContext->nAudioBufIndex, AUDIO_S16SYS, nLen1, ptrContext->nAudioVolume);
        }
        nLen -= nLen1;
        stream += nLen1;
        ptrContext->nAudioBufIndex += nLen1;
    }
    ptrContext->nAudioWriteBufSize = ptrContext->nAudioBufSize - ptrContext->nAudioBufIndex;
    /* Let's assume the audio driver that is used by SDL has two periods. */
    if (!isnan(ptrContext->fAudioClock))
    {
        ptrContext->audclk.SetClockAt(ptrContext->fAudioClock - (double)(2 * ptrContext->nAudioHWBufSize + ptrContext->nAudioWriteBufSize) / ptrContext->objAudioTgt.bytes_per_sec, ptrContext->nAudioClockSerial, ptrContext->nAudioCallbackTime / 1000000.0);
        ptrContext->extclk.SyncClockToSlave(ptrContext->audclk);
    }
}

int AudioOpen(SharePtr<CYMediaContext>& ptrContext, AVChannelLayoutPtr& ptrChLayout, int wanted_sample_rate, struct CYAudioParams* pAudioHWParams)
{
    SDL_AudioSpec objWantedSpec, objSpec;
    const char* env = nullptr;
    static const int arrNextNbChannels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };
    static const int arrNextSampleRates[] = { 0, 44100, 48000, 96000, 192000 };
    int nNextSampleRateIdx = FF_ARRAY_ELEMS(arrNextSampleRates) - 1;
    int nWantedNbChannels = ptrChLayout->nb_channels;

    env = SDL_getenv("SDL_AUDIO_CHANNELS");
    if (env)
    {
        nWantedNbChannels = atoi(env);
        av_channel_layout_uninit(ptrChLayout.get());
        av_channel_layout_default(ptrChLayout.get(), nWantedNbChannels);
    }
    if (ptrChLayout->order != AV_CHANNEL_ORDER_NATIVE)
    {
        av_channel_layout_uninit(ptrChLayout.get());
        av_channel_layout_default(ptrChLayout.get(), nWantedNbChannels);
    }
    nWantedNbChannels = ptrChLayout->nb_channels;
    objWantedSpec.channels = nWantedNbChannels;
    objWantedSpec.freq = wanted_sample_rate;
    if (objWantedSpec.freq <= 0 || objWantedSpec.channels <= 0)
    {
        av_log(nullptr, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
        return -1;
    }
    while (nNextSampleRateIdx && arrNextSampleRates[nNextSampleRateIdx] >= objWantedSpec.freq)
        nNextSampleRateIdx--;
    objWantedSpec.format = AUDIO_S16SYS;
    objWantedSpec.silence = 0;
    objWantedSpec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(objWantedSpec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    objWantedSpec.callback = SDLAudioCallback;

    objContext.ptrContext = ptrContext;
    objWantedSpec.userdata = &objContext;
    while (!(ptrContext->hAudioDev = SDL_OpenAudioDevice(nullptr, 0, &objWantedSpec, &objSpec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE)))
    {
        av_log(nullptr, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",
            objWantedSpec.channels, objWantedSpec.freq, SDL_GetError());
        objWantedSpec.channels = arrNextNbChannels[FFMIN(7, objWantedSpec.channels)];
        if (!objWantedSpec.channels)
        {
            objWantedSpec.freq = arrNextSampleRates[nNextSampleRateIdx--];
            objWantedSpec.channels = nWantedNbChannels;
            if (!objWantedSpec.freq)
            {
                av_log(nullptr, AV_LOG_ERROR, "No more combinations to try, audio open failed\n");
                return -1;
            }
        }
        av_channel_layout_default(ptrChLayout.get(), objWantedSpec.channels);
    }
    if (objSpec.format != AUDIO_S16SYS)
    {
        av_log(nullptr, AV_LOG_ERROR, "SDL advised audio format %d is not supported!\n", objSpec.format);
        return -1;
    }
    if (objSpec.channels != objWantedSpec.channels)
    {
        av_channel_layout_uninit(ptrChLayout.get());
        av_channel_layout_default(ptrChLayout.get(), objSpec.channels);
        if (ptrChLayout->order != AV_CHANNEL_ORDER_NATIVE)
        {
            av_log(nullptr, AV_LOG_ERROR, "SDL advised channel count %d is not supported!\n", objSpec.channels);
            return -1;
        }
    }

    pAudioHWParams->fmt = AV_SAMPLE_FMT_S16;
    pAudioHWParams->freq = objSpec.freq;
    if (av_channel_layout_copy(&pAudioHWParams->ch_layout, ptrChLayout.get()) < 0)
        return -1;
    pAudioHWParams->frame_size = av_samples_get_buffer_size(nullptr, pAudioHWParams->ch_layout.nb_channels, 1, pAudioHWParams->fmt, 1);
    pAudioHWParams->bytes_per_sec = av_samples_get_buffer_size(nullptr, pAudioHWParams->ch_layout.nb_channels, pAudioHWParams->freq, pAudioHWParams->fmt, 1);
    if (pAudioHWParams->bytes_per_sec <= 0 || pAudioHWParams->frame_size <= 0)
    {
        av_log(nullptr, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
        return -1;
    }
    return objSpec.size;
}

CYPLAYER_NAMESPACE_END