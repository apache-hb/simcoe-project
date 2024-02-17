#include "archive/io.hpp"
#include "core/arena.hpp"
#include "core/backtrace.hpp"
#include "core/format.hpp"
#include "core/macros.h"
#include "core/reflect.hpp" // IWYU pragma: keep
#include "core/text.hpp"
#include "core/units.hpp"
// #include "math/format.hpp"
#include "logs/sink.inl" // IWYU pragma: keep

#include "service/freetype.hpp"

#include "system/input.hpp"
#include "system/io.hpp"
#include "system/system.hpp"

#include "threads/threads.hpp"

#include "render/render.hpp"
#include "render/draw.hpp"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#include "backtrace/backtrace.h"
#include "base/panic.h"
#include "format/backtrace.h"
#include "format/colour.h"
#include "io/console.h"
#include "io/io.h"
#include "std/str.h"

using namespace sm;
using namespace render;
using namespace math;

using GlobalSink = logs::Sink<logs::Category::eGlobal>;

// void *operator new(size_t size) {
//     NEVER("operator new called");
// }

// void operator delete(void *ptr) {
//     NEVER("operator delete called");
// }

// void *operator new[](size_t size) {
//     NEVER("operator new[] called");
// }

// void operator delete[](void *ptr) {
//     NEVER("operator delete[] called");
// }

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

// TODO: clean up loggers

static std::string_view format_log(sm::FormatBuffer &buffer, const logs::Message &message,
                                   const char *colour, const char *reset) {
    // we dont have fmt/chrono.h because we use it in header only mode
    // so pull out the hours/minutes/seconds/milliseconds manually

    auto hours = (message.timestamp / (60 * 60 * 1000)) % 24;
    auto minutes = (message.timestamp / (60 * 1000)) % 60;
    auto seconds = (message.timestamp / 1000) % 60;
    auto milliseconds = message.timestamp % 1000;

    fmt::format_to(std::back_inserter(buffer), "{}[{}]{}[{:02}:{:02}:{:02}.{:03}] {}:", colour,
                   message.severity, reset, hours, minutes, seconds, milliseconds,
                   message.category);

    std::string_view header{buffer.data(), buffer.size()};

    return header;
}

class FileLog final : public logs::ILogger {
    sm::FormatBuffer m_buffer;
    io_t *io;

    void accept(const logs::Message &message) override {
        std::string_view header = format_log(m_buffer, message, "", "");

        // ranges is impossible to use without going through a bunch of hoops
        // just iterate over the message split by newlines

        auto it = message.message.begin();
        auto end = message.message.end();

        while (it != end) {
            auto next = std::find(it, end, '\n');
            auto line = std::string_view{&*it, static_cast<size_t>(std::distance(it, next))};
            it = next;

            io_write(io, header.data(), header.size());
            io_write(io, line.data(), line.size());
            io_write(io, "\n", 1);

            if (it != end) {
                ++it;
            }
        }

        m_buffer.clear();
    }

public:
    constexpr FileLog(IArena &arena, io_t *io)
        : ILogger(logs::Severity::eInfo)
        , m_buffer(arena)
        , io(io) {}
};

class ConsoleLog final : public logs::ILogger {
    sm::FormatBuffer m_buffer;

    static constexpr colour_t get_colour(logs::Severity severity) {
        using Reflect = ctu::TypeInfo<logs::Severity>;
        CTASSERTF(severity.is_valid(), "invalid severity: %s", Reflect::to_string(severity).data());

        using logs::Severity;
        switch (severity) {
        case Severity::eTrace: return eColourWhite;
        case Severity::eInfo: return eColourGreen;
        case Severity::eWarning: return eColourYellow;
        case Severity::eError: return eColourRed;
        case Severity::ePanic: return eColourMagenta;
        default: return eColourDefault;
        }
    }

    void accept(const logs::Message &message) override {
        const auto pallete = &kColourDefault;

        const char *colour = colour_get(pallete, get_colour(message.severity));
        const char *reset = colour_reset(pallete);

        std::string_view header = format_log(m_buffer, message, colour, reset);

        // ranges is impossible to use without going through a bunch of hoops
        // just iterate over the message split by newlines

        auto it = message.message.begin();
        auto end = message.message.end();

        while (it != end) {
            auto next = std::find(it, end, '\n');
            auto line = std::string_view{&*it, static_cast<size_t>(std::distance(it, next))};
            it = next;

            fmt::println("{} {}", header, line);

            if (it != end) {
                ++it;
            }
        }

        m_buffer.clear();
    }

public:
    constexpr ConsoleLog(IArena &arena, logs::Severity severity)
        : ILogger(severity)
        , m_buffer(arena) {}
};

class BroadcastLog final : public logs::ILogger {
    sm::Vector<logs::ILogger *> m_loggers;

    void accept(const logs::Message &message) override {
        for (ILogger *logger : m_loggers) {
            logger->log(message);
        }
    }

public:
    constexpr BroadcastLog(logs::Severity severity)
        : ILogger(severity) {}

    void add_logger(logs::ILogger *logger) {
        m_loggers.push_back(logger);
    }
};

class DefaultArena final : public IArena {
    using IArena::IArena;

    void *impl_alloc(size_t size) override {
        return std::malloc(size);
    }

    void *impl_resize(void *ptr, size_t new_size, size_t old_size) override {
        CT_UNUSED(old_size);

        return std::realloc(ptr, new_size);
    }

    void impl_release(void *ptr, size_t size) override {
        CT_UNUSED(size);

        std::free(ptr);
    }
};

class TraceArena final : public IArena {
    logs::Sink<logs::Category::eDebug> m_log;
    IArena &m_source;

    void *impl_alloc(size_t size) override {
        void *ptr = m_source.alloc(size);
        m_log.trace("[{}] alloc({:#x}) = {}\n", name, size, ptr);
        return ptr;
    }

    void *impl_resize(void *ptr, size_t new_size, size_t old_size) override {
        void *new_ptr = m_source.resize(ptr, new_size, old_size);
        m_log.trace("[{}] resize({}, {:#x}, {}) = {}\n", name, ptr, new_size, old_size, new_ptr);
        return new_ptr;
    }

    void impl_release(void *ptr, size_t size) override {
        m_source.release(ptr, size);
        m_log.trace("[{}] release({}, {:#x})\n", name, ptr, size);
    }

    void impl_rename(const void *ptr, const char *ptr_name) override {
        m_log.trace("[{}] rename({}, {})\n", name, ptr, ptr_name);
    }

    void impl_reparent(const void *ptr, const void *parent) override {
        m_log.trace("[{}] reparent({}, {})\n", name, ptr, parent);
    }

public:
    TraceArena(const char *name, IArena &source, logs::ILogger &logger)
        : IArena(name)
        , m_log(logger)
        , m_source(source) {}
};

static print_backtrace_t print_options_make(arena_t *arena, io_t *io) {
    print_backtrace_t print = {
        .options = {.arena = arena, .io = io, .pallete = &kColourDefault},
        .heading_style = eHeadingGeneric,
        .zero_indexed_lines = false,
        .project_source_path = SMC_SOURCE_DIR,
    };

    return print;
}

class DefaultSystemError final : public ISystemError {
    IArena &m_arena;
    bt_report_t *m_report = nullptr;

    void error_begin(os_error_t error) override {
        m_report = bt_report_new(&m_arena);
        io_t *io = io_stderr();
        io_printf(io, "System error detected: (%s)\n", os_error_string(error, &m_arena));
    }

    void error_frame(const bt_frame_t *it) override {
        bt_report_add(m_report, it);
    }

    void error_end() override {
        const print_backtrace_t kPrintOptions = print_options_make(&m_arena, io_stderr());
        print_backtrace(kPrintOptions, m_report);
        std::exit(CT_EXIT_INTERNAL); // NOLINT
    }

public:
    constexpr DefaultSystemError(IArena &arena)
        : m_arena(arena) {}
};

class DefaultWindowEvents final : public sys::IWindowEvents {
    sys::FileMapping &m_store;

    sys::WindowPlacement *m_placement = nullptr;
    sys::RecordLookup m_lookup;

    render::Context *m_context = nullptr;
    sys::DesktopInput *m_input = nullptr;

    LRESULT event(sys::Window &window, UINT message, WPARAM wparam, LPARAM lparam) override {
        if (m_input) m_input->window_event(message, wparam, lparam);
        return ImGui_ImplWin32_WndProcHandler(window.get_handle(), message, wparam, lparam);
    }

    void resize(sys::Window &, math::int2 size) override {
        if (m_context != nullptr) {
            m_context->resize((uint)size.width, (uint)size.height);
        }
    }

    void create(sys::Window &window) override {
        if (m_lookup = m_store.get_record(&m_placement); m_lookup == sys::RecordLookup::eOpened) {
            window.set_placement(*m_placement);
        } else {
            window.center_window(sys::MultiMonitor::ePrimary);
        }
    }

    bool close(sys::Window &window) override {
        if (m_lookup.has_valid_data()) *m_placement = window.get_placement();
        return true;
    }

public:
    DefaultWindowEvents(sys::FileMapping &store)
        : m_store(store)
    { }

    void attach_render(render::Context *context) {
        m_context = context;
    }

    void attach_input(sys::DesktopInput *input) {
        m_input = input;
    }
};

constinit static DefaultArena gGlobalArena{"default"};
constinit static DefaultSystemError gDefaultError{gGlobalArena};
static constinit ConsoleLog gConsoleLog{gGlobalArena, logs::Severity::eInfo};

struct System {
    System(HINSTANCE hInstance) {
        sys::create(hInstance, gConsoleLog);
    }
    ~System() {
        sys::destroy();
    }
};

static std::atomic_bool gShouldExit = false;

#if 0
static int ctrlc_handler(DWORD signal) {
    gShouldExit = true;
    return TRUE;
}
#endif

static void common_init(void) {
    bt_init();
    os_init();

    gSystemError = gDefaultError;

    gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
        io_t *io = io_stderr();

        const print_backtrace_t kPrintOptions = print_options_make(&gGlobalArena, io);

        auto message = sm::vformat(msg, args);

        gConsoleLog.log(logs::Category::eGlobal, logs::Severity::ePanic, message.data());

        bt_report_t *report = bt_report_collect(&gGlobalArena);
        print_backtrace(kPrintOptions, report);

        std::exit(CT_EXIT_INTERNAL); // NOLINT
    };

    //SetConsoleCtrlHandler(ctrlc_handler, TRUE);
}

class ImGuiRenderPass {
    render::DescriptorIndex mSrvIndex = render::DescriptorIndex::eInvalid;

public:
    ImGuiRenderPass() = default;

    void create(HWND hwnd, const render::RenderConfig& config, render::Context& context) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        auto& heap = context.srv_heap();
        mSrvIndex = heap.acquire();

        ImGui_ImplWin32_Init(hwnd);
        ImGui_ImplDX12_Init(*context.device(), config.frame_count, DXGI_FORMAT_R8G8B8A8_UNORM, heap.get(), heap.cpu(mSrvIndex), heap.gpu(mSrvIndex));
    }

    void destroy(render::Context& context) {
        auto& heap = context.srv_heap();
        heap.release(mSrvIndex);

        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void begin_frame() {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void end_frame() {
        ImGui::Render();
    }

    void render(render::Context& context, CommandList& commands) {
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commands.get());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault(NULL, commands.get());
        }
    }
};

struct RenderObject {
    // TODO: lifetime management
    DeviceResource vbo_upload;
    DeviceResource ibo_upload;

    DeviceResource vbo_data;
    DeviceResource ibo_data;

    VertexBufferView vbo;
    IndexBufferView ibo;
    uint32 length;
};

static sm::Vector<uint8> load_shader_bytecode(const char *path) {
    GlobalSink general{gConsoleLog};
    auto file = Io::file(path, eAccessRead);
    if (auto err = file.error()) {
        general.error("failed to open {}: {}", file.name(), err);
        return {};
    }

    sm::Vector<uint8> data;
    auto size = file.size();
    data.resize(size);
    if (file.read_bytes(data.data(), size) != size) {
        general.error("failed to read {} bytes from {}: {}", size, file.name(), file.error());
        return {};
    }

    return data;
}

class SceneRenderPass {
    draw::Scene mScene;
    sm::Vector<RenderObject> mResources;

    draw::Camera mCamera;

    RootSignature mRootSignature;
    PipelineState mPipelineState;

    D3D12_VIEWPORT mViewport;
    D3D12_RECT mScissorRect;

    constexpr static RootSignatureFlags kFlags =
        RootSignatureFlags::eAllowInputAssemblerInputLayout |
        RootSignatureFlags::eDenyAmplificationShaderRootAccess |
        RootSignatureFlags::eDenyMeshShaderRootAccess |
        RootSignatureFlags::eDenyDomainShaderRootAccess |
        RootSignatureFlags::eDenyHullShaderRootAccess |
        RootSignatureFlags::eDenyGeometryShaderRootAccess;

    constexpr static InputElement kVertexLayout[] = {
        { "POSITION", DataFormat::eR32G32B32_FLOAT, offsetof(draw::Vertex, position) },
        { "TEXCOORD", DataFormat::eR32G32_FLOAT, offsetof(draw::Vertex, uv) },
    };

    constexpr static RootParameter kParams[] = {
        RootParameter::consts(0, 0, 16, ShaderVisibility::eVertex)
    };

    constexpr static render::RootSignatureConfig kSignatureInfo = {
        .flags = kFlags,
        .params = kParams,
    };

    static const ImGuiTableFlags kSceneTreeTableFlags
        = ImGuiTableFlags_BordersV
        | ImGuiTableFlags_BordersOuterH
        | ImGuiTableFlags_Resizable
        | ImGuiTableFlags_RowBg
        | ImGuiTableFlags_NoHostExtendX
        | ImGuiTableFlags_NoBordersInBody;

    static const ImGuiTreeNodeFlags kGroupNodeFlags
        = ImGuiTreeNodeFlags_SpanAllColumns
        | ImGuiTreeNodeFlags_AllowOverlap;

    static const ImGuiTreeNodeFlags kLeafNodeFlags
        = kGroupNodeFlags
        | ImGuiTreeNodeFlags_Leaf
        | ImGuiTreeNodeFlags_Bullet
        | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    void draw_group_node(const draw::RenderNode& node) {
        bool is_open = ImGui::TreeNodeEx((void*)&node, kGroupNodeFlags, "node (%zu children, %zu meshes)", node.children.size(), node.meshes.size());

        ImGui::TableNextColumn();

        ImGui::TableNextColumn();
        ImGui::Text("%s", "node");

        bool visible = true; // TODO: get from node
        ImGui::Checkbox("##visible", &visible);

        if (is_open) {
            for (uint16_t child : node.children) {
                draw_item(child);
            }

            ImGui::TreePop();
        }
    }

    void draw_leaf_node(const draw::RenderNode& node) {
        ImGui::TreeNodeEx((void*)&node, kLeafNodeFlags, "node (%zu meshes)", node.meshes.size());
        bool visible = true; // TODO: get from node

        ImGui::TableNextColumn();
        ImGui::Text("node (%zu meshes)", node.meshes.size());

        ImGui::TableNextColumn();
        ImGui::Checkbox("##visible", &visible);
    }

    void draw_item(uint16_t index) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();

        auto& node = mScene.nodes[index];
        if (node.children.size() > 0) {
            draw_group_node(node);
        } else {
            draw_leaf_node(node);
        }
    }

    void imgui_draw() {
        if (ImGui::Begin("Scene")) {
            if (ImGui::BeginTable("Scene", 3, kSceneTreeTableFlags)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Type");
                ImGui::TableSetupColumn("Visible");
                ImGui::TableHeadersRow();

                draw_item(mScene.root);

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void render_node(uint16_t index, CommandList& commands, const float4x4& transform) const {
        const auto& node = mScene.nodes[index];

        static float3 position = {3.f, 3.f, 3.f};
        static float3 focus = {0.1f, 0.1f, 0.1f};
        static float fov = math::to_radians(90.f);

        auto view = float4x4::lookAtRH(position, focus, float3(0.f, 0.f, 1.f));
        auto proj = float4x4::perspectiveRH(fov, 800.f / 600.f, 0.1f, 100.f);
        float4x4 mvp = (transform * view * proj).transpose();

        auto *cmd = commands.get();
        cmd->SetGraphicsRoot32BitConstants(0, 16, &mvp, 0);

        for (uint16_t mesh_id : node.meshes) {
            //const auto& mesh = mScene.meshes[mesh_id];
            const auto& resource = mResources[mesh_id];

            commands.set_index_buffer(resource.ibo);
            commands.set_vertex_buffer(resource.vbo);

            cmd->DrawIndexedInstanced(resource.length, 1, 0, 0, 0);
        }
    }

public:
    SceneRenderPass(draw::Scene scene)
        : mScene(std::move(scene))
    { }

    void create(const render::RenderConfig& config, render::Context& context) {
        // TODO: get bundles working already
        auto ps = load_shader_bytecode("build/bundle/shaders/primitive.ps.cso");
        auto vs = load_shader_bytecode("build/bundle/shaders/primitive.vs.cso");

        const render::PipelineConfig kPipelineInfo = {
            .input = kVertexLayout,
            .vs = {vs},
            .ps = {ps},
        };

        mRootSignature.create(context, kSignatureInfo);
        mPipelineState.create(context, mRootSignature, kPipelineInfo);

        sm::Vector<Barrier> barriers;

        auto& copy = context.acquire_copy_list("scene upload");
        auto& direct = context.acquire_direct_list("scene upload");

        for (auto& info : mScene.meshes) {
            auto [bounds, vertices, indices] = draw::primitive(info);
            uint vbo_size = uint(vertices.size() * sizeof(draw::Vertex));
            uint ibo_size = uint(indices.size() * sizeof(uint16_t));

            auto vbo_upload = context.create_staging_buffer(vbo_size);
            auto ibo_upload = context.create_staging_buffer(ibo_size);

            vbo_upload.write(vertices.data(), vbo_size);
            ibo_upload.write(indices.data(), ibo_size);

            auto vbo = context.create_buffer(vbo_size);
            auto ibo = context.create_buffer(ibo_size);

            copy.copy_buffer(vbo_upload, vbo, vbo_size);
            copy.copy_buffer(ibo_upload, ibo, ibo_size);

            barriers.push_back(transition_barrier(vbo, ResourceState::eCopyTarget, ResourceState::eVertexBuffer));
            barriers.push_back(transition_barrier(ibo, ResourceState::eCopyTarget, ResourceState::eIndexBuffer));

            auto vbo_view = render::vbo_view(vbo, sizeof(draw::Vertex), vbo_size);
            auto ibo_view = render::ibo_view(ibo, ibo_size, DataFormat::eR16_UINT);

            mResources.push_back({
                .vbo_upload = std::move(vbo_upload),
                .ibo_upload = std::move(ibo_upload),
                .vbo_data = std::move(vbo),
                .ibo_data = std::move(ibo),
                .vbo = vbo_view,
                .ibo = ibo_view,
                .length = uint32(indices.size()),
            });
        }

        context.submit_copy_list(copy);
        context.submit_direct_list(direct);

        auto size = config.window.get_client_coords().size();

        mViewport = {
            .TopLeftX = 0.f,
            .TopLeftY = 0.f,
            .Width = float(size.width),
            .Height = float(size.height),
            .MinDepth = 0.f,
            .MaxDepth = 1.f,
        };

        mScissorRect = {
            .left = 0,
            .top = 0,
            .right = LONG(size.width),
            .bottom = LONG(size.height),
        };
    }

    void render(render::Context& context, CommandList& commands) {
        commands.bind_signature(mRootSignature);
        commands.bind_pipeline(mPipelineState);

        auto *cmd = commands.get();
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        cmd->RSSetViewports(1, &mViewport);
        cmd->RSSetScissorRects(1, &mScissorRect);

        render_node(mScene.root, commands, float4x4::identity());

        //imgui_draw();
    }
};

static void message_loop(sys::ShowWindow show, sys::FileMapping &store) {
    sys::WindowConfig window_config = {
        .mode = sys::WindowMode::eWindowed,
        .width = 1280,
        .height = 720,
        .title = "Priority Zero",
        .logger = gConsoleLog,
    };

    DefaultWindowEvents events{store};

    sys::Window window{window_config, &events};
    sys::DesktopInput desktop_input{window};

    input::InputService input;
    input.add_source(&desktop_input);

    window.show_window(show);

    render::DebugFlags flags = render::DebugFlags::mask();
    flags.clear(render::DebugFlags::eWarpAdapter);

    render::RenderConfig config = {
        .window = window,
        .logger = gConsoleLog,
        .adapter_index = 0,
        .adapter_preference = render::AdapterPreference::eMinimumPower,
        .feature_level = render::FeatureLevel::eLevel_11_0,
        .debug_flags = flags,

        .frame_count = 2,
        .swapchain_format = render::DataFormat::eR8G8B8A8_UNORM,

        .direct_command_pool_size = 4,
        .copy_command_pool_size = 4,
        .compute_command_pool_size = 4,
        .resource_pool_size = 256,

        .rtv_heap_size = 4,
        .dsv_heap_size = 4,
        .srv_heap_size = 16,
    };

    draw::Scene draw;
    draw.root = 0;
    draw.nodes.push_back({
        .transform = {
            .scale = 1.f,
        },
        .meshes = {0}
    });
    draw.meshes.push_back({
        .type = draw::MeshType::eCube,
        .cube = {
            .width = 1.f,
            .height = 1.f,
            .depth = 1.f,
        }
    });

    render::Context context{config};
    events.attach_render(&context);

    {
        //ImGuiRenderPass imgui;
        SceneRenderPass scene{draw};

        //imgui.create(window.get_handle(), config, context);
        scene.create(config, context);

        context.flush_copy_queue();
        context.wait_for_gpu();

        bool done = false;
        while (!done) {
            if (gShouldExit) PostQuitMessage(0);

            // more complex message loop to avoid imgui
            // destroying the main window, then attempting to access it
            // in RenderPlatformWindowsDefault.
            // fun thing to debug
            MSG msg = {};
            while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
                if (msg.message == WM_QUIT) {
                    done = true;
                }
            }

            if (done) break;
            input.poll();

            // dear imgui rendering

            //imgui.begin_frame();

            //ImGui::ShowDemoWindow();

            // actual rendering

            context.begin_frame();

            auto& commands = context.current_frame_list();
            scene.render(context, commands);

            //imgui.end_frame();
            //imgui.render(context, commands);

            context.end_frame();
        }

        context.wait_for_gpu();

        //imgui.destroy(context);
    }
}

static int common_main(sys::ShowWindow show) {
    GlobalSink general{gConsoleLog};
    general.info("SMC_DEBUG = {}", SMC_DEBUG);
    general.info("CTU_DEBUG = {}", CTU_DEBUG);

    TraceArena ft_arena{"freetype", gGlobalArena, gConsoleLog};
    service::init_freetype(&ft_arena, &gConsoleLog);

    sys::MappingConfig store_config = {
        .path = "client.bin",
        .size = {1, Memory::eMegabytes},
        .record_count = 256,
        .logger = gConsoleLog,
    };

    sys::FileMapping store{store_config};

    threads::CpuGeometry geometry = threads::global_cpu_geometry(gConsoleLog);

    threads::SchedulerConfig thread_config = {
        .worker_count = 8,
        .process_priority = threads::PriorityClass::eNormal,
    };
    threads::Scheduler scheduler{thread_config, geometry, gConsoleLog};

    if (!store.is_valid()) {
        store.reset();
    }

    message_loop(show, store);

    service::deinit_freetype();
    return 0;
}

int main(int argc, const char **argv) {
    GlobalSink general{gConsoleLog};
    common_init();

    FormatBuffer args{gGlobalArena};
    auto it = std::back_inserter(args);
    fmt::format_to(it, "args[{}] = {{", argc);
    for (int i = 0; i < argc; ++i) {
        if (i != 0) fmt::format_to(it, ", ");
        fmt::format_to(it, "[{}] = \"{}\"", i, argv[i]);
    }
    fmt::format_to(it, "}}\0");

    general.info("{}", std::string_view{args.data(), args.size()});

    System sys{GetModuleHandleA(nullptr)};

    return common_main(sys::ShowWindow::eShow);
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    GlobalSink general{gConsoleLog};
    common_init();

    general.info("lpCmdLine = {}", lpCmdLine);
    general.info("nShowCmd = {}", nShowCmd);

    System sys{hInstance};

    return common_main(sys::ShowWindow{nShowCmd});
}
