#ifndef CLIENTPARAMCALL_H
#define CLIENTPARAMCALL_H

#include "eventtag.h"
#include <core/global.h>
#include <optional>

RCLAP_BEGIN_NAMESPACE

class ClientParamCall : public EventTag
{
public:
    ClientParamCall(CqEventHandler *parent, grpc::ServerCompletionQueue *cq);
    ~ClientParamCall() override;

    ClientParamCall(ClientParamCall &&) = delete;
    ClientParamCall &operator=(ClientParamCall &&) = delete;

    ClientParamCall(const ClientParamCall &) = delete;
    ClientParamCall &operator=(const ClientParamCall &) = delete;

    void process(bool ok) override;
    std::uint64_t hash() const noexcept override { return idHash; }
    void kill() override;

private:
    GrpcMetadata metadata() const noexcept;
    std::optional<std::string> extractMetadata(const std::string_view &cmp) const noexcept;
    grpc::Status handleEvent();

private:
    [[maybe_unused]] grpc::ServerCompletionQueue *cq = nullptr;
    grpc::ServerContext ctx;

    grpc::ServerAsyncResponseWriter<None> writer;
    ClientParams request;
    None response;

    std::uint64_t idHash = {};
    enum State { PROCESS, FINISH };
    State state = PROCESS;
};

RCLAP_END_NAMESPACE

#endif // CLIENTPARAMCALL_H
