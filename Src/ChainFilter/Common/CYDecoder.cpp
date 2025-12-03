#include "ChainFilter/Common/CYDecoder.hpp"
#include "ChainFilter/Context/CYMediaContext.hpp"

extern int decoder_reorder_pts;

CYPLAYER_NAMESPACE_BEGIN

CYDecoder::CYDecoder()
{
}

CYDecoder::~CYDecoder()
{
}

int CYDecoder::Init(SharePtr<CYMediaContext>& ptrContext, AVCodecContext* pAVCtx, SharePtr<CYPacketQueue>& ptrQueue, SharePtr<CYCondition>& ptrCond)
{
    m_ptrContext = ptrContext;
    m_ptrPkt = AVPacketPtrCreate();// av_packet_alloc();
    if (!m_ptrPkt)
        return AVERROR(ENOMEM);
    m_ptrAVCtx.reset(pAVCtx);
    m_ptrQueue = ptrQueue;
    m_ptrEmptyCond = ptrCond;
    m_nStartPts = AV_NOPTS_VALUE;
    m_nPktSerial = -1;
    m_ptrQueue->Start();
    return 0;
}

int CYDecoder::DecodeFrame(AVFrame* pFrame, AVSubtitle* pSub, int ndecoderReorderPts)
{
    int ret = AVERROR(EAGAIN);

    for (;;)
    {
        if (m_ptrQueue->serial == m_nPktSerial)
        {
            do
            {
                if (m_ptrQueue->bAbortRequest)
                    return -1;

                switch (m_ptrAVCtx->codec_type)
                {
                case AVMEDIA_TYPE_VIDEO:
                    ret = avcodec_receive_frame(m_ptrAVCtx.get(), pFrame);
                    if (ret >= 0)
                    {
                        if (ndecoderReorderPts == -1)
                        {
                            pFrame->pts = pFrame->best_effort_timestamp;
                        }
                        else if (!ndecoderReorderPts)
                        {
                            pFrame->pts = pFrame->pkt_dts;
                        }
                    }
                    break;
                case AVMEDIA_TYPE_AUDIO:
                    ret = avcodec_receive_frame(m_ptrAVCtx.get(), pFrame);
                    if (ret >= 0)
                    {
                        AVRational tb = { 1, pFrame->sample_rate };
                        if (pFrame->pts != AV_NOPTS_VALUE)
                            pFrame->pts = av_rescale_q(pFrame->pts, m_ptrAVCtx->pkt_timebase, tb);
                        else if (m_nNextPts != AV_NOPTS_VALUE)
                            pFrame->pts = av_rescale_q(m_nNextPts, m_objNextPtsTb, tb);
                        if (pFrame->pts != AV_NOPTS_VALUE)
                        {
                            m_nNextPts = pFrame->pts + pFrame->nb_samples;
                            m_objNextPtsTb = tb;
                        }
                    }
                    break;
                }
                if (ret == AVERROR_EOF)
                {
                    m_nFinished = m_nPktSerial;
                    avcodec_flush_buffers(m_ptrAVCtx.get());
                    return 0;
                }
                if (ret >= 0)
                    return 1;
            } while (ret != AVERROR(EAGAIN));
        }

        do
        {
            if (m_ptrQueue->nb_packets == 0)
                m_ptrEmptyCond->NotifyOne();
            if (m_nPacketPending)
            {
                m_nPacketPending = 0;
            }
            else
            {
                int old_serial = m_nPktSerial;
                if (m_ptrQueue->Get(m_ptrPkt, 1, &m_nPktSerial) < 0)
                    return -1;

                if (old_serial != m_nPktSerial)
                {
                    avcodec_flush_buffers(m_ptrAVCtx.get());
                    m_nFinished = 0;
                    m_nNextPts = m_nStartPts;
                    m_objNextPtsTb = m_objStartPtsTb;
                }
            }
            if (m_ptrQueue->serial == m_nPktSerial)
                break;
            av_packet_unref(m_ptrPkt.get());
        } while (1);

        if (m_ptrAVCtx->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            int nGotFrame = 0;
            ret = avcodec_decode_subtitle2(m_ptrAVCtx.get(), pSub, &nGotFrame, m_ptrPkt.get());
            if (ret < 0)
            {
                ret = AVERROR(EAGAIN);
            }
            else
            {
                if (nGotFrame && !m_ptrPkt->data)
                {
                    m_nPacketPending = 1;
                }
                ret = nGotFrame ? 0 : (m_ptrPkt->data ? AVERROR(EAGAIN) : AVERROR_EOF);
            }
            av_packet_unref(m_ptrPkt.get());
        }
        else
        {
            if (m_ptrPkt->buf && !m_ptrPkt->opaque_ref)
            {
                FrameData* fd;

                m_ptrPkt->opaque_ref = av_buffer_allocz(sizeof(*fd));
                if (!m_ptrPkt->opaque_ref)
                    return AVERROR(ENOMEM);
                fd = (FrameData*)m_ptrPkt->opaque_ref->data;
                fd->pkt_pos = m_ptrPkt->pos;
            }

#ifdef DEBUG
            //---------------------------
            int64_t pkt_ts = m_ptrPkt->pts == AV_NOPTS_VALUE ? m_ptrPkt->dts : m_ptrPkt->pts;

            double fTimeBaseSec = av_q2d(m_ptrContext->ptrIC->streams[m_ptrPkt->stream_index]->time_base);
            int64_t stream_start = (m_ptrContext->ptrIC->streams[m_ptrPkt->stream_index]->start_time != AV_NOPTS_VALUE) ? m_ptrContext->ptrIC->streams[m_ptrPkt->stream_index]->start_time : 0;
            int64_t start_time_offset_us = (m_ptrContext->nStartTime != AV_NOPTS_VALUE) ? m_ptrContext->nStartTime : 0;
            double fDurationSec = (m_ptrContext->nDuration != AV_NOPTS_VALUE) ? (double)m_ptrContext->nDuration / 1000000.0 : -1.0;

            double fPktTimeSec = (pkt_ts - stream_start) * fTimeBaseSec;
            double fStartTimeOffsetSec = (double)start_time_offset_us / 1000000.0;
            //---------------------------

            av_log(nullptr, AV_LOG_INFO, "avcodec send packet fPktTimeSec: %.3f, fStartTimeOffsetSec: %.3f, fDurationSec: %.3f\n", fPktTimeSec, fStartTimeOffsetSec, fDurationSec);
#endif

            if (avcodec_send_packet(m_ptrAVCtx.get(), m_ptrPkt.get()) == AVERROR(EAGAIN))
            {
                av_log(m_ptrAVCtx.get(), AV_LOG_ERROR, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                m_nPacketPending = 1;
            }
            else
            {
                av_packet_unref(m_ptrPkt.get());
            }
        }
    }
}

void CYDecoder::Destroy()
{
    m_objStartDecodeCond.NotifyALL();
    m_ptrPkt.reset();
    m_ptrAVCtx.reset();
}

int CYDecoder::Start(std::function<void()> fun, const char* pThreadName)
{
    if (m_ptrQueue)
        m_ptrQueue->Start();

    m_thread = std::thread(fun);
    return 0;
}

void CYDecoder::Abort(CYFrameQueue& objQueue)
{
    m_objStartDecodeCond.NotifyALL();
    if (m_ptrQueue) m_ptrQueue->Abort();
    objQueue.NotifyOne();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    if (m_ptrQueue) m_ptrQueue->Flush();
}

AVCodecContext* CYDecoder::GetCodecContent()
{
    return m_ptrAVCtx.get();
}

CYPLAYER_NAMESPACE_END