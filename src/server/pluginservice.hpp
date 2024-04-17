#pragma once

#include <clap-rci/coreplugin.h>
#include <clap-rci/gen/rci.grpc.pb.h>
#include <clap-rci/global.h>
#include <clap-rci/registry.h>

#include <absl/log/log.h>

CLAP_RCI_BEGIN_NAMESPACE
using namespace CLAP_RCI_API_VERSION;

class ClapPluginService : public api::ClapPluginService::CallbackService
{
public:

    grpc::ServerBidiReactor<api::ClientEventMessage, api::PluginEventMessage>*
    EventStream(grpc::CallbackServerContext* context) override;

    grpc::ServerUnaryReactor* GetPluginInstances(
        grpc::CallbackServerContext* context, const api::None* /* request */,
        api::PluginInstances* response
    ) override;

    grpc::ServerUnaryReactor* GetPluginState(
        grpc::CallbackServerContext* /*context*/, const api::None* /*request*/,
        api::PluginState* /*response*/
    ) override
    {
        return nullptr;
    }

private:
};

CLAP_RCI_END_NAMESPACE
