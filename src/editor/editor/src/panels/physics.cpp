#include "editor/panels/physics.hpp"

using namespace sm;
using namespace sm::ed;
using namespace sm::math;

static const math::quatf kUpQuat = math::quatf::from_axis_angle(world::kVectorForward, 90._deg);

static constexpr ImGuiTableFlags kFlags
    = ImGuiTableFlags_Resizable
    | ImGuiTableFlags_Hideable
    | ImGuiTableFlags_RowBg
    | ImGuiTableFlags_ScrollY;

static void drawContent(PhysicsDebug& self) {
    game::Context ctx = game::getContext();
    world::World& world = ctx.getWorld();

    float3 gravity = ctx.getGravity();
    ImGui::Text("Gravity: %f, %f, %f", gravity.x, gravity.y, gravity.z);

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
        node.transform.rotation = body.body.getRotation() * kUpQuat;
    }
}
