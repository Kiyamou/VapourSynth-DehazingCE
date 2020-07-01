

#include <memory>
#include <string>

#include "DehazingCE.h"
#include "DehazingCE.cpp"

struct FilterData
{
    VSNodeRef* node;
    const VSVideoInfo* vi;
    VSNodeRef* rnode;
    const VSVideoInfo* rvi;
    bool rdef;
    dehazing* dehazing_clip;
};

static void VS_CC filterInit(VSMap* in, VSMap* out, void** instanceData, VSNode* node, VSCore* core, const VSAPI* vsapi)
{
    FilterData* d = static_cast<FilterData*>(*instanceData);
    vsapi->setVideoInfo(d->vi, 1, node);
}

template<typename T>
static void process(const VSFrameRef* src, const VSFrameRef* ref, VSFrameRef* dst, const FilterData* const VS_RESTRICT d, const VSAPI* vsapi) noexcept
{
    int width = vsapi->getFrameWidth(src, 0);
    int height = vsapi->getFrameHeight(src, 0);
    int stride = vsapi->getStride(dst, 0) / sizeof(T);

    int ref_width = vsapi->getFrameWidth(ref, 0);
    int ref_height = vsapi->getFrameHeight(ref, 0);
    int ref_stride = vsapi->getStride(ref, 0) / sizeof(T);

    //// Convert RGB to 1-D ararry ////
    const T* srcpR = reinterpret_cast<const T*>(vsapi->getReadPtr(src, 0));
    const T* srcpG = reinterpret_cast<const T*>(vsapi->getReadPtr(src, 1));
    const T* srcpB = reinterpret_cast<const T*>(vsapi->getReadPtr(src, 2));

    const T* refpR = reinterpret_cast<const T*>(vsapi->getReadPtr(ref, 0));
    const T* refpG = reinterpret_cast<const T*>(vsapi->getReadPtr(ref, 1));
    const T* refpB = reinterpret_cast<const T*>(vsapi->getReadPtr(ref, 2));

    // Interleaved
    T* srcInterleaved = new (std::nothrow) T[(d->vi->width) * d->vi->height * 3];
    T* dstInterleaved = new (std::nothrow) T[(d->vi->width) * d->vi->height * 3];

    for (auto y = 0; y < height; y++)
    {
        for (auto x = 0; x < width; x++)
        {
            const auto pos = (x + y * width) * 3;
            srcInterleaved[pos] = srcpB[x];
            srcInterleaved[pos + 1] = srcpG[x];
            srcInterleaved[pos + 2] = srcpR[x];
        }
        srcpB += stride;
        srcpG += stride;
        srcpR += stride;
    }

    d->dehazing_clip->RemoveHaze((const T*)srcInterleaved, refpB, refpG, refpR, dstInterleaved, stride, ref_width, ref_height);

    //// change back from Interleaved
    T* VS_RESTRICT dstpR = reinterpret_cast<T*>(vsapi->getWritePtr(dst, 0));
    T* VS_RESTRICT dstpG = reinterpret_cast<T*>(vsapi->getWritePtr(dst, 1));
    T* VS_RESTRICT dstpB = reinterpret_cast<T*>(vsapi->getWritePtr(dst, 2));

    for (auto y = 0; y < height; y++)
    {
        for (auto x = 0; x < width; x++)
        {
            const unsigned pos = (x + y * width) * 3;
            dstpB[x] = dstInterleaved[pos];
            dstpG[x] = dstInterleaved[pos + 1];
            dstpR[x] = dstInterleaved[pos + 2];
        }
        dstpB += stride;
        dstpG += stride;
        dstpR += stride;
    }

    delete[] srcInterleaved;
    delete[] dstInterleaved;
}

static const VSFrameRef* VS_CC filterGetFrame(int n, int activationReason, void** instanceData, void** frameData,
    VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi)
{
    const FilterData* d = static_cast<const FilterData*>(*instanceData);
    if (activationReason == arInitial)
    {
        vsapi->requestFrameFilter(n, d->node, frameCtx);
        if (d->rdef)
            vsapi->requestFrameFilter(n, d->rnode, frameCtx);
    }
    else if (activationReason == arAllFramesReady)
    {
        const VSFrameRef* src = vsapi->getFrameFilter(n, d->node, frameCtx);
        VSFrameRef* dst = vsapi->newVideoFrame(d->vi->format, d->vi->width, d->vi->height, src, core);

        const VSFrameRef* ref;
        if (d->rdef)
            ref = vsapi->getFrameFilter(n, d->rnode, frameCtx);
        else
            ref = src;

        if (d->vi->format->bytesPerSample == 1)
            process<uint8_t>(src, ref, dst, d, vsapi);
        else if (d->vi->format->bytesPerSample == 2)
            process<uint16_t>(src, ref, dst, d, vsapi);

        vsapi->freeFrame(src);
        if (d->rdef)
            vsapi->freeFrame(ref);

        return dst;
    }

    return 0;
}

static void VS_CC filterFree(void* instanceData, VSCore* core, const VSAPI* vsapi)
{
    FilterData* d = static_cast<FilterData*>(instanceData);
    vsapi->freeNode(d->node);
    delete d;
}

static void VS_CC filterCreate(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* vsapi)
{
    std::unique_ptr<FilterData> d = std::make_unique<FilterData>();
    int err;

    d->node = vsapi->propGetNode(in, "src", 0, 0);
    d->vi = vsapi->getVideoInfo(d->node);

    int width = d->vi->width;
    int height = d->vi->height;
    int bits = d->vi->format->bitsPerSample;

    try
    {
        if (!isConstantFormat(d->vi) || d->vi->format->colorFamily != cmRGB ||
            (d->vi->format->sampleType == stInteger && d->vi->format->bitsPerSample > 16))
            throw std::string{ "only constant format RGB 8-16 bit integer input supported" };

        // donwscale clip for Trans Estimation
        d->rnode = vsapi->propGetNode(in, "ref", 0, &err);

        if (err)
        {
            d->rdef = false;
            d->rnode = d->node;
            d->rvi = d->vi;
        }
        else
        {
            d->rdef = true;
            d->rvi = vsapi->getVideoInfo(d->rnode);

            // the scale of width and height of ref should be same with src
            if (!isConstantFormat(d->rvi))
                throw std::string("Invalid clip \"ref\", only constant format input supported");
            if (d->rvi->format != d->vi->format)
                throw std::string("input clip and clip \"ref\" must be of the same format");
            if (d->rvi->numFrames != d->vi->numFrames)
                throw std::string("input clip and clip \"ref\" must have the same number of frames");
        }

        int ABlockSize = int64ToIntS(vsapi->propGetInt(in, "air_size", 0, &err));
        if (err)
            ABlockSize = 200;

        int GBlockSize = int64ToIntS(vsapi->propGetInt(in, "guide_size", 0, &err));
        if (err)
            GBlockSize = 40;

        int TBlockSize = int64ToIntS(vsapi->propGetInt(in, "trans_size", 0, &err));
        if (err)
            TBlockSize = 16;

        float TransInit = (float)(vsapi->propGetFloat(in, "trans", 0, &err));
        if (err)
            TransInit = 0.3f;

        double lamdaA = vsapi->propGetFloat(in, "lamda", 0, &err);
        if (err)
            lamdaA = 5.0;

		float gamma = (float)(vsapi->propGetFloat(in, "gamma", 0, &err));
        if (err)
            gamma = 0.7f;

        bool PostFlag = vsapi->propGetInt(in, "post", 0, &err) == 0 ? false : true;
        if (err)
            PostFlag = false;

        d->dehazing_clip = new dehazing(width, height, bits, ABlockSize, TBlockSize, TransInit, false, PostFlag, lamdaA, 1.f, GBlockSize);

        //d->dehazing_clip->MakeExpLUT(); // called in NFTrsEstimationPColor(), NFTrsEstimationP()
        //d->dehazing_clip->GuideLUTMaker(); // called in FastGuideFilter()
        d->dehazing_clip->GammaLUTMaker(gamma);
    }
    catch (const std::string & error)
    {
        vsapi->setError(out, ("Dehazing: " + error).c_str());
        vsapi->freeNode(d->node);
        return;
    }

    vsapi->createFilter(in, out, "Dehazing", filterInit, filterGetFrame, filterFree, fmParallel, 0, d.release(), core);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin* plugin)
{
    configFunc("com.vapoursynth.dehazingce", "dhce", "Dehazing based on contrast enhancement", VAPOURSYNTH_API_VERSION, 1, plugin);

    registerFunc("Dehazing",
        "src:clip;"
        "ref:clip:opt;"
        "trans:float:opt;"
        "air_size:int:opt;"
        "guide_size:int:opt;"
        "trans_size:int:opt;"
        "lamda:float:opt;"
        "gamma:float:opt;"
        "post:int:opt",
        filterCreate, 0, plugin);
}
