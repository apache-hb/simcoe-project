#include "render/render.hpp"

using namespace sm;
using namespace sm::render;

Context::Context(RenderConfig config)
    : mSink(config.logger)
    , mInstance({ config.debug_flags, config.logger })
    , mFrames(config.frame_count)
    , mCommandLists(config.command_pool_size)
    , mResources(config.resource_pool_size)
{ }
