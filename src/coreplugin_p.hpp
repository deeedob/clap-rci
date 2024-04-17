#pragma once

#include "queueworker.hpp"
#include "transportwatcher.hpp"
#include <clap-rci/coreplugin.h>
#include <clap-rci/gen/rci.pb.h>
#include <clap-rci/global.h>
#include <clap-rci/mpmcqueue.h>
#include <clap-rci/server.h>

#include <absl/log/log.h>

#include <atomic>
#include <list>
#include <memory>
#include <vector>

CLAP_RCI_BEGIN_NAMESPACE
using namespace CLAP_RCI_API_VERSION;

class EventStreamReactor;

class CorePluginPrivate : public std::enable_shared_from_this<CorePluginPrivate>
{
    using PluginQueue = MpMcQueue<api::PluginEventMessage, 256>;
    using ClientQueue = MpMcQueue<api::ClientEventMessage, 256>;

    struct Private{};
public:
    explicit CorePluginPrivate(const clap_host* host, Private);
    static std::shared_ptr<CorePluginPrivate> create(const clap_host *host);

    void connect(std::unique_ptr<EventStreamReactor>&& client);
    bool disconnect(EventStreamReactor* client);
    void cancelAllClients();

    static bool notifyPluginQueueReady();
    PluginQueue& pluginToClientsQueue();
    ClientQueue& clientsToPluginQueue();
    void writeToClients(api::PluginEventMessage msg);

    void hostRequestRestart();
    void hostRequestProcess();
    void requestTransport(bool value);

private:
    inline static Server sServer;
    inline static QueueWorker sQueueWorker;
    inline static std::atomic<size_t> sConnectedClients = 0;

    const clap_host* mHost = nullptr;
    std::list<std::unique_ptr<EventStreamReactor>> mClients;
    std::mutex mClientsMtx;

    PluginQueue mPluginToClients;
    ClientQueue mClientsToPlugin;

    TransportWatcher mTransportWatcher;
    std::atomic<bool> mWantsTransport = true;

    std::vector<clap_note_port_info> mNotePortInfosIn;
    std::vector<clap_note_port_info> mNotePortInfosOut;

    friend class CorePlugin;
};

inline std::shared_ptr<CorePluginPrivate> getPimpl(const CorePlugin* plugin)
{
    return plugin->dPtr;
}

inline const std::shared_ptr<CorePluginPrivate>& getPimplRef(const CorePlugin* plugin)
{
    return plugin->dPtr;
}

CLAP_RCI_END_NAMESPACE
