#ifndef EVENTTAG_H
#define EVENTTAG_H

#include <api.pb.h>
#include <api.grpc.pb.h>
using namespace api::v0;

#include <core/global.h>
#include <core/timestamp.h>

#include <functional>

RCLAP_BEGIN_NAMESPACE

class CqEventHandler;
using GrpcMetadata = const std::multimap<grpc::string_ref, grpc::string_ref>&;

// EventTag is the base class for all tags inside a completion queue.
// It provides a callback function that is used for polling.
// This class is meant to be used as an interface class, but shouldn't be used directly
// except for polling.
class EventTag
{
public:
    using FnType = std::function<void(bool)>;

    explicit EventTag(CqEventHandler *parent);
    explicit EventTag(CqEventHandler *parent, FnType &&fn);
    virtual ~EventTag();

    EventTag(EventTag &&other) noexcept;
    EventTag &operator=(EventTag &&other) noexcept;

    EventTag(const EventTag &) = delete;
    EventTag &operator=(const EventTag &) = delete;

    void setFn(FnType &&fn) noexcept;

    [[nodiscard]] virtual std::size_t hash() const noexcept { return 0; }
    virtual void process(bool ok);
    virtual void kill();

protected:
    CqEventHandler *parent = nullptr;
    ClapInterface::AsyncService *service = nullptr;

    std::unique_ptr<FnType> func{};
    Timestamp ts{};
};

RCLAP_END_NAMESPACE

#endif // EVENTTAG_H
