#include "core/arena.hpp"

#include "base/panic.h"
#include "core/backtrace.hpp"
#include "core/text.hpp"
#include "format/backtrace.h"
#include "format/colour.h"
#include "io/console.h"

#include "backtrace/backtrace.h"
#include "os/os.h"

#include <iostream>

using namespace sm;

class DefaultArena final : public IArena {
    using IArena::IArena;

    void *impl_alloc(size_t size) override {
        return std::malloc(size);
    }

    void *impl_resize(void *ptr, size_t new_size, size_t old_size) override {
        CTU_UNUSED(old_size);

        return std::realloc(ptr, new_size);
    }

    void impl_release(void *ptr, size_t size) override {
        CTU_UNUSED(size);

        std::free(ptr);
    }
};

class TraceArena final : public IArena {
    void *impl_alloc(size_t size) override {
        void *ptr = m_source->alloc(size);
        std::printf("[%s] alloc(%zu) = %p\n", name, size, ptr);
        return ptr;
    }

    void *impl_resize(void *ptr, size_t new_size, size_t old_size) override {
        void *new_ptr = m_source->resize(ptr, new_size, old_size);
        std::printf("[%s] resize(0x%p, %zu, %zu) = 0x%p\n", name, ptr, new_size, old_size, new_ptr);
        return new_ptr;
    }

    void impl_release(void *ptr, size_t size) override {
        m_source->release(ptr, size);
        std::printf("[%s] release(0x%p, %zu)\n", name, ptr, size);
    }

    void impl_rename(const void *ptr, const char *ptr_name) override {
        std::printf("[%s] rename(0x%p, %s)\n", name, ptr, ptr_name);
    }

    void impl_reparent(const void *ptr, const void *parent) override {
        std::printf("[%s] reparent(0x%p, %p)\n", name, ptr, parent);
    }

    IArena *m_source;

public:
    TraceArena(const char *name, IArena *source)
        : IArena(name)
        , m_source(source)
    { }
};

static print_backtrace_t print_options_make(arena_t *arena, io_t *io) {
    print_backtrace_t print = {
        .options = {
            .arena = arena,
            .io = io,
            .pallete = &kColourDefault
        },
        .heading_style = eHeadingGeneric,
        .zero_indexed_lines = false,
    };

    return print;
}

class DefaultSystemError final : public ISystemError {
    void error_begin(size_t error) override {
        m_report = bt_report_new(m_arena);
        std::printf("system_error(0x%zX)\n", error);
    }

    void error_frame(const bt_frame_t *it) override {
        bt_report_add(m_report, it);
    }

    void error_end(void) override {
        const print_backtrace_t kPrintOptions = print_options_make(m_arena, io_stderr());
        print_backtrace(kPrintOptions, m_report);
        std::exit(CT_EXIT_INTERNAL);
    }

    arena_t *m_arena = nullptr;
    bt_report_t *m_report = nullptr;

public:
    DefaultSystemError(arena_t *arena)
        : m_arena(arena)
    { }
};

static DefaultArena gGlobalArena{"default"};
static TraceArena gTraceArena{"trace", &gGlobalArena};
static DefaultSystemError gDefaultError{&gGlobalArena};

int main(int argc, const char **argv) {
    bt_init();
    os_init();

    gSystemError = gDefaultError;

    gPanicHandler = [](panic_t info, const char *msg, va_list args) {
        const print_backtrace_t kPrintOptions = print_options_make(&gGlobalArena, io_stderr());

        Text message = Text::vformat(&gGlobalArena, msg, args);

        std::printf("[%s:%zu] %s\npanic: %.*s\n", info.file, info.line, info.function, (int)message.count(), message.data());

        bt_report_t *report = bt_report_collect(&gGlobalArena);
        print_backtrace(kPrintOptions, report);
    };

    CTU_UNUSED(argc);
    CTU_UNUSED(argv);

    std::cout << "CTU_DEBUG = " << CTU_DEBUG << std::endl;
    std::cout << "SMC_DEBUG = " << SMC_DEBUG << std::endl;

    volatile int *ptr = nullptr;
    *ptr = 0;
}
