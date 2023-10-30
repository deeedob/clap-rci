#ifndef WRAPPERS_H
#define WRAPPERS_H

#include <core/global.h>
#include <api.pb.h>
#include <api.grpc.pb.h>
using namespace api::v0;

#include <variant>

RCLAP_BEGIN_NAMESPACE

struct ClapEventNoteWrapper {
    int32_t noteId = 0;
    int32_t portIndex = 0;
    int32_t channel = 0;
    int32_t key = 0;
    double velocity = 0;
};

struct ClapEventParamWrapper {
    uint32_t paramId = 0;
    double value = 0;
    double modulation = 0;
};

struct ClapEventParamInfoWrapper {
    uint32_t  paramId = 0;
    std::string name;
    std::string module;
    double minValue = 0;
    double maxValue = 0;
    double defaultValue = 0;
};

struct ClapEventMainSyncWrapper {
    int64_t windowId = 0;
};

struct ServerEventWrapper
{
    using T = std::variant<
       ClapEventNoteWrapper, ClapEventParamWrapper,
       ClapEventParamInfoWrapper, ClapEventMainSyncWrapper
    >;

    ServerEventWrapper()
        : ev(Event::EventInvalid), data(ClapEventParamWrapper()) {}
    ServerEventWrapper(Event e, ClapEventNoteWrapper &&data)
        : ev(e), data(std::move(data)) {}
    ServerEventWrapper(Event e, ClapEventParamWrapper &&data)
        : ev(e), data(std::move(data)) {}
    ServerEventWrapper(Event e, ClapEventParamInfoWrapper &&data)
        : ev(e), data(std::move(data)) {}
    ServerEventWrapper(Event e, ClapEventMainSyncWrapper &&data)
        : ev(e), data(std::move(data)) {}

    Event ev;
    T data;
};

struct ClientParamWrapper
{
    ClientParamWrapper()
        : ev(Event::EventInvalid), paramId(0), value(0) {}
    explicit ClientParamWrapper(const ClientParam &other)
        : ev(other.event()), paramId(other.param().param_id()), value(other.param().value()) {}
    Event ev;
    uint32_t paramId;
    double value;
};

RCLAP_END_NAMESPACE

#endif // WRAPPERS_H
