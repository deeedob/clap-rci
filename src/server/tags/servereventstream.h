#ifndef SERVEREVENTSTREAM_H
#define SERVEREVENTSTREAM_H

#include "eventtag.h"
#include <core/global.h>
#include <grpcpp/alarm.h>

RCLAP_BEGIN_NAMESPACE

class SharedData;

class ServerEventStream : public EventTag
{
public:
    ServerEventStream(CqEventHandler *parent, grpc::ServerCompletionQueue *cq);
    ~ServerEventStream() override;

    ServerEventStream(ServerEventStream &&) = delete;
    ServerEventStream &operator=(ServerEventStream &&) = delete;

    ServerEventStream(const ServerEventStream &) = delete;
    ServerEventStream &operator=(const ServerEventStream &) = delete;

    void process(bool ok) override;
    std::uint64_t hash() const noexcept override { return idHash; }
    void kill() override;

    bool sendEventNow(const ServerEvent &ev);
    bool sendEvents(const ServerEvents &evs);
    bool endStream();

private:
    bool connectClient();
    GrpcMetadata metadata() const noexcept { return ctx.client_metadata(); }
    std::optional<std::string> extractMetadata(const std::string_view &cmp) const noexcept
    {
        for (const auto &[key, value] : std::as_const(metadata())) {
            if (strcmp(key.data(), cmp.data()) == 0)
                return std::string(value.data(), value.length());
        }
        return std::nullopt;
    }

private:
    grpc::ServerCompletionQueue *cq = nullptr;
    grpc::ServerContext ctx;

    grpc::ServerAsyncWriter<ServerEvents> stream;
    ServerEvents response;
    ClientRequest request;

    std::uint64_t sharedHash = {};
    std::shared_ptr<SharedData> sharedData;
    grpc::Alarm alarmSignal;

    enum State { CONNECT, WRITE, DISCONNECT, FINISH };
    std::atomic<State> state = CONNECT;
    static_assert(std::atomic<State>::is_always_lock_free);

    std::uint64_t idHash = {};
};

RCLAP_END_NAMESPACE

#endif // SERVEREVENTSTREAM_H
