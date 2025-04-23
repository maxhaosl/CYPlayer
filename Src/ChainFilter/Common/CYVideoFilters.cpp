#include "ChainFilter/Common/CYVideoFilters.hpp"
#include "ChainFilter/Common/CYAudioFilters.hpp"

#if __cplusplus
extern "C" {
#endif
#include "cmdutils.h"
#if __cplusplus
}
#endif

CYPLAYER_NAMESPACE_BEGIN

struct TextureFormatEntry sdl_texture_format_map[20] = {
    { AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
    { AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
    { AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
    { AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
    { AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
    { AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
    { AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
    { AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
    { AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
    { AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
    { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
    { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
    { AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
    { AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
    { AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
    { AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
    { AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
    { AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
    { AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
    { AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN },
};

void CalculateDisplayRect(SDL_Rect* pRect, int nScrXleft, int nScrYtop, int nScrWidth, int nScrHeight, int nPicWidth, int nPicHeight, AVRational objPicSar)
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

void SetDefaultWindowSize(SharePtr<CYMediaContext>& ptrContext, int nWidth, int nHeight, AVRational objSar)
{
    SDL_Rect rect;
    int max_width = ptrContext->nScreenWidth ? ptrContext->nScreenWidth : INT_MAX;
    int max_height = ptrContext->nScreenHeight ? ptrContext->nScreenHeight : INT_MAX;
    if (max_width == INT_MAX && max_height == INT_MAX)
        max_height = nHeight;
    CalculateDisplayRect(&rect, 0, 0, max_width, max_height, nWidth, nHeight, objSar);
    ptrContext->nDefaultWidth = rect.w;
    ptrContext->nDefaultHeight = rect.h;
}

int ConfigureVideoFilters(AVFilterGraph* graph, SharePtr<CYMediaContext>& ptrContext, const char* vfilters, AVFrame* pFrame)
{
    enum AVPixelFormat pix_fmts[FF_ARRAY_ELEMS(sdl_texture_format_map)];
    char sws_flags_str[512] = "";
    char buffersrc_args[256];
    int ret;
    AVFilterContext* filt_src = nullptr, * filt_out = nullptr, * last_filter = nullptr;
    AVCodecParameters* codecpar = ptrContext->pVideoStream->codecpar;
    AVRational fr = av_guess_frame_rate(ptrContext->ptrIC.get(), ptrContext->pVideoStream, nullptr);
    const AVDictionaryEntry* e = nullptr;
    int nb_pix_fmts = 0;
    int i, j;
    AVBufferSrcParameters* par = av_buffersrc_parameters_alloc();

    if (!par)
        return AVERROR(ENOMEM);

    for (i = 0; i < ptrContext->objRendererInfo.num_texture_formats; i++)
    {
        for (j = 0; j < FF_ARRAY_ELEMS(sdl_texture_format_map) - 1; j++)
        {
            if (ptrContext->objRendererInfo.texture_formats[i] == sdl_texture_format_map[j].texture_fmt)
            {
                pix_fmts[nb_pix_fmts++] = sdl_texture_format_map[j].format;
                break;
            }
        }
    }
    pix_fmts[nb_pix_fmts] = AV_PIX_FMT_NONE;

    while ((e = av_dict_iterate(sws_dict, e)))
    {
        if (!strcmp(e->key, "sws_flags"))
        {
            av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", "flags", e->value);
        }
        else
            av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", e->key, e->value);
    }
    if (strlen(sws_flags_str))
        sws_flags_str[strlen(sws_flags_str) - 1] = '\0';

    graph->scale_sws_opts = av_strdup(sws_flags_str);

    snprintf(buffersrc_args, sizeof(buffersrc_args),
        "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d:"
        "colorspace=%d:range=%d",
        pFrame->width, pFrame->height, pFrame->format,
        ptrContext->pVideoStream->time_base.num, ptrContext->pVideoStream->time_base.den,
        codecpar->sample_aspect_ratio.num, FFMAX(codecpar->sample_aspect_ratio.den, 1),
        pFrame->colorspace, pFrame->color_range);
    if (fr.num && fr.den)
        av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d", fr.num, fr.den);

    if ((ret = avfilter_graph_create_filter(&filt_src,
        avfilter_get_by_name("buffer"),
        "ffplay_buffer", buffersrc_args, nullptr,
        graph)) < 0)
        goto fail;
    par->hw_frames_ctx = pFrame->hw_frames_ctx;
    ret = av_buffersrc_parameters_set(filt_src, par);
    if (ret < 0)
        goto fail;

    ret = avfilter_graph_create_filter(&filt_out,
        avfilter_get_by_name("buffersink"),
        "ffplay_buffersink", nullptr, nullptr, graph);
    if (ret < 0)
        goto fail;

    if ((ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto fail;

#if HAVE_VULKAN_RENDERER
    if (!m_ptrVKRenderer &&
        (ret = av_opt_set_int_list(filt_out, "color_spaces", sdl_supported_color_spaces, AVCOL_SPC_UNSPECIFIED, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto fail;
#endif

    last_filter = filt_out;

    /* Note: this macro adds a filter before the lastly added filter, so the
     * processing order of the filters is in reverse */
#define INSERT_FILT(name, arg) do {                                          \
    AVFilterContext *filt_ctx;                                               \
                                                                             \
    ret = avfilter_graph_create_filter(&filt_ctx,                            \
                                       avfilter_get_by_name(name),           \
                                       "ffplay_" name, arg, nullptr, graph);    \
    if (ret < 0)                                                             \
        goto fail;                                                           \
                                                                             \
    ret = avfilter_link(filt_ctx, 0, last_filter, 0);                        \
    if (ret < 0)                                                             \
        goto fail;                                                           \
                                                                             \
    last_filter = filt_ctx;                                                  \
} while (0)

    if (ptrContext->bAutoRotate)
    {
        double theta = 0.0;
        int32_t* displaymatrix = nullptr;
        AVFrameSideData* sd = av_frame_get_side_data(pFrame, AV_FRAME_DATA_DISPLAYMATRIX);
        if (sd)
            displaymatrix = (int32_t*)sd->data;
        if (!displaymatrix)
        {
            const AVPacketSideData* psd = av_packet_side_data_get(ptrContext->pVideoStream->codecpar->coded_side_data,
                ptrContext->pVideoStream->codecpar->nb_coded_side_data,
                AV_PKT_DATA_DISPLAYMATRIX);
            if (psd)
                displaymatrix = (int32_t*)psd->data;
        }
        theta = get_rotation(displaymatrix);

        if (fabs(theta - 90) < 1.0)
        {
            INSERT_FILT("transpose", displaymatrix[3] > 0 ? "cclock_flip" : "clock");
        }
        else if (fabs(theta - 180) < 1.0)
        {
            if (displaymatrix[0] < 0)
                INSERT_FILT("hflip", nullptr);
            if (displaymatrix[4] < 0)
                INSERT_FILT("vflip", nullptr);
        }
        else if (fabs(theta - 270) < 1.0)
        {
            INSERT_FILT("transpose", displaymatrix[3] < 0 ? "clock_flip" : "cclock");
        }
        else if (fabs(theta) > 1.0)
        {
            char rotate_buf[64];
            snprintf(rotate_buf, sizeof(rotate_buf), "%f*PI/180", theta);
            INSERT_FILT("rotate", rotate_buf);
        }
        else
        {
            if (displaymatrix && displaymatrix[4] < 0)
                INSERT_FILT("vflip", nullptr);
        }
    }

    if ((ret = ConfigureFilterGraph(graph, vfilters, filt_src, last_filter)) < 0)
        goto fail;

    ptrContext->pInVideoFilter = filt_src;
    ptrContext->pOutVideoFilter = filt_out;

fail:
    av_freep(&par);
    return ret;
}

CYPLAYER_NAMESPACE_END