#include "ChainFilter/RenderFilter/CYVideoRenderFilter.hpp"
#include "Common/CYFFmpegDefine.hpp"
#include "ChainFilter/Common/CYVideoFilters.hpp"
#include "ChainFilter/Common/CYHWAccel.hpp"
#include "ChainFilter/Common/CYAudioFilters.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#if __cplusplus
extern "C" {
#endif
#include "ChainFilter/Common/cmdutils.h"
#if __cplusplus
}
#endif

CYPLAYER_NAMESPACE_BEGIN

CYVideoRenderFilter::CYVideoRenderFilter(EVideoRenderType eVideoType)
    : CYBaseFilter()
{
}

CYVideoRenderFilter::~CYVideoRenderFilter()
{
}

int16_t CYVideoRenderFilter::Init(SharePtr<EPlayerParam>& ptrParam)
{
    m_ptrParam = ptrParam;
    m_bDisableAudio = ptrParam->bDisableAudio;
    m_bDisableVideo = ptrParam->bDisableVideo;
    m_bDisableSubTitle = ptrParam->bDisableSubTitle;

    m_nSDLFlag = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
    if (ptrParam->bDisableAudio)
        m_nSDLFlag &= ~SDL_INIT_AUDIO;
    else
    {
        /* Try to work around an occasional ALSA buffer underflow issue when the
         * period size is NPOT due to ALSA resampling by forcing the buffer size. */
        if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE"))
            SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE", "1", 1);
    }
    if (ptrParam->bDisableVideo)
        m_nSDLFlag &= ~SDL_INIT_VIDEO;

    if (SDL_InitSubSystem(m_nSDLFlag) != 0)
    {
        av_log(nullptr, AV_LOG_FATAL, "Could not SDL_InitSubSystem - %s\n", SDL_GetError());
        return false;
    };

    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

    if (!ptrParam->bDisableVideo)
    {
        int flags = SDL_WINDOW_HIDDEN;
        if (m_bAlwaysOnTop)
#if SDL_VERSION_ATLEAST(2,0,5)
            flags |= SDL_WINDOW_ALWAYS_ON_TOP;
#else
            av_log(nullptr, AV_LOG_WARNING, "Your SDL version doesn't support SDL_WINDOW_ALWAYS_ON_TOP. Feature will be inactive.\n");
#endif
        if (m_bBorderLess)
            flags |= SDL_WINDOW_BORDERLESS;
        else
            flags |= SDL_WINDOW_RESIZABLE;

#ifdef SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR
        SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif
        if (strlen(m_szHWaccel) > 0 && !m_bEnableVulkan)
        {
            av_log(nullptr, AV_LOG_INFO, "Enable vulkan renderer to support hwaccel %s\n", m_szHWaccel);
            m_bEnableVulkan = true;
        }
#if HAVE_VULKAN_RENDERER
        if (m_bEnableVulkan)
        {
            m_ptrVKRenderer = CreateVkRender();
            if (m_ptrVKRenderer)
            {
#if SDL_VERSION_ATLEAST(2, 0, 6)
                flags |= SDL_WINDOW_VULKAN;
#endif
            }
            else
            {
                av_log(nullptr, AV_LOG_WARNING, "Doesn't support vulkan renderer, fallback to SDL renderer\n");
                m_bEnableVulkan = false;
            }
        }
#endif
    }

    return CYBaseFilter::Init(ptrParam);
}

int16_t CYVideoRenderFilter::UnInit()
{
    DoExit(m_ptrContext);

    m_ptrWindow.reset();
    m_ptrRenderer.reset();
    if (m_ptrContext)
    {
        m_ptrContext->ptrWindow.reset();
        m_ptrContext->ptrRenderer.reset();
    }

#if HAVE_VULKAN_RENDERER
    m_ptrVKRenderer.reset();
    m_ptrContext->ptrVKRenderer.reset();
#endif

    av_log(nullptr, AV_LOG_QUIET, "%s", "");
    int16_t nRet = CYBaseFilter::UnInit();

    SDL_QuitSubSystem(m_nSDLFlag);
    m_nSDLFlag = 0;
    SDL_Quit();
    return nRet;
}

int16_t CYVideoRenderFilter::Start(SharePtr<CYMediaContext>& ptrContext)
{
    m_ptrContext = ptrContext;
    m_ptrContext->nScreenWidth = m_nScreenWidth;
    m_ptrContext->nScreenHeight = m_nScreenHeight;
    if (m_ptrWindow) m_ptrContext->ptrWindow = std::move(m_ptrWindow);
    m_ptrContext->objRendererInfo = m_objRendererInfo;
    if (m_ptrRenderer) m_ptrContext->ptrRenderer = std::move(m_ptrRenderer);
#if HAVE_VULKAN_RENDERER
    if (m_ptrVKRenderer) m_ptrContext->ptrVKRenderer = std::move(m_ptrVKRenderer);
#endif
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    m_thread = std::thread(&CYVideoRenderFilter::OnEntry, this);
    return CYBaseFilter::Start(ptrContext);
}

int16_t CYVideoRenderFilter::Stop(SharePtr<CYMediaContext>& ptrContext)
{
    SDL_Event objEvent;
    objEvent.type = FF_QUIT_EVENT;
    objEvent.user.data1 = ptrContext.get();
    SDL_PushEvent(&objEvent);

    if (m_thread.joinable())
    {
        m_thread.join();
    }
    DoExit(m_ptrContext);
    return CYBaseFilter::Stop(ptrContext);
}

int16_t CYVideoRenderFilter::Pause()
{
    if (!m_ptrContext->bPaused)
    {
        StreamTogglePause(m_ptrContext);
    }
    return CYBaseFilter::Pause();
}

int16_t CYVideoRenderFilter::Resume()
{
    if (m_ptrContext->bPaused)
    {
        StreamTogglePause(m_ptrContext);
    }
    return CYBaseFilter::Resume();
}

int16_t CYVideoRenderFilter::SetDisplaySize(int nWidth, int nHeight)
{
    m_nScreenWidth = nWidth;
    m_nScreenHeight = nHeight;

    if (m_ptrWindow)
    {
        SDL_SetWindowSize(m_ptrWindow.get(), nWidth, nHeight);

        m_ptrRenderer = SDLRendererPtr(SDL_CreateRenderer(m_ptrWindow.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
        if (!m_ptrRenderer)
        {
            av_log(nullptr, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError());
            //renderer = SDL_CreateRenderer(window, -1, 0);
            m_ptrRenderer = SDLRendererPtr(SDL_CreateRenderer(m_ptrWindow.get(), -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_SOFTWARE));
            if (!m_ptrRenderer)
            {
                av_log(nullptr, AV_LOG_WARNING, "Failed to create software renderer.: %s\n", SDL_GetError());
            }
        }
        if (m_ptrRenderer)
        {
            if (!SDL_GetRendererInfo(m_ptrRenderer.get(), &m_objRendererInfo))
                av_log(nullptr, AV_LOG_VERBOSE, "Initialized %s renderer.\n", m_objRendererInfo.name);
        }
        if (!m_ptrRenderer || !m_objRendererInfo.num_texture_formats)
        {
            av_log(nullptr, AV_LOG_FATAL, "Failed to create window or renderer: %s", SDL_GetError());
            return ERR_SDL_CREATE_RENDERDER_FAILED;
        }
    }
    else if (m_ptrContext && m_ptrContext->ptrWindow)
    {
        SDL_SetWindowSize(m_ptrContext->ptrWindow.get(), nWidth, nHeight);

        m_ptrRenderer = SDLRendererPtr(SDL_CreateRenderer(m_ptrContext->ptrWindow.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
        if (!m_ptrRenderer)
        {
            av_log(nullptr, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError());
            //renderer = SDL_CreateRenderer(window, -1, 0);
            m_ptrRenderer = SDLRendererPtr(SDL_CreateRenderer(m_ptrContext->ptrWindow.get(), -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_SOFTWARE));
            if (!m_ptrRenderer)
            {
                av_log(nullptr, AV_LOG_WARNING, "Failed to create software renderer.: %s\n", SDL_GetError());
            }
        }
        if (m_ptrRenderer)
        {
            if (!SDL_GetRendererInfo(m_ptrRenderer.get(), &m_objRendererInfo))
                av_log(nullptr, AV_LOG_VERBOSE, "Initialized %s renderer.\n", m_objRendererInfo.name);
        }
        if (!m_ptrRenderer || !m_objRendererInfo.num_texture_formats)
        {
            av_log(nullptr, AV_LOG_FATAL, "Failed to create window or renderer: %s", SDL_GetError());
            return ERR_SDL_CREATE_RENDERDER_FAILED;
        }

        if (m_ptrRenderer) m_ptrContext->ptrRenderer = std::move(m_ptrRenderer);
    }

    return CYBaseFilter::SetDisplaySize(nWidth, nHeight);
}

int16_t CYVideoRenderFilter::ProcessPacket(SharePtr<CYMediaContext>& ptrContext, AVPacketPtr& ptrPacket)
{
    return CYBaseFilter::ProcessPacket(ptrContext, ptrPacket);
}

int16_t CYVideoRenderFilter::ProcessFrame(SharePtr<CYMediaContext>& ptrContext, AVFramePtr& ptrFrame)
{
    return CYBaseFilter::ProcessFrame(ptrContext, ptrFrame);
}

/**
 * Set Render Window.
 */
int16_t CYVideoRenderFilter::SetWindow(void* hWnd)
{
#ifdef _WIN32
    RECT rcClient;
    GetClientRect((HWND)hWnd, &rcClient);
    m_nScreenWidth = rcClient.right - rcClient.left;
    m_nScreenHeight = rcClient.bottom - rcClient.top;
#endif

    if (!m_bDisableVideo)
    {
        // window = SDL_CreateWindow(program_name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, nDefaultWidth, nDefaultHeight, flags);
        m_ptrWindow = SDLWindowPtr(SDL_CreateWindowFrom((const void*)hWnd));
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        if (!m_ptrWindow)
        {
            av_log(nullptr, AV_LOG_FATAL, "Failed to create window: %s", SDL_GetError());
            {
                SDL_QuitSubSystem(SDL_INIT_VIDEO);
                SDL_SetHint("SDL_VIDEODRIVER", "x11");
                if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
                {
                    av_log(nullptr, AV_LOG_FATAL, "Failed to initialize video subsystem - %s\n", SDL_GetError());
                    UnInit();
                    return ERR_INIT_SDL_SUBSYSTEM_FAILED;
                }

                m_ptrWindow = SDLWindowPtr(SDL_CreateWindowFrom((const void*)hWnd));
                if (!m_ptrWindow)
                {
                    av_log(nullptr, AV_LOG_FATAL, "Failed to create window from handle. - %s\n", SDL_GetError());
                    UnInit();
                    return ERR_SDL_CREATE_WINDOW_FAILED;
                }
                else
                {
                    av_log(nullptr, AV_LOG_DEBUG, "Window creation succeeded with SDL_VIDEODRIVER=x11\n");
                }
            }
        }
#if HAVE_VULKAN_RENDERER
        if (m_ptrVKRenderer)
        {
            AVDictionary* dict = nullptr;

            if (m_pszVulkanParams)
            {
                int ret = av_dict_parse_string(&dict, m_pszVulkanParams, "=", ":", 0);
                if (ret < 0)
                {
                    av_log(nullptr, AV_LOG_FATAL, "Failed to parse, %s\n", m_pszVulkanParams);
                    UnInit();
                    return ERR_SDL_VULKAN_PARAM_FAILED;
                }
            }

            int nRet = vk_renderer_create(m_ptrContext->ptrVKRenderer.get(), m_ptrContext->ptrWindow.get(), dict);
            av_dict_free(&dict);
            if (nRet < 0)
            {
                char szTmp[AV_ERROR_MAX_STRING_SIZE] = { 0 };
                av_log(nullptr, AV_LOG_FATAL, "Failed to create vulkan renderer, %s\n", av_make_error_string(szTmp, AV_ERROR_MAX_STRING_SIZE, ret));
                UnInit();
                return ERR_SDL_VULKAN_RENDERER_CREATE_FAILED;
            }
        }
        else
#endif
        {
            if (SDL_GL_LoadLibrary(nullptr) != 0)
            {
                av_log(nullptr, AV_LOG_WARNING, "Failed to initialize OpenGL, attempting to continue with initialization.\n");
            }

            m_ptrRenderer = SDLRendererPtr(SDL_CreateRenderer(m_ptrWindow.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
            if (!m_ptrRenderer)
            {
                av_log(nullptr, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError());
                //renderer = SDL_CreateRenderer(window, -1, 0);
                m_ptrRenderer = SDLRendererPtr(SDL_CreateRenderer(m_ptrWindow.get(), -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_SOFTWARE));
                if (!m_ptrRenderer)
                {
                    av_log(nullptr, AV_LOG_WARNING, "Failed to create software renderer.: %s\n", SDL_GetError());
                }
            }
            if (m_ptrRenderer)
            {
                if (!SDL_GetRendererInfo(m_ptrRenderer.get(), &m_objRendererInfo))
                    av_log(nullptr, AV_LOG_VERBOSE, "Initialized %s renderer.\n", m_objRendererInfo.name);
            }
            if (!m_ptrRenderer || !m_objRendererInfo.num_texture_formats)
            {
                av_log(nullptr, AV_LOG_FATAL, "Failed to create window or renderer: %s", SDL_GetError());
                UnInit();
                return ERR_SDL_CREATE_RENDERDER_FAILED;
            }

            int nWidth = 0;
            int nHeight = 0;

#ifdef _WIN32
            RECT rcClient;
            HWND hWindowWnd = (HWND)hWnd;
            ::GetClientRect(hWindowWnd, &rcClient);
            nWidth = rcClient.right - rcClient.left;
            nHeight = rcClient.bottom - rcClient.top;
#endif
            SDL_SetWindowSize(m_ptrWindow.get(), nWidth, nHeight);
        }
    }
    return ERR_SUCESS;
}

// Video settings
int16_t CYVideoRenderFilter::SetVideoScale(EVideoScaleType eScale)
{
    return ERR_SUCESS;
}

int16_t CYVideoRenderFilter::SetVideoRotation(ERotationType eRotation)
{
    return ERR_SUCESS;
}

int16_t CYVideoRenderFilter::SetVideoMirror(bool bMirror)
{
    return ERR_SUCESS;
}

int16_t CYVideoRenderFilter::SetAspectRatio(float fRatio)
{
    return ERR_SUCESS;
}

int16_t CYVideoRenderFilter::SetMute(bool bMute)
{
    m_ptrContext->bMuted = bMute;
    return ERR_SUCESS;
}

int CYVideoRenderFilter::GetMasterSyncType(SharePtr<CYMediaContext>& ptrContext)
{
    if (ptrContext->nAVSyncType == TYPE_SYNC_CLOCK_VIDEO)
    {
        if (ptrContext->pVideoStream)
            return TYPE_SYNC_CLOCK_VIDEO;
        else
            return TYPE_SYNC_CLOCK_AUDIO;
    }
    else if (ptrContext->nAVSyncType == TYPE_SYNC_CLOCK_AUDIO)
    {
        if (ptrContext->pAudioStream)
            return TYPE_SYNC_CLOCK_AUDIO;
        else
            return TYPE_SYNC_CLOCK_EXTERNAL;
    }
    else
    {
        return TYPE_SYNC_CLOCK_EXTERNAL;
    }
}

void CYVideoRenderFilter::CheckExternalClockSpeed(SharePtr<CYMediaContext>& ptrContext)
{
    if (ptrContext->nVideoStreamIndex >= 0 && ptrContext->ptrVideoQueue->nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES ||
        ptrContext->nAudioStreamIndex >= 0 && ptrContext->ptrAudioQueue->nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES)
    {
        ptrContext->extclk.SetClockSpeed(FFMAX(EXTERNAL_CLOCK_SPEED_MIN, ptrContext->extclk.m_fSpeed - EXTERNAL_CLOCK_SPEED_STEP));
    }
    else if ((ptrContext->nVideoStreamIndex < 0 || ptrContext->ptrVideoQueue->nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
        (ptrContext->nAudioStreamIndex < 0 || ptrContext->ptrAudioQueue->nb_packets > EXTERNAL_CLOCK_MAX_FRAMES))
    {
        ptrContext->extclk.SetClockSpeed(FFMIN(EXTERNAL_CLOCK_SPEED_MAX, ptrContext->extclk.m_fSpeed + EXTERNAL_CLOCK_SPEED_STEP));
    }
    else
    {
        double speed = ptrContext->extclk.m_fSpeed;
        if (speed != 1.0)
        {
            ptrContext->extclk.SetClockSpeed(speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
        }
    }
}

int CYVideoRenderFilter::VideoOpen(SharePtr<CYMediaContext>& ptrContext)
{
    int w, h;

    w = m_ptrContext->nScreenWidth ? m_ptrContext->nScreenWidth : ptrContext->nDefaultWidth;
    h = m_ptrContext->nScreenHeight ? m_ptrContext->nScreenHeight : ptrContext->nDefaultHeight;

    //     if (!pszWindowTitle)
    //         pszWindowTitle = input_filename;
        //SDL_SetWindowTitle(window, pszWindowTitle);

        // SDL_SetWindowSize(window, w, h);
        //SDL_SetWindowPosition(window, screen_left, screen_top);
    if (ptrContext->isFullScreen)
        SDL_SetWindowFullscreen(ptrContext->ptrWindow.get(), SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_ShowWindow(ptrContext->ptrWindow.get());

    ptrContext->nShowWidth = w;
    ptrContext->nShowHeight = h;

    return 0;
}

int CYVideoRenderFilter::ComputeMod(int a, int b)
{
    return a < 0 ? a % b + b : a % b;
}

void CYVideoRenderFilter::FillRectangle(int x, int y, int w, int h)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    if (w && h)
        SDL_RenderFillRect(m_ptrContext->ptrRenderer.get(), &rect);
}

int CYVideoRenderFilter::ReallocTexture(SDL_Texture** pTexture, Uint32 nNewFormat, int nNewWidth, int nNewHeight, SDL_BlendMode eBlendMode, int nInitTexture)
{
    Uint32 format;
    int access, w, h;
    if (!*pTexture || SDL_QueryTexture(*pTexture, &format, &access, &w, &h) < 0 || nNewWidth != w || nNewHeight != h || nNewFormat != format)
    {
        void* pixels;
        int pitch;
        if (*pTexture)
            SDL_DestroyTexture(*pTexture);
        if (!(*pTexture = SDL_CreateTexture(m_ptrContext->ptrRenderer.get(), nNewFormat, SDL_TEXTUREACCESS_STREAMING, nNewWidth, nNewHeight)))
            return -1;
        if (SDL_SetTextureBlendMode(*pTexture, eBlendMode) < 0)
            return -1;
        if (nInitTexture)
        {
            if (SDL_LockTexture(*pTexture, nullptr, &pixels, &pitch) < 0)
                return -1;
            memset(pixels, 0, pitch * nNewHeight);
            SDL_UnlockTexture(*pTexture);
        }
        av_log(nullptr, AV_LOG_VERBOSE, "Created %dx%d pTexture with %s.\n", nNewWidth, nNewHeight, SDL_GetPixelFormatName(nNewFormat));
    }
    return 0;
}

void CYVideoRenderFilter::VideoAudioDisplay(SharePtr<CYMediaContext>& ptrContext)
{
    int i, i_start, x, y1, y, ys, delay, n, nb_display_channels;
    int ch, channels, h, h2;
    int64_t time_diff;
    int nRdftBits, nb_freq;

    for (nRdftBits = 1; (1 << nRdftBits) < 2 * ptrContext->nShowHeight; nRdftBits++)
        ;
    nb_freq = 1 << (nRdftBits - 1);

    /* compute display index : center on currently output samples */
    channels = ptrContext->objAudioTgt.ch_layout.nb_channels;
    nb_display_channels = channels;
    if (!ptrContext->bPaused)
    {
        int data_used = ptrContext->eShowMode == SHOW_MODE_WAVES ? ptrContext->nShowWidth : (2 * nb_freq);
        n = 2 * channels;
        delay = ptrContext->nAudioWriteBufSize;
        delay /= n;

        /* to be more precise, we take into account the time spent since
           the last buffer computation */
        if (ptrContext->nAudioCallBackTime)
        {
            time_diff = av_gettime_relative() - ptrContext->nAudioCallBackTime;
            delay -= (time_diff * ptrContext->objAudioTgt.freq) / 1000000;
        }

        delay += 2 * data_used;
        if (delay < data_used)
            delay = data_used;

        i_start = x = ComputeMod(ptrContext->nSampleArrayIndex - delay * channels, SAMPLE_ARRAY_SIZE);
        if (ptrContext->eShowMode == SHOW_MODE_WAVES)
        {
            h = INT_MIN;
            for (i = 0; i < 1000; i += channels)
            {
                int idx = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
                int a = ptrContext->arraySample[idx];
                int b = ptrContext->arraySample[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
                int c = ptrContext->arraySample[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
                int d = ptrContext->arraySample[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
                int score = a - d;
                if (h < score && (b ^ c) < 0)
                {
                    h = score;
                    i_start = idx;
                }
            }
        }

        ptrContext->nLastIStart = i_start;
    }
    else
    {
        i_start = ptrContext->nLastIStart;
    }

    if (ptrContext->eShowMode == SHOW_MODE_WAVES)
    {
        SDL_SetRenderDrawColor(ptrContext->ptrRenderer.get(), 255, 255, 255, 255);

        /* total height for one channel */
        h = ptrContext->nShowHeight / nb_display_channels;
        /* graph height / 2 */
        h2 = (h * 9) / 20;
        for (ch = 0; ch < nb_display_channels; ch++)
        {
            i = i_start + ch;
            y1 = ptrContext->nShowYTop + ch * h + (h / 2); /* position of center line */
            for (x = 0; x < ptrContext->nShowWidth; x++)
            {
                y = (ptrContext->arraySample[i] * h2) >> 15;
                if (y < 0)
                {
                    y = -y;
                    ys = y1 - y;
                }
                else
                {
                    ys = y1;
                }
                FillRectangle(ptrContext->nShowXLeft + x, ys, 1, y);
                i += channels;
                if (i >= SAMPLE_ARRAY_SIZE)
                    i -= SAMPLE_ARRAY_SIZE;
            }
        }

        SDL_SetRenderDrawColor(ptrContext->ptrRenderer.get(), 0, 0, 255, 255);

        for (ch = 1; ch < nb_display_channels; ch++)
        {
            y = ptrContext->nShowYTop + ch * h;
            FillRectangle(ptrContext->nShowXLeft, y, ptrContext->nShowWidth, 1);
        }
    }
    else
    {
        int err = 0;
        if (ReallocTexture(&ptrContext->vis_texture, SDL_PIXELFORMAT_ARGB8888, ptrContext->nShowWidth, ptrContext->nShowHeight, SDL_BLENDMODE_NONE, 1) < 0)
            return;

        if (ptrContext->xpos >= ptrContext->nShowWidth)
            ptrContext->xpos = 0;
        nb_display_channels = FFMIN(nb_display_channels, 2);
        if (nRdftBits != ptrContext->nRdftBits)
        {
            const float rdft_scale = 1.0;
            av_tx_uninit(&ptrContext->pRdftContext);
            av_freep(&ptrContext->pfRealData);
            av_freep(&ptrContext->pRdftData);
            ptrContext->nRdftBits = nRdftBits;
            ptrContext->pfRealData = (float*)av_malloc_array(nb_freq, 4 * sizeof(*ptrContext->pfRealData));
            ptrContext->pRdftData = (AVComplexFloat*)av_malloc_array(nb_freq + 1, 2 * sizeof(*ptrContext->pRdftData));
            err = av_tx_init(&ptrContext->pRdftContext, &ptrContext->funRdft, AV_TX_FLOAT_RDFT,
                0, 1 << nRdftBits, &rdft_scale, 0);
        }
        if (err < 0 || !ptrContext->pRdftData)
        {
            av_log(nullptr, AV_LOG_ERROR, "Failed to allocate buffers for RDFT, switching to waves display\n");
            ptrContext->eShowMode = SHOW_MODE_WAVES;
        }
        else
        {
            float* data_in[2];
            AVComplexFloat* data[2];
            SDL_Rect rect = { ptrContext->xpos, 0, 1, ptrContext->nShowHeight };
            uint32_t* pixels;
            int pitch;
            for (ch = 0; ch < nb_display_channels; ch++)
            {
                data_in[ch] = ptrContext->pfRealData + 2 * nb_freq * ch;
                data[ch] = ptrContext->pRdftData + nb_freq * ch;
                i = i_start + ch;
                for (x = 0; x < 2 * nb_freq; x++)
                {
                    double w = (x - nb_freq) * (1.0 / nb_freq);
                    data_in[ch][x] = ptrContext->arraySample[i] * (1.0 - w * w);
                    i += channels;
                    if (i >= SAMPLE_ARRAY_SIZE)
                        i -= SAMPLE_ARRAY_SIZE;
                }
                ptrContext->funRdft(ptrContext->pRdftContext, data[ch], data_in[ch], sizeof(float));
                data[ch][0].im = data[ch][nb_freq].re;
                data[ch][nb_freq].re = 0;
            }
            /* Least efficient way to do this, we should of course
             * directly access it but it is more than fast enough. */
            if (!SDL_LockTexture(ptrContext->vis_texture, &rect, (void**)&pixels, &pitch))
            {
                pitch >>= 2;
                pixels += pitch * ptrContext->nShowHeight;
                for (y = 0; y < ptrContext->nShowHeight; y++)
                {
                    double w = 1 / sqrt(nb_freq);
                    int a = sqrt(w * sqrt(data[0][y].re * data[0][y].re + data[0][y].im * data[0][y].im));
                    int b = (nb_display_channels == 2) ? sqrt(w * hypot(data[1][y].re, data[1][y].im))
                        : a;
                    a = FFMIN(a, 255);
                    b = FFMIN(b, 255);
                    pixels -= pitch;
                    *pixels = (a << 16) + (b << 8) + ((a + b) >> 1);
                }
                SDL_UnlockTexture(ptrContext->vis_texture);
            }
            SDL_RenderCopy(ptrContext->ptrRenderer.get(), ptrContext->vis_texture, nullptr, nullptr);
        }
        if (!ptrContext->bPaused)
            ptrContext->xpos++;
    }
}

void CYVideoRenderFilter::CalculateDisplayRect(SDL_Rect* pRect, int nScrXleft, int nScrYtop, int nScrWidth, int nScrHeight, int nPicWidth, int nPicHeight, AVRational objPicSar)
{
    AVRational aspect_ratio = objPicSar;
    int64_t width, height, x, y;

    if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
        aspect_ratio = av_make_q(1, 1);

    aspect_ratio = av_mul_q(aspect_ratio, av_make_q(nPicWidth, nPicHeight));

    /* XXX: we suppose the screen has a 1.0 pixel ratio */
    height = nScrHeight;
    width = av_rescale(height, aspect_ratio.num, aspect_ratio.den) & ~1;
    if (width > nScrWidth)
    {
        width = nScrWidth;
        height = av_rescale(width, aspect_ratio.den, aspect_ratio.num) & ~1;
    }
    x = (nScrWidth - width) / 2;
    y = (nScrHeight - height) / 2;
    pRect->x = nScrXleft + x;
    pRect->y = nScrYtop + y;
    pRect->w = FFMAX((int)width, 1);
    pRect->h = FFMAX((int)height, 1);
}

static enum AVColorSpace sdl_supported_color_spaces[] = {
    AVCOL_SPC_BT709,
    AVCOL_SPC_BT470BG,
    AVCOL_SPC_SMPTE170M,
    AVCOL_SPC_UNSPECIFIED,
};

void SetSDLYuvConversionMode(AVFrame* pFrame)
{
#if SDL_VERSION_ATLEAST(2,0,8)
    SDL_YUV_CONVERSION_MODE mode = SDL_YUV_CONVERSION_AUTOMATIC;
    if (pFrame && (pFrame->format == AV_PIX_FMT_YUV420P || pFrame->format == AV_PIX_FMT_YUYV422 || pFrame->format == AV_PIX_FMT_UYVY422))
    {
        if (pFrame->color_range == AVCOL_RANGE_JPEG)
            mode = SDL_YUV_CONVERSION_JPEG;
        else if (pFrame->colorspace == AVCOL_SPC_BT709)
            mode = SDL_YUV_CONVERSION_BT709;
        else if (pFrame->colorspace == AVCOL_SPC_BT470BG || pFrame->colorspace == AVCOL_SPC_SMPTE170M)
            mode = SDL_YUV_CONVERSION_BT601;
    }
    SDL_SetYUVConversionMode(mode); /* FIXME: no support for linear transfer */
#endif
}

static void GetSDLPixFmtAndBlendMode(int nFormat, Uint32* pSDLPixFmt, SDL_BlendMode* pSDLBlendMode)
{
    int i;
    *pSDLBlendMode = SDL_BLENDMODE_NONE;
    *pSDLPixFmt = SDL_PIXELFORMAT_UNKNOWN;
    if (nFormat == AV_PIX_FMT_RGB32 ||
        nFormat == AV_PIX_FMT_RGB32_1 ||
        nFormat == AV_PIX_FMT_BGR32 ||
        nFormat == AV_PIX_FMT_BGR32_1)
        *pSDLBlendMode = SDL_BLENDMODE_BLEND;
    for (i = 0; i < FF_ARRAY_ELEMS(sdl_texture_format_map) - 1; i++)
    {
        if (nFormat == sdl_texture_format_map[i].format)
        {
            *pSDLPixFmt = sdl_texture_format_map[i].texture_fmt;
            return;
        }
    }
}

int CYVideoRenderFilter::UploadTexture(SDL_Texture** pTex, AVFrame* pFrame)
{
    int ret = 0;
    Uint32 sdl_pix_fmt;
    SDL_BlendMode sdl_blendmode;
    GetSDLPixFmtAndBlendMode(pFrame->format, &sdl_pix_fmt, &sdl_blendmode);
    if (ReallocTexture(pTex, sdl_pix_fmt == SDL_PIXELFORMAT_UNKNOWN ? SDL_PIXELFORMAT_ARGB8888 : sdl_pix_fmt, pFrame->width, pFrame->height, sdl_blendmode, 0) < 0)
        return -1;
    switch (sdl_pix_fmt)
    {
    case SDL_PIXELFORMAT_IYUV:
        if (pFrame->linesize[0] > 0 && pFrame->linesize[1] > 0 && pFrame->linesize[2] > 0)
        {
            ret = SDL_UpdateYUVTexture(*pTex, nullptr, pFrame->data[0], pFrame->linesize[0],
                pFrame->data[1], pFrame->linesize[1],
                pFrame->data[2], pFrame->linesize[2]);
        }
        else if (pFrame->linesize[0] < 0 && pFrame->linesize[1] < 0 && pFrame->linesize[2] < 0)
        {
            ret = SDL_UpdateYUVTexture(*pTex, nullptr, pFrame->data[0] + pFrame->linesize[0] * (pFrame->height - 1), -pFrame->linesize[0],
                pFrame->data[1] + pFrame->linesize[1] * (AV_CEIL_RSHIFT(pFrame->height, 1) - 1), -pFrame->linesize[1],
                pFrame->data[2] + pFrame->linesize[2] * (AV_CEIL_RSHIFT(pFrame->height, 1) - 1), -pFrame->linesize[2]);
        }
        else
        {
            av_log(nullptr, AV_LOG_ERROR, "Mixed negative and positive linesizes are not supported.\n");
            return -1;
        }
        break;
    default:
        if (pFrame->linesize[0] < 0)
        {
            ret = SDL_UpdateTexture(*pTex, nullptr, pFrame->data[0] + pFrame->linesize[0] * (pFrame->height - 1), -pFrame->linesize[0]);
        }
        else
        {
            ret = SDL_UpdateTexture(*pTex, nullptr, pFrame->data[0], pFrame->linesize[0]);
        }
        break;
    }
    return ret;
}

void CYVideoRenderFilter::VideoImageDisplay(SharePtr<CYMediaContext>& ptrContext)
{
    CYFrame* vp;
    CYFrame* sp = nullptr;
    SDL_Rect rect;

    vp = ptrContext->pictq.PeekLast();
#if HAVE_VULKAN_RENDERER
    if (m_ptrVKRenderer)
    {
        vk_renderer_display(m_ptrVKRenderer.get(), vp->frame);
        return;
    }
#endif

    if (ptrContext->pSubTitleStream)
    {
        if (ptrContext->subpq.NbRemaining() > 0)
        {
            sp = ptrContext->subpq.Peek();

            if (vp->pts >= sp->pts + ((float)sp->sub.start_display_time / 1000))
            {
                if (!sp->uploaded)
                {
                    uint8_t* pixels[4];
                    int pitch[4];
                    int i;
                    if (!sp->width || !sp->height)
                    {
                        sp->width = vp->width;
                        sp->height = vp->height;
                    }
                    if (ReallocTexture(&ptrContext->sub_texture, SDL_PIXELFORMAT_ARGB8888, sp->width, sp->height, SDL_BLENDMODE_BLEND, 1) < 0)
                        return;

                    for (i = 0; i < sp->sub.num_rects; i++)
                    {
                        AVSubtitleRect* sub_rect = sp->sub.rects[i];

                        sub_rect->x = av_clip(sub_rect->x, 0, sp->width);
                        sub_rect->y = av_clip(sub_rect->y, 0, sp->height);
                        sub_rect->w = av_clip(sub_rect->w, 0, sp->width - sub_rect->x);
                        sub_rect->h = av_clip(sub_rect->h, 0, sp->height - sub_rect->y);

                        ptrContext->ptrSubConvertCtx = SwsContextPtr(sws_getCachedContext(ptrContext->ptrSubConvertCtx.get(),
                            sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8,
                            sub_rect->w, sub_rect->h, AV_PIX_FMT_BGRA,
                            0, nullptr, nullptr, nullptr));
                        if (!ptrContext->ptrSubConvertCtx)
                        {
                            av_log(nullptr, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
                            return;
                        }
                        if (!SDL_LockTexture(ptrContext->sub_texture, (SDL_Rect*)sub_rect, (void**)pixels, pitch))
                        {
                            sws_scale(ptrContext->ptrSubConvertCtx.get(), (const uint8_t* const*)sub_rect->data, sub_rect->linesize,
                                0, sub_rect->h, pixels, pitch);
                            SDL_UnlockTexture(ptrContext->sub_texture);
                        }
                    }
                    sp->uploaded = 1;
                }
            }
            else
                sp = nullptr;
        }
    }

    CalculateDisplayRect(&rect, ptrContext->nShowXLeft, ptrContext->nShowYTop, ptrContext->nShowWidth, ptrContext->nShowHeight, vp->width, vp->height, vp->sar);
    SetSDLYuvConversionMode(vp->pFrame);

    if (!vp->uploaded)
    {
        if (UploadTexture(&ptrContext->vid_texture, vp->pFrame) < 0)
        {
            SetSDLYuvConversionMode(nullptr);
            return;
        }
        vp->uploaded = 1;
        vp->flip_v = vp->pFrame->linesize[0] < 0;
    }

    SDL_RenderCopyEx(ptrContext->ptrRenderer.get(), ptrContext->vid_texture, nullptr, &rect, 0, nullptr, vp->flip_v ? SDL_FLIP_VERTICAL : (const SDL_RendererFlip)0);
    SetSDLYuvConversionMode(nullptr);
    if (sp)
    {
#if USE_ONEPASS_SUBTITLE_RENDER
        SDL_RenderCopy(ptrContext->ptrRenderer.get(), ptrContext->sub_texture, nullptr, &rect);
#else
        int i;
        double xratio = (double)rect.w / (double)sp->width;
        double yratio = (double)rect.h / (double)sp->height;
        for (i = 0; i < sp->sub.num_rects; i++)
        {
            SDL_Rect* sub_rect = (SDL_Rect*)sp->sub.rects[i];
            SDL_Rect target = { .x = rect.x + sub_rect->x * xratio,
                               .y = rect.y + sub_rect->y * yratio,
                               .w = sub_rect->w * xratio,
                               .h = sub_rect->h * yratio };
            SDL_RenderCopy(renderer, ptrContext->sub_texture, sub_rect, &target);
        }
#endif
    }
}

/* display the current picture, if any */
void CYVideoRenderFilter::VideoDisplay(SharePtr<CYMediaContext>& ptrContext)
{
    if (!ptrContext->nShowWidth)
        VideoOpen(ptrContext);

    SDL_SetRenderDrawColor(ptrContext->ptrRenderer.get(), 0, 0, 0, 255);
    SDL_RenderClear(ptrContext->ptrRenderer.get());
    if (ptrContext->pAudioStream && ptrContext->eShowMode != SHOW_MODE_VIDEO)
        VideoAudioDisplay(ptrContext);
    else if (ptrContext->pVideoStream)
        VideoImageDisplay(ptrContext);
    SDL_RenderPresent(ptrContext->ptrRenderer.get());
}

double CYVideoRenderFilter::VPDuration(SharePtr<CYMediaContext>& ptrContext, CYFrame* pVP, CYFrame* pNextVP)
{
    if (pVP->serial == pNextVP->serial)
    {
        double duration = pNextVP->pts - pVP->pts;
        if (isnan(duration) || duration <= 0 || duration > ptrContext->fMaxFrameDuration)
            return pVP->duration;
        else
            return duration;
    }
    else
    {
        return 0.0;
    }
}

/* get the current master clock value */
double CYVideoRenderFilter::GetMasterClock(SharePtr<CYMediaContext>& ptrContext)
{
    double val;

    switch (GetMasterSyncType(ptrContext))
    {
    case TYPE_SYNC_CLOCK_VIDEO:
        val = ptrContext->vidclk.GetClock();
        break;
    case TYPE_SYNC_CLOCK_AUDIO:
        val = ptrContext->audclk.GetClock();
        break;
    default:
        val = ptrContext->extclk.GetClock();
        break;
    }
    return val;
}

double CYVideoRenderFilter::ComputeTargetDelay(double delay, SharePtr<CYMediaContext>& ptrContext)
{
    double sync_threshold, fDiff = 0;

    /* update delay to follow master synchronisation source */
    if (GetMasterSyncType(ptrContext) != TYPE_SYNC_CLOCK_VIDEO)
    {
        /* if video is slave, we try to correct big delays by
           duplicating or deleting a frame */
           //fDiff = get_clock(&ptrContext->vidclk) - GetMasterClock(is);
        fDiff = ptrContext->vidclk.GetClock() - GetMasterClock(ptrContext);

        /* skip or repeat frame. We take into account the
           delay to compute the threshold. I still don't know
           if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (!isnan(fDiff) && fabs(fDiff) < ptrContext->fMaxFrameDuration)
        {
            if (fDiff <= -sync_threshold)
                delay = FFMAX(0, delay + fDiff);
            else if (fDiff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
                delay = delay + fDiff;
            else if (fDiff >= sync_threshold)
                delay = 2 * delay;
        }
    }

    av_log(nullptr, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n",
        delay, -fDiff);

    return delay;
}

void CYVideoRenderFilter::UpdateVideoPTS(SharePtr<CYMediaContext>& ptrContext, double fPts, int nSerial)
{
    /* update current video pts */
    //set_clock(&ptrContext->vidclk, pts, serial);
    ptrContext->vidclk.SetClock(fPts, nSerial);
    //sync_clock_to_slave(&ptrContext->extclk, &ptrContext->vidclk);
    ptrContext->extclk.SyncClockToSlave(ptrContext->vidclk);
}

void CYVideoRenderFilter::StreamTogglePause(SharePtr<CYMediaContext>& ptrContext)
{
    if (ptrContext->bPaused)
    {
        ptrContext->fFrameTimer += av_gettime_relative() / 1000000.0 - ptrContext->vidclk.m_fLastUpdated;
        if (ptrContext->nReadPauseReturn != AVERROR(ENOSYS))
        {
            ptrContext->vidclk.m_bPaused = false;
        }
        //set_clock(&ptrContext->vidclk, get_clock(&ptrContext->vidclk), ptrContext->vidclk.serial);
        ptrContext->vidclk.SetClock(ptrContext->vidclk.GetClock(), ptrContext->vidclk.m_fSerial);
    }
    //set_clock(&ptrContext->extclk, get_clock(&ptrContext->extclk), ptrContext->extclk.serial);
    ptrContext->extclk.SetClock(ptrContext->extclk.GetClock(), ptrContext->extclk.m_fSerial);
    ptrContext->bPaused = ptrContext->audclk.m_bPaused = ptrContext->vidclk.m_bPaused = ptrContext->extclk.m_bPaused = !ptrContext->bPaused;
}

/* called to display each frame */
void CYVideoRenderFilter::VideoRefresh(double* pfRemainingTime)
{
    double time;

    CYFrame* sp, * sp2;

    if (!m_ptrContext->bPaused && GetMasterSyncType(m_ptrContext) == TYPE_SYNC_CLOCK_EXTERNAL && m_ptrContext->bRealTime)
        CheckExternalClockSpeed(m_ptrContext);

    if (!m_bDisableVideo && m_ptrContext->eShowMode != SHOW_MODE_VIDEO && m_ptrContext->pAudioStream)
    {
        time = av_gettime_relative() / 1000000.0;
        if (m_ptrContext->bForceRefresh || m_ptrContext->fLastVisTime + m_ptrContext->fRdftSpeed < time)
        {
            VideoDisplay(m_ptrContext);
            m_ptrContext->fLastVisTime = time;
        }
        *pfRemainingTime = FFMIN(*pfRemainingTime, m_ptrContext->fLastVisTime + m_ptrContext->fRdftSpeed - time);
    }

    if (m_ptrContext->pVideoStream)
    {
    retry:
        if (m_ptrContext->pictq.NbRemaining() == 0)
        {
            // nothing to do, no picture to display in the queue
        }
        else
        {
            double last_duration, duration, delay;
            CYFrame* vp, * lastvp;

            /* dequeue the picture */
            lastvp = m_ptrContext->pictq.PeekLast();
            vp = m_ptrContext->pictq.Peek();

            if (vp->serial != m_ptrContext->ptrVideoQueue->serial)
            {
                m_ptrContext->pictq.Next();
                goto retry;
            }

            if (lastvp->serial != vp->serial)
                m_ptrContext->fFrameTimer = av_gettime_relative() / 1000000.0;

            if (m_ptrContext->bPaused)
                goto display;

            /* compute nominal last_duration */
            last_duration = VPDuration(m_ptrContext, lastvp, vp);
            delay = ComputeTargetDelay(last_duration, m_ptrContext);

            time = av_gettime_relative() / 1000000.0;
            if (time < m_ptrContext->fFrameTimer + delay)
            {
                *pfRemainingTime = FFMIN(m_ptrContext->fFrameTimer + delay - time, *pfRemainingTime);
                goto display;
            }

            m_ptrContext->fFrameTimer += delay;
            if (delay > 0 && time - m_ptrContext->fFrameTimer > AV_SYNC_THRESHOLD_MAX)
                m_ptrContext->fFrameTimer = time;

            {
                UniqueLock locker(m_ptrContext->pictq.m_mutex);
                //SDL_LockMutex(m_ptrContext->pictq.mutex);
                if (!isnan(vp->pts))
                    UpdateVideoPTS(m_ptrContext, vp->pts, vp->serial);
                //SDL_UnlockMutex(m_ptrContext->pictq.mutex);
            }

            if (m_ptrContext->pictq.NbRemaining() > 1)
            {
                CYFrame* nextvp = m_ptrContext->pictq.PeekNext();
                duration = VPDuration(m_ptrContext, vp, nextvp);
                if (!m_ptrContext->bStep && (m_ptrParam->nFrameDrop > 0 || (m_ptrParam->nFrameDrop && GetMasterSyncType(m_ptrContext) != TYPE_SYNC_CLOCK_VIDEO)) && time > m_ptrContext->fFrameTimer + duration)
                {
                    m_ptrContext->nFrameDropsLate++;
                    m_ptrContext->pictq.Next();
                    goto retry;
                }
            }

            if (m_ptrContext->pSubTitleStream)
            {
                while (m_ptrContext->subpq.NbRemaining() > 0)
                {
                    sp = m_ptrContext->subpq.Peek();

                    if (m_ptrContext->subpq.NbRemaining() > 1)
                        sp2 = m_ptrContext->subpq.PeekNext();
                    else
                        sp2 = nullptr;

                    if (sp->serial != m_ptrContext->ptrSubTitleQueue->serial
                        || (m_ptrContext->vidclk.m_fPTS > (sp->pts + ((float)sp->sub.end_display_time / 1000)))
                        || (sp2 && m_ptrContext->vidclk.m_fPTS > (sp2->pts + ((float)sp2->sub.start_display_time / 1000))))
                    {
                        if (sp->uploaded)
                        {
                            int i;
                            for (i = 0; i < sp->sub.num_rects; i++)
                            {
                                AVSubtitleRect* sub_rect = sp->sub.rects[i];
                                uint8_t* pixels;
                                int pitch, j;

                                if (!SDL_LockTexture(m_ptrContext->sub_texture, (SDL_Rect*)sub_rect, (void**)&pixels, &pitch))
                                {
                                    for (j = 0; j < sub_rect->h; j++, pixels += pitch)
                                        memset(pixels, 0, sub_rect->w << 2);
                                    SDL_UnlockTexture(m_ptrContext->sub_texture);
                                }
                            }
                        }
                        m_ptrContext->subpq.Next();
                    }
                    else
                    {
                        break;
                    }
                }
            }

            m_ptrContext->pictq.Next();
            m_ptrContext->bForceRefresh = true;

            if (m_ptrContext->bStep && !m_ptrContext->bPaused)
                StreamTogglePause(m_ptrContext);
        }
    display:
        /* display picture */
        if (!m_bDisableVideo && m_ptrContext->bForceRefresh && m_ptrContext->eShowMode == SHOW_MODE_VIDEO && m_ptrContext->pictq.rindex_shown)
            VideoDisplay(m_ptrContext);
    }
    m_ptrContext->bForceRefresh = false;
    if (m_ptrContext->nShowStatus)
    {
        AVBPrint buf;
        static int64_t last_time;
        int64_t cur_time;
        int aqsize, vqsize, sqsize;
        double av_diff;

        cur_time = av_gettime_relative();
        if (!last_time || (cur_time - last_time) >= 30000)
        {
            aqsize = 0;
            vqsize = 0;
            sqsize = 0;
            if (m_ptrContext->pAudioStream)
                aqsize = m_ptrContext->ptrAudioQueue->size;
            if (m_ptrContext->pVideoStream)
                vqsize = m_ptrContext->ptrVideoQueue->size;
            if (m_ptrContext->pSubTitleStream)
                sqsize = m_ptrContext->ptrSubTitleQueue->size;
            av_diff = 0;
            if (m_ptrContext->pAudioStream && m_ptrContext->pVideoStream)
            {
                //av_diff = get_clock(&m_ptrContext->audclk) - get_clock(&m_ptrContext->vidclk);
                av_diff = m_ptrContext->audclk.GetClock() - m_ptrContext->vidclk.GetClock();
            }
            else if (m_ptrContext->pVideoStream)
                av_diff = GetMasterClock(m_ptrContext) - m_ptrContext->vidclk.GetClock();
            else if (m_ptrContext->pAudioStream)
            {
                //av_diff = GetMasterClock(is) - get_clock(&m_ptrContext->audclk);
                av_diff = GetMasterClock(m_ptrContext) - m_ptrContext->audclk.GetClock();
            }

            av_bprint_init(&buf, 0, AV_BPRINT_SIZE_AUTOMATIC);
            av_bprintf(&buf,
                "%7.2f %s:%7.3f fd=%4d aq=%5dKB vq=%5dKB sq=%5dB \r",
                GetMasterClock(m_ptrContext),
                (m_ptrContext->pAudioStream && m_ptrContext->pVideoStream) ? "A-V" : (m_ptrContext->pVideoStream ? "M-V" : (m_ptrContext->pAudioStream ? "M-A" : "   ")),
                av_diff,
                m_ptrContext->nFrameDropsEarly + m_ptrContext->nFrameDropsLate,
                aqsize / 1024,
                vqsize / 1024,
                sqsize);

            if (m_ptrContext->nShowStatus == 1 && AV_LOG_INFO > av_log_get_level())
                fprintf(stderr, "%s", buf.str);
            else
                av_log(nullptr, AV_LOG_INFO, "%s", buf.str);

            fflush(stderr);
            av_bprint_finalize(&buf, nullptr);

            last_time = cur_time;
        }
    }
}

void CYVideoRenderFilter::RefreshLoopWaitEvent(SharePtr<CYMediaContext>& ptrContext, SDL_Event* pEvent)
{
    double pfRemainingTime = 0.0;
    SDL_PumpEvents();
    while (!SDL_PeepEvents(pEvent, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT))
    {
        if (!ptrContext->bCursorHidden && av_gettime_relative() - ptrContext->nCursorLastShown > CURSOR_HIDE_DELAY)
        {
            SDL_ShowCursor(0);
            ptrContext->bCursorHidden = true;
        }
        if (pfRemainingTime > 0.0)
            av_usleep((int64_t)(pfRemainingTime * 1000000.0));
        pfRemainingTime = REFRESH_RATE;
        if (ptrContext->eShowMode != SHOW_MODE_NONE && (!ptrContext->bPaused || ptrContext->bForceRefresh))
            VideoRefresh(&pfRemainingTime);

        if (m_ptrContext && m_ptrContext->funPositionCallBack)
        {
            if (m_nLastMircoSecond == 0)
                m_nLastMircoSecond = m_ptrContext->extclk.GetClock() * 1000;

            int64_t nMicroSecond = m_ptrContext->extclk.GetClock() * 1000;
            if (nMicroSecond - m_nLastMircoSecond < 0)
            {
                m_nLastMircoSecond = nMicroSecond;
            }

            if (!m_bPlayOver && (abs(nMicroSecond - m_ptrContext->nFileDuration) <= 100) && m_ptrContext->bEof)
            {
                m_bPlayOver = true;
                if (m_ptrContext->funStateCallBack) m_ptrContext->funStateCallBack(TYPE_STATUS_COMPLETED);
            }
            else if (!m_ptrContext->bEof)
            {
                m_bPlayOver = false;
            }

            if (nMicroSecond - m_nLastMircoSecond > 100)
            {
                if (m_bPlayOver)
                {
                    m_ptrContext->funPositionCallBack(m_ptrContext->nFileDuration, m_ptrContext->nFileDuration);
                }
                else
                {
                    m_ptrContext->funPositionCallBack(nMicroSecond > m_ptrContext->nFileDuration ? m_ptrContext->nFileDuration : nMicroSecond, m_ptrContext->nFileDuration);
                }

                m_nLastMircoSecond = nMicroSecond;
            }
        }

        SDL_PumpEvents();
    }
}

void StreamComponentClose(SharePtr<CYMediaContext>& ptrContext, int nStreamIndex)
{
    AVCodecParameters* codecpar;

    if (nStreamIndex < 0 || (!ptrContext->ptrIC || nStreamIndex >= ptrContext->ptrIC->nb_streams))
        return;
    codecpar = ptrContext->ptrIC->streams[nStreamIndex]->codecpar;

    switch (codecpar->codec_type)
    {
    case AVMEDIA_TYPE_AUDIO:
        ptrContext->auddec.Abort(ptrContext->sampq);
        SDL_CloseAudioDevice(ptrContext->hAudioDev);
        ptrContext->auddec.Destroy();
        ptrContext->ptrSwrCtx.reset();
        //av_freep(&ptrContext->ptrAudioBuffer1);
        ptrContext->ptrAudioBuffer1.reset();
        ptrContext->nAudioBuf1Size = 0;
        ptrContext->ptrAudioBuffer.get();

        if (ptrContext->pRdftContext)
        {
            av_tx_uninit(&ptrContext->pRdftContext);
            av_freep(&ptrContext->pfRealData);
            av_freep(&ptrContext->pRdftData);
            ptrContext->pRdftContext = nullptr;
            ptrContext->nRdftBits = 0;
        }
        break;
    case AVMEDIA_TYPE_VIDEO:
        ptrContext->viddec.Abort(ptrContext->pictq);
        ptrContext->viddec.Destroy();
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        ptrContext->subdec.Abort(ptrContext->subpq);
        ptrContext->subdec.Destroy();
        break;
    default:
        break;
    }

    ptrContext->ptrIC->streams[nStreamIndex]->discard = AVDISCARD_ALL;
    switch (codecpar->codec_type)
    {
    case AVMEDIA_TYPE_AUDIO:
        ptrContext->pAudioStream = nullptr;
        ptrContext->nAudioStreamIndex = -1;
        break;
    case AVMEDIA_TYPE_VIDEO:
        ptrContext->pVideoStream = nullptr;
        ptrContext->nVideoStreamIndex = -1;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        ptrContext->pSubTitleStream = nullptr;
        ptrContext->nSubtitleStreamIndex = -1;
        break;
    default:
        break;
    }
}

void StreamClose(SharePtr<CYMediaContext>& ptrContext)
{
    UniqueLock lokcer(ptrContext->AbortMutex);
    ptrContext->bAbortRequest = true;

    /* close each stream */
    StreamComponentClose(ptrContext, ptrContext->nAudioStreamIndex);
    StreamComponentClose(ptrContext, ptrContext->nVideoStreamIndex);
    StreamComponentClose(ptrContext, ptrContext->nSubtitleStreamIndex);
    ptrContext->ptrIC.reset();

    if (ptrContext->ptrVideoQueue) ptrContext->ptrVideoQueue->Destroy();
    if (ptrContext->ptrAudioQueue) ptrContext->ptrAudioQueue->Destroy();
    if (ptrContext->ptrSubTitleQueue) ptrContext->ptrSubTitleQueue->Destroy();

    /* free all pictures */
    ptrContext->pictq.Destroy();
    ptrContext->sampq.Destroy();
    ptrContext->subpq.Destroy();

    ptrContext->ptrSubConvertCtx.reset();
    if (ptrContext->pszFileName)
    {
        av_free(ptrContext->pszFileName);
        ptrContext->pszFileName = nullptr;
    }

    if (ptrContext->vis_texture)
        SDL_DestroyTexture(ptrContext->vis_texture);
    if (ptrContext->vid_texture)
        SDL_DestroyTexture(ptrContext->vid_texture);
    if (ptrContext->sub_texture)
        SDL_DestroyTexture(ptrContext->sub_texture);
    ptrContext->vis_texture = nullptr;
    ptrContext->vid_texture = nullptr;
    ptrContext->sub_texture = nullptr;
}

void CYVideoRenderFilter::DoExit(SharePtr<CYMediaContext>& ptrContext)
{
    UniqueLock locker(m_mutex);
    if (!ptrContext)
        return;

    StreamClose(ptrContext);

    uninit_opts();
    for (int i = 0; i < ptrContext->nNBVFilters; i++)
        av_freep(&ptrContext->pVfiltersList[i]);
    av_freep(&ptrContext->pVfiltersList);
}

static void ToggleFullScreen(SharePtr<CYMediaContext>& ptrContext)
{
    ptrContext->isFullScreen = !ptrContext->isFullScreen;
    SDL_SetWindowFullscreen(ptrContext->ptrWindow.get(), ptrContext->isFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

static void StreamTogglePause(SharePtr<CYMediaContext>& ptrContext)
{
    if (ptrContext->bPaused)
    {
        ptrContext->fFrameTimer += av_gettime_relative() / 1000000.0 - ptrContext->vidclk.m_fLastUpdated;
        if (ptrContext->nReadPauseReturn != AVERROR(ENOSYS))
        {
            ptrContext->vidclk.m_bPaused = false;
        }
        ptrContext->vidclk.SetClock(ptrContext->vidclk.GetClock(), ptrContext->vidclk.m_fSerial);
    }
    ptrContext->extclk.SetClock(ptrContext->extclk.GetClock(), ptrContext->extclk.m_fSerial);
    ptrContext->bPaused = ptrContext->audclk.m_bPaused = ptrContext->vidclk.m_bPaused = ptrContext->extclk.m_bPaused = !ptrContext->bPaused;
}

static void TogglePause(SharePtr<CYMediaContext>& ptrContext)
{
    StreamTogglePause(ptrContext);
    ptrContext->bStep = false;
}

static void ToggleMute(SharePtr<CYMediaContext>& ptrContext)
{
    ptrContext->bMuted = !ptrContext->bMuted;
}

static void UpdateVolume(SharePtr<CYMediaContext>& ptrContext, int nSign, double fStep)
{
    double fVolumeLevel = ptrContext->nAudioVolume ? (20 * log(ptrContext->nAudioVolume / (double)SDL_MIX_MAXVOLUME) / log(10)) : -1000.0;
    int nNewVolume = lrint(SDL_MIX_MAXVOLUME * pow(10.0, (fVolumeLevel + nSign * fStep) / 20.0));
    ptrContext->nAudioVolume = av_clip(ptrContext->nAudioVolume == nNewVolume ? (ptrContext->nAudioVolume + nSign) : nNewVolume, 0, SDL_MIX_MAXVOLUME);
}

static void ToggleAudioDisplay(SharePtr<CYMediaContext>& ptrContext)
{
    int next = ptrContext->eShowMode;
    do
    {
        next = (next + 1) % SHOW_MODE_NB;
    } while (next != ptrContext->eShowMode && (next == SHOW_MODE_VIDEO && !ptrContext->pVideoStream || next != SHOW_MODE_VIDEO && !ptrContext->pAudioStream));
    if (ptrContext->eShowMode != next)
    {
        ptrContext->bForceRefresh = true;
        ptrContext->eShowMode = (ShowMode)next;
    }
}

static int GetMasterSyncType(SharePtr<CYMediaContext>& ptrContext)
{
    if (ptrContext->nAVSyncType == TYPE_SYNC_CLOCK_VIDEO)
    {
        if (ptrContext->pVideoStream)
            return TYPE_SYNC_CLOCK_VIDEO;
        else
            return TYPE_SYNC_CLOCK_AUDIO;
    }
    else if (ptrContext->nAVSyncType == TYPE_SYNC_CLOCK_AUDIO)
    {
        if (ptrContext->pAudioStream)
            return TYPE_SYNC_CLOCK_AUDIO;
        else
            return TYPE_SYNC_CLOCK_EXTERNAL;
    }
    else
    {
        return TYPE_SYNC_CLOCK_EXTERNAL;
    }
}

/* get the current master clock value */
double GetMasterClock(SharePtr<CYMediaContext>& ptrContext)
{
    double val;

    switch (GetMasterSyncType(ptrContext))
    {
    case TYPE_SYNC_CLOCK_VIDEO:
        val = ptrContext->vidclk.GetClock();
        break;
    case TYPE_SYNC_CLOCK_AUDIO:
        val = ptrContext->audclk.GetClock();
        break;
    default:
        val = ptrContext->extclk.GetClock();
        break;
    }
    return val;
}

/* seek in the stream */
static void StreamSeek(SharePtr<CYMediaContext>& ptrContext, int64_t nPos, int64_t nRel, int nByBytes)
{
    if (!ptrContext->bSeekReq)
    {
        ptrContext->nSeekPos = nPos;
        ptrContext->nSeekRel = nRel;
        ptrContext->nSeekFlags &= ~AVSEEK_FLAG_BYTE;
        if (nByBytes)
            ptrContext->nSeekFlags |= AVSEEK_FLAG_BYTE;
        ptrContext->bSeekReq = true;
        ptrContext->ptrReadCond->NotifyOne();
    }
}

static void SeekChapter(SharePtr<CYMediaContext>& ptrContext, int nIncr)
{
    int64_t pos = GetMasterClock(ptrContext) * AV_TIME_BASE;
    int i;

    if (!ptrContext->ptrIC->nb_chapters)
        return;

    /* find the current chapter */
    for (i = 0; i < ptrContext->ptrIC->nb_chapters; i++)
    {
        AVChapter* ch = ptrContext->ptrIC->chapters[i];
        if (av_compare_ts(pos, AV_TIME_BASE_Q, ch->start, ch->time_base) < 0)
        {
            i--;
            break;
        }
    }

    i += nIncr;
    i = FFMAX(i, 0);
    if (i >= ptrContext->ptrIC->nb_chapters)
        return;

    av_log(nullptr, AV_LOG_VERBOSE, "Seeking to chapter %d.\n", i);
    StreamSeek(ptrContext, av_rescale_q(ptrContext->ptrIC->chapters[i]->start, ptrContext->ptrIC->chapters[i]->time_base, AV_TIME_BASE_Q), 0, 0);
}

static void StepToNextFrame(SharePtr<CYMediaContext>& ptrContext)
{
    /* if the stream is bPaused unpause it, then step */
    if (ptrContext->bPaused)
        StreamTogglePause(ptrContext);
    ptrContext->bStep = true;
}

int StreamComponentOpen(SharePtr<CYMediaContext>& ptrContext, int nStreamIndex);
void CYVideoRenderFilter::StreamCycleChannel(SharePtr<CYMediaContext>& ptrContext, int nCodecType)
{
    int nStartIndex, stream_index;
    int nOldIndex;
    AVStream* st;
    AVProgram* p = nullptr;
    int nb_streams = ptrContext->ptrIC->nb_streams;

    if (nCodecType == AVMEDIA_TYPE_VIDEO)
    {
        nStartIndex = ptrContext->nLastVideoStream;
        nOldIndex = ptrContext->nVideoStreamIndex;
    }
    else if (nCodecType == AVMEDIA_TYPE_AUDIO)
    {
        nStartIndex = ptrContext->nLastAudioStream;
        nOldIndex = ptrContext->nAudioStreamIndex;
    }
    else
    {
        nStartIndex = ptrContext->nLastSubTitleStream;
        nOldIndex = ptrContext->nSubtitleStreamIndex;
    }
    stream_index = nStartIndex;

    if (nCodecType != AVMEDIA_TYPE_VIDEO && ptrContext->nVideoStreamIndex != -1)
    {
        p = av_find_program_from_stream(ptrContext->ptrIC.get(), nullptr, ptrContext->nVideoStreamIndex);
        if (p)
        {
            nb_streams = p->nb_stream_indexes;
            for (nStartIndex = 0; nStartIndex < nb_streams; nStartIndex++)
                if (p->stream_index[nStartIndex] == stream_index)
                    break;
            if (nStartIndex == nb_streams)
                nStartIndex = -1;
            stream_index = nStartIndex;
        }
    }

    for (;;)
    {
        if (++stream_index >= nb_streams)
        {
            if (nCodecType == AVMEDIA_TYPE_SUBTITLE)
            {
                stream_index = -1;
                ptrContext->nLastSubTitleStream = -1;
                goto the_end;
            }
            if (nStartIndex == -1)
                return;
            stream_index = 0;
        }
        if (stream_index == nStartIndex)
            return;
        st = ptrContext->ptrIC->streams[p ? p->stream_index[stream_index] : stream_index];
        if (st->codecpar->codec_type == nCodecType)
        {
            /* check that parameters are OK */
            switch (nCodecType)
            {
            case AVMEDIA_TYPE_AUDIO:
                if (st->codecpar->sample_rate != 0 &&
                    st->codecpar->ch_layout.nb_channels != 0)
                    goto the_end;
                break;
            case AVMEDIA_TYPE_VIDEO:
            case AVMEDIA_TYPE_SUBTITLE:
                goto the_end;
            default:
                break;
            }
        }
    }
the_end:
    if (p && stream_index != -1)
        stream_index = p->stream_index[stream_index];
    av_log(nullptr, AV_LOG_INFO, "Switch %s stream from #%d to #%d\n", av_get_media_type_string((AVMediaType)nCodecType), nOldIndex, stream_index);

    StreamComponentClose(ptrContext, nOldIndex);
    StreamComponentOpen(ptrContext, stream_index);
}

void CYVideoRenderFilter::OnEntry()
{
    SDL_Event event;
    double nIncr, pos, frac;

    for (;;)
    {
        double x;
        RefreshLoopWaitEvent(m_ptrContext, &event);
        switch (event.type)
        {
        case SDL_KEYDOWN:
            if (m_ptrParam->bExitOnKeyDown || event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q)
            {
                return;
            }
            // If we don't yet have a window, skip all key events, because read_thread might still be initializing...
            if (!m_ptrContext->nShowWidth)
                continue;
            switch (event.key.keysym.sym)
            {
            case SDLK_f:
                ToggleFullScreen(m_ptrContext);
                m_ptrContext->bForceRefresh = true;
                break;
            case SDLK_p:
            case SDLK_SPACE:
                TogglePause(m_ptrContext);
                break;
            case SDLK_m:
                ToggleMute(m_ptrContext);
                break;
            case SDLK_KP_MULTIPLY:
            case SDLK_0:
                UpdateVolume(m_ptrContext, 1, SDL_VOLUME_STEP);
                break;
            case SDLK_KP_DIVIDE:
            case SDLK_9:
                UpdateVolume(m_ptrContext, -1, SDL_VOLUME_STEP);
                break;
            case SDLK_s: // S: Step to next frame
                StepToNextFrame(m_ptrContext);
                break;
            case SDLK_a:
                StreamCycleChannel(m_ptrContext, AVMEDIA_TYPE_AUDIO);
                break;
            case SDLK_v:
                StreamCycleChannel(m_ptrContext, AVMEDIA_TYPE_VIDEO);
                break;
            case SDLK_c:
                StreamCycleChannel(m_ptrContext, AVMEDIA_TYPE_VIDEO);
                StreamCycleChannel(m_ptrContext, AVMEDIA_TYPE_AUDIO);
                StreamCycleChannel(m_ptrContext, AVMEDIA_TYPE_SUBTITLE);
                break;
            case SDLK_t:
                StreamCycleChannel(m_ptrContext, AVMEDIA_TYPE_SUBTITLE);
                break;
            case SDLK_w:
                if (m_ptrContext->eShowMode == SHOW_MODE_VIDEO && m_ptrContext->nVFilterIndex < m_ptrContext->nNBVFilters - 1)
                {
                    if (++m_ptrContext->nVFilterIndex >= m_ptrContext->nNBVFilters)
                        m_ptrContext->nVFilterIndex = 0;
                }
                else
                {
                    m_ptrContext->nVFilterIndex = 0;
                    ToggleAudioDisplay(m_ptrContext);
                }
                break;
            case SDLK_PAGEUP:
                if (m_ptrContext->ptrIC->nb_chapters <= 1)
                {
                    nIncr = 600.0;
                    goto do_seek;
                }
                SeekChapter(m_ptrContext, 1);
                break;
            case SDLK_PAGEDOWN:
                if (m_ptrContext->ptrIC->nb_chapters <= 1)
                {
                    nIncr = -600.0;
                    goto do_seek;
                }
                SeekChapter(m_ptrContext, -1);
                break;
            case SDLK_LEFT:
                nIncr = m_ptrParam->fSeekInterval ? -m_ptrParam->fSeekInterval : -10.0;
                goto do_seek;
            case SDLK_RIGHT:
                nIncr = m_ptrParam->fSeekInterval ? m_ptrParam->fSeekInterval : 10.0;
                goto do_seek;
            case SDLK_UP:
                nIncr = 60.0;
                goto do_seek;
            case SDLK_DOWN:
                nIncr = -60.0;
            do_seek:
                if (m_ptrParam->nSeekByBytes)
                {
                    pos = -1;
                    if (pos < 0 && m_ptrContext->nVideoStreamIndex >= 0)
                        pos = m_ptrContext->pictq.LastPos();
                    if (pos < 0 && m_ptrContext->nAudioStreamIndex >= 0)
                        pos = m_ptrContext->sampq.LastPos();
                    if (pos < 0)
                        pos = avio_tell(m_ptrContext->ptrIC->pb);
                    if (m_ptrContext->ptrIC->bit_rate)
                        nIncr *= m_ptrContext->ptrIC->bit_rate / 8.0;
                    else
                        nIncr *= 180000.0;
                    pos += nIncr;
                    StreamSeek(m_ptrContext, pos, nIncr, 1);
                }
                else
                {
                    pos = GetMasterClock(m_ptrContext);
                    if (isnan(pos))
                        pos = (double)m_ptrContext->nSeekPos / AV_TIME_BASE;
                    pos += nIncr;
                    if (m_ptrContext->ptrIC->start_time != AV_NOPTS_VALUE && pos < m_ptrContext->ptrIC->start_time / (double)AV_TIME_BASE)
                        pos = m_ptrContext->ptrIC->start_time / (double)AV_TIME_BASE;
                    StreamSeek(m_ptrContext, (int64_t)(pos * AV_TIME_BASE), (int64_t)(nIncr * AV_TIME_BASE), 0);
                }
                break;
            default:
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                static int64_t last_mouse_left_click = 0;
                if (av_gettime_relative() - last_mouse_left_click <= 500000)
                {
                    ToggleFullScreen(m_ptrContext);
                    m_ptrContext->bForceRefresh = true;
                    last_mouse_left_click = 0;
                }
                else
                {
                    last_mouse_left_click = av_gettime_relative();
                }
            }
        case SDL_MOUSEMOTION:
            if (m_ptrContext->bCursorHidden)
            {
                SDL_ShowCursor(1);
                m_ptrContext->bCursorHidden = false;
            }
            m_ptrContext->nCursorLastShown = av_gettime_relative();
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                if (event.button.button != SDL_BUTTON_RIGHT)
                    break;
                x = event.button.x;
            }
            else
            {
                if (!(event.motion.state & SDL_BUTTON_RMASK))
                    break;
                x = event.motion.x;
            }
            if (m_ptrParam->nSeekByBytes || m_ptrContext->ptrIC->duration <= 0)
            {
                uint64_t nSize = avio_size(m_ptrContext->ptrIC->pb);
                StreamSeek(m_ptrContext, nSize * x / m_ptrContext->nShowWidth, 0, 1);
            }
            else
            {
                int64_t ts;
                int ns, hh, mm, ss;
                int tns, thh, tmm, tss;
                tns = m_ptrContext->ptrIC->duration / 1000000LL;
                thh = tns / 3600;
                tmm = (tns % 3600) / 60;
                tss = (tns % 60);
                frac = x / m_ptrContext->nShowWidth;
                ns = frac * tns;
                hh = ns / 3600;
                mm = (ns % 3600) / 60;
                ss = (ns % 60);
                av_log(nullptr, AV_LOG_INFO, "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac * 100, hh, mm, ss, thh, tmm, tss);
                ts = frac * m_ptrContext->ptrIC->duration;
                if (m_ptrContext->ptrIC->start_time != AV_NOPTS_VALUE)
                    ts += m_ptrContext->ptrIC->start_time;
                StreamSeek(m_ptrContext, ts, 0, 0);
            }
            break;
        case SDL_WINDOWEVENT:
            switch (event.window.event)
            {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                m_ptrContext->nScreenWidth = m_ptrContext->nShowWidth = event.window.data1;
                m_ptrContext->nScreenHeight = m_ptrContext->nShowHeight = event.window.data2;
                if (m_ptrContext->vis_texture)
                {
                    SDL_DestroyTexture(m_ptrContext->vis_texture);
                    m_ptrContext->vis_texture = nullptr;
                }
#if HAVE_VULKAN_RENDERER
                if (m_ptrVKRenderer)
                    vk_renderer_resize(m_ptrVKRenderer.get(), nScreenWidth, nScreenHeight);
#endif
            case SDL_WINDOWEVENT_EXPOSED:
                m_ptrContext->bForceRefresh = true;
            }
            break;
        case SDL_QUIT:
        case FF_QUIT_EVENT:
            return;
        default:
            break;
        }
    }
}

CYPLAYER_NAMESPACE_END