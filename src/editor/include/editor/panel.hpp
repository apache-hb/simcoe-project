#pragma once

#include "core/string.hpp"

#include "world/world.hpp"

#include "imgui/imgui.h"

namespace sm::ed {
    static constexpr const char *kNodePayload = "NodeIndex";
    static constexpr const char *kMeshPayload = "MeshIndex";
    static constexpr const char *kCameraPayload = "CameraIndex";
    static constexpr const char *kMaterialPayload = "MaterialIndex";
    static constexpr const char *kTexturePayload = "TextureIndex";

    class IEditorPanel {
        bool mOpen = true;
        const char *mShortcut = nullptr;
        sm::String mTitle;

        virtual void draw_content() = 0;
    protected:
        ImGuiWindowFlags mFlags;

        void set_window_flags(ImGuiWindowFlags flags);
        ImGuiWindowFlags get_window_flags() const;

        void set_shortcut(const char *shortcut);
        const char *get_shortcut() const;

        void set_title(sm::StringView title);
        const char *get_title() const;

        void set_open(bool open);
        bool is_open() const;
        bool *get_open();

        IEditorPanel(sm::StringView title, ImGuiWindowFlags flags = ImGuiWindowFlags_None)
            : mTitle(title)
            , mFlags(flags)
        { }

    public:
        virtual ~IEditorPanel() = default;

        virtual bool draw_menu_item(const char *shortcut = nullptr);
        virtual bool draw_window();
        void draw_section();
    };

    void edit_transform(world::Transform& transform);
}
