#pragma once
#include "kinoko/act_layout_records.hpp"
namespace kinoko::act {
int32_t bind_layout_2d(KinokoActLayout *layout,KinokoActLayer *layer);
int32_t update_layout_2d(KinokoActLayout *layout);
int32_t draw_layout_2d(KinokoActLayout *layout,float x,float y);
void set_layout_blend(int32_t mode); // 42AC20, distinct from renderer's 402770
}
