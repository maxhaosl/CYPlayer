#include "CYPlayer/CYPlayerFactory.hpp"
#include "CYPlayerImpl.hpp"

CYPLAYER_NAMESPACE_BEGIN

CYPlayerFactory::CYPlayerFactory()
{
}

CYPlayerFactory::~CYPlayerFactory()
{
}

ICYPlayer* CYPlayerFactory::CreatePlayer()
{
    return new CYPlayerImpl();
}

void CYPlayerFactory::DestroyPlayer(ICYPlayer*& pDevice)
{
    if (pDevice)
    {
        delete pDevice;
        pDevice = nullptr;
    }
}

CYPLAYER_NAMESPACE_END