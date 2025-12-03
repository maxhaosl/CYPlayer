#include "ChainFilter/DecodeFilter/CYAudioDecodeFilter.hpp"
#include "ChainFilter/Common/CYAudioFilters.hpp"

CYPLAYER_NAMESPACE_BEGIN

CYAudioDecodeFilter::CYAudioDecodeFilter()
    : CYBaseFilter()
{
}

CYAudioDecodeFilter::~CYAudioDecodeFilter()
{
}

int16_t CYAudioDecodeFilter::Init(SharePtr<EPlayerParam>& ptrParam)
{
    return CYBaseFilter::Init(ptrParam);
}

int16_t CYAudioDecodeFilter::UnInit()
{
    return CYBaseFilter::UnInit();
}

int16_t CYAudioDecodeFilter::Start(SharePtr<CYMediaContext>& ptrContext)
{
    m_bStop = false;
    m_ptrContext = ptrContext;
    std::function<void()> func = std::bind(&CYAudioDecodeFilter::OnEntry, this);
    m_ptrContext->auddec.Start(func, "AudioDecodeThread");
    return CYBaseFilter::Start(ptrContext);
}

int16_t CYAudioDecodeFilter::Stop(SharePtr<CYMediaContext>& ptrContext)
{
    m_bStop = true;
    ptrContext->auddec.Abort(ptrContext->sampq);
    ptrContext->auddec.Destroy();
    return CYBaseFilter::Stop(ptrContext);
}

int16_t CYAudioDecodeFilter::Pause()
{
    return CYBaseFilter::Pause();
}

int16_t CYAudioDecodeFilter::Resume()
{
    return CYBaseFilter::Resume();
}

int16_t CYAudioDecodeFilter::ProcessPacket(SharePtr<CYMediaContext>& ptrContext, AVPacketPtr& ptrPacket)
{
    return CYBaseFilter::ProcessPacket(ptrContext, ptrPacket);
}

int16_t CYAudioDecodeFilter::ProcessFrame(SharePtr<CYMediaContext>& ptrContext, AVFramePtr& ptrFrame)
{
    return CYBaseFilter::ProcessFrame(ptrContext, ptrFrame);
}

int CYAudioDecodeFilter::CmpAudioFmts(enum AVSampleFormat eFmt, int64_t nChannelCount, enum AVSampleFormat eFmt2, int64_t nChannelCount2)
{
    /* If channel count == 1, planar and non-planar formats are the same */
    if (nChannelCount == 1 && nChannelCount2 == 1)
        return av_get_packed_sample_fmt(eFmt) != av_get_packed_sample_fmt(eFmt2);
    else
        return nChannelCount != nChannelCount2 || eFmt != eFmt2;
}

void CYAudioDecodeFilter::OnEntry()
{
    m_ptrContext->auddec.WaitStart();

    if (m_bStop) return;

    AVFrame* pFrame = av_frame_alloc();
    CYFrame* af = nullptr;
    int last_serial = -1;
    int reconfigure;
    int got_frame = 0;
    AVRational tb;
    int ret = 0;

    if (!pFrame)
    {
        av_log(nullptr, AV_LOG_FATAL, "alloc frame failed.");
        return;
    }

    do
    {
        if ((got_frame = m_ptrContext->auddec.DecodeFrame(pFrame, nullptr, m_ptrContext->nDecoderReorderPTS)) < 0)
            goto the_end;

        if (got_frame)
        {
            tb = { 1, pFrame->sample_rate };

            reconfigure = CmpAudioFmts(m_ptrContext->objAudioFilterSrc.fmt, m_ptrContext->objAudioFilterSrc.ch_layout.nb_channels, (AVSampleFormat)pFrame->format, pFrame->ch_layout.nb_channels) ||
                av_channel_layout_compare(&m_ptrContext->objAudioFilterSrc.ch_layout, &pFrame->ch_layout) || m_ptrContext->objAudioFilterSrc.freq != pFrame->sample_rate || m_ptrContext->auddec.m_nPktSerial != last_serial;

            if (reconfigure)
            {
                char buf1[1024], buf2[1024];
                av_channel_layout_describe(&m_ptrContext->objAudioFilterSrc.ch_layout, buf1, sizeof(buf1));
                av_channel_layout_describe(&pFrame->ch_layout, buf2, sizeof(buf2));
                av_log(nullptr, AV_LOG_DEBUG, "Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d fmt:%s layout:%s serial:%d\n",
                    m_ptrContext->objAudioFilterSrc.freq, m_ptrContext->objAudioFilterSrc.ch_layout.nb_channels, av_get_sample_fmt_name(m_ptrContext->objAudioFilterSrc.fmt), buf1, last_serial,
                    pFrame->sample_rate, pFrame->ch_layout.nb_channels, av_get_sample_fmt_name((AVSampleFormat)pFrame->format), buf2, m_ptrContext->auddec.m_nPktSerial);

                m_ptrContext->objAudioFilterSrc.fmt = (AVSampleFormat)pFrame->format;
                ret = av_channel_layout_copy(&m_ptrContext->objAudioFilterSrc.ch_layout, &pFrame->ch_layout);
                if (ret < 0)
                    goto the_end;
                m_ptrContext->objAudioFilterSrc.freq = pFrame->sample_rate;
                last_serial = m_ptrContext->auddec.m_nPktSerial;

                if ((ret = ConfigureAudioFilters(m_ptrContext, m_ptrContext->pAFilters, 1)) < 0)
                    goto the_end;
            }

            if ((ret = av_buffersrc_add_frame(m_ptrContext->pInAudioFilter, pFrame)) < 0)
                goto the_end;

            while ((ret = av_buffersink_get_frame_flags(m_ptrContext->pOutAudioFilter, pFrame, 0)) >= 0)
            {
                FrameData* fd = pFrame->opaque_ref ? (FrameData*)pFrame->opaque_ref->data : nullptr;
                tb = av_buffersink_get_time_base(m_ptrContext->pOutAudioFilter);
                if (!(af = m_ptrContext->sampq.PeekWritable()))
                    goto the_end;

                af->pts = (pFrame->pts == AV_NOPTS_VALUE) ? NAN : pFrame->pts * av_q2d(tb);
                af->pos = fd ? fd->pkt_pos : -1;
                af->serial = m_ptrContext->auddec.m_nPktSerial;
                af->duration = av_q2d({ pFrame->nb_samples, pFrame->sample_rate });

                av_frame_move_ref(af->pFrame, pFrame);
                m_ptrContext->sampq.Push();

                if (m_ptrContext->ptrAudioQueue->serial != m_ptrContext->auddec.m_nPktSerial)
                    break;
            }
            if (ret == AVERROR_EOF)
                m_ptrContext->auddec.m_nFinished = m_ptrContext->auddec.m_nPktSerial;
        }
    } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
the_end:
    m_ptrContext->ptrAgraph.reset();

    av_frame_free(&pFrame);

    av_log(nullptr, AV_LOG_DEBUG, "CYAudioDecoderFilter ret = %d.\n", ret);
    return;
}

/* return the number of undisplayed frames in the queue */
int FrameQueueNbRemaining(CYFrameQueue* f)
{
    return f->size - f->rindex_shown;
}

CYFrame* FrameQueuePeekReadable(CYFrameQueue* f)
{
    /* wait until we have a readable a new frame */
    UniqueLock locker(f->m_mutex);
    while (f->size - f->rindex_shown <= 0 &&
        !f->ptrQueue->bAbortRequest)
    {
        f->m_cvCond.wait(locker);
    }

    if (f->ptrQueue->bAbortRequest)
        return NULL;

    return &f->m_lstQueue[(f->rindex + f->rindex_shown) % f->max_size];
}

static int GetMasterSyncType(SharePtr<CYMediaContext>& ptrContext)
{
    if (ptrContext->nAVSyncType == TYPE_SYNC_CLOCK_VIDEO)
    {
        if (ptrContext->pVideoStream)
            return TYPE_SYNC_CLOCK_VIDEO;
        else
            return TYPE_SYNC_CLOCK_AUDIO;
    }
    else if (ptrContext->nAVSyncType == TYPE_SYNC_CLOCK_AUDIO)
    {
        if (ptrContext->pAudioStream)
            return TYPE_SYNC_CLOCK_AUDIO;
        else
            return TYPE_SYNC_CLOCK_EXTERNAL;
    }
    else
    {
        return TYPE_SYNC_CLOCK_EXTERNAL;
    }
}

/* get the current master clock value */
static double GetMasterClock(SharePtr<CYMediaContext>& ptrContext)
{
    double val;

    switch (GetMasterSyncType(ptrContext))
    {
    case TYPE_SYNC_CLOCK_VIDEO:
        val = ptrContext->vidclk.GetClock();
        break;
    case TYPE_SYNC_CLOCK_AUDIO:
        val = ptrContext->audclk.GetClock();
        break;
    default:
        val = ptrContext->extclk.GetClock();
        break;
    }
    return val;
}

/* return the wanted number of samples to get better sync if sync_type is video
 * or external master clock */
int SynchronizeAudio(SharePtr<CYMediaContext>& ptrContext, int nNbSamples)
{
    int nWantedNbSamples = nNbSamples;

    /* if not master, then we try to remove or add samples to correct the clock */
    if (GetMasterSyncType(ptrContext) != TYPE_SYNC_CLOCK_AUDIO)
    {
        double fDiff, fAvgDiff;
        int nMinNbSamples, nMaxNbSamples;

        fDiff = ptrContext->audclk.GetClock() - GetMasterClock(ptrContext);

        if (!isnan(fDiff) && fabs(fDiff) < AV_NOSYNC_THRESHOLD)
        {
            ptrContext->fAudioDiffCum = fDiff + ptrContext->fAudioDiffAvgCoef * ptrContext->fAudioDiffCum;
            if (ptrContext->nAudioDiffAvgCount < AUDIO_DIFF_AVG_NB)
            {
                /* not enough measures to have a correct estimate */
                ptrContext->nAudioDiffAvgCount++;
            }
            else
            {
                /* estimate the A-V difference */
                fAvgDiff = ptrContext->fAudioDiffCum * (1.0 - ptrContext->fAudioDiffAvgCoef);

                if (fabs(fAvgDiff) >= ptrContext->fAudioDiffThreshold)
                {
                    nWantedNbSamples = nNbSamples + (int)(fDiff * ptrContext->objAudioSrc.freq);
                    nMinNbSamples = ((nNbSamples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    nMaxNbSamples = ((nNbSamples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
                    nWantedNbSamples = av_clip(nWantedNbSamples, nMinNbSamples, nMaxNbSamples);
                }
                av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n", fDiff, fAvgDiff, nWantedNbSamples - nNbSamples, ptrContext->fAudioClock, ptrContext->fAudioDiffThreshold);
            }
        }
        else
        {
            /* too big difference : may be initial PTS errors, so
               reset A-V filter */
            ptrContext->nAudioDiffAvgCount = 0;
            ptrContext->fAudioDiffCum = 0;
        }
    }

    return nWantedNbSamples;
}

/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in ptrContext->audio_buf, with size in bytes given by the return
 * value.
 */
int AudioDecodeFrame(SharePtr<CYMediaContext>& ptrContext)
{
    int data_size, resampled_data_size;
    av_unused double audio_clock0;
    int nWantedNbSamples;
    CYFrame* af;

    if (ptrContext->bPaused)
        return -1;

    do
    {
#if defined(_WIN32)
        while (FrameQueueNbRemaining(&ptrContext->sampq) == 0)
        {
            if ((av_gettime_relative() - ptrContext->nAudioCallbackTime) > 1000000LL * ptrContext->nAudioHWBufSize / ptrContext->objAudioTgt.bytes_per_sec / 2)
                return -1;
            av_usleep(1000);
        }
#endif
        if (!(af = FrameQueuePeekReadable(&ptrContext->sampq)))
            return -1;

        //frame_queue_next(&ptrContext->sampq);
        ptrContext->sampq.Next();
    } while (af->serial != ptrContext->ptrAudioQueue->serial);

    if (af->pFrame->format == -1)
        return -1;

#ifdef DEBUG
    //////////////////////////////////////////////////////////////////////////
    AVRational tb = ptrContext->auddec.GetCodecContent()->time_base;
    double pts_seconds = af->pFrame->pts * av_q2d(tb);
    av_log(nullptr, AV_LOG_INFO, "decode audio time: %.3f sec\n", pts_seconds);
    //////////////////////////////////////////////////////////////////////////
#endif

    data_size = av_samples_get_buffer_size(NULL, af->pFrame->ch_layout.nb_channels,
        af->pFrame->nb_samples,
        (AVSampleFormat)af->pFrame->format, 1);

    nWantedNbSamples = SynchronizeAudio(ptrContext, af->pFrame->nb_samples);

    if (af->pFrame->format != ptrContext->objAudioSrc.fmt ||
        av_channel_layout_compare(&af->pFrame->ch_layout, &ptrContext->objAudioSrc.ch_layout) ||
        af->pFrame->sample_rate != ptrContext->objAudioSrc.freq ||
        (nWantedNbSamples != af->pFrame->nb_samples && !ptrContext->ptrSwrCtx))
    {
        int ret;
        ptrContext->ptrSwrCtx.reset();
        SwrContext* swr_ctx = nullptr;
        ret = swr_alloc_set_opts2(&swr_ctx,
            &ptrContext->objAudioTgt.ch_layout, ptrContext->objAudioTgt.fmt, ptrContext->objAudioTgt.freq,
            &af->pFrame->ch_layout, (AVSampleFormat)af->pFrame->format, af->pFrame->sample_rate,
            0, NULL);

        ptrContext->ptrSwrCtx.reset(swr_ctx);
        if (ret < 0 || swr_init(ptrContext->ptrSwrCtx.get()) < 0)
        {
            av_log(NULL, AV_LOG_ERROR,
                "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                af->pFrame->sample_rate, av_get_sample_fmt_name((AVSampleFormat)af->pFrame->format), af->pFrame->ch_layout.nb_channels,
                ptrContext->objAudioTgt.freq, av_get_sample_fmt_name(ptrContext->objAudioTgt.fmt), ptrContext->objAudioTgt.ch_layout.nb_channels);
            ptrContext->ptrSwrCtx.reset();
            return -1;
        }

        if (av_channel_layout_copy(&ptrContext->objAudioSrc.ch_layout, &af->pFrame->ch_layout) < 0)
            return -1;
        ptrContext->objAudioSrc.freq = af->pFrame->sample_rate;
        ptrContext->objAudioSrc.fmt = (AVSampleFormat)af->pFrame->format;
    }

    if (ptrContext->ptrSwrCtx)
    {
        const uint8_t** in = (const uint8_t**)af->pFrame->extended_data;
        uint8_t* audio_buf1 = nullptr;
        uint8_t** out = &audio_buf1;
        int out_count = (int64_t)nWantedNbSamples * ptrContext->objAudioTgt.freq / af->pFrame->sample_rate + 256;
        int out_size = av_samples_get_buffer_size(NULL, ptrContext->objAudioTgt.ch_layout.nb_channels, out_count, ptrContext->objAudioTgt.fmt, 0);
        int len2;
        if (out_size < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
            return -1;
        }
        if (nWantedNbSamples != af->pFrame->nb_samples)
        {
            if (swr_set_compensation(ptrContext->ptrSwrCtx.get(), (nWantedNbSamples - af->pFrame->nb_samples) * ptrContext->objAudioTgt.freq / af->pFrame->sample_rate,
                nWantedNbSamples * ptrContext->objAudioTgt.freq / af->pFrame->sample_rate) < 0)
            {
                av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
                return -1;
            }
        }

        if (!audio_buf1) ptrContext->nAudioBuf1Size = 0;
        av_fast_malloc(&audio_buf1, &ptrContext->nAudioBuf1Size, out_size);

        if (!audio_buf1)
            return AVERROR(ENOMEM);

        ptrContext->ptrAudioBuffer1.reset(audio_buf1);
        len2 = swr_convert(ptrContext->ptrSwrCtx.get(), out, out_count, in, af->pFrame->nb_samples);
        if (len2 < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
            return -1;
        }
        if (len2 == out_count)
        {
            av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
            if (swr_init(ptrContext->ptrSwrCtx.get()) < 0)
                ptrContext->ptrSwrCtx.reset();
        }
        ptrContext->ptrAudioBuffer.reset(ptrContext->ptrAudioBuffer1.release());
        resampled_data_size = len2 * ptrContext->objAudioTgt.ch_layout.nb_channels * av_get_bytes_per_sample(ptrContext->objAudioTgt.fmt);
    }
    else
    {
        ptrContext->ptrAudioBuffer.release();
        ptrContext->ptrAudioBuffer.reset(af->pFrame->data[0]);
        resampled_data_size = data_size;
    }

    audio_clock0 = ptrContext->fAudioClock;
    /* update the audio clock with the pts */
    if (!isnan(af->pts))
        ptrContext->fAudioClock = af->pts + (double)af->pFrame->nb_samples / af->pFrame->sample_rate;
    else
        ptrContext->fAudioClock = NAN;
    ptrContext->nAudioClockSerial = af->serial;
#ifdef DEBUG
    {
        static double last_clock;
        av_log(nullptr, AV_LOG_INFO, "audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n", ptrContext->fAudioClock - last_clock, ptrContext->fAudioClock, audio_clock0);
        last_clock = ptrContext->fAudioClock;
    }
#endif
    return resampled_data_size;
}

CYPLAYER_NAMESPACE_END