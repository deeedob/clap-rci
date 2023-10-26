#ifndef SERVER_H
#define SERVER_H

#include "cqeventhandler.h"
#include <core/global.h>

#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <string_view>

RCLAP_BEGIN_NAMESPACE

// A single-shot gRPC Server. This class is not thread-safe and must be
// destroyed after use. The Server stores all completion queues and threads
// used to handle RPCs.
class Server
{
public:
    enum State { CREATE = 0, RUNNING, FINISHED };

    explicit Server(std::string_view address = "0.0.0.0:0");
    ~Server();

    Server(const Server &) = delete;
    Server(Server &&) = delete;
    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

    bool start();
    bool stop();

    [[nodiscard]] bool isRunning() const noexcept { return state == RUNNING; }
    [[nodiscard]] State currentState() const { return state; }
    [[nodiscard]] ClapInterface::AsyncService *service() noexcept { return &aservice; }
    [[nodiscard]] std::vector<std::unique_ptr<CqEventHandler>> *cqHandles() noexcept { return &cqHandlers; }
    // TODO: this is bad. Find something better
    [[nodiscard]] CqEventHandler *getServerStreamCqHandle() noexcept { return cqHandlers[PosStreamCq].get(); }
    [[nodiscard]] std::optional<std::string> address() const
    {
        if (auto p = port())
            return uri() + ":" + std::to_string(*p);
        return std::nullopt;
    }
    // TODO: remove optional, this makes no sense... Improve the class structure.
    // This should be a true single use class by-design.
    [[nodiscard]] std::optional<int> port() const noexcept
    {
        if (state != RUNNING)
            return std::nullopt;
        return mActivePort;
    }
    [[nodiscard]] std::string uri() const
    {
        return serverAddress.substr(0, serverAddress.find_last_of(':'));
    }

    static void wait(std::uint32_t ms);

private:
    ClapInterface::AsyncService aservice;
    std::unique_ptr<grpc::Server> server;

    static constexpr uint8_t  PosStreamCq = 0;
    std::vector<std::unique_ptr<CqEventHandler>> cqHandlers;
    std::vector<std::jthread> threads;

    std::string serverAddress;
    int mActivePort = -1;

    std::atomic<Server::State> state = Server::CREATE;
    static_assert(std::atomic<Server::State>::is_always_lock_free);
};

RCLAP_END_NAMESPACE

#endif // SERVER_H
