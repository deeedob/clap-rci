#ifndef CLAPSERVERCTRL_H
#define CLAPSERVERCTRL_H

#include <core/global.h>
#include "shareddata.h"
#include "server.h"

#include <map>
#include <cstdint>

RCLAP_BEGIN_NAMESPACE

class CorePlugin;

// The static mServer controller. This class is thread-safe.
class ServerCtrl
{
public:
    static ServerCtrl &instance();

    ServerCtrl(const ServerCtrl &) = delete;
    ServerCtrl &operator=(const ServerCtrl &) = delete;
    ServerCtrl(ServerCtrl &&) = delete;
    ServerCtrl &operator=(ServerCtrl &&) = delete;
    ~ServerCtrl();

    bool start();
    bool stop();
    [[nodiscard]] bool isRunning() const noexcept
    {
        if (!mServer)
            return false;
        return mServer->isRunning();
    }
    Server* server() noexcept { return mServer.get(); }

    [[nodiscard]] std::optional<uint64_t> addPlugin(CorePlugin *plugin) noexcept;
    bool removePlugin(uint64_t hash) noexcept;

    std::shared_ptr<SharedData> getSharedData(uint64_t hash) noexcept;
    bool connectClient(ServerEventStream *stream, std::uint64_t hash) noexcept;
    bool connectClient(ServerEventStream *stream, std::string_view shash) noexcept;
    bool disconnectClient(ServerEventStream *stream, std::uint64_t hash = 0) noexcept;

    [[nodiscard]] std::size_t nPlugins() const noexcept { return mSharedData.size(); }
    [[nodiscard]] std::size_t nClients(uint64_t hash) const noexcept { return mSharedData.at(hash)->nStreams(); }
    [[nodiscard]] std::size_t totalClients()
    {
        std::size_t total = 0;
        for (auto &data : mSharedData)
            total += data.second->nStreams();
        return total;
    }

    [[nodiscard]] std::optional<CqEventHandler*> tryGetServerStreamCqHandle() {
        if (mSharedData.empty() || !isRunning())
            return std::nullopt;
        return mServer->getServerStreamCqHandle();
    }

    [[nodiscard]] std::chrono::milliseconds getInitTimeout() const noexcept { return mInitTimeout; }
    std::optional<std::string> address() const;

private:
    ServerCtrl();

private:
    std::unique_ptr<Server> mServer;
    std::mutex mServerMtx;

    std::map<uint64_t, std::shared_ptr<SharedData>> mSharedData;
    std::mutex mSharedDataMtx;

    std::chrono::milliseconds mInitTimeout{10'000}; // TODO: for now. Remove entirely in the future.
};

RCLAP_END_NAMESPACE

#endif // CLAPSERVERCTRL_H
