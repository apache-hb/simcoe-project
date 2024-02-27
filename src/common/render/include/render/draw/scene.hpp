#include "render/task/begin.hpp"

TASK_BEGIN(Scene)
    TASK_TEXTURE_CREATE(depth, ResourceState::eDepthWrite, RELATIVE_SIZE(), RELATIVE_SIZE(), Format::eF32_DEPTH)
    TASK_TEXTURE_CREATE(colour, ResourceState::eRenderTarget, RELATIVE_SIZE(), RELATIVE_SIZE(), Format::eRGBA8_UNORM)
TASK_END()

#include "render/task/end.hpp"
