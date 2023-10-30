#ifndef MACROS_H
#define MACROS_H

#include <cstdint>
#include <string_view>
#include <cstring>

#define QCLAP_NAMESPACE qclap

# define RCLAP_BEGIN_NAMESPACE namespace QCLAP_NAMESPACE {
# define RCLAP_END_NAMESPACE }

RCLAP_BEGIN_NAMESPACE

#if !defined(CLRE_EXPORT)
#   if defined _WIN32 || defined __CYGWIN__
#      ifdef __GNUC__
#         define CLRE_EXPORT __attribute__((dllexport))
#      else
#         define CLRE_EXPORT __declspec(dllexport)
#      endif
#   else
#      if __GNUC__ >= 4 || defined(__clang__)
#         define CLRE_EXPORT __attribute__((visibility("default")))
#      else
#         define CLRE_EXPORT
#      endif
#   endif
#endif

// MurmurHash3 finalizer
// Since (*this) is already unique in this process, all we really want to do is
// propagate that uniqueness evenly across all the bits, so that we can use
// a subset of the bits while reducing collisions significantly.
[[maybe_unused]] inline std::uint64_t toHash(void *ptr)
{
    uint64_t h = reinterpret_cast<uint64_t>(ptr); // NOLINT
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    return h ^ (h >> 33);
}

template <typename T>
inline void *toTag(T *t) { return reinterpret_cast<void *>(t); }

inline void safe_str_copy(char* dest, size_t dest_size, const char* src) {
#if defined(__STDC_LIB_EXT1__)
    strcpy_s(dest, dest_size, src);
#else
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
#endif
}

namespace Metadata {
    static constexpr std::string_view PluginHashId = "plugin-hash-id";
}

RCLAP_END_NAMESPACE

#endif // MACROS_H
