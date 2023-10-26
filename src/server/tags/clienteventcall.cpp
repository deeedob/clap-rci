#include <core/logging.h>
#include "clienteventcall.h"
#include "../cqeventhandler.h"
#include "../serverctrl.h"

RCLAP_BEGIN_NAMESPACE

ClientEventCallHandler::ClientEventCallHandler(CqEventHandler *parent, grpc::ServerCompletionQueue *cq)
    : EventTag(parent), cq(cq), writer(&ctx), idHash(toHash(this))
{
    service->RequestClientEventCall(&ctx, &request, &writer, cq, cq, this);
}

ClientEventCallHandler::~ClientEventCallHandler() = default;

void ClientEventCallHandler::process(bool ok)
{
    if (!ok)
        return kill();

    if (state == PROCESS) {
        // Spawn a new Handler to serve new clients while we process the
        // one for this Handler.
        parent->create<ClientEventCallHandler>();

        state = FINISH;
        writer.Finish(response, handleEvent(), this);
    } else {
        return kill();
    }
}

void ClientEventCallHandler::kill()
{
    parent->destroyTag(hash());
}

GrpcMetadata ClientEventCallHandler::metadata() const noexcept { return ctx.client_metadata(); }

std::optional<std::string> ClientEventCallHandler::extractMetadata(const std::string_view &cmp) const noexcept
{
    for (const auto &[key, value] : std::as_const(metadata())) {
        if (strcmp(key.data(), cmp.data()) == 0)
            return std::string(value.data(), value.length());
    }
    return std::nullopt;
}

grpc::Status ClientEventCallHandler::handleEvent()
{
    auto id = extractMetadata(Metadata::PluginHashId);
    if (!id)
        return {grpc::StatusCode::INVALID_ARGUMENT, "No PluginHashId"};

    auto sharedData = ServerCtrl::instance().getSharedData(std::stoull(*id));
    if (!sharedData)
        return {grpc::StatusCode::NOT_FOUND, "Plugin not found"};
    SPDLOG_TRACE("ClientEventCall: enqueue blocking event {}", static_cast<int>(request.event()));
    bool ok = sharedData->blockingPushClientEvent(request.event());
    if (!ok)
        return {grpc::StatusCode::INTERNAL, "Failed to push event"};

    return grpc::Status::OK;
}

RCLAP_END_NAMESPACE
