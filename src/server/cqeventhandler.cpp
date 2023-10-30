#include <core/logging.h>

#include "server.h"
#include "cqeventhandler.h"
#include "tags/clienteventcall.h"
#include "tags/clientparamcall.h"
#include "tags/servereventstream.h"

#include <utility>

RCLAP_BEGIN_NAMESPACE

CqEventHandler::CqEventHandler(Server *parent, std::unique_ptr<grpc::ServerCompletionQueue> cq)
    : parent(parent), cq(std::move(cq))
{
    if (!Log::Private::gLoggerSetup)
        Log::setupLogger("cqeventhandler");
}

CqEventHandler::CqEventHandler(CqEventHandler &&other) noexcept
{
    *this = std::move(other);
}

CqEventHandler &CqEventHandler::operator=(CqEventHandler &&other) noexcept
{
    if (this == &other)
        return *this;
    parent = std::exchange(other.parent, nullptr);
    cq = std::move(other.cq);
    pendingAlarmTags = std::move(other.pendingAlarmTags);
    pendingTags = std::move(other.pendingTags);
    return *this;
}

bool CqEventHandler::enqueueFn(EventTag::FnType &&f, std::uint64_t deferNs /*= 0*/)
{
    // Ref: https://www.gresearch.com/blog/article/lessons-learnt-from-writing-asynchronous-streaming-grpc-services-in-c/
    auto eventFn = pendingAlarmTags.try_emplace(
        std::make_unique<EventTag>(this, std::move(f)),
        grpc::Alarm()
    );

    if (!eventFn.second) {
        SPDLOG_ERROR("Failed to enqueue AlarmTag");
        return false;
    }

    if (deferNs == 0) { // Execute the function immediately.
        eventFn.first->second.Set(
            cq.get(), gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME), toTag(eventFn.first->first.get())
        );
    } else { // Otherwise, defer the function for deferNs nanoseconds. Note that this is using a syscall
        const auto tp = gpr_time_from_nanos(deferNs, GPR_TIMESPAN);
        eventFn.first->second.Set(cq.get(), tp, toTag(eventFn.first->first.get()));
    }

    return true;
}
// TODO: This is not optimal, find a better way to organize and sync with shareddata...
// See also servereventstream::kill
bool CqEventHandler::destroyTag(std::uint64_t hash)
{
    if (pendingTags.erase(hash) != 0) { // We have removed something
        return true;
    }
    SPDLOG_ERROR("Failed to destroyTag tag {}", hash);
    return false;
}

bool CqEventHandler::destroyAlarmTag(EventTag *tag)
{
    auto findFromRawTag = [tag](const auto &pair) {
        return pair.first.get() == tag;
    };

    auto it = std::find_if(pendingAlarmTags.begin(), pendingAlarmTags.end(), findFromRawTag);
    if (it == pendingAlarmTags.end()) {
        SPDLOG_WARN("Failed to destroy AlarmTag tag {}", toTag(tag));
        return false;
    }
    pendingAlarmTags.erase(it);
    return true;
}

void CqEventHandler::cancelAllPendingTags()
{
    for (auto &pair : pendingAlarmTags)
        pair.second.Cancel();
}

void CqEventHandler::teardown()
{
    cq->Shutdown();
    // Drains all remaining events from the completion queue. If any
    void *rawTag = nullptr;
    bool ok = false;
    while (cq->Next(&rawTag, &ok)) {
        auto *tag = static_cast<EventTag*>(rawTag);
        tag->process(ok);
    };
}

void CqEventHandler::run()
{
    state = RUNNING;

    void *rawTag = nullptr;
    bool ok = false;
    // Block until the next event is available in the completion queue.
    while (cq->Next(&rawTag, &ok)) {
        auto *tag = static_cast<EventTag*>(rawTag);
        tag->process(ok);
    }

    // We are finished. All tags must be destroyed by now!.
    while(!pendingTags.empty() && !pendingAlarmTags.empty()) {
        SPDLOG_WARN("Pending Tags: {}, Pending Alarms: {}", pendingTags.size(), pendingAlarmTags.size());
    }

    state = SHUTDOWN;
}

ClapInterface::AsyncService *CqEventHandler::service() noexcept
{
    return parent->service();
}

template <typename T>
bool CqEventHandler::create()
{ return false; }

template <>
bool CqEventHandler::create<ClientEventCallHandler>()
{
    try {
        auto client = std::make_unique<ClientEventCallHandler>(this, cq.get());
        const auto h = client->hash();
        auto res = pendingTags.try_emplace(h, std::move(client));
        if (!res.second) {
            SPDLOG_ERROR("Failed to create UnaryCallHandler");
            return false;
        }
    } catch (const std::exception &e) {
        SPDLOG_CRITICAL("{}", e.what());
        return false;
    }
    return true;
}

template <>
bool CqEventHandler::create<ClientParamCall>()
{
    try {
        auto client = std::make_unique<ClientParamCall>(this, cq.get());
        const auto h = client->hash();
        auto res = pendingTags.try_emplace(h, std::move(client));
        if (!res.second) {
            SPDLOG_ERROR("Failed to create UnaryCallHandler");
            return false;
        }
    } catch (const std::exception &e) {
        SPDLOG_CRITICAL("{}", e.what());
        return false;
    }
    return true;
}

template <>
bool CqEventHandler::create<ServerEventStream>()
{
    try {
        auto client = std::make_unique<ServerEventStream>(this, cq.get());
        const auto h = client->hash();
        auto res = pendingTags.try_emplace(h, std::move(client));
        if (!res.second) {
            SPDLOG_ERROR("Failed to create ServerEventStream");
            return false;
        }
    } catch (const std::exception &e) {
        SPDLOG_CRITICAL("{}", e.what());
        return false;
    }
    return true;
}

RCLAP_END_NAMESPACE
