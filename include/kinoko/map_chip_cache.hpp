#pragma once
#include "kinoko/map_layout_records.hpp"
namespace kinoko::map {
// Original 435B20/435860; throws on allocation failure, caught by host entry.
void rebuild_chip_index(KinokoActLayout *layout);
void prepare_placements(KinokoActLayout *layout);
// Original 435FD0 mutates the shared definition before checking cache bounds.
bool set_chip_rectangle(KinokoActLayout *layout,int32_t id,int16_t x,int16_t y,int16_t width,int16_t height);
int32_t refresh_chip_sprite(KinokoActLayout *layout,const ChipDefinition *chip);
const render::QuadRecord *find_chip_sprite(KinokoActLayout *layout,const ChipDefinition *chip);
void flush_changed_chips(KinokoActLayout *layout);
bool initialize_chip_quad(render::QuadRecord *quad,int32_t handle,const ChipDefinition *chip);
}
