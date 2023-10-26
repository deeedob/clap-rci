#include <core/logging.h>
#include "shareddata.h"
#include "server/tags/servereventstream.h"
#include "serverctrl.h"
#include "cqeventhandler.h"
#include <plugin/coreplugin.h>

#include <crill/progressive_backoff_wait.h>

RCLAP_BEGIN_NAMESPACE

SharedData::SharedData(CorePlugin *plugin)
    : coreplugin(plugin), mPluginProcessToClientsQueue(64), mPluginMainToClientsQueue(64), mClientsToPluginQueue(64)
{
    assert(plugin != nullptr);
}

bool SharedData::addCorePlugin(CorePlugin *plugin)
{
    assert(plugin != nullptr);
    if (coreplugin != nullptr)
        return false;
    coreplugin = plugin;
    return true;
}

bool SharedData::addStream(ServerEventStream *stream)
{
    assert(stream != nullptr);
    return streams.insert(stream).second;
}

bool SharedData::removeStream(ServerEventStream *stream)
{
    assert(stream != nullptr);
    return streams.erase(stream) == 1;
}

std::size_t SharedData::nStreams() const
{
    return streams.size();
}

std::optional<ServerEventStream*> SharedData::findStream(ServerEventStream *that)
{
    const auto it = std::find_if(streams.begin(), streams.end(), [&](const auto &stream) {
        return stream == that;
    });
    if (it == streams.end())
        return std::nullopt;
    return *it;
}

bool SharedData::isValid() const noexcept
{
    return coreplugin && !streams.empty();
}

bool SharedData::blockingVerifyEvent(Event e) {
    Event extracted;
    SPDLOG_TRACE("Waiting for event: {}", static_cast<int>(e));
    bool ok = mBlockingClientEventQueue.waitDequeueTimed(extracted, ServerCtrl::instance().getInitTimeout());
    if (!ok) {
        SPDLOG_ERROR("create error; No response from GUI proc");
        return false;
    }

    if (extracted != e) {
        SPDLOG_ERROR("Unexpected event! Got: {}, Want: {}", static_cast<int>(extracted), static_cast<int>(e));
        return false;
    }

    return true;
}

// The previous event _has_ to be consumed. This is a blocking call and can only enqueue a single event.
// It is used to synchronize the GUI startup.
// TODO: Upon switch to client-streams/bi-dir streams, improve this.
bool SharedData::blockingPushClientEvent(Event e)
{
    return mBlockingClientEventQueue.waitEnqueueTimed(e, ServerCtrl::instance().getInitTimeout());
}

// Potentially blocks the Client upon equeueing.
void SharedData::pushClientParam(const ClientParams &ev)
{
    for (const auto &p : ev.params()) {
        // If the timestamp is older than the last one we will discard this event to avoid
        // incorrect behavior if multiple clients are connected.
        if (!mLastClientStamp.setIfNewer(p.timestamp().seconds(), p.timestamp().nanos())) {
            SPDLOG_TRACE("ClientParam timestamp is in the past");
            continue;
        }
        crill::progressive_backoff_wait([&] {
            SPDLOG_TRACE("Pushing client param: {}s {}ns, value {}", p.timestamp().seconds(), p.timestamp().nanos(), p.param().value());
            return mClientsToPluginQueue.push(ClientParamWrapper(p));
        });
    }
}

// Start polling. This will run the callback and enqueue itself again, if there are active clients.
// We poll all events out of the queues and send them to _all_ connected clients.
bool SharedData::tryStartPolling()
{
    if (!isValid() || pollRunning)
        return false;

    auto cq = ServerCtrl::instance().tryGetServerStreamCqHandle();
    if (cq && *cq != nullptr) {
        mServerStreamCq = *cq;
    } else {
        SPDLOG_ERROR("Failed to get CqEventHandler");
        return false;
    }

    pollRunning = true;
    mServerStreamCq->enqueueFn([this](bool ok){ this->pollCallback(ok); }, mPollFreqNs);
    return true;
}

void SharedData::pollCallback(bool ok)
{
    static auto endCallback = [&]() -> void {
        pollRunning = false;
        drainPollingQueue();
    };

    if (!ok || !coreplugin || !mServerStreamCq) {
        SPDLOG_TRACE(
            "PollCallback stopped: ok: {}, coreplugin: {}, mServerStreamCq: {}",
            ok, coreplugin == nullptr, mServerStreamCq == nullptr
        );
        return endCallback();
    }

    if (streams.empty()) {
        SPDLOG_ERROR("No streams connected. Stop polling.");
        return endCallback();
    }

    if (pollStop) { // Signal the end of streaming to our clients.
        for (auto stream : streams) {
            if (!stream->endStream())
                SPDLOG_ERROR("Failed to end stream {}", toTag(stream));
        }
        SPDLOG_TRACE("PollCallback stopped: stop signal received");
        return endCallback();
    }

    const auto nProcessEvs = consumeEventToStream(mPluginProcessToClientsQueue); // Consume events from RT audio thread
    const auto nMainEvs = consumeEventToStream(mPluginMainToClientsQueue); // Consume events from main thread
//    SPDLOG_TRACE("{} {}, time: {}", nProcessEvs, nMainEvs, mCurrExpBackoff);
    if (nProcessEvs == 0 && nMainEvs == 0) {
        // We have no events to send, so we can just wait for the next callback with an increased backoff.
        mServerStreamCq->enqueueFn([this](bool ok){ this->pollCallback(ok); }, nextExpBackoff());
        return;
    }
    // If we reached this point, we have events to send.
    bool success = false;
    for (auto stream : streams) {       // For all streams/clients
        if (stream->sendEvents(mPluginToClientsData))     // try to pump some events.
            success = true;
    }

    if (!success) {
        SPDLOG_ERROR("Failed to send events to {} clients.", streams.size());
        mServerStreamCq->enqueueFn([this](bool ok){ this->pollCallback(ok); }, nextExpBackoff());
        return;
    }

    // Succefully completed a round. Enqueue the next callback with refgular poll-frequency and reset the backoff.
    mPluginToClientsData.Clear();
    mCurrExpBackoff = mPollFreqNs;
    mServerStreamCq->enqueueFn([this](bool ok){ this->pollCallback(ok); }, mPollFreqNs);
    SPDLOG_TRACE("PollCallback has sent: {} Process Events and {} Main Events", nProcessEvs, nMainEvs);
}

bool SharedData::stopPoll()
{
    if (!coreplugin || !mServerStreamCq || !pollRunning) {
        SPDLOG_TRACE(
            "stopPoll: coreplugin: {}, mServerStreamCq: {}, pollRunning: {}",
             coreplugin == nullptr, mServerStreamCq == nullptr, pollRunning
        );
        return false;
    }

    pollStop = true;
    return true;
}


size_t SharedData::drainPollingQueue()
{
    size_t cnt = 0;
    ServerEventWrapper tmp;
    while (mPluginProcessToClientsQueue.pop(tmp))
        ++cnt;
    while (mPluginMainToClientsQueue.pop(tmp))
        ++cnt;
    SPDLOG_TRACE("Drained {} events from polling queue", cnt);
    return cnt;
}


uint64_t SharedData::nextExpBackoff()
{
    mCurrExpBackoff = (mCurrExpBackoff < mExpBackoffLimitNs) ? mCurrExpBackoff * 2 : mExpBackoffLimitNs;
    return mCurrExpBackoff;
}
RCLAP_END_NAMESPACE
