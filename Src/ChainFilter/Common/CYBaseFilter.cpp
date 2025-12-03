#include "CYBaseFilter.hpp"

CYPLAYER_NAMESPACE_BEGIN

CYBaseFilter::CYBaseFilter()
{
}

CYBaseFilter::~CYBaseFilter()
{
}

SharePtr<CYBaseFilter> CYBaseFilter::SetNext(SharePtr<CYBaseFilter> ptrNext)
{
    m_ptrNext = ptrNext;
    ptrNext->m_ptrPreNext = shared_from_this();
    return ptrNext;
}

SharePtr<CYBaseFilter> CYBaseFilter::GetNext() const
{
    return m_ptrNext;
}

SharePtr<CYBaseFilter> CYBaseFilter::GetPreNext() const
{
    return m_ptrPreNext;
}

int16_t CYBaseFilter::Init(SharePtr<EPlayerParam>& ptrParam)
{
    if (m_ptrNext)
    {
        return m_ptrNext->Init(ptrParam);
    }
    return ERR_SUCESS;
}

int16_t CYBaseFilter::UnInit()
{
    if (m_ptrNext)
    {
        return m_ptrNext->UnInit();
    }
    return ERR_SUCESS;
}

int16_t CYBaseFilter::Start(SharePtr<CYMediaContext>& ptrContext)
{
    if (m_ptrNext)
    {
        return m_ptrNext->Start(ptrContext);
    }
    return ERR_SUCESS;
}

int16_t CYBaseFilter::Stop(SharePtr<CYMediaContext>& ptrContext)
{
    if (m_ptrNext)
    {
        return m_ptrNext->Stop(ptrContext);
    }
    return ERR_SUCESS;
}

int16_t CYBaseFilter::Pause()
{
    if (m_ptrNext)
    {
        return m_ptrNext->Pause();
    }
    return ERR_SUCESS;
}

int16_t CYBaseFilter::Resume()
{
    if (m_ptrNext)
    {
        return m_ptrNext->Resume();
    }
    return ERR_SUCESS;
}

int16_t CYBaseFilter::SetDisplaySize(int nWidth, int nHeight)
{
    if (m_ptrNext)
    {
        return m_ptrNext->SetDisplaySize(nWidth, nHeight);
    }
    return ERR_SUCESS;
}

int16_t CYBaseFilter::ProcessPacket(SharePtr<CYMediaContext>& ptrContext, AVPacketPtr& ptrPacket)
{
    if (m_ptrNext)
    {
        return m_ptrNext->ProcessPacket(ptrContext, ptrPacket);
    }
    return ERR_SUCESS;
}

int16_t CYBaseFilter::ProcessFrame(SharePtr<CYMediaContext>& ptrContext, AVFramePtr& ptrFrame)
{
    if (m_ptrNext)
    {
        return m_ptrNext->ProcessFrame(ptrContext, ptrFrame);
    }
    return ERR_SUCESS;
}

int16_t CYBaseFilter::ProcessReversePacket(SharePtr<CYMediaContext>& ptrContext, AVPacketPtr& ptrPacket)
{
    if (m_ptrPreNext)
    {
        return m_ptrPreNext->ProcessReversePacket(ptrContext, ptrPacket);
    }
    return ERR_SUCESS;
}

int16_t CYBaseFilter::ProcessReversePreFrame(SharePtr<CYMediaContext>& ptrContext, AVFramePtr& ptrFrame)
{
    if (m_ptrPreNext)
    {
        return m_ptrPreNext->ProcessReversePreFrame(ptrContext, ptrFrame);
    }
    return ERR_SUCESS;
}

CYPLAYER_NAMESPACE_END