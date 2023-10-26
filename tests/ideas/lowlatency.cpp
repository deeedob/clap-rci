#include <array>
#include <atomic>
#include <cstddef>
#include <complex>

template <typename T, size_t N>
struct LockFreeQueue
{
    bool push(const T& element)
    {
        auto oldWritePos = write_pos.load();
        auto newWritePos = getPositionAfter(oldWritePos);

        if (newWritePos == readPos.load())
            return false;

        ringBuffer[oldWritePos] = element;
        write_pos.store(newWritePos);

        return true;
    }

    bool pop(T& element)
    {
        auto oldWritePos = write_pos.load();
        auto oldReadPos = readPos.load();

        if (oldWritePos == oldReadPos)
            return false;

        element = std::move(ringBuffer[oldReadPos]);
        return true;
    }

private:
    static constexpr size_t  getPositionAfter(size_t pos) noexcept
    {
        return ++pos == size ? 0 : pos;
    }

    static constexpr size_t  size = N + 1;
    std::array<T, size> ringBuffer;
    std::atomic<size_t> readPos = 0, write_pos = 0;
};

int main()
{
    LockFreeQueue<int, 256> queue;
    for (int i = 0; i < 256; ++i)
        queue.push(i);

    std::atomic<float> gain; // with custom memory order for optimization
    static_assert(std::atomic<float>::is_always_lock_free); // important that the stl is not silenty inserting mutexes!!!!!

    // static_assert(std::atomic<std::complex<double>>::is_always_lock_free); -- See :)
    // Sad: DWCAS
    // cant store more than double with... on atomics

    // cril::spin_mutex mtx // use spinlock with progressive fallback
    // crill::spin_on_write_object ??

    return 0;
}