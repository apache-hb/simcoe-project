#pragma once

#include "graph.meta.hpp"

namespace sm::graph {
    REFLECT_ENUM(ResourceType)
    enum class ResourceType {
        eTransient,
        eImported,
        eManaged,
    };

    REFLECT_ENUM(Usage)
    enum class Usage {
        eUnknown,

        ePresent,

        ePixelShaderResource,
        eComputeShaderResource,

        eTextureRead,
        eTextureWrite,
        eRenderTarget,
        eBufferRead,
        eBufferWrite,
        eCopySource,
        eCopyTarget,
        eDepthRead,
        eDepthWrite,
        eIndexBuffer,
        eVertexBuffer,
        eManualOverride,
    };
}
