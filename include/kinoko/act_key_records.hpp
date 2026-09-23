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
using KeyView = kinoko::native::RecordView<KeyRecord>;
static_assert(sizeof(KeyRecord) == 36);
static_assert(offsetof(KeyRecord, layout) == 4);
static_assert(offsetof(KeyRecord, script_name) == 8);
}
