#pragma once

#include "json.hpp"
#include <fmtlib/format.h>
#include <stdint.h>

#include <map>
#include <string>
#include <vector>

typedef void CURL;

namespace sm::docker {
    class ContainerId {
        std::string mId;

    public:
        ContainerId() = default;
        explicit ContainerId(std::string id) : mId(std::move(id)) { }

        const std::string& getId() const { return mId; }
    };

    class Image {
        std::string mId;
        std::string mParentId;
        int mSize;
        int mSharedSize;
        int mVirtualSize;
        int mContainers;
        std::vector<std::string> tags;
        std::vector<std::string> repositories;
        std::map<std::string, std::string> labels;

    public:
        Image() = default;
        Image(const argo::json& json);

        const std::string& getId() const { return mId; }
        const std::string& getParentId() const { return mParentId; }
        int getSize() const { return mSize; }
        int getSharedSize() const { return mSharedSize; }
        int getVirtualSize() const { return mVirtualSize; }
        int getContainers() const { return mContainers; }
        const std::vector<std::string>& getTags() const { return tags; }
        const std::vector<std::string>& getRepositories() const { return repositories; }
        const std::map<std::string, std::string>& getLabels() const { return labels; }
    };

    enum class ContainerStatus {
        Running,
        Exited,
        Created,
        Paused,
        Dead,
        Unknown
    };

    struct MappedPort {
        uint16_t privatePort;
        uint16_t publicPort;
        std::string protocol;
    };

    class Container {
        std::string mImageId;
        ContainerStatus mState;
        std::string mId;
        std::vector<std::string> mNames;
        std::vector<MappedPort> mPorts;

    public:
        Container() = default;
        Container(const argo::json& json);

        uint16_t getMappedPort(uint16_t privatePort) const {
            for (const auto& port : mPorts) {
                if (port.privatePort == privatePort) {
                    return port.publicPort;
                }
            }

            return 0; // Not found
        }

        std::string getId() const { return mId; }
        const std::vector<std::string>& getNames() const { return mNames; }
        bool isNamed(const std::string& name) const {
            return std::find(mNames.begin(), mNames.end(), "/" + name) != mNames.end();
        }

        ContainerStatus getState() const { return mState; }

        void start();
        void stop();
        void restart();
    };

    struct ContainerCreateInfo {
        std::string name;
        std::string image;
        std::string tag;
        std::map<std::string, std::string> labels;
        std::vector<std::string> entrypoint;
        std::vector<std::string> commands;
        std::map<std::string, std::string> env;
        std::vector<MappedPort> ports;
    };

    class DockerClient {
        bool mLocal;
        std::string mHost;

        DockerClient(bool local, std::string host)
            : mLocal(local)
            , mHost(std::move(host))
        { }

        void get(std::string_view path, std::ostream& stream);
        std::string get(std::string_view path);

        void del(std::string_view path);

        void post(std::string_view path, const std::string& fields, const std::string& contentType, std::istream& input, std::ostream& output);

    public:
        DockerClient();

        static DockerClient connect(std::string host);
        static DockerClient local();

        std::vector<Container> listContainers();
        std::vector<Image> listImages();

        std::optional<Container> findContainer(const std::string& id);
        ContainerId createContainer(const ContainerCreateInfo& createInfo);
        void destroyContainer(const std::string& id);
        Container getContainer(const ContainerId& id);

        void importImage(std::string_view name, std::istream& stream);
        void exportImage(std::string_view name, std::string_view tag, std::ostream& stream);

        void pullImage(std::string_view name, std::string_view tag);
    };

    void init();
    void cleanup();
}

template<>
struct fmt::formatter<sm::docker::Image> {
    constexpr auto parse(fmt::format_parse_context& ctx) const {
        return ctx.begin();
    }

    constexpr auto format(const sm::docker::Image& image, fmt::format_context& ctx) const {
        return format_to(ctx.out(), "DockerImage(Id: {}, ParentId: {}, Size: {}, SharedSize: {}, VirtualSize: {}, Containers: {})",
                         image.getId(), image.getParentId(), image.getSize(), image.getSharedSize(), image.getVirtualSize(), image.getContainers());
    }
};
