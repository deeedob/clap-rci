#include "coreplugin.hpp"
#include "server/eventstreamreactor.hpp"

#include <clap-rci/registry.h>

CLAP_RCI_BEGIN_NAMESPACE

CorePluginPrivate::CorePluginPrivate(const clap_host* host)
    : mHost(host)
{
    if (!sServer.start("localhost:3000")) // TODO: rm debug
        DLOG(INFO) << "Server already running";
}

CorePluginPrivate::~CorePluginPrivate() = default;

void CorePluginPrivate::connect(std::unique_ptr<EventStreamReactor>&& client)
{
    std::unique_lock<std::mutex> guard(mClientsMtx);

    if (mClients.empty() && sConnectedClients == 0)
        startQueueWorker();

    mClients.push_back(std::move(client));
    ++sConnectedClients;

    DLOG(INFO) << "Connected client, got: " << mClients.size();
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
        if (!sQueueWorker.joinable())
            LOG(ERROR) << "queue worker not joinable!";
        DLOG(INFO) << "request stop for queue worker";
        sQueueWorker.request_stop();
        sQueueWorker.join();
    }

    DLOG(INFO) << "Disconnected client: " << client;
    return true;
}

bool CorePluginPrivate::notifyPluginQueueReady()
{
    // TODO: CAS
    if (sQueueIsReady)
        return false;
    sQueueIsReady = true;
    sQueueWorkerCv.notify_one();
    return true;
}

CorePluginPrivate::PluginQueue& CorePluginPrivate::pluginToClientsQueue()
{
    return mPluginToClients;
}

CorePluginPrivate::ClientQueue& CorePluginPrivate::clientsToPluginQueue()
{
    return mClientsToPlugin;
}

void CorePluginPrivate::hostRequestRestart()
{
    mHost->request_restart(mHost);
}

void CorePluginPrivate::hostRequestProcess()
{
    mHost->request_process(mHost);
}

// Private
void CorePluginPrivate::startQueueWorker()
{
    if (sQueueWorker.joinable())
        LOG(ERROR) << "queue worker is already running";

    sQueueWorker = std::jthread([](std::stop_token stoken) {
        DLOG(INFO) << "queue worker started!";
        while (!stoken.stop_requested()) {
            std::unique_lock<std::mutex> lock(sQueueWorkerMtx);
            sQueueWorkerCv.wait(lock, stoken, [] {
                return sQueueIsReady.load();
            });
            sQueueIsReady = false;
            DLOG(INFO) << "queue thread woke up";

            for (const auto& i : PluginInstances::instances()) {
                auto* pp = getImplPtr(i.second.get());
                api::PluginEventMessage temp;
                // Iterate through all plugin instances and push
                // the events to all connected clients.
                while (pp->mPluginToClients.pop(&temp)) {
                    // use a shared_pointer to control the point
                    // of destruction for @msg; that is when all
                    // clients destroyed it after OnWriteDone.
                    const auto msg = std::make_shared<api::PluginEventMessage>(
                        temp
                    );
                    for (const auto& c : pp->mClients) {
                        c->StartSharedWrite(msg);
                    }
                    DLOG(INFO)
                        << "Shared use count: " << msg.use_count() << " with "
                        << pp->mClients.size() << " clients ";
                }
            }
        }
        DLOG(INFO) << "Thread stopped";
    });
}

CLAP_RCI_END_NAMESPACE
