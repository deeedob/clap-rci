#pragma once

#include <clap-rci/coreplugin.h>
#include <clap-rci/gen/rci.pb.h>
#include <clap-rci/global.h>
#include <clap-rci/mpmcqueue.h>
#include <clap-rci/server.h>

#include <absl/log/log.h>

#include <condition_variable>
#include <list>
#include <memory>
#include <thread>

CLAP_RCI_BEGIN_NAMESPACE
using namespace CLAP_RCI_API_VERSION;

class EventStreamReactor;

class CorePluginPrivate {
    using PluginQueue = MpMcQueue<api::PluginEventMessage, 256>;
    using ClientQueue = MpMcQueue<api::ClientEventMessage, 256>;

public:
    CorePluginPrivate(const clap_host* host);
    ~CorePluginPrivate();

    void connect(std::unique_ptr<EventStreamReactor>&& client);
    bool disconnect(EventStreamReactor* client);

    static bool notifyPluginQueueReady();
    PluginQueue& pluginToClientsQueue();
    ClientQueue& clientsToPluginQueue();

    void hostRequestRestart();
    void hostRequestProcess();

private:
    static void startQueueWorker();

private:
    inline static Server sServer;
    inline static std::jthread sQueueWorker;
    inline static std::mutex sQueueWorkerMtx;
    inline static std::condition_variable_any sQueueWorkerCv;
    inline static std::atomic<bool> sQueueIsReady {false};
    inline static std::atomic<size_t> sConnectedClients = 0;

    std::list<std::unique_ptr<EventStreamReactor>> mClients;
    std::mutex mClientsMtx;

    PluginQueue mPluginToClients;
    ClientQueue mClientsToPlugin;

    const clap_host* mHost = nullptr;
};

inline CorePluginPrivate* getImplPtr(const CorePlugin* plugin)
{
    return plugin->dPtr.get();
}

CLAP_RCI_END_NAMESPACE
