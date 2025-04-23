#include "Common/Queue/CYPacketQueue.hpp"

CYPLAYER_NAMESPACE_BEGIN

CYPacketQueue::CYPacketQueue()
{

}

CYPacketQueue::~CYPacketQueue()
{

}

int  CYPacketQueue::Init()
{
    bAbortRequest = true;
    return 0;
}

void CYPacketQueue::Abort()
{
    UniqueLock locker(m_mutex);
    bAbortRequest = true;
    m_cvCond.notify_one();
}

void CYPacketQueue::Flush()
{
    UniqueLock locker(m_mutex);
    m_lstPkt.clear();
    nb_packets = 0;
    size = 0;
    duration = 0;
    serial++;
}

void CYPacketQueue::Destroy()
{
    Flush();
}

int  CYPacketQueue::Put(AVPacketPtr& ptrPkt2)
{    

    AVPacketPtr pkt1 = AVPacketPtrCreate();
    if (!pkt1)
    {
        av_packet_unref(ptrPkt2.get());
        return -1;
    }
    av_packet_move_ref(pkt1.get(), ptrPkt2.get());



    SharePtr<CYPacketWrapper> ptrPacket = MakeShared<CYPacketWrapper>();
    int ret;

    if (bAbortRequest)
        return -1;


    ptrPacket->ptrPkt = std::move(pkt1);
    ptrPacket->nSerial = serial;

    UniqueLock locker(m_mutex);
    m_lstPkt.push_back(ptrPacket);
    nb_packets++;
    size += ptrPacket->ptrPkt->size + sizeof(CYPacketWrapper);
    duration += ptrPacket->ptrPkt->duration;
    m_cvCond.notify_one();

    return 0;
}

int  CYPacketQueue::PutNullPacket(AVPacketPtr& ptrPkt, int stream_index)
{
    ptrPkt->stream_index = stream_index;
    return Put(ptrPkt);
}

int  CYPacketQueue::Get(AVPacketPtr& ptrPkt, int block, int* serial)
{
    SharePtr<CYPacketWrapper> ptrPacket;
    int ret;

    UniqueLock locker(m_mutex);
    for (;;)
    {
        if (bAbortRequest)
        {
            ret = -1;
            break;
        }

        if (m_lstPkt.size() > 0)
        {
            ptrPacket = m_lstPkt.front();
            m_lstPkt.pop_front();

            nb_packets--;
            size -= ptrPacket->ptrPkt->size + sizeof(CYPacketWrapper);
            duration -= ptrPacket->ptrPkt->duration;

            if (serial)
                *serial = ptrPacket->nSerial;
            ptrPkt = std::move(ptrPacket->ptrPkt);
            ret = 1;
            break;
        }
        else if (!block)
        {
            ret = 0;
            break;
        }
        else
        {
            m_cvCond.wait(locker);
        }
    }
    return ret;
}

void CYPacketQueue::Start()
{
    UniqueLock locker(m_mutex);
    if (bAbortRequest)
    {
        bAbortRequest = false;
        serial++;
    }
}

CYPLAYER_NAMESPACE_END