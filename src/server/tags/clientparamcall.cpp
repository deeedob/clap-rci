#include <core/logging.h>
#include "clientparamcall.h"
#include "../cqeventhandler.h"
#include "../serverctrl.h"
#include <atomic>


RCLAP_BEGIN_NAMESPACE

ClientParamCall::ClientParamCall(CqEventHandler *parent, grpc::ServerCompletionQueue *cq)
    : EventTag(parent), cq(cq), writer(&ctx), idHash(toHash(this))
{
    service->RequestClientParamCall(&ctx, &request, &writer, cq, cq, this);
}

ClientParamCall::~ClientParamCall() = default;

void ClientParamCall::process(bool ok)
{
    if (!ok)
        return kill();

    if (state == PROCESS) {
        // Spawn a new Handler to serve new clients while we process the
        // one for this Handler.
        parent->create<ClientParamCall>();

        state = FINISH;
        writer.Finish(response, handleEvent(), this);
    } else {
        return kill();
    }
}

void ClientParamCall::kill()
{
    parent->destroyTag(hash());
}

GrpcMetadata ClientParamCall::metadata() const noexcept { return ctx.client_metadata(); }

std::optional<std::string> ClientParamCall::extractMetadata(const std::string_view &cmp) const noexcept
{
    for (const auto &[key, value] : std::as_const(metadata())) {
        if (strcmp(key.data(), cmp.data()) == 0)
            return std::string(value.data(), value.length());
    }
    return std::nullopt;
}

grpc::Status ClientParamCall::handleEvent()
{
    auto id = extractMetadata(Metadata::PluginHashId);
    if (!id)
        return {grpc::StatusCode::INVALID_ARGUMENT, "No PluginHashId"};

    auto sharedData = ServerCtrl::instance().getSharedData(std::stoull(*id));
    if (!sharedData)
        return {grpc::StatusCode::NOT_FOUND, "Plugin not found"};

    sharedData->pushClientParam(request);
    return grpc::Status::OK;
}

RCLAP_END_NAMESPACE

