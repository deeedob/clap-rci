#ifndef BLKRINGQUEUE_H
#define BLKRINGQUEUE_H

#include "global.h"

#include <semaphore>
#include <utility>
#include <memory>
#include <chrono>

#include <atomic>

RCLAP_BEGIN_NAMESPACE

// A blocking and fast single-reader single-writer queue.
template <typename T>
class BlkRingQueue
{
public:
    BlkRingQueue() : slots(std::counting_semaphore<>::max()), items(0) {}
    explicit BlkRingQueue(std::size_t cap) : capacity(cap), slots(static_cast<ptrdiff_t>(cap)), items(0)
    {
        // Fast modulo mask hack. Only works for power-of-2 capacities.
        // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
        --cap;
        cap |= cap >> 1;
        cap |= cap >> 2;
        cap |= cap >> 4;
        for (std::size_t i = 1; i < sizeof(std::size_t); i <<= 1)
            cap |= cap >> (i << 3);
        mask = cap++;
        data = new T[cap]();
    }

    ~BlkRingQueue() {
        for (std::size_t i = 0, n = size(); i != n; ++i) {
            std::size_t idx = (nextItem + i) & mask;
            data[idx].~T();
        }
    }

    BlkRingQueue(BlkRingQueue &&other) noexcept : BlkRingQueue() {
        swap(other);
    }

    BlkRingQueue &operator=(BlkRingQueue &&other) noexcept {
        swap(other);
        return *this;
    }

    BlkRingQueue(const BlkRingQueue &) = delete;
    BlkRingQueue &operator=(const BlkRingQueue &) = delete;

    void swap(BlkRingQueue &other) noexcept
    {
        std::swap(data, other.data);
        std::swap(capacity, other.capacity);
        std::swap(mask, other.mask);
//        std::swap(slots, other.slots); TODO: MinGW doesn't like this?
//        std::swap(items, other.items);
        std::swap(nextSlot, other.nextSlot);
        std::swap(nextItem, other.nextItem);
    }

    bool tryEnqueue(T &&item) {
        if (!slots.try_acquire())
            return false;
        enqueueImpl(std::move(item));
        return true;
    }

    bool tryEnqueue(const T &item) {
        if (!slots.try_acquire())
            return false;
        enqueueImpl(item);
        return true;
    }

    void waitEnqueue(T &&item) {
        slots.acquire();
        enqueueImpl(std::move(item));
    }

    void waitEnqueue(const T &item) {
        slots.acquire();
        enqueueImpl(item);
    }

    template <typename Rep, typename Period>
    inline bool waitEnqueueTimed(T &&item, const std::chrono::duration<Rep, Period> &timeout) {
        if (!slots.try_acquire_for(timeout))
            return false;
        enqueueImpl(std::move(item));
        return true;
    }

    template <typename Rep, typename Period>
    inline bool waitEnqueueTimed(const T &item, const std::chrono::duration<Rep, Period> &timeout) {
        if (!slots.try_acquire_for(timeout))
            return false;
        enqueueImpl(item);
        return true;
    }

    template <typename U>
    bool tryDequeue(U &item) {
        if (!items.try_acquire())
            return false;
        dequeueImpl(item);
        return true;
    }

    template <typename U>
    void waitDequeue(U &item) {
        items.acquire();
        dequeueImpl(item);
    }

    template <typename U, typename Rep, typename Period>
    inline bool waitDequeueTimed(U &item, const std::chrono::duration<Rep, Period> &timeout) {
        if (!items.try_acquire_for(timeout))
            return false;
        dequeueImpl(item);
        return true;
    }

    inline T *peek() {
       if (!size())
            return nullptr;
        return peekImpl();
    }

    inline bool tryPop() {
        if (!items.try_acquire())
            return false;
        popImpl();
        slots.release();
        return true;
    }

    [[nodiscard]] inline std::size_t size() const {
        return itemCount.load();
    }

    [[nodiscard]] inline std::size_t maxCapacity() const {
        return capacity;
    }

private:
    template <typename U>
    void enqueueImpl(U &&item) {
        new (&data[nextSlotIdx()]) T(std::forward<U>(item));
        itemCount.fetch_add(1);
        items.release();
    }

    template <typename U>
    void dequeueImpl(U &item) {
        T &elem = data[nextItemIdx()];
        item = std::move(elem);
        elem.~T();
        itemCount.fetch_sub(1);
        slots.release();
    }

    T *peekImpl() const {
        return &data[itemIdx()];
    }

    void popImpl() {
        data[nextItemIdx()].~T();
    }

    inline auto itemIdx() const {
        return nextItem & mask;
    }

    inline auto nextItemIdx() {
        return nextItem++ & mask;
    };

    inline auto nextSlotIdx() {
        return nextSlot++ & mask;
    };

private:
    T *data = nullptr;
    std::size_t capacity = {};

    std::counting_semaphore<> slots;
    std::counting_semaphore<> items;

    std::atomic<std::size_t> itemCount = 0;
    static_assert(std::atomic<std::size_t>::is_always_lock_free);

    std::size_t mask = {}; // capacity mask for cheap modulo

    std::size_t nextSlot = {};
    std::size_t nextItem = {};
};

RCLAP_END_NAMESPACE

#endif
