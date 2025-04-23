#include "Common/Queue/CYFrameQueue.hpp"
#include "Common/Queue/CYPacketQueue.hpp"

CYPLAYER_NAMESPACE_BEGIN

CYFrameQueue::CYFrameQueue()
{

}

CYFrameQueue::~CYFrameQueue()
{

}

int CYFrameQueue::Init(std::shared_ptr<CYPLAYER_NAMESPACE::CYPacketQueue> ptrQueue, int max_size, int keep_last)
{
    int i;
    this->ptrQueue = ptrQueue;
    this->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
    this->keep_last = !!keep_last;
    for (i = 0; i < this->max_size; i++)
        if (!(this->m_lstQueue[i].pFrame = av_frame_alloc()))
            return AVERROR(ENOMEM);
    return 0;
}


void CYFrameQueue::NotifyOne()
{
    UniqueLock locker(m_mutex);
    m_cvCond.notify_one();
}

void CYFrameQueue::Destroy()
{
    int i;
    for (i = 0; i < this->max_size; i++)
    {
        CYFrame* vp = &this->m_lstQueue[i];
        UnRefItem(vp);
        av_frame_free(&vp->pFrame);
    }
}

/* return the number of undisplayed frames in the queue */
int CYFrameQueue::NbRemaining()
{
    return this->size - this->rindex_shown;
}


CYFrame* CYFrameQueue::PeekReadable()
{
    /* wait until we have a readable a new frame */
    UniqueLock locker(this->m_mutex);
    while (this->size - this->rindex_shown <= 0 &&
        !this->ptrQueue->bAbortRequest)
    {
        //SDL_CondWait(this->cond, this->mutex);
        this->m_cvCond.wait(locker);
    }
    //SDL_UnlockMutex(this->mutex);

    if (this->ptrQueue->bAbortRequest)
        return nullptr;

    return &this->m_lstQueue[(this->rindex + this->rindex_shown) % this->max_size];
}

void CYFrameQueue::Push()
{
    if (++this->windex == this->max_size)
        this->windex = 0;
    UniqueLock locker(this->m_mutex);
    //SDL_LockMutex(this->mutex);
    this->size++;
    //SDL_CondSignal(this->cond);
    this->m_cvCond.notify_one();
    //SDL_UnlockMutex(this->mutex);
}

void CYFrameQueue::Next()
{
    if (this->keep_last && !this->rindex_shown)
    {
        this->rindex_shown = 1;
        return;
    }
    UnRefItem(&this->m_lstQueue[this->rindex]);
    if (++this->rindex == this->max_size)
        this->rindex = 0;
    UniqueLock locker(this->m_mutex);
    //SDL_LockMutex(this->mutex);
    this->size--;
    //SDL_CondSignal(this->cond);
    this->m_cvCond.notify_one();
    //SDL_UnlockMutex(this->mutex);
}

CYFrame* CYFrameQueue::PeekWritable()
{
    /* wait until we have space to put a new frame */
    //SDL_LockMutex(this->mutex);
    UniqueLock locker(this->m_mutex);
    while (this->size >= this->max_size &&
        !this->ptrQueue->bAbortRequest)
    {
        //SDL_CondWait(this->cond, this->mutex);
        this->m_cvCond.wait(locker);
    }
    //SDL_UnlockMutex(this->mutex);

    if (this->ptrQueue->bAbortRequest)
        return nullptr;

    return &this->m_lstQueue[this->windex];
}

CYFrame* CYFrameQueue::Peek()
{
    return &this->m_lstQueue[(this->rindex + this->rindex_shown) % this->max_size];
}

CYFrame* CYFrameQueue::PeekNext()
{
    return &this->m_lstQueue[(this->rindex + this->rindex_shown + 1) % this->max_size];
}

CYFrame* CYFrameQueue::PeekLast()
{
    return &this->m_lstQueue[this->rindex];
}

/* return last shown position */
int64_t CYFrameQueue::LastPos()
{
    CYFrame* fp = &this->m_lstQueue[this->rindex];
    if (this->rindex_shown && fp->serial == this->ptrQueue->serial)
        return fp->pos;
    else
        return -1;
}

void CYFrameQueue::UnRefItem(CYFrame* vp)
{
    av_frame_unref(vp->pFrame);
    avsubtitle_free(&vp->sub);
}

CYPLAYER_NAMESPACE_END