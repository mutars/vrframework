#pragma once

#include <sl.h>

#include <windows.h>

namespace slcompat {

#if defined(STREAMLINE_LEGACY)
#ifndef SL_STRUCT_TYPE_VIEWPORT
namespace sl {
struct ViewportHandle {
    uint32_t id = 0;
};
} // namespace sl
#endif

#ifndef SL_STRUCT_TYPE_RESOURCE_TAG
namespace sl {
struct ResourceTag;
} // namespace sl
#endif

#ifndef SL_STRUCT_TYPE_FRAME_TOKEN
namespace sl {
using FrameToken = uint32_t;
} // namespace sl
#endif
#endif

#if defined(STREAMLINE_LEGACY)
using SlSetTagFn = sl::Result (*)(sl::ViewportHandle, const sl::ResourceTag*, uint32_t, sl::CommandBuffer*);
using SlEvaluateFeatureFn = sl::Result (*)(sl::Feature, const sl::FrameToken&, sl::BaseStructure* const*, uint32_t, sl::CommandBuffer*);
using SlSetConstantsFn = sl::Result (*)(sl::Constants*, const sl::FrameToken&, sl::ViewportHandle&);
#else
using SlSetTagFn = sl::Result (*)(sl::ViewportHandle&, const sl::ResourceTag*, uint32_t, sl::CommandBuffer*);
using SlEvaluateFeatureFn = sl::Result (*)(sl::Feature, const sl::FrameToken&, sl::BaseStructure**, uint32_t, sl::CommandBuffer*);
using SlSetConstantsFn = sl::Result (*)(sl::Constants&, const sl::FrameToken&, sl::ViewportHandle&);
#endif

inline bool slCompatGetFeatureFunction(HMODULE module, sl::Feature feature, const char* functionName, void*& function)
{
    using GetFeatureFunctionFn = sl::Result (*)(sl::Feature, const char*, void*&);
    auto get_feature_function = reinterpret_cast<GetFeatureFunctionFn>(GetProcAddress(module, "slGetFeatureFunction"));
    if (get_feature_function) {
        return get_feature_function(feature, functionName, function) == sl::Result::eOk;
    }

    function = reinterpret_cast<void*>(GetProcAddress(module, functionName));
    return function != nullptr;
}

inline sl::Result slCompatSetTag(SlSetTagFn fn, sl::ViewportHandle& viewport, const sl::ResourceTag* tags, uint32_t numTags, sl::CommandBuffer* cmdBuffer)
{
#if defined(STREAMLINE_LEGACY)
    return fn(viewport, tags, numTags, cmdBuffer);
#else
    return fn(viewport, tags, numTags, cmdBuffer);
#endif
}

inline sl::Result slCompatEvaluateFeature(SlEvaluateFeatureFn fn, sl::Feature feature, const sl::FrameToken& frame, sl::BaseStructure** inputs, uint32_t numInputs, sl::CommandBuffer* cmdBuffer)
{
#if defined(STREAMLINE_LEGACY)
    return fn(feature, frame, const_cast<sl::BaseStructure* const*>(inputs), numInputs, cmdBuffer);
#else
    return fn(feature, frame, inputs, numInputs, cmdBuffer);
#endif
}

inline sl::Result slCompatSetConstants(SlSetConstantsFn fn, sl::Constants& values, const sl::FrameToken& frame, sl::ViewportHandle& viewport)
{
#if defined(STREAMLINE_LEGACY)
    return fn(&values, frame, viewport);
#else
    return fn(values, frame, viewport);
#endif
}

} // namespace slcompat
