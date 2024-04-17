#pragma once

#include <clap-rci/coreplugin.h>
#include <clap-rci/gen/rci.grpc.pb.h>
#include <clap-rci/gen/rci.pb.h>
#include <clap-rci/global.h>

#include <absl/log/log.h>

CLAP_RCI_BEGIN_NAMESPACE

class TransportWatcher
{
public:
    bool update(const clap_event_transport& other);
    const api::PluginEventMessage& message() const { return mMessage; }

    [[nodiscard]] bool isPlaying() const noexcept;
    [[nodiscard]] bool isRecording() const noexcept;
    [[nodiscard]] bool isLoopActive() const noexcept;
    [[nodiscard]] bool isWithinPreRoll() const noexcept;

    bool equalsPosition(const clap_event_transport& other) const;
    bool equalsTempo(const clap_event_transport& other) const;
    bool equalsLoop(const clap_event_transport& other) const;
    bool equalsTimeSignature(const clap_event_transport& other) const;

private:
    clap_event_transport mCurrent = {};
    api::PluginEventMessage mMessage;

    api::TransportPosition mPosition;
    api::TransportTempo mTempo;
    api::TransportLoop mLoop;
    api::TransportTimeSignature mTimeSignature;
};

inline bool TransportWatcher::isPlaying() const noexcept
{
    return (mCurrent.flags & CLAP_TRANSPORT_IS_PLAYING) != 0;
}

inline bool TransportWatcher::isRecording() const noexcept
{
    return (mCurrent.flags & CLAP_TRANSPORT_IS_RECORDING) != 0;
}

inline bool TransportWatcher::isLoopActive() const noexcept
{
    return (mCurrent.flags & CLAP_TRANSPORT_IS_LOOP_ACTIVE) != 0;
}

inline bool TransportWatcher::isWithinPreRoll() const noexcept
{
    return (mCurrent.flags & CLAP_TRANSPORT_IS_WITHIN_PRE_ROLL) != 0;
}

inline bool TransportWatcher::equalsPosition(const clap_event_transport& other
) const
{
    return mCurrent.song_pos_beats == other.song_pos_beats
           && mCurrent.song_pos_seconds == other.song_pos_seconds;
}
inline bool TransportWatcher::equalsTempo(const clap_event_transport& other
) const
{
    return mCurrent.tempo == other.tempo
           && mCurrent.tempo_inc == other.tempo_inc;
}

inline bool TransportWatcher::equalsLoop(const clap_event_transport& other
) const
{
    return mCurrent.loop_start_beats == other.loop_start_beats
           && mCurrent.loop_end_beats == other.loop_end_beats
           && mCurrent.loop_start_seconds == other.loop_start_seconds
           && mCurrent.loop_end_seconds == other.loop_end_seconds;
}

inline bool
TransportWatcher::equalsTimeSignature(const clap_event_transport& other) const
{
    return mCurrent.tsig_num == other.tsig_num
           && mCurrent.tsig_denom == other.tsig_denom;
}

CLAP_RCI_END_NAMESPACE
