#include "ChainFilter/DemuxFilter/CYDemuxFilter.hpp"
#include "Logger/CYLoggerManager.hpp"
#include "ChainFilter/Common/CYAudioFilters.hpp"
#include "ChainFilter/Common/CYVideoFilters.hpp"
#include "ChainFilter/Common/CYHWAccel.hpp"
#include "ChainFilter/Common/CYDecoder.hpp"
#include "Common/CYFFmpegDefine.hpp"

#if __cplusplus
extern "C" {
#endif
#include "ChainFilter/Common/cmdutils.h"
#if __cplusplus
}
#endif

CYPLAYER_NAMESPACE_BEGIN

CYDemuxFilter::CYDemuxFilter()
    : CYBaseFilter()
{
}

CYDemuxFilter::~CYDemuxFilter()
{
}

int16_t CYDemuxFilter::Init(SharePtr<EPlayerParam>& ptrParam)
{
    m_ptrParam = ptrParam;
    return CYBaseFilter::Init(ptrParam);
}

int16_t CYDemuxFilter::UnInit()
{
    return CYBaseFilter::UnInit();
}

int16_t CYDemuxFilter::Start(SharePtr<CYMediaContext>& ptrContext)
{
    m_bRunning = false;
    if (m_thread.joinable())
        m_thread.join();

    m_ptrContext = ptrContext;
    m_bRunning = true;
    m_thread = std::thread(&CYDemuxFilter::OnEntry, this);
    return CYBaseFilter::Start(ptrContext);
}

int16_t CYDemuxFilter::Stop(SharePtr<CYMediaContext>& ptrContext)
{
    m_bRunning = false;
    if (m_thread.joinable())
        m_thread.join();

    return CYBaseFilter::Stop(ptrContext);
}

int16_t CYDemuxFilter::Pause()
{
    return CYBaseFilter::Pause();
}

int16_t CYDemuxFilter::Resume()
{
    return CYBaseFilter::Resume();
}

int16_t CYDemuxFilter::ProcessPacket(SharePtr<CYMediaContext>& ptrContext, AVPacketPtr& ptrPacket)
{
    return CYBaseFilter::ProcessPacket(ptrContext, ptrPacket);
}

int16_t CYDemuxFilter::ProcessFrame(SharePtr<CYMediaContext>& ptrContext, AVFramePtr& ptrFrame)
{
    return CYBaseFilter::ProcessFrame(ptrContext, ptrFrame);
}

int DecodeInterruptCallBack(void* ctx)
{
    CYMediaContext* pContext = (CYMediaContext*)ctx;
    return pContext->bAbortRequest;
}

int CYDemuxFilter::IsRealtime(AVFormatContext* s)
{
    if (!strcmp(s->iformat->name, "rtp")
        || !strcmp(s->iformat->name, "rtsp")
        || !strcmp(s->iformat->name, "sdp")
        )
        return 1;

    if (s->pb && (!strncmp(s->url, "rtp:", 4)
        || !strncmp(s->url, "udp:", 4)
        )
        )
        return 1;
    return 0;
}

int StreamComponentOpen(SharePtr<CYMediaContext>& ptrContext, int nStreamIndex);
void CYDemuxFilter::OnEntry()
{
    AVFormatContext* pIC = nullptr;
    int err, i, ret;
    int st_index[AVMEDIA_TYPE_NB] = { 0 };
    AVPacketPtr ptrPkt;
    int64_t stream_start_time = 0;
    int pkt_in_play_range = 0;
    const AVDictionaryEntry* t = nullptr;
    int scan_all_pmts_set = 0;
    int64_t pkt_ts = 0;

    memset(st_index, -1, sizeof(st_index));
    m_ptrContext->bEof = false;

    ptrPkt = AVPacketPtrCreate();
    if (!ptrPkt)
    {
        av_log(nullptr, AV_LOG_WARNING, "Could not allocate packet.\n");
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    pIC = avformat_alloc_context();
    if (!pIC)
    {
        av_log(nullptr, AV_LOG_WARNING, "Could not allocate context\n");
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    pIC->interrupt_callback.callback = DecodeInterruptCallBack;
    pIC->interrupt_callback.opaque = m_ptrContext.get();
    if (!av_dict_get(format_opts, "scan_all_pmts", nullptr, AV_DICT_MATCH_CASE))
    {
        av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
        scan_all_pmts_set = 1;
    }
    err = avformat_open_input(&pIC, m_ptrContext->pszFileName, m_ptrContext->iformat, &format_opts);
    if (err < 0)
    {
        print_error(m_ptrContext->pszFileName, err);
        ret = -1;
        goto fail;
    }
    if (scan_all_pmts_set)
        av_dict_set(&format_opts, "scan_all_pmts", nullptr, AV_DICT_MATCH_CASE);
    remove_avoptions(&format_opts, codec_opts);

    ret = check_avoptions(format_opts);
    if (ret < 0)
        goto fail;
    m_ptrContext->ptrIC.reset(pIC);

    if (m_ptrParam->bAutoGenPTS)
        pIC->flags |= AVFMT_FLAG_GENPTS;

    if (m_ptrParam->bFindStreamInfo)
    {
        AVDictionary** opts = nullptr;
        int orig_nb_streams = pIC->nb_streams;

        err = setup_find_stream_info_opts(pIC, codec_opts, &opts);
        if (err < 0)
        {
            av_log(nullptr, AV_LOG_ERROR, "Error setting up avformat_find_stream_info() options\n");
            ret = err;
            goto fail;
        }

        err = avformat_find_stream_info(pIC, opts);

        for (i = 0; i < orig_nb_streams; i++)
            av_dict_free(&opts[i]);
        av_freep(&opts);

        if (err < 0)
        {
            av_log(nullptr, AV_LOG_WARNING, "%s: could not find codec parameters\n", m_ptrContext->pszFileName);
            ret = -1;
            goto fail;
        }
    }

    if (pIC->pb)
        pIC->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

    if (m_ptrParam->nSeekByBytes < 0) m_ptrParam->nSeekByBytes = !(pIC->iformat->flags & AVFMT_NO_BYTE_SEEK) && !!(pIC->iformat->flags & AVFMT_TS_DISCONT) &&
        strcmp("ogg", pIC->iformat->name);

    m_ptrContext->fMaxFrameDuration = (pIC->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

//     if (!m_pszWindowTitle && (t = av_dict_get(pIC->metadata, "title", nullptr, 0)))
//         m_pszWindowTitle = av_asprintf("%s - %s", t->value, m_ptrContext->pszFileName);

    /* if seeking requested, we execute it */
    if (m_ptrContext->nStartTime != AV_NOPTS_VALUE)
    {
        int64_t timestamp;

        timestamp = m_ptrContext->nStartTime;
        /* add the stream start time */
        if (pIC->start_time != AV_NOPTS_VALUE)
            timestamp += pIC->start_time;
        ret = avformat_seek_file(pIC, -1, INT64_MIN, timestamp, INT64_MAX, 0);
        if (ret < 0)
        {
            av_log(nullptr, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n", m_ptrContext->pszFileName, (double)timestamp / AV_TIME_BASE);
        }
    }

    m_ptrContext->bRealTime = IsRealtime(pIC);

    if (m_ptrParam->bShowStatus)
        av_dump_format(pIC, 0, m_ptrContext->pszFileName, 0);

    m_ptrContext->nFileDuration = pIC->duration / 1000.0;

    for (i = 0; i < pIC->nb_streams; i++)
    {
        AVStream* st = pIC->streams[i];
        enum AVMediaType eType = st->codecpar->codec_type;
        st->discard = AVDISCARD_ALL;
        if (eType >= 0 && strlen(&m_ptrContext->szStreamSpec[eType][0]) > 0 && st_index[eType] == -1)
            if (avformat_match_stream_specifier(pIC, st, &m_ptrContext->szStreamSpec[eType][0]) > 0)
                st_index[eType] = i;
    }
    for (i = 0; i < AVMEDIA_TYPE_NB; i++)
    {
        if (strlen(&m_ptrContext->szStreamSpec[i][0]) > 0 && st_index[i] == -1)
        {
            av_log(nullptr, AV_LOG_ERROR, "Stream specifier %s does not match any %s stream\n", &m_ptrContext->szStreamSpec[i][0], av_get_media_type_string((AVMediaType)i));
            st_index[i] = INT_MAX;
        }
    }

    if (!m_ptrParam->bDisableVideo)
        st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(pIC, AVMEDIA_TYPE_VIDEO, st_index[AVMEDIA_TYPE_VIDEO], -1, nullptr, 0);
    if (!m_ptrParam->bDisableAudio)
        st_index[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(pIC, AVMEDIA_TYPE_AUDIO, st_index[AVMEDIA_TYPE_AUDIO], st_index[AVMEDIA_TYPE_VIDEO], nullptr, 0);
    if (!m_ptrParam->bDisableVideo && !m_ptrParam->bDisableSubTitle)
        st_index[AVMEDIA_TYPE_SUBTITLE] = av_find_best_stream(pIC, AVMEDIA_TYPE_SUBTITLE, st_index[AVMEDIA_TYPE_SUBTITLE], (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
            st_index[AVMEDIA_TYPE_AUDIO] : st_index[AVMEDIA_TYPE_VIDEO]), nullptr, 0);

    m_ptrContext->eShowMode = m_ptrContext->eShowMode;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0)
    {
        AVStream* st = pIC->streams[st_index[AVMEDIA_TYPE_VIDEO]];
        AVCodecParameters* codecpar = st->codecpar;
        AVRational sar = av_guess_sample_aspect_ratio(pIC, st, nullptr);
        if (codecpar->width)
            SetDefaultWindowSize(m_ptrContext, codecpar->width, codecpar->height, sar);
    }

    /* open the streams */
    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0)
    {
        StreamComponentOpen(m_ptrContext, st_index[AVMEDIA_TYPE_AUDIO]);
    }

    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0)
    {
        ret = StreamComponentOpen(m_ptrContext, st_index[AVMEDIA_TYPE_VIDEO]);
    }
    if (m_ptrContext->eShowMode == SHOW_MODE_NONE)
        m_ptrContext->eShowMode = ret >= 0 ? SHOW_MODE_VIDEO : SHOW_MODE_RDFT;

    if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0)
    {
        StreamComponentOpen(m_ptrContext, st_index[AVMEDIA_TYPE_SUBTITLE]);
    }

    if (m_ptrContext->nVideoStreamIndex < 0 && m_ptrContext->nAudioStreamIndex < 0)
    {
        av_log(nullptr, AV_LOG_FATAL, "Failed to open file '%s' or configure filtergraph\n", m_ptrContext->pszFileName);
        ret = -1;
        goto fail;
    }

    if (m_ptrParam->nInfiniteBuffer < 0 && m_ptrContext->bRealTime)
        m_ptrParam->nInfiniteBuffer = 1;

    while (m_bRunning)
    {
        if (m_ptrContext->bAbortRequest)
            break;
        if (m_ptrContext->bPaused != m_ptrContext->nLastPaused)
        {
            m_ptrContext->nLastPaused = m_ptrContext->bPaused;
            if (m_ptrContext->bPaused)
                m_ptrContext->nReadPauseReturn = av_read_pause(pIC);
            else
                av_read_play(pIC);
        }
#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
        if (m_ptrContext->bPaused && (!strcmp(ic->iformat->name, "rtsp") || (pIC->pb && !strncmp(input_filename, "mmsh:", 5))))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
#endif
        if (m_ptrContext->bSeekReq)
        {
            int64_t nSeekTarget = m_ptrContext->nSeekPos;
            int64_t nSeekMin = m_ptrContext->nSeekRel > 0 ? nSeekTarget - m_ptrContext->nSeekRel + 2 : INT64_MIN;
            int64_t nSeekMax = m_ptrContext->nSeekRel < 0 ? nSeekTarget - m_ptrContext->nSeekRel - 2 : INT64_MAX;

            if (m_ptrContext->bAccurate)
            {
                nSeekMin = nSeekTarget;
                nSeekMax = nSeekTarget;
            }

            av_log(nullptr, AV_LOG_INFO, "------------------------------------\n");
            av_log(nullptr, AV_LOG_INFO, "seeking target = %lld\n", nSeekTarget);

            ret = avformat_seek_file(m_ptrContext->ptrIC.get(), -1, nSeekMin, nSeekTarget, nSeekMax, m_ptrContext->nSeekFlags);
            if (ret < 0)
            {
                av_log(nullptr, AV_LOG_ERROR, "%s: error while seeking\n", m_ptrContext->ptrIC->url);
            }
            else
            {
                if (m_ptrContext->nAudioStreamIndex >= 0)
                {
                    m_ptrContext->ptrAudioQueue->Flush();
                }
                if (m_ptrContext->nSubtitleStreamIndex >= 0)
                {
                    m_ptrContext->ptrSubTitleQueue->Flush();
                }

                if (m_ptrContext->nVideoStreamIndex >= 0)
                {
                    m_ptrContext->ptrVideoQueue->Flush();
                }

                if (m_ptrContext->nSeekFlags & AVSEEK_FLAG_BYTE)
                {
                    m_ptrContext->extclk.SetClock(NAN, 0);
                }
                else
                {
                    m_ptrContext->extclk.SetClock(nSeekTarget / (double)AV_TIME_BASE, 0);
                }
            }
            m_ptrContext->bAccurate = false;
            m_ptrContext->bSeekReq = false;
            m_ptrContext->nQueueAttachmentsReq = 1;
            m_ptrContext->bEof = false;
            if (m_ptrContext->bPaused)
                StepToNextFrame();
        }
        if (m_ptrContext->nQueueAttachmentsReq)
        {
            if (m_ptrContext->pVideoStream && m_ptrContext->pVideoStream->disposition & AV_DISPOSITION_ATTACHED_PIC)
            {
                if ((ret = av_packet_ref(ptrPkt.get(), &m_ptrContext->pVideoStream->attached_pic)) < 0)
                    goto fail;
                m_ptrContext->ptrVideoQueue->Put(ptrPkt);

                m_ptrContext->ptrVideoQueue->PutNullPacket(ptrPkt, m_ptrContext->nVideoStreamIndex);
            }
            m_ptrContext->nQueueAttachmentsReq = 0;
        }

        /* if the queue are full, no need to read more */
        if (m_ptrParam->nInfiniteBuffer < 1 &&
            (m_ptrContext->ptrAudioQueue->size + m_ptrContext->ptrVideoQueue->size + m_ptrContext->ptrSubTitleQueue->size > MAX_QUEUE_SIZE
                || (StreamHasEnoughPackets(m_ptrContext->pAudioStream, m_ptrContext->nAudioStreamIndex, m_ptrContext->ptrAudioQueue) &&
                    StreamHasEnoughPackets(m_ptrContext->pVideoStream, m_ptrContext->nVideoStreamIndex, m_ptrContext->ptrVideoQueue) &&
                    StreamHasEnoughPackets(m_ptrContext->pSubTitleStream, m_ptrContext->nSubtitleStreamIndex, m_ptrContext->ptrSubTitleQueue))))
        {
            m_ptrContext->ptrReadCond->WaitTimeOut(10);
            continue;
        }
        if (!m_ptrContext->bPaused &&
            (!m_ptrContext->pAudioStream || (m_ptrContext->auddec.m_nFinished == m_ptrContext->ptrAudioQueue->serial && m_ptrContext->sampq.NbRemaining() == 0)) &&
            (!m_ptrContext->pVideoStream || (m_ptrContext->viddec.m_nFinished == m_ptrContext->ptrVideoQueue->serial && m_ptrContext->pictq.NbRemaining() == 0)))
        {
            if (m_ptrContext->bLoop || (m_ptrContext->nLoop != 1 && (!m_ptrContext->nLoop || --m_ptrContext->nLoop)))
            {
                StreamSeek(m_ptrContext->nStartTime != AV_NOPTS_VALUE ? m_ptrContext->nStartTime : 0, 0, 0);
            }
            else if (m_ptrContext->bAutoExit)
            {
                ret = AVERROR_EOF;
                goto fail;
            }
        }

        if (!ptrPkt)
        {
            std::cout << "1" << std::endl;
        }

        ret = av_read_frame(pIC, ptrPkt.get());
        if (ret < 0)
        {
            if ((ret == AVERROR_EOF || avio_feof(pIC->pb)) && !m_ptrContext->bEof)
            {
                if (m_ptrContext->nVideoStreamIndex >= 0)
                    m_ptrContext->ptrVideoQueue->PutNullPacket(ptrPkt, m_ptrContext->nVideoStreamIndex);
                if (m_ptrContext->nAudioStreamIndex >= 0)
                {
                    m_ptrContext->ptrAudioQueue->PutNullPacket(ptrPkt, m_ptrContext->nAudioStreamIndex);
                }

                if (m_ptrContext->nSubtitleStreamIndex >= 0)
                {
                    m_ptrContext->ptrSubTitleQueue->PutNullPacket(ptrPkt, m_ptrContext->nAudioStreamIndex);
                }

                m_ptrContext->bEof = true;
            }
            if (pIC->pb && pIC->pb->error)
            {
                if (m_ptrContext->bAutoExit)
                    goto fail;
                else
                    break;
            }
            m_ptrContext->ptrReadCond->WaitTimeOut(10);

            continue;
        }
        else
        {
            m_ptrContext->bEof = false;
        }
        /* check if packet is in play range specified by user, then queue, otherwise discard */
        stream_start_time = pIC->streams[ptrPkt->stream_index]->start_time;
        pkt_ts = ptrPkt->pts == AV_NOPTS_VALUE ? ptrPkt->dts : ptrPkt->pts;
        pkt_in_play_range = m_ptrContext->nDuration == AV_NOPTS_VALUE || (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
            av_q2d(pIC->streams[ptrPkt->stream_index]->time_base) - (double)(m_ptrContext->nStartTime != AV_NOPTS_VALUE ? m_ptrContext->nStartTime : 0) / 1000000 <= ((double)m_ptrContext->nDuration / 1000000);
       
        //---------------------------
        double fTimeBaseSec = av_q2d(pIC->streams[ptrPkt->stream_index]->time_base);
        int64_t stream_start = (pIC->streams[ptrPkt->stream_index]->start_time != AV_NOPTS_VALUE) ? pIC->streams[ptrPkt->stream_index]->start_time : 0;
        int64_t start_time_offset_us = (m_ptrContext->nStartTime != AV_NOPTS_VALUE) ? m_ptrContext->nStartTime : 0;
        double fDurationSec = (m_ptrContext->nDuration != AV_NOPTS_VALUE) ? (double)m_ptrContext->nDuration / 1000000.0 : -1.0;

        double fPktTimeSec = (pkt_ts - stream_start) * fTimeBaseSec;
        double fStartTimeOffsetSec = (double)start_time_offset_us / 1000000.0;
        //---------------------------
        
        if (ptrPkt->stream_index == m_ptrContext->nAudioStreamIndex && pkt_in_play_range)
        {
            m_ptrContext->ptrAudioQueue->Put(ptrPkt);
            av_log(nullptr, AV_LOG_INFO, "read audio fPktTimeSec: %.3f, fStartTimeOffsetSec: %.3f, fDurationSec: %.3f\n", fPktTimeSec, fStartTimeOffsetSec, fDurationSec);
        }
        else if (ptrPkt->stream_index == m_ptrContext->nVideoStreamIndex && pkt_in_play_range && !(m_ptrContext->pVideoStream->disposition & AV_DISPOSITION_ATTACHED_PIC))
        {
            m_ptrContext->ptrVideoQueue->Put(ptrPkt);
            av_log(nullptr, AV_LOG_INFO, "read video fPktTimeSec: %.3f, fStartTimeOffsetSec: %.3f, fDurationSec: %.3f\n", fPktTimeSec, fStartTimeOffsetSec, fDurationSec);
        }
        else if (ptrPkt->stream_index == m_ptrContext->nSubtitleStreamIndex && pkt_in_play_range)
        {
            m_ptrContext->ptrSubTitleQueue->Put(ptrPkt);
            av_log(nullptr, AV_LOG_INFO, "read subtitle fPktTimeSec: %.3f, fStartTimeOffsetSec: %.3f, fDurationSec: %.3f\n", fPktTimeSec, fStartTimeOffsetSec, fDurationSec);
        }
        else
        {
            av_packet_unref(ptrPkt.get());
        }
    }

    ret = 0;
fail:
    if (pIC && !m_ptrContext->ptrIC)
    {
        pIC = nullptr;
        m_ptrContext->ptrIC.reset();
    }

    ptrPkt.reset();
    if (ret != 0)
    {
        SDL_Event event;

        event.type = FF_QUIT_EVENT;
        event.user.data1 = m_ptrContext.get();
        SDL_PushEvent(&event);
    }
}

int AudioOpen(SharePtr<CYMediaContext>& ptrContext, AVChannelLayoutPtr& ptrChLayout, int wanted_sample_rate, struct CYAudioParams* audio_hw_params);
/* open a given stream. Return 0 if OK */
int StreamComponentOpen(SharePtr<CYMediaContext>& ptrContext, int nStreamIndex)
{
    AVCodecContextPtr ptrAVCtx;
    const AVCodec* codec = nullptr;
    const char* pszForcedCodecName = nullptr;
    AVDictionary* opts = nullptr;
    ptrContext->ptrChLayout = AVChannelLayoutPtr(new AVChannelLayout());
    int ret = 0;
    int nStreamLowres = ptrContext->nLowRes;

    if (nStreamIndex < 0 || nStreamIndex >= ptrContext->ptrIC->nb_streams)
        return -1;
    
    ptrAVCtx = AVCodecContextPtrCreate(nullptr);
    if (!ptrAVCtx)
        return AVERROR(ENOMEM);

    ret = avcodec_parameters_to_context(ptrAVCtx.get(), ptrContext->ptrIC->streams[nStreamIndex]->codecpar);
    if (ret < 0)
        goto fail;
    ptrAVCtx->pkt_timebase = ptrContext->ptrIC->streams[nStreamIndex]->time_base;

    codec = avcodec_find_decoder(ptrAVCtx->codec_id);

    switch (ptrAVCtx->codec_type)
    {
    case AVMEDIA_TYPE_AUDIO: ptrContext->nLastAudioStream = nStreamIndex; pszForcedCodecName = strlen(ptrContext->szForceAudioCodecName) ? ptrContext->szForceAudioCodecName : nullptr; break;
    case AVMEDIA_TYPE_SUBTITLE: ptrContext->nLastSubTitleStream = nStreamIndex; pszForcedCodecName = strlen(ptrContext->szForceSubtitleCodecName) ? ptrContext->szForceSubtitleCodecName : nullptr; break;
    case AVMEDIA_TYPE_VIDEO: ptrContext->nLastVideoStream = nStreamIndex; pszForcedCodecName = strlen(ptrContext->szForceVideoCodecName) ? ptrContext->szForceVideoCodecName : nullptr; break;
    }
    if (pszForcedCodecName)
        codec = avcodec_find_decoder_by_name(pszForcedCodecName);
    if (!codec)
    {
        if (pszForcedCodecName)
            av_log(nullptr, AV_LOG_WARNING, "No codec could be found with name '%s'\n", pszForcedCodecName);
        else
            av_log(nullptr, AV_LOG_WARNING, "No decoder could be found for codec %s\n", avcodec_get_name(ptrAVCtx->codec_id));
        ret = AVERROR(EINVAL);
        goto fail;
    }

    ptrAVCtx->codec_id = codec->id;
    if (nStreamLowres > codec->max_lowres)
    {
        av_log(ptrAVCtx.get(), AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n", codec->max_lowres);
        nStreamLowres = codec->max_lowres;
    }
    ptrAVCtx->lowres = nStreamLowres;

    if (ptrContext->bFastDecode)
        ptrAVCtx->flags2 |= AV_CODEC_FLAG2_FAST;

    ret = filter_codec_opts(codec_opts, ptrAVCtx->codec_id, ptrContext->ptrIC.get(), ptrContext->ptrIC->streams[nStreamIndex], codec, &opts, nullptr);
    if (ret < 0)
        goto fail;

    if (!av_dict_get(opts, "threads", nullptr, 0))
        av_dict_set(&opts, "threads", "auto", 0);
    if (nStreamLowres)
        av_dict_set_int(&opts, "lowres", nStreamLowres, 0);

    av_dict_set(&opts, "flags", "+copy_opaque", AV_DICT_MULTIKEY);

    if (ptrAVCtx->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        ret = CreateHwaccel(ptrContext, &ptrAVCtx->hw_device_ctx);
        if (ret < 0)
            goto fail;
    }

    if ((ret = avcodec_open2(ptrAVCtx.get(), codec, &opts)) < 0)
    {
        goto fail;
    }
    ret = check_avoptions(opts);
    if (ret < 0)
        goto fail;

    ptrContext->bEof = false;
    ptrContext->ptrIC->streams[nStreamIndex]->discard = AVDISCARD_DEFAULT;
    switch (ptrAVCtx->codec_type)
    {
    case AVMEDIA_TYPE_AUDIO:
    {
        AVFilterContext* sink;

        ptrContext->objAudioFilterSrc.freq = ptrAVCtx->sample_rate;
        ret = av_channel_layout_copy(&ptrContext->objAudioFilterSrc.ch_layout, &ptrAVCtx->ch_layout);
        if (ret < 0)
            goto fail;
        ptrContext->objAudioFilterSrc.fmt = ptrAVCtx->sample_fmt;
        if ((ret = ConfigureAudioFilters(ptrContext, ptrContext->pAFilters, 0)) < 0)
            goto fail;
        sink = ptrContext->pOutAudioFilter;
        ptrContext->nSampleRate = av_buffersink_get_sample_rate(sink);
        ret = av_buffersink_get_ch_layout(sink, ptrContext->ptrChLayout.get());
        if (ret < 0)
            goto fail;
    }

    ptrContext->nAudioStreamIndex = nStreamIndex;
    ptrContext->pAudioStream = ptrContext->ptrIC->streams[nStreamIndex];

    //////////////////////////////////////////////////////////////////////////

    /* prepare audio output */
    if ((ret = AudioOpen(ptrContext, ptrContext->ptrChLayout, ptrContext->nSampleRate, &ptrContext->objAudioTgt)) < 0)
        goto fail;
    ptrContext->nAudioHWBufSize = ret;
    ptrContext->objAudioSrc = ptrContext->objAudioTgt;
    ptrContext->nAudioBufSize = 0;
    ptrContext->nAudioBufIndex = 0;

    /* init averaging filter */
    ptrContext->fAudioDiffAvgCoef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
    ptrContext->nAudioDiffAvgCount = 0;
    /* since we do not have a precise anough audio FIFO fullness,
       we correct audio sync only if larger than this threshold */
    ptrContext->fAudioDiffThreshold = (double)(ptrContext->nAudioHWBufSize) / ptrContext->objAudioTgt.bytes_per_sec;

    //////////////////////////////////////////////////////////////////////////

    if ((ret = ptrContext->auddec.Init(ptrAVCtx.release(), ptrContext->ptrAudioQueue, ptrContext->ptrReadCond)) < 0)
        goto fail;

    if (ptrContext->ptrIC->iformat->flags & AVFMT_NOTIMESTAMPS)
    {
        ptrContext->auddec.m_nStartPts = ptrContext->pAudioStream->start_time;
        ptrContext->auddec.m_objStartPtsTb = ptrContext->pAudioStream->time_base;
    }

    ptrContext->auddec.NotifyStart();
    SDL_PauseAudioDevice(ptrContext->hAudioDev, 0);
    break;
    case AVMEDIA_TYPE_VIDEO:
        ptrContext->nVideoStreamIndex = nStreamIndex;
        ptrContext->pVideoStream = ptrContext->ptrIC->streams[nStreamIndex];

        if ((ret = ptrContext->viddec.Init(ptrAVCtx.release(), ptrContext->ptrVideoQueue, ptrContext->ptrReadCond)) < 0)
            goto fail;

        ptrContext->viddec.NotifyStart();
        ptrContext->nQueueAttachmentsReq = 1;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        ptrContext->nSubtitleStreamIndex = nStreamIndex;
        ptrContext->pSubTitleStream = ptrContext->ptrIC->streams[nStreamIndex];

        if ((ret = ptrContext->subdec.Init(ptrAVCtx.release(), ptrContext->ptrSubTitleQueue, ptrContext->ptrReadCond)) < 0)
            goto fail;

        ptrContext->subdec.NotifyStart();
        break;
    default:
        break;
    }
    goto out;

fail:
    ptrAVCtx.reset();
out:
    ptrContext->ptrChLayout.reset();
    av_dict_free(&opts);

    return ret;
}

void CYDemuxFilter::StreamTogglePause()
{
    if (m_ptrContext->bPaused)
    {
        m_ptrContext->fFrameTimer += av_gettime_relative() / 1000000.0 - m_ptrContext->vidclk.m_fLastUpdated;
        if (m_ptrContext->nReadPauseReturn != AVERROR(ENOSYS))
        {
            m_ptrContext->vidclk.m_bPaused = false;
        }
        m_ptrContext->vidclk.SetClock(m_ptrContext->vidclk.GetClock(), m_ptrContext->vidclk.m_fSerial);
    }
    m_ptrContext->extclk.SetClock(m_ptrContext->extclk.GetClock(), m_ptrContext->extclk.m_fSerial);
    m_ptrContext->bPaused = m_ptrContext->audclk.m_bPaused = m_ptrContext->vidclk.m_bPaused = m_ptrContext->extclk.m_bPaused = !m_ptrContext->bPaused;
}

void CYDemuxFilter::StepToNextFrame()
{
    /* if the stream is bPaused unpause it, then step */
    if (m_ptrContext->bPaused)
        StreamTogglePause();
    m_ptrContext->bStep = true;
}

int CYDemuxFilter::StreamHasEnoughPackets(AVStream* st, int stream_id, std::shared_ptr<CYPacketQueue>& ptrQueue)
{
    return stream_id < 0 || ptrQueue->bAbortRequest || (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
        ptrQueue->nb_packets > MIN_FRAMES && (!ptrQueue->duration || av_q2d(st->time_base) * ptrQueue->duration > 1.0);
}

/* seek in the stream */
void CYDemuxFilter::StreamSeek(int64_t pos, int64_t rel, int by_bytes)
{
    if (!m_ptrContext->bSeekReq)
    {
        m_ptrContext->nSeekPos = pos;
        m_ptrContext->nSeekRel = rel;
        m_ptrContext->nSeekFlags &= ~AVSEEK_FLAG_BYTE;
        if (by_bytes)
            m_ptrContext->nSeekFlags |= AVSEEK_FLAG_BYTE;
        m_ptrContext->bSeekReq = true;
        m_ptrContext->ptrReadCond->NotifyOne();
    }
}


int64_t CYDemuxFilter::GetDuration() const
{
    return m_ptrContext->nFileDuration;
}

int64_t CYDemuxFilter::GetPosition() const
{
    return m_ptrContext->extclk.m_fPTS * 1000;
}

int16_t CYDemuxFilter::Seek(int64_t nTimestamp)
{
    m_ptrParam->nSeekByBytes = 0;
    m_ptrContext->bAccurate = true;
    double fPos = (double)nTimestamp / 1000.0;
    if (m_ptrContext->ptrIC->start_time != AV_NOPTS_VALUE)
    {
        double fStartTimeSec = (double)m_ptrContext->ptrIC->start_time / AV_TIME_BASE;
        if (fPos < fStartTimeSec)
            fPos = fStartTimeSec;
    }

    int64_t nSeekTargetUS = (int64_t)(fPos * AV_TIME_BASE);
    int64_t nIncrUS = (int64_t)(5 * AV_TIME_BASE);

    nIncrUS = 1;

    StreamSeek(nSeekTargetUS, nIncrUS, 0);

    return ERR_SUCESS;
}


int16_t CYDemuxFilter::SetLoop(bool bLoop)
{
    m_ptrContext->bLoop = bLoop;
    return ERR_SUCESS;
}


CYPLAYER_NAMESPACE_END