#ifndef EVT_RING_QUEUE_H
#define EVT_RING_QUEUE_H

#include "global.h"
#include "semaphores.h"

#include <utility>
#include <memory>
#include <chrono>

RCLAP_BEGIN_NAMESPACE

// A blocking and fast single-reader single-writer queue.
template <typename T>
class BlkQueue
{
public:
    BlkQueue() : slots(std::make_unique<SlimSemaphore>(0)), items(std::make_unique<SlimSemaphore>(0)) {}
    explicit BlkQueue(std::size_t cap) : capacity(cap), slots(std::make_unique<SlimSemaphore>(cap)), items(std::make_unique<SlimSemaphore>(0))
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

    ~BlkQueue() {
        for (std::size_t i = 0, n = items->availableApprox(); i != n; ++i) {
            std::size_t idx = (nextItem + i) & mask;
            data[idx].~T();
        }
    }

    BlkQueue(BlkQueue &&other) noexcept : BlkQueue() {
        swap(other);
    }

    BlkQueue &operator=(BlkQueue &&other) noexcept {
        swap(other);
        return *this;
    }

    BlkQueue(const BlkQueue &) = delete;
    BlkQueue &operator=(const BlkQueue &) = delete;

    void swap(BlkQueue &other) noexcept
    {
        std::swap(data, other.data);
        std::swap(capacity, other.capacity);
        std::swap(mask, other.mask);
        std::swap(slots, other.slots);
        std::swap(items, other.items);
        std::swap(nextSlot, other.nextSlot);
        std::swap(nextItem, other.nextItem);
    }

    bool tryEnqueue(T &&item) {
        if (!slots->tryWait())
            return false;
        enqueueImpl(std::move(item));
        return true;
    }

    bool tryEnqueue(const T &item) {
        if (!slots->tryWait())
            return false;
        enqueueImpl(item);
        return true;
    }

    void waitEnqueue(T &&item) {
        while (!slots->wait());
        enqueueImpl(std::move(item));
    }

    void waitEnqueue(const T &item) {
        while (!slots->wait());
        enqueueImpl(item);
    }

    bool waitEnqueueTimed(const T &&item, std::int64_t usecs) {
        if (!slots->wait(usecs))
            return false;
        enqueueImpl(std::move(item));
        return true;
    }

    bool waitEnqueueTimed(const T &item, std::int64_t usecs) {
        if (!slots->wait(usecs))
            return false;
        enqueueImpl(item);
        return true;
    }

    template <typename Rep, typename Period>
    inline bool waitEnqueueTimed(T &&item, const std::chrono::duration<Rep, Period> &timeout) {
        return waitEnqueueTimed(std::move(item), std::chrono::duration_cast<std::chrono::microseconds>(timeout).count());
    }

    template <typename Rep, typename Period>
    inline bool waitEnqueueTimed(const T &item, const std::chrono::duration<Rep, Period> &timeout) {
        return waitEnqueueTimed(item, std::chrono::duration_cast<std::chrono::microseconds>(timeout).count());
    }

    template <typename U>
    bool tryDequeue(U &item) {
        if (!items->tryWait())
            return false;
        dequeueImpl(item);
        return true;
    }

    template <typename U>
    void waitDequeue(U &item) {
        while (!items->wait());
        dequeueImpl(item);
    }

    template <typename U>
    bool waitDequeueTimed(U &item, std::int64_t usecs) {
        if (!items->wait(usecs))
            return false;
        dequeueImpl(item);
        return true;
    }

    template <typename U, typename Rep, typename Period>
    inline bool waitDequeueTimed(U &item, const std::chrono::duration<Rep, Period> &timeout) {
        return waitDequeueTimed(item, std::chrono::duration_cast<std::chrono::microseconds>(timeout).count());
    }

    inline T *peek() {
        if (!items->availableApprox())
            return nullptr;
        return peekImpl();
    }

    inline bool tryPop() {
        if (!items->tryWait())
            return false;
        popImpl();
        return true;
    }

    [[nodiscard]] inline std::size_t sizeApprox() const {
        return items->availableApprox();
    }

    [[nodiscard]] inline std::size_t maxCapacity() const {
        return capacity;
    }

private:
    template <typename U>
    void enqueueImpl(U &&item) {
        new (&data[nextSlotIdx()]) T(std::forward<U>(item));
        items->signal();
    }

    template <typename U>
    void dequeueImpl(U &item) {
        T &elem = data[nextItemIdx()];
        item = std::move(elem);
        elem.~T();
        slots->signal();
    }

    T *peekImpl() const {
        return &data[itemIdx()];
    }

    void popImpl() {
        data[nextItemIdx()].~T();
        slots->signal();
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

    std::unique_ptr<SlimSemaphore> slots;
    std::unique_ptr<SlimSemaphore> items;

    std::size_t mask = {}; // capacity mask for cheap modulo

    std::size_t nextSlot = {};
    std::size_t nextItem = {};
};

RCLAP_END_NAMESPACE

#endif
