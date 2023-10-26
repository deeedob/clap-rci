#ifndef CLIENTEVENTCALLHANDLER_H
#define CLIENTEVENTCALLHANDLER_H

#include "eventtag.h"
#include <core/global.h>

RCLAP_BEGIN_NAMESPACE

class ClientEventCallHandler : public EventTag
{
public:
    ClientEventCallHandler(CqEventHandler *parent, grpc::ServerCompletionQueue *cq);
    ~ClientEventCallHandler() override;

    ClientEventCallHandler(ClientEventCallHandler &&) = delete;
    ClientEventCallHandler &operator=(ClientEventCallHandler &&) = delete;

    ClientEventCallHandler(const ClientEventCallHandler &) = delete;
    ClientEventCallHandler &operator=(const ClientEventCallHandler &) = delete;

    void process(bool ok) override;
    std::uint64_t hash() const noexcept override { return idHash; }
    void kill() override;

private:
    GrpcMetadata metadata() const noexcept;
    std::optional<std::string> extractMetadata(const std::string_view &cmp) const noexcept;
    grpc::Status handleEvent();

private:
    grpc::ServerCompletionQueue *cq = nullptr;
    grpc::ServerContext ctx;

    grpc::ServerAsyncResponseWriter<None> writer;
    ClientEvent request;
    None response;

    std::uint64_t idHash = {};
    enum State { PROCESS, FINISH };
    State state = PROCESS;
};

RCLAP_END_NAMESPACE

#endif // CLIENTEVENTCALLHANDLER_H
