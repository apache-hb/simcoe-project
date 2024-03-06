#ifndef TASK_BEGIN
#   define TASK_BEGIN(name)
#endif

#ifndef TASK_END
#   define TASK_END()
#endif

#ifndef TASK_TEXTURE_READ
#   define TASK_TEXTURE_READ(name, state)
#endif

#ifndef TASK_TEXTURE_WRITE
#   define TASK_TEXTURE_WRITE(name, state)
#endif

#ifndef TASK_TEXTURE_CREATE
#   define TASK_TEXTURE_CREATE(name, state, width, height, format)
#endif

#ifndef TASK_DEPTH_CREATE
#   define TASK_DEPTH_CREATE(name, state, width, height, format)
#endif

#ifndef TEMPORAL
#   define TEMPORAL(x)
#endif

#include "render/graph.hpp" // IWYU pragma: export
