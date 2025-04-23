#include "ChainFilter/Common/CYHWAccel.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

#include "ChainFilter/Common/CYRenderer.hpp"

#ifdef __cplusplus
}
#endif

CYPLAYER_NAMESPACE_BEGIN

int CreateHwaccel(SharePtr<CYMediaContext>& ptrContext, AVBufferRef** pDeviceCtx)
{
    enum AVHWDeviceType eType;
    int ret;
    AVBufferRef* vk_dev = nullptr;

    *pDeviceCtx = nullptr;

    if (strlen(ptrContext->szHWAccel) == 0)
        return 0;

    eType = av_hwdevice_find_type_by_name(ptrContext->szHWAccel);
    if (eType == AV_HWDEVICE_TYPE_NONE)
        return AVERROR(ENOTSUP);

#if HAVE_VULKAN_RENDERER
    ret = vk_renderer_get_hw_dev(m_ptrVKRenderer.get(), &vk_dev);
    if (ret < 0)
        return ret;
#endif

    ret = av_hwdevice_ctx_create_derived(pDeviceCtx, eType, vk_dev, 0);
    if (!ret)
        return 0;

    if (ret != AVERROR(ENOSYS))
        return ret;

    av_log(nullptr, AV_LOG_WARNING, "Derive %s from vulkan not supported.\n", ptrContext->szHWAccel);
    ret = av_hwdevice_ctx_create(pDeviceCtx, eType, nullptr, nullptr, 0);
    return ret;
}

CYPLAYER_NAMESPACE_END