#include "queueworker.hpp"
#include "coreplugin_p.hpp"

#include <clap-rci/gen/rci.pb.h>
#include <clap-rci/registry.h>

#include <absl/log/log.h>

CLAP_RCI_BEGIN_NAMESPACE

bool QueueWorker::start()
{
    if (worker.joinable()) {
        DLOG(INFO) << "QueueWorker already running";
        return false;
    }

    worker = std::jthread([this](std::stop_token stoken) {
        DLOG(INFO) << "QueueWorker started";
        while (!stoken.stop_requested()) {
            std::unique_lock<std::mutex> lock(workerMtx);
            workerCv.wait(lock, stoken, [this] {
                return isReady.load();
            });
            isReady = false;
            DLOG(INFO) << "QueueWorker woke up";

            // Iterate through all plugin instances ...
            for (const auto& i : Registry::Instances::instances()) {
                auto pp = getPimplRef(i.second.get());
                api::PluginEventMessage temp;
                // ... push the events to all connected clients.
                while (pp->pluginToClientsQueue().pop(&temp)) {
                    pp->writeToClients(temp);
                }
            }
        }
        DLOG(INFO) << "QueueWorker stopped";
    });
    return true;
}

bool QueueWorker::stop()
{
    if (!worker.joinable()) {
        DLOG(INFO) << "QueueWorker not running";
        return false;
    }
    worker.request_stop();
    worker.join();
    return true;
}

bool QueueWorker::tryNotify()
{
    if (isReady)
        return false;
    isReady = true;
    workerCv.notify_one();
    return true;
}

CLAP_RCI_END_NAMESPACE
