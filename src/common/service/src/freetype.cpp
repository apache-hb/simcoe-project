#include "service/freetype.hpp"

#include "arena/arena.h"
#include "base/panic.h"
#include "core/arena.hpp"
#include "std/str.h"

#include <freetype/ftlogging.h>

using namespace sm;
using namespace sm::service;

static logs::ILogger *gFreeTypeLogger = nullptr;

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

FreeType::FreeType(arena_t *arena)
    : m_arena(arena)
{
    CTASSERTF(gFreeTypeLogger != nullptr, "service::init_freetype not called");
    CTASSERT(arena != nullptr);

    m_memory.user = this;
    m_memory.alloc = wrap_malloc;
    m_memory.realloc = wrap_realloc;
    m_memory.free = wrap_free;
    if (FT_Error error = FT_New_Library(&m_memory, &m_library)) {
        NEVER("failed to initialize freetype: %d %s", error, FT_Error_String(error));
    }

    FT_Add_Default_Modules(m_library);

    gFreeTypeLogger->log(logs::Category::eGlobal, logs::Severity::eInfo, "initialized freetype2");
}

FreeType::~FreeType() {
    if (m_library != nullptr) {
        FT_Done_Library(m_library);
        m_library = nullptr;

        gFreeTypeLogger->log(logs::Category::eGlobal, logs::Severity::eInfo, "destroyed freetype2");
    }
}

void service::init_freetype(SM_UNUSED logs::ILogger *logger) {
    gFreeTypeLogger = logger;
    FT_Trace_Set_Default_Level();
    FT_Set_Log_Handler([](const char *component, const char *fmt, va_list args) {
        logs::Sink<logs::Category::eGlobal> sink { *gFreeTypeLogger };
        sm::IArena& arena = sm::get_pool(sm::Pool::eDebug);
        text_t text = text_vformat(&arena, fmt, args);
        std::string_view view { text.text, text.text + text.length };
        sink.trace("{}: {}", component, view);
    });
}
