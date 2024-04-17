#include "coreplugin_p.hpp"
#include "server/eventstreamreactor.hpp"

#include <clap-rci/registry.h>

CLAP_RCI_BEGIN_NAMESPACE

CorePluginPrivate::CorePluginPrivate(const clap_host* host, Private /* tag */)
    : mHost(host)
{
}

std::shared_ptr<CorePluginPrivate>
CorePluginPrivate::create(const clap_host* host)
{
    if (!sServer.start("localhost:3000")) // TODO: rm debug
        DLOG(INFO) << "Server already running";
    return std::make_shared<CorePluginPrivate>(host, Private());
}

void CorePluginPrivate::connect(std::unique_ptr<EventStreamReactor>&& client)
{
    std::unique_lock<std::mutex> guard(mClientsMtx);

    if (mClients.empty() && sConnectedClients == 0)
        sQueueWorker.start();

    mClients.push_back(std::move(client));
    ++sConnectedClients;

    DLOG(INFO) << std::format(
        "Client connected. Local: {}, Global: {}", mClients.size(),
        sConnectedClients.load()
    );
}

bool CorePluginPrivate::disconnect(EventStreamReactor* client)
{
    std::unique_lock<std::mutex> guard(mClientsMtx);

    auto it = std::ranges::find_if(mClients, [client](const auto& ptr) {
        return ptr.get() == client;
    });

    if (it == mClients.end())
        return false;

    mClients.erase(it);
    --sConnectedClients;
    if (mClients.empty() && sConnectedClients == 0) {
        if (!sQueueWorker.stop()) {
            LOG(ERROR) << "Failed to stop QueueWorker";
        }
    }

    DLOG(INFO) << std::format(
        "Client disconnected. Local: {}, Global: {}", mClients.size(),
        sConnectedClients.load()
    );

    return true;
}

void CorePluginPrivate::cancelAllClients()
{
    for (const auto& c : mClients)
        c->TryCancel();
}

bool CorePluginPrivate::notifyPluginQueueReady()
{
    return sQueueWorker.tryNotify();
}

CorePluginPrivate::PluginQueue& CorePluginPrivate::pluginToClientsQueue()
{
    return mPluginToClients;
}

CorePluginPrivate::ClientQueue& CorePluginPrivate::clientsToPluginQueue()
{
    return mClientsToPlugin;
}

void CorePluginPrivate::writeToClients(api::PluginEventMessage msg)
{
    // use a shared_pointer to control the point of destruction for @smsg; that
    // is when all clients destroyed it after OnWriteDone.
    const auto smsg = std::make_shared<api::PluginEventMessage>(std::move(msg));
    for (const auto& c : mClients)
        c->StartSharedWrite(smsg);
}

void CorePluginPrivate::hostRequestRestart()
{
    mHost->request_restart(mHost);
}

void CorePluginPrivate::hostRequestProcess()
{
    mHost->request_process(mHost);
}

void CorePluginPrivate::requestTransport(bool value)
{
    mWantsTransport = value;
}

CLAP_RCI_END_NAMESPACE
