#pragma once

#include "json.hpp"
#include <fmtlib/format.h>
#include <stdint.h>

#include <memory>
#include <map>
#include <string>
#include <vector>

typedef void CURL;

namespace sm::docker {
    class Container {

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

    class DockerClient {
        bool mLocal;
        std::string mHost;

        DockerClient(bool local, std::string host)
            : mLocal(local)
            , mHost(std::move(host))
        { }

        std::string get(std::string_view path);
        std::string post(std::string_view path, const std::string& data);

    public:
        DockerClient();

        static DockerClient connect(std::string host);
        static DockerClient local();

        std::vector<Container> listContainers();
        std::vector<Image> listImages();
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
