#include "render/graph.hpp"

#include "core/map.hpp"
#include "core/stack.hpp"

using namespace sm;
using namespace sm::graph;

ResourceId FrameGraph::new_resource(sm::StringView name, ResourceState state, IResource *resource, ResourceType type) {
    auto id = mResources.size();
    NodeConfig config = { name, int_cast<uint>(id), mContext };
    mResources.emplace_back(config, 0, resource, type);
    return id;
}

void FrameGraph::compile() {
    for (auto& pass : mPasses) {
        pass.mRefCount = int_cast<uint>(pass.mWrites.size());
        for (auto& [id, _] : pass.mReads) {
            mResources[id].mRefCount += 1;
        }
        for (auto& [id, _] : pass.mWrites) {
            mResources[id].mProducer = &pass;
        }
    }

    sm::Stack<ResourceNode*> queue;
    for (auto& node : mResources) {
        if (node.mRefCount == 0) {
            queue.push(&node);
        }
    }

    while (!queue.empty()) {
        ResourceNode *node = queue.top();
        queue.pop();
        if (auto* producer = node->mProducer) {
            if (producer->has_side_effects()) continue;

            SM_ASSERTF(producer->mRefCount > 0, "Producer {} has no ref count", producer->mName);
            producer->mRefCount -= 1;
            if (producer->mRefCount != 0) continue;

            for (auto& [id, _] : producer->mReads) {
                auto& node = mResources[id];
                node.mRefCount -= 1;
                if (node.mRefCount == 0) {
                    queue.push(&node);
                }
            }
        }
    }

    for (auto& pass : mPasses) {
        if (pass.mRefCount == 0) continue;

        for (auto& [id, _] : pass.mCreates)
            mResources[id].mProducer = &pass;
        for (auto& [id, _] : pass.mWrites)
            mResources[id].mLastUser = &pass;
        for (auto& [id, _] : pass.mReads)
            mResources[id].mLastUser = &pass;
    }
}

void FrameGraph::execute(render::Context& context) {
    sm::Map<IResource*, ResourceState> states;
    sm::Vector<Barrier> barriers;

    for (auto& pass : mPasses) {
        if (!pass.has_side_effects() && pass.mRefCount == 0)
            continue;

        for (auto& [id, state] : pass.mCreates) {
            auto* resource = mResources[id].get_resource();
            states[resource] = state;

            resource->create(*this, context);
        }

        for (auto& [id, state] : pass.mReads) {
            auto& node = mResources[id];
            auto* resource = node.get_resource();
            if (node.mProducer == &pass) continue;

            auto current = states[resource];
            if (state == current) continue;

            barriers.push_back(resource->transition(current, state));

            states[resource] = state;
        }

        for (auto& [id, state] : pass.mWrites) {
            auto& node = mResources[id];
            auto* resource = node.get_resource();
            auto current = states[resource];
            if (state == current) continue;

            barriers.push_back(resource->transition(current, state));

            states[resource] = state;
        }

        // TODO: submit barriers to current command list
        pass.execute(*this, context);

        for (auto& node : mResources) {
            if (node.mLastUser != &pass) continue;
            if (node.is_imported()) continue;

            node.destroy(*this, context);
        }
    }
}
