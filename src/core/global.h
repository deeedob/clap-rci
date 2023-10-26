#ifndef MACROS_H
#define MACROS_H

#include <cstdint>
#include <string_view>

#define QCLAP_NAMESPACE qclap

# define RCLAP_BEGIN_NAMESPACE namespace QCLAP_NAMESPACE {
# define RCLAP_END_NAMESPACE }

RCLAP_BEGIN_NAMESPACE

// MurmurHash3 finalizer
// Since (*this) is already unique in this process, all we really want to do is
// propagate that uniqueness evenly across all the bits, so that we can use
// a subset of the bits while reducing collisions significantly.
[[maybe_unused]] static std::uint64_t toHash(void *ptr)
{
    uint64_t h = reinterpret_cast<uint64_t>(ptr); // NOLINT
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    return h ^ (h >> 33);
}

template <typename T>
void *toTag(T *t) { return reinterpret_cast<void *>(t); }

namespace Metadata {
    static constexpr std::string_view PluginHashId = "plugin-hash-id";
}

RCLAP_END_NAMESPACE

#endif // MACROS_H
