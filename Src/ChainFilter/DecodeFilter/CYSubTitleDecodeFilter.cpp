#include "ChainFilter/DecodeFilter/CYSubTitleDecodeFilter.hpp"

CYPLAYER_NAMESPACE_BEGIN

CYSubTitleDecodeFilter::CYSubTitleDecodeFilter()
    : CYBaseFilter()
{
}

CYSubTitleDecodeFilter::~CYSubTitleDecodeFilter()
{
}

int16_t CYSubTitleDecodeFilter::Init(SharePtr<EPlayerParam>& ptrParam)
{
    m_ptrParam = ptrParam;
    return CYBaseFilter::Init(ptrParam);
}

int16_t CYSubTitleDecodeFilter::UnInit()
{
    return CYBaseFilter::UnInit();
}

int16_t CYSubTitleDecodeFilter::Start(SharePtr<CYMediaContext>& ptrContext)
{
    m_ptrContext = ptrContext;
    std::function<void()> func = std::bind(&CYSubTitleDecodeFilter::OnEntry, this);
    m_ptrContext->subdec.Start(func, "VideoDecodeThread");
    return CYBaseFilter::Start(ptrContext);
}

int16_t CYSubTitleDecodeFilter::Stop(SharePtr<CYMediaContext>& ptrContext)
{
    ptrContext->subdec.Abort(ptrContext->subpq);
    ptrContext->subdec.Destroy();
    return CYBaseFilter::Stop(ptrContext);
}

int16_t CYSubTitleDecodeFilter::Pause()
{
    return CYBaseFilter::Pause();
}

int16_t CYSubTitleDecodeFilter::Resume()
{
    return CYBaseFilter::Resume();
}

int16_t CYSubTitleDecodeFilter::ProcessPacket(SharePtr<CYMediaContext>& ptrContext, AVPacketPtr& ptrPacket)
{
    return CYBaseFilter::ProcessPacket(ptrContext, ptrPacket);
}

int16_t CYSubTitleDecodeFilter::ProcessFrame(SharePtr<CYMediaContext>& ptrContext, AVFramePtr& ptrFrame)
{
    return CYBaseFilter::ProcessFrame(ptrContext, ptrFrame);
}

void CYSubTitleDecodeFilter::OnEntry()
{
    m_ptrContext->subdec.WaitStart();
    CYFrame* sp = nullptr;
    int got_subtitle;
    double pts;

    for (;;)
    {
        if (!(sp = m_ptrContext->subpq.PeekWritable()))
            return;

        if ((got_subtitle = m_ptrContext->subdec.DecodeFrame(nullptr, &sp->sub, m_ptrContext->nDecoderReorderPTS)) < 0)
            break;

        pts = 0;

        if (got_subtitle && sp->sub.format == 0)
        {
            if (sp->sub.pts != AV_NOPTS_VALUE)
                pts = sp->sub.pts / (double)AV_TIME_BASE;
            sp->pts = pts;
            sp->serial = m_ptrContext->subdec.m_nPktSerial;
            sp->width = m_ptrContext->subdec.m_ptrAVCtx->width;
            sp->height = m_ptrContext->subdec.m_ptrAVCtx->height;
            sp->uploaded = 0;

            /* now we can update the picture count */
            m_ptrContext->subpq.Push();
        }
        else if (got_subtitle)
        {
            avsubtitle_free(&sp->sub);
        }
    }
    return;
}

CYPLAYER_NAMESPACE_END