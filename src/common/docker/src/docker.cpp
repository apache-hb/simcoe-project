#include "docker/docker.hpp"
#include "docker/error.hpp"

#include "logs/logging.hpp"
#include "base/defer.hpp"

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

    defer {
        curl_slist_free_all(headers);
    };

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

    defer {
        curl_easy_cleanup(curl);
    };

    if (res != CURLE_OK) {
        throw sm::docker::DockerException("GET {} CURL error: {}", url, curl_easy_strerror(res));
    }

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (status != 200) {
        throw sm::docker::DockerException("Failed to GET {}: {}", url, status);
    }
}

void DockerClient::post(std::string_view path, const std::string& fields, const std::string& contentType, std::istream& input, std::ostream& output) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw sm::docker::DockerException("Failed to initialize CURL");
    }

    std::string url = std::format("{}{}", mHost, path);
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, contentType.c_str());

    defer {
        curl_slist_free_all(headers);
    };

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
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, curlReadCallback);
    curl_easy_setopt(curl, CURLOPT_READDATA, (void*)&input);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&output);

    if (mLocal) {
        curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, kDockerSocketPath);
    }

    CURLcode res = curl_easy_perform(curl);

    defer {
        curl_easy_cleanup(curl);
    };

    if (res != CURLE_OK) {
        throw sm::docker::DockerException("POST {} CURL error: {}", url, curl_easy_strerror(res));
    }

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (status != 200 && status != 201 && status != 204) {
        throw sm::docker::DockerException("Failed to POST {}: {}", url, status);
    }
}

std::string DockerClient::get(std::string_view path) {
    std::stringstream response;
    get(path, response);
    return response.str();
}

void DockerClient::del(std::string_view path) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw sm::docker::DockerException("Failed to initialize CURL");
    }

    std::string url = std::format("{}{}", mHost, path);
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");

    defer {
        curl_slist_free_all(headers);
    };

    LOG_TRACE(DockerLog, "DELETE: {}", url);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.81.0");
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (mLocal) {
        curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, kDockerSocketPath);
    }

    CURLcode res = curl_easy_perform(curl);

    defer {
        curl_easy_cleanup(curl);
    };

    if (res != CURLE_OK) {
        throw sm::docker::DockerException("DELETE {} CURL error: {}", url, curl_easy_strerror(res));
    }

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (status != 204) {
        throw sm::docker::DockerException("Failed to DELETE {}: {}", url, status);
    }
}

std::vector<sm::docker::Container> DockerClient::listContainers() {
    std::string response = get("/containers/json?all=true");
    std::unique_ptr<argo::json> json = argo::parser::parse(response);
    LOG_INFO(DockerLog, "Received JSON response: {}", response);

    std::vector<Container> containers;
    auto& array = json->get_array();
    for (const auto& item : array) {
        containers.emplace_back(Container::ofListElement(item));
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

sm::docker::ContainerId DockerClient::createContainer(const sm::docker::ContainerCreateInfo& createInfo) {
    argo::json requestJson{argo::json::object_e};
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

    requestJson["HostConfig"] = argo::json{argo::json::object_e};

    if (!createInfo.ports.empty()) {
        argo::json ports{argo::json::object_e};
        for (const auto& port : createInfo.ports) {
            ports[fmt::format("{}/{}", port.privatePort, port.protocol)] = argo::json::from_array({
                argo::json::from_object({
                    {"HostPort", port.publicPort == 0 ? "" : std::to_string(port.publicPort)}
                })
            });
        }
        requestJson["HostConfig"]["PortBindings"] = ports;
    }

    std::stringstream requestStream;
    requestStream << requestJson;
    std::string requestBody = requestStream.str();
    requestStream.seekg(0);
    LOG_TRACE(DockerLog, "Creating container with request JSON: {}", requestBody);
    requestStream.seekg(0);

    std::stringstream responseStream;

    post("/containers/create?name=" + createInfo.name, requestBody, "Content-Type: application/json", requestStream, responseStream);
    responseStream.seekg(0);
    std::unique_ptr<argo::json> responseJson = argo::parser::parse(responseStream);
    return ContainerId((std::string)(*responseJson)["Id"]);
}

void DockerClient::destroyContainer(const ContainerId& id) {
    auto path = std::format("/containers/{}", id.getId());
    LOG_TRACE(DockerLog, "Destroying container with ID: {}", id);
    del(path);
}

sm::docker::Container DockerClient::getContainer(const ContainerId& id) {
    auto path = std::format("/containers/{}/json", id.getId());
    LOG_TRACE(DockerLog, "Getting container with ID: {}", id.getId());
    std::string response = get(path);
    LOG_TRACE(DockerLog, "Received container JSON: {}", response);
    std::unique_ptr<argo::json> json = argo::parser::parse(response);
    return Container::ofInspect(*json);
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
    post("/images/create", params, "Content-Type: application/x-www-form-urlencoded", input, response);
}

bool DockerClient::isConnected() {
    try {
        get("/_ping");
        return true;
    } catch (const sm::docker::DockerException& e) {
        LOG_ERROR(DockerLog, "Docker connection failed: {}", e.what());
        return false;
    }
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

sm::docker::Container sm::docker::Container::ofInspect(const argo::json& json) {
    const auto& object = json.get_object();

    Container result;

    result.mId = ContainerId((std::string)object.at("Id"));
    result.mNames.push_back((std::string)object.at("Name"));
    result.mImageId = (std::string)object.at("Image");

    const auto& state = object.at("State").get_object();
    result.mState = parseContainerStatus((std::string)state.at("Status"));

    const auto& networkSettings = object.at("NetworkSettings").get_object();
    const auto& ports = networkSettings.at("Ports").get_object();
    for (const auto& [portKey, portValue] : ports) {
        if (portValue.get_instance_type() == argo::json::array_e) {
            for (const auto& portItem : portValue.get_array()) {
                if (portItem.get_instance_type() == argo::json::object_e) {
                    const auto& portObject = portItem.get_object();
                    uint16_t privatePort = std::stoi(portKey.substr(0, portKey.find('/')));
                    uint16_t publicPort = portObject.contains("HostPort") ? std::stoi(portObject.at("HostPort")) : 0;
                    std::string protocol = portObject.contains("Type") ? (std::string)portObject.at("Type") : "tcp";
                    result.mPorts.push_back({privatePort, publicPort, protocol});
                }
            }
        }
    }

    return result;
}

sm::docker::Container sm::docker::Container::ofListElement(const argo::json& json) {
    const auto& object = json.get_object();

    Container result;

    if (object.contains("Names") && object.at("Names").get_instance_type() == argo::json::array_e) {
        auto& namesArray = object.at("Names").get_array();
        for (const auto& name : namesArray) {
            result.mNames.push_back((std::string)name);
        }
    }

    if (object.contains("Ports") && object.at("Ports").get_instance_type() == argo::json::array_e) {
        auto& portsArray = object.at("Ports").get_array();
        for (const auto& port : portsArray) {
            if (port.get_instance_type() == argo::json::object_e) {
                const auto& portObject = port.get_object();
                uint16_t privatePort = portObject.contains("PrivatePort") ? (int)portObject.at("PrivatePort") : 0;
                uint16_t publicPort = portObject.contains("PublicPort") ? (int)portObject.at("PublicPort") : 0;
                std::string protocol = portObject.contains("Type") ? (std::string)portObject.at("Type") : "tcp";
                result.mPorts.push_back({privatePort, publicPort, protocol});
            }
        }
    }

    result.mId = ContainerId((std::string)object.at("Id"));
    result.mImageId = (std::string)object.at("Image");
    result.mState = parseContainerStatus((std::string)object.at("State"));

    return result;
}

void DockerClient::start(const ContainerId& id) {
    auto path = fmt::format("/containers/{}/start", id.getId());
    LOG_TRACE(DockerLog, "Starting container with ID: {}", id.getId());
    std::stringstream requestStream;
    std::stringstream responseStream;
    post(path, "", "Content-Type: application/json", requestStream, responseStream);
}

void DockerClient::stop(const ContainerId& id) {
    auto path = fmt::format("/containers/{}/stop", id.getId());
    LOG_TRACE(DockerLog, "Stopping container with ID: {}", id.getId());
    std::stringstream requestStream;
    std::stringstream responseStream;
    post(path, "", "Content-Type: application/json", requestStream, responseStream);
}

void DockerClient::restart(const ContainerId& id) {
    auto path = fmt::format("/containers/{}/restart", id.getId());
    LOG_TRACE(DockerLog, "Restarting container with ID: {}", id.getId());
    std::stringstream requestStream;
    std::stringstream responseStream;
    post(path, "", "Content-Type: application/json", requestStream, responseStream);
}
