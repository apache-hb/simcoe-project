#include "editor/panels/physics.hpp"

#include "editor/draw.hpp"
#include "game/entity.hpp"

using namespace sm;
using namespace sm::ed;
using namespace sm::math;

static constexpr ImGuiTableFlags kFlags
    = ImGuiTableFlags_Resizable
    | ImGuiTableFlags_Hideable
    | ImGuiTableFlags_RowBg
    | ImGuiTableFlags_ScrollY;

static bool nodePickCombo(world::World& world, world::IndexOf<world::Node>& node) {
    bool changed = false;
    if (ImGui::BeginCombo("Node", world.get(node).name.c_str())) {
        for (world::IndexOf i : world.indices<world::Node>()) {
            const world::Node& n = world.get(i);
            if (n.models.empty()) continue;

            if (ImGui::Selectable(n.name.c_str())) {
                node = i;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}

static void drawContent(PhysicsDebug& self) {
    game::Context ctx = game::getContext();
    world::World& world = ctx.getWorld();

    float3 gravity = ctx.getGravity();
    ImGui::Text("Gravity: %f, %f, %f", gravity.x, gravity.y, gravity.z);

    if (ImGui::Button("Debug Draw All")) {
        for (auto& body : self.bodies) {
            body.debugDraw = true;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Debug Draw None")) {
        for (auto& body : self.bodies) {
            body.debugDraw = false;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Add Physics Body")) {
        ImGui::OpenPopup("Add Physics Body");
    }

    if (ImGui::BeginPopup("Add Physics Body")) {
        nodePickCombo(world, self.pickedNode);
        ImGui::Checkbox("Dynamic Object", &self.dynamicObject);
        ImGui::Checkbox("Activate On Create", &self.activateOnCreate);

        if (ImGui::Button("Accept")) {
            self.createPhysicsBody(self.pickedNode, false, self.dynamicObject);

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginTable("Objects", 4, kFlags)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Linear Velocity", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Center of Mass", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Debug Draw", ImGuiTableColumnFlags_WidthFixed);

        ImGui::TableHeadersRow();

        for (auto& body : self.bodies) {
            ImGui::PushID(&body);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(world.get(body.node).name.c_str());
            ImGui::TableNextColumn();
            auto v = body.body.getLinearVelocity();
            ImGui::Text("%f, %f, %f", v.x, v.y, v.z);
            ImGui::TableNextColumn();
            auto com = body.body.getCenterOfMass();
            ImGui::Text("%f, %f, %f", com.x, com.y, com.z);
            ImGui::TableNextColumn();
            ImGui::Checkbox("##DebugDraw", &body.debugDraw);

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

void PhysicsDebug::draw_window() {
    game::Context ctx = game::getContext();
    for (auto& body : bodies) {
        if (body.debugDraw) {
            ctx.debugDrawPhysicsBody(body.body);
        }
    }

    if (!isOpen) return;

    if (ImGui::Begin("Physics World", &isOpen)) {
        drawContent(*this);
    }
    ImGui::End();
}

void PhysicsDebug::update() {
    game::Context ctx = game::getContext();
    world::World& world = ctx.getWorld();

    for (auto& body : bodies) {
        auto& node = world.get(body.node);
        node.transform.position = body.body.getCenterOfMass();
        node.transform.rotation = body.body.getRotation();
    }
}

void PhysicsDebug::addPhysicsBody(world::IndexOf<world::Node> node, game::PhysicsBody&& body) {
    bodies.emplace_back(PhysicsObjectData { node, std::move(body) });
}

void PhysicsDebug::createPhysicsBody(world::IndexOf<world::Node> index, bool sphere, bool dynamic) {
    game::Context ctx = game::getContext();
    world::World& world = ctx.getWorld();

    const auto& node = world.get(index);

    // get node bounds
    const auto& bounds = context.mMeshes.at(node.models.front()).bounds;
    world::Transform transform = world::computeNodeTransform(world, index);

    if (sphere) {
        auto r = bounds.getExtents().length() / 2.0f;
        world::Sphere sphere = { r, 5, 5 };
        game::PhysicsBody body = ctx.addPhysicsBody(sphere, transform.position + bounds.getCenter(), quatf::identity(), dynamic);

        addPhysicsBody(index, std::move(body));
    }
    else {
        auto [w, h, d] = bounds.getExtents();
        world::Cube cube = { w, h, d };
        game::PhysicsBody body = ctx.addPhysicsBody(cube, transform.position + bounds.getCenter(), quatf::identity(), dynamic);

        addPhysicsBody(index, std::move(body));
    }
}
