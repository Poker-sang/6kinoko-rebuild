#include "kinoko/boost_hash.h"
#include <boost/functional/hash.hpp>
#include <cstddef>
#include <cstdint>

static_assert(sizeof(std::size_t)==4, "original serialized Boost hashes are Win32");

extern "C" int32_t kinoko_boost_hash_range(int32_t begin,int32_t end) {
    const auto first=reinterpret_cast<const char*>(static_cast<uintptr_t>(begin));
    const auto last=reinterpret_cast<const char*>(static_cast<uintptr_t>(end));
    return static_cast<int32_t>(boost::hash_range(first,last));
}
