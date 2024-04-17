#include "pluginservice.hpp"
#include "eventstreamreactor.hpp"

CLAP_RCI_BEGIN_NAMESPACE

grpc::ServerBidiReactor<api::ClientEventMessage, api::PluginEventMessage>*
ClapPluginService::EventStream(grpc::CallbackServerContext* context)
{
    // The client must provide the id of the plugin it wants to connect to.
    const auto metadata = context->client_metadata();
    for (const auto& [key, value] : std::as_const(metadata)) {
        if (std::string(key.data(), key.size()) == "plugin_id") {
            const auto hashId = std::stoull(
                std::string(value.data(), value.length())
            );
            const auto* plugin = Registry::Instances::instance(hashId);
            if (!plugin) {
                context->DefaultReactor()->Finish(
                    {grpc::StatusCode::UNAUTHENTICATED, "Couldn't find plugin"}
                );
                return nullptr;
            }
            // register the connecting client with the coreplugin
            auto pimpl = getPimpl(plugin);
            auto* pimplPtr = pimpl.get();
            auto newClient = std::make_unique<EventStreamReactor>(
                std::move(pimpl), context
            );
            auto* newClientPtr = newClient.get();
            pimplPtr->connect(std::move(newClient));
            DLOG(INFO) << "New event stream connection started";
            return newClientPtr;
        } else {
            DLOG(INFO) << "New connection didn't provide any metadata";
            for (const auto& entry : Registry::Instances::instances()) {
                context->AddInitialMetadata(
                    std::string(entry.first),
                    std::to_string(entry.second->pluginId())
                );
            }
        }
    }
    context->DefaultReactor()->Finish(
        {grpc::StatusCode::UNAUTHENTICATED, "No hash_id in metadata provided"}
    );

    return nullptr;
}

grpc::ServerUnaryReactor* ClapPluginService::GetPluginInstances(
    grpc::CallbackServerContext* context, const api::None* /* request */,
    api::PluginInstances* response
)
{
    const auto& plugins = Registry::Instances::instances();
    auto* reactor = context->DefaultReactor();
    for (const auto& p : plugins) {
        const auto it = response->mutable_instances()->emplace(
            p.first, p.second->pluginId()
        );
        DLOG(INFO) << "plugin hash: " << p.second->pluginId();
        if (!it.second) {
            reactor->Finish(
                {grpc::StatusCode::RESOURCE_EXHAUSTED, "Map creation failed."}
            );
            return reactor;
        }
    }
    reactor->Finish(grpc::Status::OK);
    return reactor;
}

CLAP_RCI_END_NAMESPACE
