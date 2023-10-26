#include <core/global.h>

RCLAP_BEGIN_NAMESPACE

class Context
{
public:
    Context() = default;
    ~Context() = default;

    Context(const Context&) = delete;
    Context(Context&&) noexcept = default;
    Context& operator=(const Context&) = delete;
    Context& operator=(Context&&) noexcept = default;

    double sampleRate() const noexcept { return mSampleRate; }
    void setSampleRate(double sampleRate) noexcept { mSampleRate = sampleRate; }

private:
    double mSampleRate;
};

RCLAP_END_NAMESPACE
