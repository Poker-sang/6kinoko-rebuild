#pragma once
#include "kinoko/act_types.h"
#include "kinoko/legacy_string.hpp"
#include "kinoko/native_record_view.hpp"
#include <cstddef>
#include <cstdint>
namespace kinoko::act {
// CActKey owns its optional layout and script name. The last word is still
// opaque; destructor behavior depends on the concrete key/timeline vtable.
struct KeyRecord {
    const void *methods;
    KinokoActLayout *layout;
    legacy::StringRecord script_name;
    uint32_t unknown32;
};
struct TimelinePair { int32_t begin, length; };
struct TimelineBuffer { TimelinePair *begin, *end; void* owner; };
struct TimelineRecord {
    const void* methods;
    int32_t begin_time, duration;
    TimelineBuffer pairs;
    uint32_t reserved24;
};
static_assert(sizeof(TimelinePair) == 8 && sizeof(TimelineRecord) == 28);
static_assert(offsetof(TimelineRecord, pairs) == 12);
using KeyView = kinoko::native::RecordView<KeyRecord>;
static_assert(sizeof(KeyRecord) == 36);
static_assert(offsetof(KeyRecord, layout) == 4);
static_assert(offsetof(KeyRecord, script_name) == 8);
}
