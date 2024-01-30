#include "service/freetype.hpp"

#include "arena/arena.h"
#include "base/panic.h"
#include "core/arena.hpp"
#include "core/macros.hpp"
#include "std/str.h"

#include <freetype/ftlogging.h>

using namespace sm;
using namespace sm::service;

static logs::ILogger *gFreeTypeLogger = nullptr;
static FreeType gFreeType;

CT_NORETURN
service::assert_ft2(source_info_t info, FT_Error error, const char *expr, const char *msg) {
    const char *str = FT_Error_String(error);
    ctu_panic(info, "freetype2 error: %s\n - expr: %s\n - error: %s (%d)", msg, expr, str, error);
}

void *FreeType::wrap_malloc(FT_Memory memory, long size) {
    FreeType *service = static_cast<FreeType *>(memory->user);
    return ARENA_MALLOC(size, "freetype2", memory, service->m_arena);
}

void *FreeType::wrap_realloc(FT_Memory memory, long cur_size, long new_size, void *block) {
    FreeType *service = static_cast<FreeType *>(memory->user);
    return arena_realloc(block, new_size, cur_size, service->m_arena);
}

void FreeType::wrap_free(FT_Memory memory, void *block) {
    FreeType *service = static_cast<FreeType *>(memory->user);
    arena_free(block, CT_ALLOC_SIZE_UNKNOWN, service->m_arena);
}

void FreeType::init(arena_t *arena) {
    m_arena = arena;

    CTASSERTF(gFreeTypeLogger != nullptr, "service::init_freetype not called");
    CTASSERT(arena != nullptr);

    m_memory.user = this;
    m_memory.alloc = wrap_malloc;
    m_memory.realloc = wrap_realloc;
    m_memory.free = wrap_free;
    SM_ASSERT_FT2(FT_New_Library(&m_memory, &m_library), "failed to initialize freetype");

    FT_Add_Default_Modules(m_library);

    gFreeTypeLogger->log(logs::Category::eAssets, logs::Severity::eInfo, "initialized freetype2");
}

void FreeType::deinit() {
    CTASSERTF(gFreeTypeLogger != nullptr, "service::init_freetype not called");
    CTASSERTF(m_library != nullptr, "service::init_freetype not called");

    FT_Done_Library(m_library);
    m_library = nullptr;

    gFreeTypeLogger->log(logs::Category::eAssets, logs::Severity::eInfo, "destroyed freetype2");
}

void service::init_freetype(arena_t *arena, SM_UNUSED logs::ILogger *logger) {
    gFreeTypeLogger = logger;
    FT_Trace_Set_Default_Level();
    FT_Set_Log_Handler([](const char *component, const char *fmt, va_list args) {
        logs::Sink<logs::Category::eAssets> sink { *gFreeTypeLogger };
        sm::IArena& arena = sm::get_pool(sm::Pool::eDebug);
        text_t text = text_vformat(&arena, fmt, args);
        std::string_view view { text.text, text.text + text.length };
        sink.trace("{}: {}", component, view);
    });

    gFreeType.init(arena);
}

void service::deinit_freetype() {
    gFreeType.deinit();
    gFreeTypeLogger = nullptr;
}

FreeType& service::get_freetype() {
    CTASSERTF(gFreeTypeLogger != nullptr, "service::init_freetype not called");
    return gFreeType;
}
