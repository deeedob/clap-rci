#include <core/logging.h>
#include "eventtag.h"
#include "../cqeventhandler.h"

#include <utility>

RCLAP_BEGIN_NAMESPACE

EventTag::EventTag(CqEventHandler *parent) : parent(parent), service(parent->service()) {}

EventTag::EventTag(CqEventHandler *parent, FnType &&fn)
    : parent(parent), service(parent->service()), func(std::make_unique<FnType>(std::move(fn)))
{
}

EventTag::~EventTag() = default;

EventTag::EventTag(EventTag &&other) noexcept
{
    *this = std::move(other);
}

EventTag &EventTag::operator=(EventTag &&other) noexcept
{
    if (this == &other)
        return *this;
    ts = other.ts;
    func = std::move(other.func);
    parent = std::exchange(other.parent, nullptr);
    return *this;
}

void EventTag::process(bool ok)
{
    (*func)(ok);
    kill();
}

void EventTag::kill()
{
    parent->destroyAlarmTag(this);
}

void EventTag::setFn(FnType &&fn) noexcept
{
    func = std::make_unique<FnType>(std::move(fn));
}

RCLAP_END_NAMESPACE