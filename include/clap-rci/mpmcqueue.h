#include <clap-rci/global.h>

#include <array>
#include <atomic>

CLAP_RCI_BEGIN_NAMESPACE

template <typename T, size_t Size>
    requires(Size >= 2 && (Size & (Size - 1)) == 0)
class MpMcQueue
{

public:
    MpMcQueue()
    {
        for (size_t i = 0; i != Size; i += 1)
            mBuffer[i].sequence.store(i, std::memory_order_relaxed);
        mHead.store(0, std::memory_order_relaxed);
        mTail.store(0, std::memory_order_relaxed);
    }
    ~MpMcQueue() = default;
    MpMcQueue(const MpMcQueue&) = delete;
    void operator=(const MpMcQueue&) = delete;
    MpMcQueue(MpMcQueue&&) = default;
    MpMcQueue& operator=(MpMcQueue&&) = default;

    bool push(const T& data)
    {
        if (!tryPush(data)) {
            T temp;
            if (!pop(&temp))
                return false;
            if (!tryPush(data))
                return false;
        }
        return true;
    }

    bool tryPush(auto&& data)
    {
        size_t pos = mHead.load(std::memory_order_relaxed);
        while (true) {
            Cell* cell = &mBuffer[pos & mBufferMask];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t dif = static_cast<intptr_t>(seq)
                           - static_cast<intptr_t>(pos);
            if (dif == 0) { // Try to advance the enqueue pos
                if (mHead.compare_exchange_weak(
                        pos, pos + 1, std::memory_order_relaxed
                    ))
                    break;
            } else if (dif < 0) { // Queue is full
                return false;
            } else {
                pos = mHead.load(std::memory_order_relaxed);
            }
        }
        Cell* cell = &mBuffer[pos & mBufferMask];
        cell->data = std::forward<decltype(data)>(data);
        cell->sequence.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool pop(T* data)
    {
        size_t pos = mTail.load(std::memory_order_relaxed);
        while (true) {
            Cell* cell = &mBuffer[pos & mBufferMask];
            size_t seq = cell->sequence.load(std::memory_order_acquire);
            intptr_t dif = static_cast<intptr_t>(seq)
                           - static_cast<intptr_t>(pos + 1);
            if (dif == 0) { // Try to advance the dequeue pos
                if (mTail.compare_exchange_weak(
                        pos, pos + 1, std::memory_order_relaxed
                    ))
                    break;
            } else if (dif < 0) { // Queue is empty
                return false;
            } else { // Retry from current position
                pos = mTail.load(std::memory_order_relaxed);
            }
        }
        Cell* cell = &mBuffer[pos & mBufferMask];
        *data = cell->data;
        // Increase sequence by buffer size for wrap-around
        cell->sequence.store(pos + Size, std::memory_order_release);
        return true;
    }

    size_t size() const { return mHead.load() - mTail.load(); }

    bool isEmpty() const { return size() <= 0; }

private:
    struct Cell {
        std::atomic<size_t> sequence;
        T data;
    };
    std::array<Cell, Size> mBuffer = {};
    const size_t mBufferMask = Size - 1;
    alignas(64) std::atomic<size_t> mHead;
    alignas(64) std::atomic<size_t> mTail;
    static_assert(std::atomic<size_t>::is_always_lock_free);
};

CLAP_RCI_END_NAMESPACE
