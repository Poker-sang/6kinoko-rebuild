#pragma once
#include "kinoko/quad_records.hpp"
namespace kinoko::render {
void scale_quad(QuadRecord &quad, const Position3 &scale, const Position3 &pivot);
void rotate_quad(QuadRecord &quad, const Position3 &degrees, const Position3 &pivot);
void translate_quad(QuadRecord &quad, const Position3 &offset);
}
