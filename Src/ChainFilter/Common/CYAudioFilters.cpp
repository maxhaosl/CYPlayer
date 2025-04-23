#include "ChainFilter/Common/CYAudioFilters.hpp"

#if __cplusplus
extern "C" {
#endif
#include "cmdutils.h"
#if __cplusplus
}
#endif

CYPLAYER_NAMESPACE_BEGIN

int ConfigureAudioFilters(SharePtr<CYMediaContext>& ptrContext, const char* pAFilters, int nForceOutputFormat)
{
    static const enum AVSampleFormat eSampleFmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
    int arrNSampleRates[2] = { 0, -1 };
    AVFilterContext* pFiltAsrc = nullptr, * pFiltAsink = nullptr;
    char szAreSampleSwrOpts[512] = "";
    const AVDictionaryEntry* pDictEntry = nullptr;
    AVBPrint bPrint;
    char szAsrcArgs[256];

    ptrContext->ptrAgraph.reset();
    if (!(ptrContext->ptrAgraph = CreateAVFilterGraph()))
        return AVERROR(ENOMEM);

    ptrContext->ptrAgraph->nb_threads = FILTER_NB_THREADS;

    av_bprint_init(&bPrint, 0, AV_BPRINT_SIZE_AUTOMATIC);

    while ((pDictEntry = av_dict_iterate(swr_opts, pDictEntry)))
        av_strlcatf(szAreSampleSwrOpts, sizeof(szAreSampleSwrOpts), "%s=%s:", pDictEntry->key, pDictEntry->value);
    if (strlen(szAreSampleSwrOpts))
        szAreSampleSwrOpts[strlen(szAreSampleSwrOpts) - 1] = '\0';
    av_opt_set(ptrContext->ptrAgraph.get(), "szAreSampleSwrOpts", szAreSampleSwrOpts, 0);

    av_channel_layout_describe_bprint(&ptrContext->objAudioFilterSrc.ch_layout, &bPrint);

    int nRet = snprintf(szAsrcArgs, sizeof(szAsrcArgs), "sample_rate=%d:sample_fmt=%s:time_base=%d/%d:channel_layout=%s", ptrContext->objAudioFilterSrc.freq, av_get_sample_fmt_name(ptrContext->objAudioFilterSrc.fmt), 1, ptrContext->objAudioFilterSrc.freq, bPrint.str);

    nRet = avfilter_graph_create_filter(&pFiltAsrc, avfilter_get_by_name("abuffer"), "ffplay_abuffer", szAsrcArgs, nullptr, ptrContext->ptrAgraph.get());
    if (nRet < 0)
        goto end;

    nRet = avfilter_graph_create_filter(&pFiltAsink, avfilter_get_by_name("abuffersink"), "ffplay_abuffersink", nullptr, nullptr, ptrContext->ptrAgraph.get());
    if (nRet < 0)
        goto end;

    if ((nRet = av_opt_set_int_list(pFiltAsink, "sample_fmts", eSampleFmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto end;
    if ((nRet = av_opt_set_int(pFiltAsink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto end;

    if (nForceOutputFormat)
    {
        av_bprint_clear(&bPrint);
        av_channel_layout_describe_bprint(&ptrContext->objAudioTgt.ch_layout, &bPrint);
        arrNSampleRates[0] = ptrContext->objAudioTgt.freq;
        if ((nRet = av_opt_set_int(pFiltAsink, "all_channel_counts", 0, AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
        if ((nRet = av_opt_set(pFiltAsink, "ch_layouts", bPrint.str, AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
        if ((nRet = av_opt_set_int_list(pFiltAsink, "sample_rates", arrNSampleRates, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
    }

    if ((nRet = ConfigureFilterGraph(ptrContext->ptrAgraph.get(), pAFilters, pFiltAsrc, pFiltAsink)) < 0)
        goto end;

    ptrContext->pInAudioFilter = pFiltAsrc;
    ptrContext->pOutAudioFilter = pFiltAsink;

end:
    if (nRet < 0)
    {
        ptrContext->ptrAgraph.reset();
    }
    av_bprint_finalize(&bPrint, nullptr);

    return nRet;
}

int ConfigureFilterGraph(AVFilterGraph* pGraph, const char* szFilterGraph, AVFilterContext* pSourceCtx, AVFilterContext* pSinkCtx)
{
    int nRet;
    int nBFilters = pGraph->nb_filters;
    AVFilterInOut* pOutputs = nullptr, * pInputs = nullptr;

    if (szFilterGraph)
    {
        pOutputs = avfilter_inout_alloc();
        pInputs = avfilter_inout_alloc();
        if (!pOutputs || !pInputs)
        {
            nRet = AVERROR(ENOMEM);
            goto fail;
        }

        pOutputs->name = av_strdup("in");
        pOutputs->filter_ctx = pSourceCtx;
        pOutputs->pad_idx = 0;
        pOutputs->next = nullptr;

        pInputs->name = av_strdup("out");
        pInputs->filter_ctx = pSinkCtx;
        pInputs->pad_idx = 0;
        pInputs->next = nullptr;

        if ((nRet = avfilter_graph_parse_ptr(pGraph, szFilterGraph, &pInputs, &pOutputs, nullptr)) < 0)
            goto fail;
    }
    else
    {
        if ((nRet = avfilter_link(pSourceCtx, 0, pSinkCtx, 0)) < 0)
            goto fail;
    }

    /* Reorder the filters to ensure that inputs of the custom filters are merged first */
    for (int i = 0; i < pGraph->nb_filters - nBFilters; i++)
        FFSWAP(AVFilterContext*, pGraph->filters[i], pGraph->filters[i + nBFilters]);

    nRet = avfilter_graph_config(pGraph, nullptr);
fail:
    avfilter_inout_free(&pOutputs);
    avfilter_inout_free(&pInputs);
    return nRet;
}

CYPLAYER_NAMESPACE_END