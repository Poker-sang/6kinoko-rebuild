#pragma once
#include "kinoko/act_layout_records.hpp"
namespace kinoko::act {
int32_t bind_layout_3d(Layout3DRecord *layout, KinokoActLayer *layer);
int32_t update_layout_3d(Layout3DRecord *layout);
int32_t draw_layout_3d(Layout3DRecord *layout);
Layout3DRecord *clone_layout_3d(const Layout3DRecord *layout);
}
