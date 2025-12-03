#include "ChainFilter/DecodeFilter/CYVideoDecodeFilter.hpp"
#include "ChainFilter/Common/CYVideoFilters.hpp"

CYPLAYER_NAMESPACE_BEGIN

CYVideoDecodeFilter::CYVideoDecodeFilter()
    : CYBaseFilter()
{
}

CYVideoDecodeFilter::~CYVideoDecodeFilter()
{
}

int16_t CYVideoDecodeFilter::Init(SharePtr<EPlayerParam>& ptrParam)
{
    m_ptrParam = ptrParam;
    return CYBaseFilter::Init(ptrParam);
}

int16_t CYVideoDecodeFilter::UnInit()
{
    return CYBaseFilter::UnInit();
}

int16_t CYVideoDecodeFilter::Start(SharePtr<CYMediaContext>& ptrContext)
{
    m_bStop = false;
    m_ptrContext = ptrContext;
    std::function<void()> func = std::bind(&CYVideoDecodeFilter::OnEntry, this);
    m_ptrContext->viddec.Start(func, "VideoDecodeThread");
    return CYBaseFilter::Start(ptrContext);
}

int16_t CYVideoDecodeFilter::Stop(SharePtr<CYMediaContext>& ptrContext)
{
    m_bStop = true;
    ptrContext->viddec.Abort(ptrContext->pictq);
    ptrContext->viddec.Destroy();
    return CYBaseFilter::Stop(ptrContext);
}

int16_t CYVideoDecodeFilter::Pause()
{
    return CYBaseFilter::Pause();
}

int16_t CYVideoDecodeFilter::Resume()
{
    return CYBaseFilter::Resume();
}

int16_t CYVideoDecodeFilter::ProcessPacket(SharePtr<CYMediaContext>& ptrContext, AVPacketPtr& ptrPacket)
{
    return CYBaseFilter::ProcessPacket(ptrContext, ptrPacket);
}

int16_t CYVideoDecodeFilter::ProcessFrame(SharePtr<CYMediaContext>& ptrContext, AVFramePtr& ptrFrame)
{
    return CYBaseFilter::ProcessFrame(ptrContext, ptrFrame);
}

int CYVideoDecodeFilter::GetMasterSyncType(SharePtr<CYMediaContext>& ptrContext)
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

int CYVideoDecodeFilter::GetVideoFrame(AVFrame* pFrame)
{
    int got_picture;

    if ((got_picture = m_ptrContext->viddec.DecodeFrame(pFrame, nullptr, m_ptrContext->nDecoderReorderPTS)) < 0)
        return -1;

    if (got_picture)
    {
        double dpts = NAN;

        if (pFrame->pts != AV_NOPTS_VALUE)
            dpts = av_q2d(m_ptrContext->pVideoStream->time_base) * pFrame->pts;

        pFrame->sample_aspect_ratio = av_guess_sample_aspect_ratio(m_ptrContext->ptrIC.get(), m_ptrContext->pVideoStream, pFrame);

        if (m_ptrParam->nFrameDrop > 0 || (m_ptrParam->nFrameDrop && GetMasterSyncType(m_ptrContext) != TYPE_SYNC_CLOCK_VIDEO))
        {
            if (pFrame->pts != AV_NOPTS_VALUE)
            {
                double fDiff = dpts - GetMasterSyncType(m_ptrContext);
                if (!isnan(fDiff) && fabs(fDiff) < AV_NOSYNC_THRESHOLD &&
                    fDiff - m_ptrContext->fFrameLastFilterDelay < 0 &&
                    m_ptrContext->viddec.m_nPktSerial == m_ptrContext->vidclk.m_fSerial &&
                    m_ptrContext->ptrVideoQueue->nb_packets)
                {
                    m_ptrContext->nFrameDropsEarly++;
                    av_frame_unref(pFrame);
                    got_picture = 0;
                }
            }
        }
    }

    return got_picture;
}

int CYVideoDecodeFilter::QueuePicture(AVFrame* pSrcFrame, double pts, double duration, int64_t pos, int serial)
{
    CYFrame* vp;

#if defined(DEBUG_SYNC)
    printf("frame_type=%c pts=%0.3f\n",
        av_get_picture_type_char(pSrcFrame->pict_type), pts);
#endif

    if (!(vp = m_ptrContext->pictq.PeekWritable()))
        return -1;

    vp->sar = pSrcFrame->sample_aspect_ratio;
    vp->uploaded = 0;

    vp->width = pSrcFrame->width;
    vp->height = pSrcFrame->height;
    vp->format = pSrcFrame->format;

    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;

    SetDefaultWindowSize(m_ptrContext, vp->width, vp->height, vp->sar);

    av_frame_move_ref(vp->pFrame, pSrcFrame);
    m_ptrContext->pictq.Push();
    return 0;
}

void CYVideoDecodeFilter::OnEntry()
{
    m_ptrContext->viddec.WaitStart();

    if (m_bStop) return;

    AVFrame* pFrame = av_frame_alloc();
    double pts = 0;
    double duration = 0;
    int ret = 0;
    AVRational tb = m_ptrContext->pVideoStream->time_base;
    AVRational frame_rate = av_guess_frame_rate(m_ptrContext->ptrIC.get(), m_ptrContext->pVideoStream, nullptr);

    AVFilterGraph* graph = nullptr;
    AVFilterContext* filt_out = nullptr, * filt_in = nullptr;
    int last_w = 0;
    int last_h = 0;
    enum AVPixelFormat last_format = (AVPixelFormat)-2;
    int last_serial = -1;
    int last_vfilter_idx = 0;

    if (!pFrame)
    {
        av_log(nullptr, AV_LOG_FATAL, "alloc frame failed.");
        // return AVERROR(ENOMEM);
        return;
    }

    for (;;)
    {
        ret = GetVideoFrame(pFrame);
        if (ret < 0)
            goto the_end;
        if (!ret)
            continue;

        if (last_w != pFrame->width
            || last_h != pFrame->height
            || last_format != pFrame->format
            || last_serial != m_ptrContext->viddec.m_nPktSerial
            || last_vfilter_idx != m_ptrContext->nVFilterIndex)
        {
            av_log(nullptr, AV_LOG_DEBUG,
                "Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s serial:%d\n",
                last_w, last_h,
                (const char*)av_x_if_null(av_get_pix_fmt_name(last_format), "none"), last_serial,
                pFrame->width, pFrame->height,
                (const char*)av_x_if_null(av_get_pix_fmt_name((AVPixelFormat)pFrame->format), "none"), m_ptrContext->viddec.m_nPktSerial);
            avfilter_graph_free(&graph);
            graph = avfilter_graph_alloc();
            if (!graph)
            {
                ret = AVERROR(ENOMEM);
                goto the_end;
            }
            graph->nb_threads = FILTER_NB_THREADS;
            if ((ret = ConfigureVideoFilters(graph, m_ptrContext, m_ptrContext->pVfiltersList ? m_ptrContext->pVfiltersList[m_ptrContext->nVFilterIndex] : nullptr, pFrame)) < 0)
            {
                SDL_Event event;
                event.type = FF_QUIT_EVENT;
                event.user.data1 = m_ptrContext.get();
                SDL_PushEvent(&event);
                goto the_end;
            }
            filt_in = m_ptrContext->pInVideoFilter;
            filt_out = m_ptrContext->pOutVideoFilter;
            last_w = pFrame->width;
            last_h = pFrame->height;
            last_format = (AVPixelFormat)pFrame->format;
            last_serial = m_ptrContext->viddec.m_nPktSerial;
            last_vfilter_idx = m_ptrContext->nVFilterIndex;
            frame_rate = av_buffersink_get_frame_rate(filt_out);
        }

        ret = av_buffersrc_add_frame(filt_in, pFrame);
        if (ret < 0)
            goto the_end;

        while (ret >= 0)
        {
            FrameData* fd;

            m_ptrContext->fFrameLastReturnedTime = av_gettime_relative() / 1000000.0;

            ret = av_buffersink_get_frame_flags(filt_out, pFrame, 0);
            if (ret < 0)
            {
                if (ret == AVERROR_EOF)
                    m_ptrContext->viddec.m_nFinished = m_ptrContext->viddec.m_nPktSerial;
                ret = 0;
                break;
            }

            fd = pFrame->opaque_ref ? (FrameData*)pFrame->opaque_ref->data : nullptr;

            m_ptrContext->fFrameLastFilterDelay = av_gettime_relative() / 1000000.0 - m_ptrContext->fFrameLastReturnedTime;
            if (fabs(m_ptrContext->fFrameLastFilterDelay) > AV_NOSYNC_THRESHOLD / 10.0)
                m_ptrContext->fFrameLastFilterDelay = 0;
            tb = av_buffersink_get_time_base(filt_out);
            duration = (frame_rate.num && frame_rate.den ? av_q2d(
                {
                    frame_rate.den, frame_rate.num
                }) : 0);
            pts = (pFrame->pts == AV_NOPTS_VALUE) ? NAN : pFrame->pts * av_q2d(tb);
            ret = QueuePicture(pFrame, pts, duration, fd ? fd->pkt_pos : -1, m_ptrContext->viddec.m_nPktSerial);
            av_frame_unref(pFrame);
            if (m_ptrContext->ptrVideoQueue->serial != m_ptrContext->viddec.m_nPktSerial)
                break;
        }

        if (ret < 0)
            goto the_end;
    }
the_end:
    avfilter_graph_free(&graph);
    av_frame_free(&pFrame);
    return;
}

CYPLAYER_NAMESPACE_END