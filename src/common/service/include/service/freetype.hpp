#pragma once

#include <simcoe_config.h>

#include "logs/logs.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H

typedef struct arena_t arena_t;

#define SM_ASSERT_FT2(expr, ...) \
    do { \
        if (FT_Error error = (expr)) { \
            sm::service::assert_ft2(CT_SOURCE_HERE, error, #expr, __VA_ARGS__); \
        } \
    } while (0)

namespace sm::service {
    CT_NORETURN
    assert_ft2(source_info_t info, FT_Error error, const char *expr, const char *fmt, ...);

    class FreeType {
        arena_t *m_arena = nullptr;
        FT_MemoryRec_ m_memory{};
        FT_Library m_library = nullptr;

        static void *wrap_malloc(FT_Memory memory, long size);
        static void *wrap_realloc(FT_Memory memory, long cur_size, long new_size, void *block);
        static void wrap_free(FT_Memory memory, void *block);

        friend void init_freetype(arena_t *arena, logs::ILogger *logger);
        friend void deinit_freetype();

        void init(arena_t *arena);
        void deinit();

    public:
        FT_Library get_library() const {
            return m_library;
        }
    };

    void init_freetype(arena_t *arena, logs::ILogger *logger);
    void deinit_freetype();
    FreeType& get_freetype();
}
