#include "Common/Thread/CYCondition.hpp"
#include "Common/CYCommonDefine.hpp"

CYPLAYER_NAMESPACE_BEGIN

CYCondition::CYCondition()
{

}

CYCondition::~CYCondition()
{

}

ECondRetCode CYCondition::WaitTimeOut(int nMilliSeconds)
{
    UniqueLock locker(m_mutex);
    if (m_cv.wait_for(locker, std::chrono::microseconds(nMilliSeconds)) == std::cv_status::timeout)
    {
        return COND_RET_TIMEOUT;
    }
    return COND_RET_OK;
}

ECondRetCode CYCondition::Wait()
{
    UniqueLock locker(m_mutex);
    m_cv.wait(locker);
    return COND_RET_OK;
}

void CYCondition::NotifyOne()
{
    UniqueLock locker(m_mutex);
    m_cv.notify_one();
}

void CYCondition::NotifyALL()
{
    UniqueLock locker(m_mutex);
    m_cv.notify_all();
}

CYPLAYER_NAMESPACE_END