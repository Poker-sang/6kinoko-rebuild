#include "kinoko/act_array.hpp"
#include "kinoko/native_record_view.hpp"
#include <stdexcept>
#include <cstdint>
namespace {
using kinoko::ActArray;
struct ArrayRecord { void **begin, **end; ActArray* owner; };
using View = kinoko::native::RecordView<ArrayRecord>;
static_assert(sizeof(ArrayRecord) == 12);
void publish(void* slot, ActArray* values) noexcept {
    auto* begin = values && !values->empty() ? values->data() : nullptr;
    const View view(slot);
    view.set(&ArrayRecord::begin, begin);
    view.set(&ArrayRecord::end, begin ? begin + values->size() : nullptr);
    view.set(&ArrayRecord::owner, values);
}
}
void kinoko::replace_act_array(void* slot, std::unique_ptr<ActArray> values) noexcept {
    delete View(slot).get(&ArrayRecord::owner);
    publish(slot, values.release());
}
extern "C" int32_t kinoko_act_array_prepare(void* slot, uint32_t count) {
    if (count > 0x10000) return 0;
    try {
        auto values = std::make_unique<ActArray>(count);
        kinoko::replace_act_array(slot, std::move(values));
        const View view(slot);
        view.set(&ArrayRecord::end, view.get(&ArrayRecord::begin));
        return 1;
    } catch (...) { return 0; }
}
extern "C" void kinoko_act_array_clone(void* destination, const void* source) {
    const auto record = View(const_cast<void*>(source)).load();
    const auto begin = reinterpret_cast<uintptr_t>(record.begin);
    const auto end = reinterpret_cast<uintptr_t>(record.end);
    if (end < begin || (end-begin) % sizeof(void*) || (!begin && end) ||
        (end-begin) / sizeof(void*) > 0x10000) throw std::length_error("ACT array");
    auto values = std::make_unique<ActArray>();
    if (end != begin) values->assign(record.begin, record.end);
    kinoko::replace_act_array(destination, std::move(values));
}
extern "C" void kinoko_act_array_append(void* slot, void* value) {
    auto* values = View(slot).get(&ArrayRecord::owner);
    if (!values) {
        auto owner = std::make_unique<ActArray>(); owner->push_back(value);
        kinoko::replace_act_array(slot, std::move(owner)); return;
    }
    if (values->size() >= 0x10000) throw std::length_error("ACT array");
    values->push_back(value); publish(slot, values);
}
extern "C" void kinoko_act_array_destroy(void* slot) {
    delete View(slot).get(&ArrayRecord::owner); publish(slot, nullptr);
}
