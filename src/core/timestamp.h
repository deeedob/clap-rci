#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include "global.h"

#include <spdlog/fmt/bundled/core.h>

#include <chrono>
#include <string>
#include <string_view>

#ifdef _MSC_VER
#include <intrin.h> // For __rdtsc
#endif

RCLAP_BEGIN_NAMESPACE

struct Stamp
{
    Stamp() = default;
    explicit Stamp(int64_t seconds, int64_t nanos) : mSeconds(seconds), mNanos(nanos) {}
    auto operator<=>(const Stamp &rhs) const = default;
    [[nodiscard]] int64_t seconds() const noexcept { return mSeconds; }
    [[nodiscard]] int64_t nanos() const noexcept { return mNanos; }
    void setStamp(int64_t seconds, int64_t nanos) { mSeconds = seconds; mNanos = nanos; }
    bool setIfNewer(int64_t seconds, int64_t nanos) {
        if (seconds > mSeconds || (seconds == mSeconds && nanos > mNanos)) {
            mSeconds = seconds;
            mNanos = nanos;
            return true;
        }
        return false;
    }
    Stamp delta(Stamp older) {
        int64_t deltaSec = mSeconds - older.mSeconds;
        int64_t deltaNanos = mNanos - older.mNanos;
        if (deltaNanos < 0) {
            deltaSec -= 1;
            deltaNanos += 1000000000;
        }
        return Stamp(deltaSec, deltaNanos);
    }

    void subtractMilliseconds(int64_t millis) {
        int64_t secondsToSubtract = millis / 1000;
        int64_t nanosToSubtract = (millis % 1000) * 1000000;

        mSeconds -= secondsToSubtract;
        if (mNanos < nanosToSubtract) {
            mSeconds -= 1;
            mNanos += 1000000000;
        }
        mNanos -= nanosToSubtract;
    }

    double toDouble() const {
        return static_cast<double>(mSeconds) + static_cast<double>(mNanos) / 1e9;
    }

    [[nodiscard]] std::string fmt() const {
        return fmt::format("{:0>10}.{:0>9}", mSeconds, mNanos);
    }
private:
    int64_t mSeconds = {};
    int64_t mNanos = {};
};

class Timestamp
{
    using Clock = std::chrono::high_resolution_clock;

    std::chrono::nanoseconds elapsed() const
    { return Clock::now() - timepoint; }

    template <typename DurationType>
    [[nodiscard]] auto elapsedTime() const
    { return std::chrono::duration_cast<DurationType>(elapsed()).count(); }

public:
    enum Type { MS, NS };

    Timestamp() : timepoint(Clock::now()) {}

    void reset()
    {
        timepoint = Clock::now();
    }

    [[nodiscard]] std::int64_t elapsedNanos() const
    {
        return elapsedTime<std::chrono::nanoseconds>();
    }

    [[nodiscard]] std::int64_t elapsedMicros() const
    {
        return elapsedTime<std::chrono::microseconds>();
    }

    [[nodiscard]] std::int64_t elapsedMillis() const
    {
        return elapsedTime<std::chrono::milliseconds>();
    }

    [[nodiscard]] std::int64_t nanos() const
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            timepoint.time_since_epoch()
        ).count();
    }

    [[nodiscard]] std::int64_t millis() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            timepoint.time_since_epoch()
        ).count();
    }

    [[nodiscard]] std::string fmtElapsed(Type t) const
    {
        switch (t) {
        case MS:
            return fmt::format(Ms, elapsedMillis());
        case NS:
            return fmt::format(Ns, elapsedNanos());
        default: return { "Unknown type" };
        }
    }

    [[nodiscard]] std::string fmtStamp(Type t) const
    {
        switch (t) {
        case MS:
            return fmt::format(Ms, millis());
        case NS:
            return fmt::format(Ns, nanos());
        default: return { "Unknown type" };
        }
    }
    // Check if compiler supports this
    static inline std::uint64_t getFastTicks()
    {
#if defined(_MSC_VER)
        return __rdtsc();
#elif defined(__GNUC__) || defined(__clang__) // For GCC and Clang
        unsigned int lo, hi;
        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
        return static_cast<std::uint64_t>(hi) << 32 | lo;
#else
#error "Compiler not supported"
            return 0;
#endif
    }

    static inline Stamp stamp()
    {
        auto now = std::chrono::system_clock::now();
        auto now_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
        auto epoch = now_ns.time_since_epoch();
        return Stamp(
            std::chrono::duration_cast<std::chrono::seconds>(epoch).count(),
            epoch.count() % 1000000000
        );
    }

private:
    Clock::time_point timepoint;
    static constexpr std::string_view Ms = "{}ms";
    static constexpr std::string_view Ns = "{}ns";
};

RCLAP_END_NAMESPACE

#endif // TIMESTAMP_H
