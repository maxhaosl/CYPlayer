#include "ChainFilter/ProcessFilter/CYProcessFilter.hpp"

CYPLAYER_NAMESPACE_BEGIN

CYProcessFilter::CYProcessFilter()
    : CYBaseFilter()
{
}

CYProcessFilter::~CYProcessFilter()
{
}

int16_t CYProcessFilter::Init(SharePtr<EPlayerParam>& ptrParam)
{
    return CYBaseFilter::Init(ptrParam);
}

int16_t CYProcessFilter::UnInit()
{
    return CYBaseFilter::UnInit();
}

int16_t CYProcessFilter::Start(SharePtr<CYMediaContext>& ptrContext)
{
    return CYBaseFilter::Start(ptrContext);
}

int16_t CYProcessFilter::Stop(SharePtr<CYMediaContext>& ptrContext)
{
    return CYBaseFilter::Stop(ptrContext);
}

int16_t CYProcessFilter::Pause()
{
    return CYBaseFilter::Pause();
}

int16_t CYProcessFilter::Resume()
{
    return CYBaseFilter::Resume();
}

int16_t CYProcessFilter::ProcessPacket(SharePtr<CYMediaContext>& ptrContext, AVPacketPtr& ptrPacket)
{
    return CYBaseFilter::ProcessPacket(ptrContext, ptrPacket);
}

int16_t CYProcessFilter::ProcessFrame(SharePtr<CYMediaContext>& ptrContext, AVFramePtr& ptrFrame)
{
    return CYBaseFilter::ProcessFrame(ptrContext, ptrFrame);
}

CYPLAYER_NAMESPACE_END