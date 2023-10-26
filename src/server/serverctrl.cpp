#include <core/logging.h>
#include <plugin/coreplugin.h>

#include "cqeventhandler.h"
#include "serverctrl.h"

#include <chrono>
#include <netdb.h>
#include <optional>
#include <string>

#ifdef __linux__
#include <unistd.h>
#include <cstdint>
#endif

RCLAP_BEGIN_NAMESPACE

// Singleton
ServerCtrl &ServerCtrl::instance()
{
    static ServerCtrl singleton;
    return singleton;
}

ServerCtrl::~ServerCtrl()
{
    if (isRunning())
        stop();
}

// ########### PRIVATE ###########
ServerCtrl::ServerCtrl() = default;

// ########### PUBLIC ###########

bool ServerCtrl::start()  // TODO: noexcept?
{
    std::unique_lock lock(mServerMtx);

    if (!mServer)
        mServer = std::make_unique<Server>();

    if (isRunning()) {
        SPDLOG_DEBUG("Server already running.");
        return false;
    }

    if (!mServer->start()) {
        SPDLOG_ERROR("Failed to start server");
        return false;
    }

    mServer->start();

    return true;
}

// Stop the Server instance and reset it along with the thread.
// We have to destroy the Server instance since a grpc Server
// cannot be restarted. This is thread-safe and can be called
// at any point in time.
bool ServerCtrl::stop()
{
    if (!isRunning() || nPlugins() != 0)
        return false;

    std::unique_lock lock(mServerMtx);
    if (!mServer->stop())
        return false;

    mServer.reset();
    return true;
}

std::shared_ptr<SharedData> ServerCtrl::getSharedData(uint64_t hash) noexcept
{
    std::scoped_lock lock(mSharedDataMtx);
    const auto it = mSharedData.find(hash);
    if (it == mSharedData.end())
        return nullptr;
    return it->second;
}

/**
 * @brief Adds a plugin instance to the static ServerCtrl. Constructs the SharedData needed for communication.
 * with the servers' clients.
 * @return The hash identifier if the plugin was successfully added, std::nullopt otherwise. This hash
 * is used to identify the plugin to the Server.
 */
std::optional<uint64_t> ServerCtrl::addPlugin(CorePlugin *plugin) noexcept
{
    assert(plugin != nullptr);
    const auto hash = toHash(plugin);

    std::scoped_lock lock(mSharedDataMtx);
    const auto ret = mSharedData.emplace(hash, std::make_shared<SharedData>(plugin));
    if (!ret.second) {
        SPDLOG_ERROR("Failed to add plugin; Plugin is already contained.");
        return std::nullopt;
    }
    return hash;
}

bool ServerCtrl::removePlugin(uint64_t hash) noexcept
{
    if (hash == 0)
        return false;

    std::scoped_lock lock(mSharedDataMtx);
    if (mSharedData.erase(hash) == 0) {
        SPDLOG_ERROR("Failed to remove plugin with hash {}; Plugin is not contained.", hash);
        return false;
    }
    return true;
}

bool ServerCtrl::connectClient(ServerEventStream *stream, std::uint64_t hash) noexcept
{
    std::scoped_lock lock(mSharedDataMtx);
    const auto it = mSharedData.find(hash);
    if (it == mSharedData.end()) {
        SPDLOG_ERROR("Failed to connect handle; Plugin is not contained.");
        return false;
    }
    return it->second->addStream(stream);
}

bool ServerCtrl::connectClient(ServerEventStream *stream, std::string_view shash) noexcept
{
    return connectClient(stream, std::stoull(std::string(shash)));
}

bool ServerCtrl::disconnectClient(ServerEventStream *stream, std::uint64_t hash) noexcept
{
    if (hash != 0) {        // Fast path
        std::scoped_lock lock(mSharedDataMtx);
        const auto it = mSharedData.find(hash);
        if (it == mSharedData.end()) {
            SPDLOG_ERROR("Failed to disconnect client; Plugin is not contained.");
            return false;
        }
        return it->second->removeStream(stream);
    }

    std::scoped_lock lock(mSharedDataMtx);
    auto it = mSharedData.begin();
    while (it != mSharedData.end()) {
        if (it->second->removeStream(stream)) {
            return true;
        }
        ++it;
    }

    SPDLOG_ERROR("Failed to disconnect client; Client is not contained.");
    return false;
}

std::optional<std::string> ServerCtrl::address() const
{
    return mServer->address();
}

RCLAP_END_NAMESPACE
