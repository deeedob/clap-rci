#ifndef SEMAPHORES_H
#define SEMAPHORES_H

#include "global.h"

#ifdef __unix__
#include <semaphore.h>
#include <atomic>
#include <climits>
#include <cassert>
#else
#error "Unsupported platform"
#endif

RCLAP_BEGIN_NAMESPACE

// Lightweight wrapper around sem_t.
class Semaphore
{
public:
    explicit Semaphore(unsigned int initialCount = 0)
    {
        assert(initialCount <= SEM_VALUE_MAX);
        // The pshared argument indicates whether this semaphore is to be shared
        int rc = sem_init(&mSem, 0, initialCount);
        assert(rc == 0);
        (void)rc;
    }
    ~Semaphore() { sem_destroy(&mSem); }
    Semaphore(const Semaphore &) = delete;
    Semaphore &operator=(const Semaphore &) = delete;

    /**
     * @brief Waits until the semaphore's count becomes greater than zero.
     *
     * @return `true` if the wait operation was successful, `false` on error.
     * @details This function blocks the calling thread until the semaphore's count
     *          becomes greater than zero, or an error occurs. It returns `true` if
     *          the wait operation is successful, and `false` if an error occurs.
     */
    bool wait() {
        int res;
        do {
            res = sem_wait(&mSem);
        } while (res == -1 && errno == EINTR);
        return res == 0;
    }
    /**
     * @brief Attempts to decrement the semaphore's count without blocking.
     *
     * @return `true` if the tryWait operation was successful, `false` if the count is
     *         zero or on error.
     * @details This function attempts to decrement the semaphore's count without
     *          blocking. It returns `true` if the decrement is successful, `false`
     *          if the count is already zero or an error occurs.
     */
    bool tryWait() {
        int rc;
        do {
            rc = sem_trywait(&mSem);
        } while (rc == -1 && errno == EINTR);
        return rc == 0;
    }
    /**
     * @brief Waits for a specified amount of time for the semaphore's count to become
     *        greater than zero.
     *
     * @param usecs The timeout in microseconds.
     * @return `true` if the timed wait operation was successful, `false` on timeout
     *         or error.
     * @details This function waits for the semaphore's count to become greater than
     *          zero for the specified amount of time. If the count becomes greater
     *          than zero within the specified time, it returns `true`. Otherwise, it
     *          returns `false` on timeout or error.
     */
    bool timedWait(std::uint64_t usecs) {
        struct timespec ts = {};

        const int usecPerSec = 1'000'000;
        const int nsecPerSec = 1'000'000'000;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += static_cast<time_t>(usecs / usecPerSec);
        ts.tv_nsec += static_cast<time_t>(usecs % usecPerSec) * 1'000;
        // Normalize, because sem_timedwait() expects tv_nsec to be in the range [0, 999'999'999]
        if (ts.tv_nsec >= nsecPerSec) {
            ts.tv_nsec -= nsecPerSec;
            ++ts.tv_sec;
        }

        int ret;
        do {
            ret = sem_timedwait(&mSem, &ts);
        } while (ret == -1 && errno == EINTR);
        return ret == 0;
    }

    /**
     * @brief Increments the semaphore's count, releasing one waiting thread.
     * @details This function increments the semaphore's count and wakes up one
     *          waiting thread. It doesn't block and always succeeds.
     */
    void signal() {
        while (sem_post(&mSem) == -1 && errno == EINTR);
    }
    /**
     * @brief Increments the semaphore's count multiple times, releasing multiple
     *        waiting threads.
     *
     * @param count The number of times to increment the semaphore's count.
     * @details This function increments the semaphore's count multiple times, waking
     *          up multiple waiting threads. It doesn't block and always succeeds.
     */
    void signal(int count) {
        while (count-- > 0) signal();
    }

    /**
     * @brief Gets the current count of the semaphore.
     *
     * @return The current count of the semaphore.
     * @details This function gets the current count of the semaphore. It doesn't
     *          block and always succeeds.
     */
    [[nodiscard]] int value() {
        int val = -1;
        sem_getvalue(&mSem, &val);
        return val;
    }

private:
    sem_t mSem{};
};

class SlimSemaphore
{
    using sizetype = std::make_signed<std::size_t>::type;

public:
    explicit SlimSemaphore(sizetype initialCount = 0) : mCount(initialCount) {
        assert(initialCount >= 0);
    }

    bool tryWait() {
        if (mCount.load() > 0) {
            mCount.fetch_sub(1, std::memory_order_acquire);
            return true;
        }
        return false;
    }

    bool wait() {
        return tryWait() || waitPartialSpin();
    }

    bool wait(std::int64_t usec) {
        return tryWait() || waitPartialSpin(usec);
    }

    void signal(sizetype cnt = 1) {
        assert(cnt >= 0);
        sizetype old = mCount.fetch_add(cnt, std::memory_order_release);
        assert(old >= -1);
        if (old < 0)
            mSem.signal(1);
    }

    [[nodiscard]] std::size_t availableApprox() const {
        auto c = mCount.load();
        return c > 0 ? static_cast<std::size_t>(c) : 0;
    }

private:
    bool waitPartialSpin(std::int64_t toutUsec = -1) {
        sizetype oldCount = {};
        int spin = 1024;
        while (--spin >= 0) {
            if (mCount.load() > 0) {
                mCount.fetch_add(-1, std::memory_order_acquire);
                return true;
            }
            // Prevent the compiler from collapsing the loop
            std::atomic_signal_fence(std::memory_order_acquire);
        }

        oldCount = mCount.fetch_add(-1, std::memory_order_acquire);
        if (oldCount > 0)
            return true;

        if (toutUsec < 0) {
            if (mSem.wait())
                return true;
        }

        if (toutUsec > 0 && mSem.timedWait(static_cast<std::uint64_t>(toutUsec)))
            return true;

        while (true) {
            oldCount = mCount.fetch_add(1, std::memory_order_release);
            if (oldCount < 0)
                return false;
            oldCount = mCount.fetch_add(-1, std::memory_order_release);
            if (oldCount > 0 && mSem.tryWait())
                return true;
        }
    }

private:
    std::atomic<sizetype> mCount;
    Semaphore mSem;
};

RCLAP_END_NAMESPACE

#endif // SEMAPHORES_H
