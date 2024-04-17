#pragma once

#include "../coreplugin_p.hpp"
#include <clap-rci/gen/rci.grpc.pb.h>
#include <clap-rci/global.h>

#include <absl/log/log.h>
#include <grpcpp/server_context.h>
#include <queue>

CLAP_RCI_BEGIN_NAMESPACE
using namespace CLAP_RCI_API_VERSION;

class EventStreamReactor : public grpc::ServerBidiReactor<
                               api::ClientEventMessage, api::PluginEventMessage>
{
public:
    explicit EventStreamReactor(
        std::shared_ptr<CorePluginPrivate>&& plugin_,
        grpc::CallbackServerContext* context_
    )
        : plugin(std::move(plugin_))
        , context(context_)
    {
        StartRead(&clientMsg);
    }

    ~EventStreamReactor() override { DLOG(INFO) << "~BiDirStream"; }

    EventStreamReactor(const EventStreamReactor&) = delete;
    EventStreamReactor& operator=(const EventStreamReactor&) = delete;

    EventStreamReactor(EventStreamReactor&&) = delete;
    EventStreamReactor& operator=(EventStreamReactor&&) = delete;

    void OnDone() override
    {
        DLOG(INFO) << "OnDone";
        if (!plugin->disconnect(this))
            LOG(ERROR) << std::format("Couldn't disconnect {:p}", (void*)this);
    }

    void OnCancel() override
    {
        DLOG(INFO) << "OnCancel";
        Finish(grpc::Status::CANCELLED);
    }

    void OnReadDone(bool ok) override
    {
        DLOG(INFO) << "OnReadDone " << ok;
        if (!ok) {
            if (!context->IsCancelled())
                Finish(grpc::Status::OK);
            return;
        }

        switch (clientMsg.event()) {
            case api::ClientEventMessage::RequestRestart:
                DLOG(INFO) << "request restart";
                plugin->hostRequestRestart();
                break;
            case api::ClientEventMessage::RequestProcess:
                DLOG(INFO) << "request process";
                plugin->hostRequestProcess();
                break;
            case api::ClientEventMessage::EnableTransportEvents:
                DLOG(INFO) << "request start transport";
                plugin->requestTransport(true);
                break;
            case api::ClientEventMessage::DisableTransportEvents:
                DLOG(INFO) << "request stop transport";
                plugin->requestTransport(false);
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
        if (isWriting) {
            DLOG(INFO) << "Buffering write call!" << pluginMsgBuffer.size();
            pluginMsgBuffer.push(std::move(event));
            return;
        }
        isWriting = true;
        pluginMsg = std::move(event);
        StartWrite(pluginMsg.get());
    }

    void OnWriteDone(bool ok) override
    {
        DLOG(INFO) << "OnWriteDone " << ok;
        if (!ok) {
            if (!context->IsCancelled())
                Finish(grpc::Status::OK);
            return;
        }

        if (!pluginMsgBuffer.empty()) {
            pluginMsg = std::move(pluginMsgBuffer.front());
            pluginMsgBuffer.pop();
            return StartWrite(pluginMsg.get());
        }

        pluginMsg.reset();
        isWriting = false;
        (void)context;
    }

    void TryCancel() { context->TryCancel(); }

private:
    std::shared_ptr<CorePluginPrivate> plugin;
    api::ClientEventMessage clientMsg;
    std::shared_ptr<api::PluginEventMessage> pluginMsg;
    std::queue<std::shared_ptr<api::PluginEventMessage>> pluginMsgBuffer;
    std::atomic<bool> isWriting = false;
    grpc::CallbackServerContext* context;
};

CLAP_RCI_END_NAMESPACE
