#ifndef CQEVENTHANDLER_H
#define CQEVENTHANDLER_H

#include <api/api.pb.h>
#include <api/api.grpc.pb.h>
using namespace api::v0;

#include "tags/eventtag.h"
#include <core/global.h>

#include <absl/container/flat_hash_map.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/alarm.h>

#include <map>
#include <memory>
#include <functional>

RCLAP_BEGIN_NAMESPACE

class Server;

// Base for all handlers on a completion queue.
class CqEventHandler
{
public:
    enum State { STARTUP, RUNNING, SHUTDOWN };

    CqEventHandler(Server *parent, std::unique_ptr<grpc::ServerCompletionQueue> cq);
    ~CqEventHandler() = default;

    CqEventHandler(CqEventHandler &&other) noexcept ;
    CqEventHandler &operator=(CqEventHandler &&other) noexcept ;

    CqEventHandler(const CqEventHandler &) = delete;
    CqEventHandler &operator=(const CqEventHandler &) = delete;

    // Defers \a f to be called after \a deferMs milliseconds.
    bool enqueueFn(EventTag::FnType &&f, std::uint64_t deferNs = 0);
    bool destroyTag(std::uint64_t hash);
    bool destroyAlarmTag(EventTag *tag);
    void cancelAllPendingTags();
    void teardown();
    bool hasPendingAlarms() const noexcept { return !pendingAlarmTags.empty(); }
    State getState() const noexcept { return state; }

    ClapInterface::AsyncService *service() noexcept;

    // The main loop of the completion queue. This will block until the completion queue
    // is shutdown.
    void run();

    // Create a new RPC - handler. Returns true on success, false otherwise.
    template <typename T>
    bool create();

private:
    Server *parent = nullptr;
    std::unique_ptr<grpc::ServerCompletionQueue> cq;

    std::map<std::unique_ptr<EventTag>, grpc::Alarm> pendingAlarmTags;
    std::map<std::uint64_t, std::unique_ptr<EventTag>> pendingTags;

    std::atomic<State> state = STARTUP;
    static_assert(std::atomic<State>::is_always_lock_free);
};

RCLAP_END_NAMESPACE

#endif // CQEVENTHANDLER_H
