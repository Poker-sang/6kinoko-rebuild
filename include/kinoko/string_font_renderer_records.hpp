#pragma once
#include "kinoko/legacy_string.hpp"
#include <cstddef>
#include <cstdint>

namespace kinoko::text {
// Borrowed view of the original 404-byte renderer at atlas +24. The bitmap
// and pixel-list owner are distinct; the enclosing atlas owns this renderer.
struct FontRendererRecord {
    unsigned char runtime0[12];
    char face[256];
    uint8_t colors[6];
    unsigned char reserved274[2];
    int32_t font_height, font_weight;
    uint8_t style284, edge, style286, reserved287;
    int32_t setting288, margin_left, margin_top, character_space, line_space;
    unsigned char runtime308[36];
    void *bitmap;
    void *pixel_owner;
    unsigned char state352[8];
    uint32_t color;
    unsigned char state364[4];
    legacy::StringRecord label;
    unsigned char state392[12];
};
static_assert(sizeof(FontRendererRecord)==404);
static_assert(offsetof(FontRendererRecord,face)==12);
static_assert(offsetof(FontRendererRecord,colors)==268);
static_assert(offsetof(FontRendererRecord,font_height)==276);
static_assert(offsetof(FontRendererRecord,edge)==285);
static_assert(offsetof(FontRendererRecord,character_space)==300);
static_assert(offsetof(FontRendererRecord,color)==360);
static_assert(offsetof(FontRendererRecord,bitmap)==344);
static_assert(offsetof(FontRendererRecord,pixel_owner)==348);
static_assert(offsetof(FontRendererRecord,label)==368);
}
