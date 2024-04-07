#pragma once

#include "../coreplugin.hpp"
#include <clap-rci/global.h>
#include <clap-rci/gen/rci.grpc.pb.h>

#include <absl/log/log.h>

CLAP_RCI_BEGIN_NAMESPACE
using namespace CLAP_RCI_API_VERSION;

class EventStreamReactor : public grpc::ServerBidiReactor<
                        api::ClientEventMessage, api::PluginEventMessage> {
public:
    explicit EventStreamReactor(CorePluginPrivate* plugin_)
        : plugin(plugin_)
    {
        StartRead(&clientMsg);
    }

    ~EventStreamReactor()
    {
        DLOG(INFO) << "~BiDirStream";
    }

    void OnDone() override
    {
        DLOG(INFO) << "OnDone";
        if (!plugin->disconnect(this))
            LOG(ERROR) << std::format("Couldn't disconnect {:p}", (void*)this);
    }

    void OnCancel() override
    {
        DLOG(INFO) << "OnCancel";
        Finish(grpc::Status::OK);
    }

    void OnReadDone(bool ok) override
    {
        DLOG(INFO) << "OnReadDone " << ok;
        if (!ok) {
            Finish(grpc::Status::OK);
            return;
        }

        switch (clientMsg.event()) {
            case api::RequestRestart:
                DLOG(INFO) << "requesting restart";
                plugin->hostRequestRestart();
                break;
            case api::RequestProcess:
                DLOG(INFO) << "requesting process";
                plugin->hostRequestProcess();
                break;
            default:
                break;
        }
        // if (!plugin->clientsToPluginQueue().push(clientMsg))
        //     LOG(WARNING) << "clientToPluginQueue misbehaving";
        StartRead(&clientMsg);
    }

    void StartSharedWrite(std::shared_ptr<api::PluginEventMessage> event)
    {
        pluginMsg = std::move(event);
        StartWrite(pluginMsg.get());
    }

    void OnWriteDone(bool ok) override
    {
        DLOG(INFO) << "OnWriteDone " << ok;
        if (!ok) {
            Finish(grpc::Status::OK);
            return;
        }
        pluginMsg.reset();
    }

private:
    CorePluginPrivate* plugin = nullptr;
    api::ClientEventMessage clientMsg;
    std::shared_ptr<api::PluginEventMessage> pluginMsg;
};

CLAP_RCI_END_NAMESPACE
