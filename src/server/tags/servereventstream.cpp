#include <core/logging.h>
#include "servereventstream.h"

#include "../cqeventhandler.h"
#include "../serverctrl.h"
#include "../shareddata.h"

RCLAP_BEGIN_NAMESPACE

ServerEventStream::ServerEventStream(CqEventHandler *parent, grpc::ServerCompletionQueue *cq)
    : EventTag(parent), cq(cq), stream(&ctx), idHash(toHash(this))
{
    SPDLOG_TRACE("ServerEventStream created {}", toTag(this));
    service->RequestServerEventStream(&ctx, &request, &stream, cq, cq, this);
}

ServerEventStream::~ServerEventStream() = default;

void ServerEventStream::process(bool ok)
{
    switch (state) {

        case CONNECT: {
            if (!ok)
                return kill();
            // Create a new instance to serve new clients while we're processing this one.
            parent->create<ServerEventStream>();
            // Try to connect the client. The client must provide a valid hash-id of a plugin instance
            // in the metadata to successfully connect.
            if (!connectClient()) {
                state = FINISH;
                stream.Finish({ grpc::StatusCode::UNAUTHENTICATED, "Couldn't authenticate client" }, toTag(this));
                SPDLOG_ERROR("ServerEventStream failed to connect to client", toTag(this));
                return;
            }

            assert(sharedData != nullptr);
            if (!sharedData->tryStartPolling()) {
                SPDLOG_DEBUG("ServerEventStream couldn't start polling: {}", toTag(this));
            }
            state = WRITE;
            SPDLOG_INFO("ServerEventStream new client @ {} connected", toTag(this));
        } break;

        case WRITE: {
            if (!ok) {
                SPDLOG_DEBUG("ServerEventStream: Write finished");
                stream.Finish({ grpc::StatusCode::OK, "Client Write Finished" }, toTag(this));
                state = DISCONNECT;
                return;
            }
        } break;

        case DISCONNECT: {
            SPDLOG_TRACE("ServerEventStream DISCONNECT {}", toTag(this));
            stream.Finish({ grpc::StatusCode::OK, "Client Disconnected" }, toTag(this));
            state = FINISH;
        } break;

        case FINISH: {
            SPDLOG_TRACE("ServerEventStream FINISH {}", toTag(this));
            kill();
        } break;

    }
}

// TODO: is this safe? Or should we rather use a queue? and poll these events
bool ServerEventStream::sendEventNow(const ServerEvent &ev)
{
    if (state != WRITE)
        return false;

    response.Clear();
    response.add_events()->CopyFrom(ev);
    stream.Write(response, toTag(this));
    return true;
}

bool ServerEventStream::sendEvents(const ServerEvents &evs)
{
    if (state.load() != WRITE) {
        SPDLOG_TRACE("sendEvent() {}, not in write state", toTag(this));
        return false;
    }
    stream.Write(evs, toTag(this));
    return true;
}

bool ServerEventStream::endStream()
{
    if (state != WRITE)
        return false;
    state = DISCONNECT;
    alarmSignal.Set(cq, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME) , toTag(this));
    return true;
}

void ServerEventStream::kill()
{
    parent->destroyTag(hash());
    if (sharedData)
        sharedData->removeStream(this);
}

bool ServerEventStream::connectClient()
{
    auto id = extractMetadata(Metadata::PluginHashId);
    if (!id) {
        SPDLOG_ERROR("ServerEventStream: No PluginHashId {}", toTag(this));
        return false;
    }

    sharedHash = std::stoull(*id);
    if(!ServerCtrl::instance().connectClient(this, sharedHash)) {
        return false;
    }

    sharedData = ServerCtrl::instance().getSharedData(sharedHash);
    if (!sharedData) {
        SPDLOG_ERROR("ServerEventStream: No SharedData {}", toTag(this));
        return false;
    }

    return true;
}

RCLAP_END_NAMESPACE
