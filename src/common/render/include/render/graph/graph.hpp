#pragma once

namespace sm::graph {
    enum class ResourceType {
        eTransient,
        eImported,
        eManaged,
    };

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
