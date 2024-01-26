#pragma once

#include <simcoe_config.h>

#include "core/macros.hpp"
#include "logs/logs.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H

typedef struct arena_t arena_t;

namespace sm::service {
    class FreeType {
        arena_t *m_arena;
        FT_MemoryRec_ m_memory;
        FT_Library m_library = nullptr;

        static void *wrap_malloc(FT_Memory memory, long size);
        static void *wrap_realloc(FT_Memory memory, long cur_size, long new_size, void *block);
        static void wrap_free(FT_Memory memory, void *block);

    public:
        SM_NOCOPY(FreeType)

        FreeType(arena_t *arena);
        ~FreeType();
    };

    void init_freetype(logs::ILogger *logger);
}
