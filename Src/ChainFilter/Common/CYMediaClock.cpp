#include "ChainFilter/Common/CYMediaClock.hpp"
#include "Common/CYFFmpegDefine.hpp"

CYPLAYER_NAMESPACE_BEGIN

CYMediaClock::CYMediaClock()
{

}

CYMediaClock::~CYMediaClock()
{

}

double CYMediaClock::GetClock()
{
    if (*m_pQueueSerial != m_fSerial)
        return NAN;
    if (m_bPaused)
    {
        return m_fPTS;
    }
    else
    {
        double time = av_gettime_relative() / 1000000.0;
        return m_fPTSDrift + time - (time - m_fLastUpdated) * (1.0 - m_fSpeed);
    }
}

void CYMediaClock::SetClockAt(double fPts, int nSerial, double fTime)
{
    this->m_fPTS = fPts;
    this->m_fLastUpdated = fTime;
    this->m_fPTSDrift = this->m_fPTS - fTime;
    this->m_fSerial = nSerial;
}

void CYMediaClock::SetClock(double fPts, int nSerial)
{
    double time = av_gettime_relative() / 1000000.0;
    SetClockAt(fPts, nSerial, time);
}

void CYMediaClock::SetClockSpeed(double fSpeed)
{
    SetClock(GetClock(), m_fSerial);
    this->m_fSpeed = fSpeed;
}

void CYMediaClock::InitClock(int* nQueueSerial)
{
    this->m_fSpeed = 1.0;
    this->m_bPaused = false;
    this->m_pQueueSerial = nQueueSerial;
    SetClock(NAN, -1);
}

void CYMediaClock::SyncClockToSlave(CYMediaClock& objSlave)
{
    double fClock = GetClock();
    double fSlaveClock = objSlave.GetClock();
    if (!isnan(fSlaveClock) && (isnan(fClock) || fabs(fClock - fSlaveClock) > AV_NOSYNC_THRESHOLD))
        SetClock(fSlaveClock, objSlave.m_fSerial);
}

CYPLAYER_NAMESPACE_END