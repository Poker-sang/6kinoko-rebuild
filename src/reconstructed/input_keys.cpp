#include "kinoko/input_keys.h"
#include <vector>
#include <algorithm>

struct KinokoInputKeyStorage { std::vector<uint8_t> scans; };
extern "C" unsigned char g_retdec_keyboard_state[256];
namespace {
bool down(uint8_t scan) { return (g_retdec_keyboard_state[scan]&0x80)!=0; }
}
extern "C" void kinoko_input_keys_construct(KinokoKeyTracker* tracker) {
    tracker->keys=new KinokoInputKeyStorage;
    kinoko_input_keys_clear(tracker);
}
extern "C" void kinoko_input_keys_destroy(KinokoKeyTracker* tracker) {
    delete tracker->keys;tracker->keys=nullptr;
}
extern "C" void kinoko_input_keys_clear(KinokoKeyTracker* tracker) {
    std::fill_n(tracker->counts,256,0);
    tracker->keys->scans.clear();
    tracker->shift=tracker->alt=tracker->control=0;
}
extern "C" void kinoko_input_keys_add(KinokoKeyTracker* tracker,uint8_t scan) {
    auto& keys=tracker->keys->scans;
    if (std::find(keys.begin(),keys.end(),scan)==keys.end()) keys.push_back(scan);
}
extern "C" void kinoko_input_keys_assign(KinokoKeyTracker* destination,const KinokoKeyTracker* source) {
    if (destination==source) return;
    std::copy_n(source->counts,256,destination->counts);
    destination->keys->scans=source->keys->scans;
    destination->shift=source->shift;destination->alt=source->alt;destination->control=source->control;
}
extern "C" uint32_t kinoko_input_keys_size(const KinokoKeyTracker* tracker) {
    return static_cast<uint32_t>(tracker->keys->scans.size());
}
extern "C" uint8_t kinoko_input_keys_at(const KinokoKeyTracker* tracker,uint32_t index) {
    return tracker->keys->scans.at(index);
}
// 408320: only registered keys advance; modifiers are independent. Preserve
// original INC wraparound and return the sampled Ctrl state.
extern "C" int32_t kinoko_input_keys_update(KinokoKeyTracker* tracker) {
    for (const auto scan:tracker->keys->scans) {
        auto& count=tracker->counts[scan];
        count=down(scan)?static_cast<int32_t>(uint32_t(count)+1u):0;
    }
    tracker->shift=down(0x2a)||down(0x36);
    tracker->alt=down(0x38)||down(0xb8);
    return tracker->control=down(0x1d)||down(0x9d);
}
// 4083E0 accepts the low byte of the key and each modifier requirement.
extern "C" int32_t kinoko_input_key_pressed(const KinokoKeyTracker* tracker,
    int32_t scan,int32_t shift,int32_t alt,int32_t control) {
    return tracker->counts[uint8_t(scan)]==1 && (!uint8_t(shift)||tracker->shift) &&
        (!uint8_t(alt)||tracker->alt) && (!uint8_t(control)||tracker->control);
}
