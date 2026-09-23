#pragma once
#include "kinoko/legacy_string.hpp"
#include <cstddef>
#include <cstdint>

namespace kinoko::text {
// Borrowed view of the original 404-byte renderer at atlas +24. The bitmap
// and pixel-list owner are distinct; the enclosing atlas owns this renderer.
struct FontRendererRecord {
    unsigned char state0[344];
    void *bitmap;
    void *pixel_owner;
    unsigned char state352[16];
    legacy::StringRecord label;
    unsigned char state392[12];
};
static_assert(sizeof(FontRendererRecord)==404);
static_assert(offsetof(FontRendererRecord,bitmap)==344);
static_assert(offsetof(FontRendererRecord,pixel_owner)==348);
static_assert(offsetof(FontRendererRecord,label)==368);
}
