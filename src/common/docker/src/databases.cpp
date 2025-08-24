#include "docker/databases/oracle.hpp"
#include "docker/databases/postgres.hpp"

using namespace sm::docker;

static constexpr std::string_view kOracle23Image = "container-registry.oracle.com/database/free";
static constexpr std::string_view kOracle23FullTag = "23.8.0.0";
static constexpr std::string_view kOracle23LiteTag = "23.8.0.0-lite";

OracleDbContainer::OracleDbContainer(const OracleDbContainerCreateInfo& createInfo)
    : mDocker(DockerClient::local())
{
    sm::docker::ContainerCreateInfo containerCreateInfo;
    containerCreateInfo.image = kOracle23Image;
    containerCreateInfo.tag = createInfo.image == OracleDbImage::eOracle23Full ? kOracle23FullTag : kOracle23LiteTag;
    containerCreateInfo.env["ORACLE_PWD"] = createInfo.password;
    containerCreateInfo.ports.push_back({ 1521, 0, "tcp" });

    auto containerId = mDocker.createContainer(containerCreateInfo);
    mDocker.start(containerId);

    mContainer = mDocker.getContainer(containerId);
}

OracleDbContainer::~OracleDbContainer() {
    mDocker.stop(mContainer);
    mDocker.destroyContainer(mContainer.getId());
}

std::string OracleDbContainer::getHost() const {
    return "localhost";
}

uint16_t OracleDbContainer::getPort() const {
    return mContainer.getMappedPort(1521);
}

std::string OracleDbContainer::getDatabaseName() const {
    return "FREEPDB1";
}

std::string OracleDbContainer::getUsername() const {
    return "sys";
}

std::string OracleDbContainer::getPassword() const {
    return "oracle";
}

PostgresContainer::PostgresContainer(const PostgresContainerCreateInfo& createInfo)
    : mDocker(DockerClient::local())
    , mDatabaseName(createInfo.dbName)
    , mUsername(createInfo.username)
    , mPassword(createInfo.password)
{
    sm::docker::ContainerCreateInfo containerCreateInfo;
    containerCreateInfo.image = createInfo.image;
    containerCreateInfo.tag = createInfo.tag;
    containerCreateInfo.env["POSTGRES_DB"] = mDatabaseName;
    containerCreateInfo.env["POSTGRES_USER"] = mUsername;
    containerCreateInfo.env["POSTGRES_PASSWORD"] = mPassword;
    containerCreateInfo.ports.push_back({5432, 0, "tcp"});

    auto containerId = mDocker.createContainer(containerCreateInfo);
    mDocker.start(containerId);

    mContainer = mDocker.getContainer(containerId);
}

PostgresContainer::~PostgresContainer() {
    mDocker.stop(mContainer);
    mDocker.destroyContainer(mContainer.getId());
}

std::string PostgresContainer::getHost() const {
    return "localhost";
}

uint16_t PostgresContainer::getPort() const {
    return mContainer.getMappedPort(5432);
}

std::string PostgresContainer::getDatabaseName() const {
    return mDatabaseName;
}

std::string PostgresContainer::getUsername() const {
    return mUsername;
}

std::string PostgresContainer::getPassword() const {
    return mPassword;
}
