#pragma once

#include "../coreplugin.hpp"
#include <clap-rci/coreplugin.h>
#include <clap-rci/gen/rci.grpc.pb.h>
#include <clap-rci/gen/rci.pb.h>
#include <clap-rci/global.h>
#include <clap-rci/registry.h>

#include <absl/log/log.h>
#include <format>

using namespace v0;

CLAP_RCI_BEGIN_NAMESPACE

class ClapCoreService : public api::ClapCore::CallbackService {
public:
    grpc::ServerUnaryReactor* GetPluginInstances(
        grpc::CallbackServerContext* context, const api::None*  /* request */,
        api::PluginInstances* response
    ) override
    {
        const auto& plugins = PluginInstances::instances();
        auto* reactor = context->DefaultReactor();
        for (const auto& p : plugins) {
            const auto it = response->mutable_instances()->emplace(
                p.first, p.second->idHash()
            );
            DLOG(INFO) << "plugin hash: " << p.second->idHash();
            if (!it.second) {
                reactor->Finish(
                    {grpc::StatusCode::RESOURCE_EXHAUSTED,
                     "Map creation failed."}
                );
                return reactor;
            }
        }
        reactor->Finish(grpc::Status::OK);
        return reactor;
    }

private:
};

CLAP_RCI_END_NAMESPACE
