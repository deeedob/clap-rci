#include <atomic>
#include <cmath>
#include <iostream>
#include <thread>
#include <array>
#include <numbers>
#include <emmintrin.h> // _mm_pause, x86 intrinsic

// From Timur Doumler : https://timur.audio/using-locks-in-real-time-audio-processing-safely
struct spinmutex
{
    // good for high contention, low latency
    void exp_backoff_lock() noexcept {
        for (std::uint64_t k = 0; !try_lock(); ++k) {
            if (k < 5) ;                                                        // instant retry
            else if (k < 16) __asm__ __volatile__( "rep; nop" : : : "memory");  // processor yield on x86
            else if (k < 64) sched_yield();                                     // kernel yield, put thread back in run queue
            else {                                                              // Sleep for 1000 ns
                timespec rqtp = { 0, 0 };
                rqtp.tv_sec = 0; rqtp.tv_nsec = 1000;
                nanosleep(&rqtp, nullptr);
            }
        }
    }
    // Notes: Typical thread-scheduler timeslice is ca. 10 - 100ms,
    // yield() is an expensive syscall, can take around 230 - 510ns and keeps kernel busy and also the cpu!
    void portable_exp_backoff_lock() noexcept {
        for (std::uint64_t k = 0; !try_lock(); ++k) {
            if (k < 5) ;                                                        // spin
            else if (k < 15) _mm_pause();                                       // short pause, 35ns each
            else if (k < 25) { _mm_pause(); _mm_pause(); _mm_pause();           // longer pause, 350ns each
                _mm_pause(); _mm_pause(); _mm_pause(); _mm_pause(); _mm_pause();
                _mm_pause(); _mm_pause(); }
            else {                                                              // Sleep for 1000 ns
                std::this_thread::yield(); // system is under high cpu load!
            }
        }
    }
    // C++20 non-busy wait.
    void lock() noexcept {
        while (try_lock())
            flag.wait(true, std::memory_order_relaxed);
    }

    // In audio, there is often just 2 threads involved: the process thread and the main thread.

    bool try_lock() noexcept {
        return !flag.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept {
        flag.clear(std::memory_order_release);
        flag.notify_one(); // could be implemented with a syscall.
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

struct audio_spinmutex {
    void lock() noexcept {
        constexpr std::array Iterations = {5, 10, 3000 };
        static_assert(std::atomic<bool>::is_always_lock_free);

        for (int i = 0; i < Iterations[0]; ++i)
            if (try_lock()) return;
        for (int i = 0; i < Iterations[1]; ++i) {
            if (try_lock()) return;
            _mm_pause();
        }
        while (true) {
            for (int i = 0; i < Iterations[2]; ++i) {
                if (try_lock()) return;
                _mm_pause(); _mm_pause(); _mm_pause(); _mm_pause(); _mm_pause();
                _mm_pause(); _mm_pause(); _mm_pause(); _mm_pause(); _mm_pause();
            }
            // Put back in run queue.
            std::this_thread::yield();
        }
    }

    bool try_lock() noexcept {
        return !flag.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept {
        flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

struct sin_generator
{
    // y(t) = A * sin(2 * pi * f * t)
    sin_generator(double amplitude, double freq, double sample_rate = 44100.0)
        : amp(amplitude), phi(2 * std::numbers::pi * freq / sample_rate), dt(1 / sample_rate), t(-dt)
    {}
    double operator()() noexcept {
        return amp * std::sin(phi * (t += dt));
    }
private:
    const double amp;
    const double phi;
    const double dt;
    double t;
};

int main()
{
    auto data = std::array<double, 256>{};
    auto lock = spinmutex{};

    auto worker = std::jthread([&data, &lock](std::stop_token st) {
        sin_generator gen(0.8, 440.0);
        while (!st.stop_requested()) {
            lock.lock();
            std::cout << "worker\n" << std::flush;
            for (auto &i : data) {
                i = gen();
            }
            lock.unlock();
        }
    });

    auto worker2 = std::jthread([&data, &lock](std::stop_token st) {
        sin_generator gen(0.4, 880.0);
        while (!st.stop_requested()) {
            lock.lock();
            std::cout << "worker2\n" << std::flush;
            for (auto &i : data) {
                i = gen();
            }
            lock.unlock();
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    worker.request_stop();
    worker2.request_stop();
    worker.join();
    worker2.join();

    return 0;
}
