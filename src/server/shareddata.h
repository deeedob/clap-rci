#ifndef SHARED_H
#define SHARED_H
#include <core/logging.h>
#include <core/global.h>
#include <core/timestamp.h>
#include <core/blkringqueue.h>
#include "wrappers.h"

#include <farbot/fifo.hpp>

#include <set>
#include <memory>
#include <optional>

RCLAP_BEGIN_NAMESPACE

template <typename T>
using SPMRQueue = farbot::fifo<T,
    farbot::fifo_options::concurrency::multiple,                // consumer: server-thread
    farbot::fifo_options::concurrency::single,                  // producer: audio-thread
    farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
    farbot::fifo_options::full_empty_failure_mode::overwrite_or_return_default>;
template <typename T>
using MPMRQueue = farbot::fifo<T,
    farbot::fifo_options::concurrency::multiple,
    farbot::fifo_options::concurrency::multiple,
    farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
    farbot::fifo_options::full_empty_failure_mode::overwrite_or_return_default>;
template <typename T>
using SPSC = farbot::fifo<T,
    farbot::fifo_options::concurrency::single,
    farbot::fifo_options::concurrency::single,
    farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
    farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty>;

class CorePlugin;
class ServerEventStream;
class CqEventHandler;

// The shared data between <Audio, Main> <=> <Server, CQs>
class SharedData
{
public:
    explicit SharedData(CorePlugin *plugin);

    bool addCorePlugin(CorePlugin *plugin);
    bool addStream(ServerEventStream *stream);
    bool removeStream(ServerEventStream *stream);
    std::optional<ServerEventStream*> findStream(ServerEventStream *that);

    // Used to sync Main <-> ClientEventCall
    bool blockingVerifyEvent(Event e);
    bool blockingPushClientEvent(Event e);

    void pushClientParam(const ClientParams &ev);

    [[nodiscard]] std::size_t nStreams() const;
    [[nodiscard]] bool isValid() const noexcept;

    auto &pluginToClientsQueue() { return mPluginProcessToClientsQueue; }
    auto &pluginMainToClientsQueue() { return mPluginMainToClientsQueue; }
    auto &clientsToPluginQueue() { return mClientsToPluginQueue; }

    bool tryStartPolling();
    bool stopPoll();
    bool isPolling() const noexcept { return pollRunning; }

private:
    size_t drainPollingQueue();
    // Polling Callback responsible for handling all events from the plugin.
    void pollCallback(bool ok);
    uint64_t nextExpBackoff();

    std::string evToString(const Event &ev)
    {
        switch (ev) {
            case Event::GuiCreate: return "GuiCreate";
            case Event::GuiDestroy: return "GuiDestroy";
            case Event::GuiSetTransient: return "GuiSetTransient";
            case Event::GuiShow: return "GuiShow";
            case Event::GuiHide: return "GuiHide";
            case Event::PluginActivate: return "PluginActivate";
            case Event::PluginDeactivate: return "PluginDeactivate";
            case Event::PluginStartProcessing: return "PluginStartProcessing";
            case Event::PluginStopProcessing: return "PluginStopProcessing";
            case Event::ParamInfo: return "ParamInfo";
            case Event::ParamValue: return "ParamValue";
            case Event::ParamModulation: return "ParamModulation";
            case Event::PluginReset: return "PluginReset";
            case Event::NoteOn: return "NoteOn";
            case Event::NoteOff: return "NoteOff";
            case Event::NoteChoke: return "NoteChoke";
            case Event::NoteEnd: return "NoteEnd";
            default: return "Unknown/Unhandled Event";
        }
    }
    uint64_t consumeEventToStream(auto &queue) {
        ServerEventWrapper out;
        // consume all events and copy them to our response.
        uint64_t cnt = 0;
        while(queue.pop(out)) {
            ++cnt;
            auto *next = mPluginToClientsData.mutable_events()->Add();
            // ServerEventWrapper syncs with ServerEvent. Collect the types.
            std::visit([&](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                next->set_event(out.ev);
                SPDLOG_TRACE("Consumed event: {}", evToString(out.ev));
                if constexpr (std::is_same_v<T, ClapEventNoteWrapper>) {
                    next->mutable_note()->set_note_id(arg.noteId);
                    next->mutable_note()->set_port_index(arg.portIndex);
                    next->mutable_note()->set_channel(arg.channel);
                    next->mutable_note()->set_key(arg.key);
                    next->mutable_note()->set_velocity(arg.velocity);
                    SPDLOG_TRACE("Consumed note: {}", arg.key);
                } else if constexpr (std::is_same_v<T, ClapEventParamWrapper>) {
                    next->mutable_param()->set_param_id(arg.paramId);
                    next->mutable_param()->set_value(arg.value);
                    next->mutable_param()->set_modulation(arg.modulation);
                } else if constexpr (std::is_same_v<T, ClapEventParamInfoWrapper>) {
                    next->mutable_param_info()->set_param_id(arg.paramId);
                    next->mutable_param_info()->set_name(arg.name);
                    next->mutable_param_info()->set_module(arg.module);
                    next->mutable_param_info()->set_min_value(arg.minValue);
                    next->mutable_param_info()->set_max_value(arg.maxValue);
                    next->mutable_param_info()->set_default_value(arg.defaultValue);
                } else if constexpr (std::is_same_v<T, ClapEventMainSyncWrapper>) {
                    next->mutable_main_sync()->set_window_id(arg.windowId);
                } else {
                    assert(false);
                }
            }, out.data);
        }
        return cnt;
    }

private:
    CorePlugin *coreplugin = nullptr;
    std::set<ServerEventStream*> streams;

    // Poll callback
    ServerEvents mPluginToClientsData; // GRPC server response
    CqEventHandler *mServerStreamCq = nullptr;
    std::atomic<bool> pollRunning = false;
    std::atomic<bool> pollStop = false;
    static constexpr uint64_t mPollFreqNs = 5'000; // 5 us
    static constexpr uint64_t mExpBackoffLimitNs = 250'000;
    uint64_t mCurrExpBackoff = mPollFreqNs;

    // Plugin -> Clients
    SPMRQueue<ServerEventWrapper> mPluginProcessToClientsQueue;
    MPMRQueue<ServerEventWrapper> mPluginMainToClientsQueue;

    // Clients -> Plugin (Blocking)
    BlkRingQueue<Event> mBlockingClientEventQueue { 1 };
    std::mutex mBlockingClientEventQueueMtx;

    // Clients -> Plugin
    SPSC<ClientParamWrapper> mClientsToPluginQueue;
    Stamp mLastClientStamp;
};

RCLAP_END_NAMESPACE

#endif // SHARED_H
