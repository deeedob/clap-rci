#ifndef WRAPPERS_H
#define WRAPPERS_H

#include <core/global.h>
#include <api.pb.h>
#include <api.grpc.pb.h>
using namespace api::v0;

#include <clap/events.h>

#include <variant>

RCLAP_BEGIN_NAMESPACE

struct ClapEventNoteWrapper {
    ClapEventNoteWrapper(const clap_event_note* note, uint32_t type)
       : noteId(note->note_id), portIndex(note->port_index), channel(note->channel),
         key(note->key), value(note->velocity), type(type)
    {
        assert(type != CLAP_EVENT_NOTE_EXPRESSION);
    }
    ClapEventNoteWrapper(const clap_event_note_expression* expr, uint32_t type, int32_t exprType)
       : noteId(expr->note_id), portIndex(expr->port_index), channel(expr->channel), key(expr->key),
         value(expr->value), type(type), expression(exprType)
    {
        assert(type == CLAP_EVENT_NOTE_EXPRESSION);
    }

    [[nodiscard]] ClapEventNote::Type getType() const {
        return static_cast<ClapEventNote::Type>(type);
    }
    [[nodiscard]] ClapEventNote::ExpressionType getExpressionType() const {
        return static_cast<ClapEventNote::ExpressionType>(expression);
    }

    int32_t noteId = 0;
    int32_t portIndex = 0;
    int32_t channel = 0;
    int32_t key = 0;
    double value = 0;
    uint32_t type = 0;
    int32_t expression = ClapEventNote_ExpressionType_None;
};

struct ClapEventParamWrapper {
    ClapEventParamWrapper(const clap_event_param_value* param)
       : type(ClapEventParam_Type_Value), paramId(param->param_id), value(param->value)
    {}
    ClapEventParamWrapper(const clap_event_param_mod* param)
       : type(ClapEventParam_Type_Modulation), paramId(param->param_id), modulation(param->amount)
    {}
    ClapEventParam::Type type = ClapEventParam_Type_Value;
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
        : ev(Event::EventInvalid), data(ClapEventMainSyncWrapper()) {}
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
