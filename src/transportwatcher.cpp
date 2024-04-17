#include "transportwatcher.hpp"

CLAP_RCI_BEGIN_NAMESPACE

bool TransportWatcher::update(const clap_event_transport& other)
{
    uint32_t updatedFields = 0;
    if (mCurrent.flags != other.flags) {
        mMessage.mutable_transport()->set_flags(other.flags);
        updatedFields |= 1u;
    }
    if (!equalsPosition(other)) {
        mPosition.set_beats(other.song_pos_beats);
        mPosition.set_seconds(other.song_pos_seconds);
        updatedFields |= 2u;
    }
    if (!equalsTempo(other)) {
        mTempo.set_value(other.tempo);
        mTempo.set_increment(other.tempo_inc);
        updatedFields |= 4u;
    }
    if (!equalsLoop(other)) {
        mLoop.set_start_beats(other.loop_start_beats);
        mLoop.set_end_beats(other.loop_end_beats);
        mLoop.set_start_seconds(other.loop_start_seconds);
        mLoop.set_end_seconds(other.loop_end_seconds);
        updatedFields |= 8u;
    }
    if (!equalsTimeSignature(other)) {
        mTimeSignature.set_numerator(other.tsig_num);
        mTimeSignature.set_denominator(other.tsig_denom);
        updatedFields |= 16u;
    }

    const auto nFields = __builtin_popcount(updatedFields);
    if (nFields <= 0)
        return false;

    mCurrent = other;
    if (nFields == 1) {
        switch (updatedFields) {
            case 1: {
                auto* transport = mMessage.mutable_transport()
                                      ->mutable_transport_all();
                transport->mutable_position()->Swap(&mPosition);
                transport->mutable_tempo()->Swap(&mTempo);
                transport->mutable_loop()->Swap(&mLoop);
                transport->mutable_time_sig()->Swap(&mTimeSignature);
            } break;
            case 2: {
                mMessage.mutable_transport()->mutable_position()->Swap(
                    &mPosition
                );
            } break;
            case 4:
                mMessage.mutable_transport()->mutable_tempo()->Swap(&mTempo);
                break;
            case 8:
                mMessage.mutable_transport()->mutable_loop()->Swap(&mLoop);
                break;
            case 16:
                mMessage.mutable_transport()->mutable_time_sig()->Swap(
                    &mTimeSignature
                );
                break;
            default:
                LOG(FATAL) << "Transportwatcher should never reach this!";
                break;
        }
    } else {
        auto* transport = mMessage.mutable_transport()->mutable_transport_all();
        transport->mutable_position()->Swap(&mPosition);
        transport->mutable_tempo()->Swap(&mTempo);
        transport->mutable_loop()->Swap(&mLoop);
        transport->mutable_time_sig()->Swap(&mTimeSignature);
    }

    return true;
}

CLAP_RCI_END_NAMESPACE
