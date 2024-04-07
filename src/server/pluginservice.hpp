#pragma once

#include "eventstreamreactor.hpp"

#include <clap-rci/gen/rci.grpc.pb.h>
#include <clap-rci/global.h>
#include <clap-rci/registry.h>
#include <clap-rci/coreplugin.h>

#include <absl/log/log.h>

CLAP_RCI_BEGIN_NAMESPACE
using namespace CLAP_RCI_API_VERSION;

class ClapPluginService : public api::ClapPlugin::CallbackService {
public:
    grpc::ServerBidiReactor<api::ClientEventMessage, api::PluginEventMessage>*
    EventStream(grpc::CallbackServerContext* context) override
    {
        DLOG(INFO) << "EventStream";
        for (const auto& entry : PluginInstances::instances()) {
            context->AddInitialMetadata(
                std::string(entry.first), std::to_string(entry.second->idHash())
            );
        }
        // The client must provide the id of the plugin it wants to connect to.
        const auto metadata = context->client_metadata();
        for (const auto& [key, value] : std::as_const(metadata)) {
            if (std::string(key.data(), key.size()) == "hash_id") {
                const auto hashId = std::stoull(
                    std::string(value.data(), value.length())
                );
                const auto* plugin = PluginInstances::instance(hashId);
                if (!plugin) {
                    context->DefaultReactor()->Finish(
                        {grpc::StatusCode::UNAUTHENTICATED,
                         "Couldn't find plugin"}
                    );
                    return nullptr;
                }
                // register the connecting client with the coreplugin
                auto* pplugin = getImplPtr(plugin);
                auto newClient = std::make_unique<EventStreamReactor>(pplugin);
                auto* newClientPtr = newClient.get();
                pplugin->connect(std::move(newClient));
                return newClientPtr;
            }
        }
        context->DefaultReactor()->Finish(
            {grpc::StatusCode::UNAUTHENTICATED,
             "No hash_id in metadata provided"}
        );
        return nullptr;

        return nullptr;
    }
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
