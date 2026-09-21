#include "kinoko/input_keys.h"
#include "kinoko/legacy_memory.hpp"
#include <vector>
#include <algorithm>

namespace {
using namespace kinoko::legacy;
using Keys = std::vector<uint8_t>;
Keys*& storage(int32_t tracker) { return field<Keys*>(tracker + 1024); }
}
extern "C" void kinoko_input_keys_construct(int32_t tracker) {
    storage(tracker) = new Keys;
    kinoko_input_keys_clear(tracker);
}
extern "C" void kinoko_input_keys_destroy(int32_t tracker) {
    delete storage(tracker); storage(tracker) = nullptr;
}
extern "C" void kinoko_input_keys_clear(int32_t tracker) {
    std::memset(pointer<void>(tracker), 0, 1024);
    storage(tracker)->clear();
    std::memset(pointer<void>(tracker + 1040), 0, 3);
}
extern "C" void kinoko_input_keys_add(int32_t tracker, uint8_t scan) {
    auto& keys = *storage(tracker);
    if (std::find(keys.begin(), keys.end(), scan) == keys.end()) keys.push_back(scan);
}
extern "C" void kinoko_input_keys_assign(int32_t destination, int32_t source) {
    if (destination == source) return;
    std::memcpy(pointer<void>(destination), pointer<void>(source), 1024);
    *storage(destination) = *storage(source);
    std::memcpy(pointer<void>(destination + 1040), pointer<void>(source + 1040), 3);
}
extern "C" uint32_t kinoko_input_keys_size(int32_t tracker) {
    return static_cast<uint32_t>(storage(tracker)->size());
}
extern "C" uint8_t kinoko_input_keys_at(int32_t tracker, uint32_t index) {
    return storage(tracker)->at(index);
}
