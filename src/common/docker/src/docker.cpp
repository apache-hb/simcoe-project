#include "docker/docker.hpp"
#include "docker/error.hpp"

#include "logs/logging.hpp"

#include <json.hpp>
#include <unparser.hpp>
#include <parser.hpp>
#include <curl/curl.h>

LOG_MESSAGE_CATEGORY(DockerLog, "Docker");

static constexpr const char *kDockerSocketPath = "/var/run/docker.sock";

using sm::docker::DockerClient;

[[maybe_unused]]
static size_t curlWriteCallback(char *ptr, size_t size, size_t nmemb, void *user) {
    std::ostream *data = static_cast<std::ostream *>(user);
    size_t totalSize = size * nmemb;
    data->write(ptr, totalSize);
    return totalSize;
}

[[maybe_unused]]
static size_t curlReadCallback(char *ptr, size_t size, size_t nmemb, void *user) {
    std::istream *data = static_cast<std::istream *>(user);
    data->read(ptr, size * nmemb);
    return data->gcount();
}

void sm::docker::init() {
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK) {
        throw sm::docker::DockerException("Failed to initialize CURL: {}", curl_easy_strerror(res));
    }
}

void sm::docker::cleanup() {
    curl_global_cleanup();
}

DockerClient DockerClient::connect(std::string host) {
    // Implementation of the connection logic to the Docker daemon
    // This is a placeholder; actual implementation would involve setting up a connection
    // to the Docker API using the provided host and port.
    return DockerClient(false, std::move(host));
}

DockerClient DockerClient::local() {
    return DockerClient(true, "http://localhost/v1.47");
}

void DockerClient::get(std::string_view path, std::ostream& stream) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw sm::docker::DockerException("Failed to initialize CURL");
    }

    std::string url = std::format("{}{}", mHost, path);
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");

    LOG_TRACE(DockerLog, "GET: {}", url);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.81.0");
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_FTP_SKIP_PASV_IP, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&stream);

    if (mLocal) {
        curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, kDockerSocketPath);
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        throw sm::docker::DockerException("GET {} CURL error: {}", url, curl_easy_strerror(res));
    }

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (status != 200) {
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        throw sm::docker::DockerException("Failed to GET {}: {}", url, status);
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
}

void DockerClient::post(std::string_view path, const std::string& fields, const std::string& contentType, std::istream& input, std::ostream& output) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw sm::docker::DockerException("Failed to initialize CURL");
    }

    std::string url = std::format("{}{}", mHost, path);

    LOG_TRACE(DockerLog, "POST: {}?{}", url, fields);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.81.0");
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)fields.size());
    curl_easy_setopt(curl, CURLOPT_FTP_SKIP_PASV_IP, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

    if (mLocal) {
        curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, kDockerSocketPath);
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw sm::docker::DockerException("POST {} CURL error: {}", url, curl_easy_strerror(res));
    }

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (status != 200) {
        curl_easy_cleanup(curl);
        throw sm::docker::DockerException("Failed to POST {}: {}", url, status);
    }

    curl_easy_cleanup(curl);
}

std::string DockerClient::get(std::string_view path) {
    std::stringstream response;
    get(path, response);
    return response.str();
}

std::vector<sm::docker::Container> DockerClient::listContainers() {
    std::string response = get("/containers/json");
    std::unique_ptr<argo::json> json = argo::parser::parse(response);
    LOG_INFO(DockerLog, "Received JSON response: {}", response);

    std::vector<Container> containers;
    auto& array = json->get_array();
    for (const auto& item : array) {
        containers.emplace_back(item);
    }
    return containers;
}

std::vector<sm::docker::Image> DockerClient::listImages() {
    std::string response = get("/images/json");
    std::unique_ptr<argo::json> json = argo::parser::parse(response);
    LOG_INFO(DockerLog, "Received JSON response: {}", response);

    std::vector<Image> images;
    auto& array = json->get_array();
    for (const auto& item : array) {
        images.emplace_back(item);
    }
    return images;
}

sm::docker::Container DockerClient::createContainer(const sm::docker::ContainerCreateInfo& createInfo) {
    argo::json requestJson;
    requestJson["Image"] = std::format("{}:{}", createInfo.image, createInfo.tag);

    if (!createInfo.env.empty()) {
        argo::json::json_array env{};
        for (const auto& envVar : createInfo.env) {
            env.push_back(std::format("{}={}", envVar.first, envVar.second));
        }
        requestJson["Env"] = env;
    }

    if (!createInfo.labels.empty()) {
        argo::json::json_object labels{};
        for (const auto& label : createInfo.labels) {
            labels[label.first] = label.second;
        }
        requestJson["Labels"] = labels;
    }

    std::stringstream requestStream;
    requestStream << requestJson;
    requestStream.seekg(0);
    std::stringstream responseStream;

    post("/containers/create", "name=" + createInfo.name, "Content-Type: application/json", requestStream, responseStream);

    std::unique_ptr<argo::json> responseJson = argo::parser::parse(responseStream.str());
    return Container(*responseJson);
}

void DockerClient::importImage(std::string_view name, std::istream& stream) {
    std::stringstream response;
    post("/images/create", "fromSrc=-", "Content-Type: application/tar", stream, response);
}

void DockerClient::exportImage(std::string_view name, std::string_view tag, std::ostream& stream) {
    auto path = std::format("/images/{}%3A{}/get", name, tag);
    get(path, stream);
}

void DockerClient::pullImage(std::string_view name, std::string_view tag) {
    auto params = std::format("fromImage={}&tag={}", name, tag);
    std::stringstream input;
    std::stringstream response;
    post("/images/create", params, "Content-Type: application/json", input, response);
}

sm::docker::Image::Image(const argo::json& json) {
    const auto& object = json.get_object();

    mId = (std::string)object.at("Id");
    mParentId = object.contains("ParentId") ? (std::string)object.at("ParentId") : "";
    mSize = object.contains("Size") ? (int)object.at("Size") : -1;
    mSharedSize = object.contains("SharedSize") ? (int)object.at("SharedSize") : -1;
    mVirtualSize = object.contains("VirtualSize") ? (int)object.at("VirtualSize") : -1;
    mContainers = object.contains("Containers") ? (int)object.at("Containers") : -1;
    if (object.contains("RepoTags") && object.at("RepoTags").get_instance_type() == argo::json::array_e) {
        auto& tagsArray = object.at("RepoTags").get_array();
        for (const auto& tag : tagsArray) {
            tags.push_back((std::string)tag);
        }
    }

    if (object.contains("RepoDigests") && object.at("RepoDigests").get_instance_type() == argo::json::array_e) {
        auto& digestsArray = object.at("RepoDigests").get_array();
        for (const auto& digest : digestsArray) {
            repositories.push_back((std::string)digest);
        }
    }

    if (object.contains("Labels") && object.at("Labels").get_instance_type() == argo::json::array_e) {
        auto& labelsObject = object.at("Labels").get_object();
        for (const auto& [key, value] : labelsObject) {
            labels[key] = (std::string)value;
        }
    }
}

static sm::docker::ContainerStatus parseContainerStatus(const std::string& status) {
    using enum sm::docker::ContainerStatus;
    if (status == "created") {
        return Created;
    } else if (status == "running") {
        return Running;
    } else if (status == "paused") {
        return Paused;
    } else if (status == "exited") {
        return Exited;
    } else if (status == "dead") {
        return Dead;
    }
    return Unknown;
}

sm::docker::Container::Container(const argo::json& json) {
    const auto& object = json.get_object();

    mId = (std::string)object.at("Id");

    if (object.contains("Names") && object.at("Names").get_instance_type() == argo::json::array_e) {
        auto& namesArray = object.at("Names").get_array();
        for (const auto& name : namesArray) {
            mNames.push_back((std::string)name);
        }
    }

    mImage = Image(object.at("Image"));
    mStatus = parseContainerStatus((std::string)object.at("Status"));
}
