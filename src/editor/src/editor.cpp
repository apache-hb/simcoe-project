#include "stdafx.hpp"

#include "editor/editor.hpp"
#include "editor/mygui.hpp"

using namespace sm;
using namespace sm::ed;

Editor::Editor(ed::EditorContext& context)
    : mContext(context)
    , mLogger(LoggerPanel::get())
    , mConfig(context)
    , mScene(context)
    , mInspector(context)
    , mFeatureSupport(context)
    , mAssetBrowser(context)
    , mGraph(context)
    , mPix(context)
{
    mOpenLevelDialog.SetTitle("Open Level");
    mSaveLevelDialog.SetTitle("Save Level");
    mSaveLevelDialog.SetTypeFilters({ ".bin" });
    mSaveLevelDialog.SetInputName("level.bin");

    for (size_t i = 0; i < mContext.cameras.size(); i++) {
        mViewports.emplace_back(mContext, i);
    }
}

void Editor::draw_mainmenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New Level");
            ImGui::Separator();

            if (ImGui::MenuItem("Open")) {
                mOpenLevelDialog.Open();
            }
            ImGui::Separator();

            // TODO: implement save and save as
            if (ImGui::MenuItem("Save")) {
                mSaveLevelDialog.Open();
            }

            if (ImGui::MenuItem("Save As")) {
                mSaveLevelDialog.Open();
            }
            ImGui::Separator();

            if (ImGui::MenuItem("Import")) {
                mImportDialog.Open();
            }

            ImGui::MenuItem("Exit");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Logger", nullptr, &mLogger.mOpen);
            ImGui::MenuItem("Scene Tree", nullptr, &mScene.mOpen);
            ImGui::MenuItem("Inspector", nullptr, &mInspector.mOpen);
            ImGui::MenuItem("Asset Browser", nullptr, &mAssetBrowser.mOpen);

            ImGui::SeparatorText("Debugging");
            ImGui::MenuItem("Debug", nullptr, &mDebug.mOpen);
            mPix.draw_menu_item(nullptr);
            ImGui::MenuItem("Feature Support", nullptr, &mFeatureSupport.mOpen);

            ImGui::Separator();
            if (ImGui::BeginMenu("Theme")) {
                if (ImGui::MenuItem("Dark", nullptr, mTheme == Theme::eDark)) {
                    ImGui::StyleColorsDark();
                    ImPlot::StyleColorsDark();
                    mTheme = Theme::eDark;
                }

                if (ImGui::MenuItem("Light", nullptr, mTheme == Theme::eLight)) {
                    ImGui::StyleColorsLight();
                    ImPlot::StyleColorsLight();
                    mTheme = Theme::eLight;
                }

                if (ImGui::MenuItem("Classic", nullptr, mTheme == Theme::eClassic)) {
                    ImGui::StyleColorsClassic();
                    ImPlot::StyleColorsClassic();
                    mTheme = Theme::eClassic;
                }
                ImGui::EndMenu();
            }

            ImGui::SeparatorText("Viewports");
            for (ViewportPanel &viewport : mViewports) {
                ImGui::MenuItem(viewport.get_title(), nullptr, &viewport.mOpen);
            }
            if (ImGui::SmallButton("Add Viewport")) {
                size_t index = mContext.add_camera();
                mViewports.emplace_back(mContext, index);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("ImGui Demo", nullptr, &mShowImGuiDemo);
            ImGui::MenuItem("ImPlot Demo", nullptr, &mShowImPlotDemo);

            ImGui::Separator();

            ImGui::MenuItem("Settings", nullptr, &mConfig.mOpen);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

static constexpr ImGuiWindowFlags kDockFlags
    = ImGuiWindowFlags_NoDocking
    | ImGuiWindowFlags_NoTitleBar
    | ImGuiWindowFlags_NoCollapse
    | ImGuiWindowFlags_NoResize
    | ImGuiWindowFlags_NoMove
    | ImGuiWindowFlags_NoBringToFrontOnFocus
    | ImGuiWindowFlags_NoNavFocus;

void Editor::draw_dockspace() {
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (ImGui::Begin("Editor", nullptr, kDockFlags)) {
        ImGui::DockSpace(ImGui::GetID("EditorDock"), ImVec2(0.f, 0.f), ImGuiDockNodeFlags_None);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void Editor::begin_frame() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    // TODO: get this from the current camera
    ImGuizmo::SetOrthographic(false);
}

void Editor::end_frame() {
    ImGui::Render();

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

world::IndexOf<world::Node> Editor::get_best_node() {
    if (!mContext.selected.has_value())
        return mContext.get_scene().root;

    auto index = world::get<world::Node>(*mContext.selected);
    if (index == world::kInvalidIndex)
        return mContext.get_scene().root;

    return index;
}

void Editor::draw_create_popup() {
    if (ImGui::IsKeyChordPressed(ImGuiMod_Shift | ImGuiKey_A)) {
        ImGui::OpenPopup("Create");
    }

    if (MyGui::BeginPopupWindow("Create", ImGuiWindowFlags_NoMove)) {
        auto& world = mContext.mWorld;
        if (ImGui::BeginMenu("Object")) {
            if (ImGui::MenuItem("Empty")) {
                auto root = get_best_node();

                auto added = world.add(world::Node { .name = "New Empty", .transform = world::default_transform() });
                world.get(root).children.push_back(added);

                mContext.upload([this, added] {
                    mContext.create_node(added);
                });
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Primitive")) {
            if (ImGui::MenuItem("Cube")) {
                mContext.upload([this, &world] {
                    auto root = get_best_node();
                    auto added = world.add(world::Model { .name = "Cube", .mesh = world::Cube { 1.f, 1.f, 1.f }, .material = world.default_material });
                    world.get(root).models.push_back(added);
                    mContext.create_model(added);
                });
            }

            if (ImGui::MenuItem("Sphere")) {
                mContext.upload([this, &world] {
                    auto root = get_best_node();
                    auto added = world.add(world::Model { .name = "Sphere", .mesh = world::Sphere { 1.f }, .material = world.default_material });
                    world.get(root).models.push_back(added);
                    mContext.create_model(added);
                });
            }

            if (ImGui::MenuItem("Cylinder")) {
                mContext.upload([this, &world] {
                    auto root = get_best_node();
                    auto added = world.add(world::Model { .name = "Cylinder", .mesh = world::Cylinder { 1.f, 1.f }, .material = world.default_material });
                    world.get(root).models.push_back(added);
                    mContext.create_model(added);
                });
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Light")) {
            if (ImGui::MenuItem("Point Light")) {
                auto root = get_best_node();

                auto added = world.add(world::Light {
                    .name = "Point Light",
                    .light = world::PointLight {
                        .colour = 1.f,
                        .intensity = 1.f
                    }
                });
                world.get(root).lights.push_back(added);
            }

            if (ImGui::MenuItem("Spot Light")) {
                auto root = get_best_node();

                auto added = world.add(world::Light {
                    .name = "Spot Light",
                    .light = world::SpotLight {
                        .direction = math::radf3(0.f),
                        .colour = 1.f,
                        .intensity = 1.f
                    }
                });
                world.get(root).lights.push_back(added);
            }

            if (ImGui::MenuItem("Directional Light")) {
                auto root = get_best_node();

                auto added = world.add(world::Light {
                    .name = "Directional Light",
                    .light = world::DirectionalLight {
                        .direction = math::radf3(0.f),
                        .colour = 1.f,
                        .intensity = 1.f
                    }
                });
                world.get(root).lights.push_back(added);
            }

            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
}

void Editor::import_dds(const fs::path& path) {
    DirectX::ScratchImage image;
    render::Result hr = DirectX::LoadFromDDSFile(path.c_str(), DirectX::DDS_FLAGS_FORCE_RGB, nullptr, image);

    if (hr.failed()) {
        alert_error(fmt::format("Failed to load DDS image {}", path.string()));
        return;
    }

    size_t len = image.GetPixelsSize();

    world::Buffer buffer = {
        .name = path.filename().string(),
        .data = std::vector<uint8>(image.GetPixels(), image.GetPixels() + len),
    };

    auto idx = mContext.mWorld.add(std::move(buffer));

    world::BufferView view = {
        .source = idx,
        .offset = 0,
        .source_size = uint32(len),
    };

    auto metadata = image.GetMetadata();

    world::Image info = {
        .name = path.filename().string(),
        .source = view,
        .format = metadata.format,
        .size = { uint(metadata.width), uint(metadata.height) },
        .mips = uint(metadata.mipLevels),
    };

    auto img = mContext.mWorld.add(std::move(info));

    mContext.upload([this, img] {
        mContext.load_image(img);
    });
}

void Editor::import_file(const fs::path& path) {
    auto ext = path.extension();
    if (ext == ".dds") {
        import_dds(path);
        return;
    }

    if (!(ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")) {
        alert_error(fmt::format("Unsupported file format {}", ext.string()));
        return;
    }

    ImageData image = sm::open_image(path);
    if (!image.is_valid()) {
        alert_error(fmt::format("Failed to load image {}", path.string()));
        return;
    }

    size_t len = image.data.size();

    world::Buffer buffer = {
        .name = path.filename().string(),
        .data = std::move(image.data),
    };

    auto idx = mContext.mWorld.add(std::move(buffer));

    world::BufferView view = {
        .source = idx,
        .offset = 0,
        .source_size = uint32(len),
    };

    world::Image info = {
        .name = path.filename().string(),
        .source = view,
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .size = image.size,
        .mips = 1,
    };

    auto img = mContext.mWorld.add(std::move(info));

    mContext.upload([this, img] {
        mContext.load_image(img);
    });
}

void Editor::alert_error(std::string message) {
    mErrorMessage = std::move(message);
    ImGui::OpenPopup("Error");
}

void Editor::draw_error_modal() {
    if (mErrorMessage.empty())
        return;

    if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", mErrorMessage.c_str());
        if (ImGui::Button("OK")) {
            mErrorMessage.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Editor::draw() {
    draw_mainmenu();
    draw_dockspace();
    draw_create_popup();

    mLogger.draw_window();
    mScene.draw_window();
    mInspector.draw_window();
    mFeatureSupport.draw_window();
    mDebug.draw_window();
    mAssetBrowser.draw_window();
    mGraph.draw_window();
    mPix.draw_window();
    mConfig.draw_window();

    for (ViewportPanel &viewport : mViewports) {
        viewport.draw_window();
    }

    draw_error_modal();

    mOpenLevelDialog.Display();
    mSaveLevelDialog.Display();
    mImportDialog.Display();

    if (mShowImGuiDemo)
        ImGui::ShowDemoWindow(&mShowImGuiDemo);

    if (mShowImPlotDemo)
        ImPlot::ShowDemoWindow(&mShowImPlotDemo);

    if (mImportDialog.HasSelected()) {
        for (const auto &file : mImportDialog.GetMultiSelected()) {
            import_file(file);
        }
        mImportDialog.ClearSelected();
    }
}
