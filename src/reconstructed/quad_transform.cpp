#include "kinoko/quad_transform.hpp"
#include "kinoko/angle_math.h"
namespace kinoko::render {
// 405180/405220/4052C0: scale current positions around an explicit pivot.
void scale_quad(QuadRecord &quad, const Position3 &scale, const Position3 &pivot) {
    for (auto &p : quad.positions) {
        p.x = (p.x - pivot.x) * scale.x + pivot.x;
        p.y = (p.y - pivot.y) * scale.y + pivot.y;
        p.z = (p.z - pivot.z) * scale.z + pivot.z;
    }
}
// 405320: complete Z, then Y, then X passes; each pass stores float positions.
void rotate_quad(QuadRecord &quad, const Position3 &degrees, const Position3 &pivot) {
    if (degrees.z != 0) {
        const float c = kinoko_cos_degrees(degrees.z), s = kinoko_sin_degrees(degrees.z);
        for (auto &p : quad.positions) {
            const float dx = p.x - pivot.x, dy = p.y - pivot.y;
            p.x = dx * c + pivot.x - dy * s;
            p.y = dx * s + pivot.y + dy * c;
        }
    }
    if (degrees.y != 0) {
        const float c = kinoko_cos_degrees(degrees.y), s = kinoko_sin_degrees(degrees.y);
        for (auto &p : quad.positions) {
            const float dx = p.x - pivot.x, dz = p.z - pivot.z;
            p.x = dx * c + pivot.x + dz * s;
            p.z = dz * c + pivot.z - dx * s;
        }
    }
    if (degrees.x != 0) {
        const float c = kinoko_cos_degrees(degrees.x), s = kinoko_sin_degrees(degrees.x);
        for (auto &p : quad.positions) {
            const float dy = p.y - pivot.y, dz = p.z - pivot.z;
            p.y = dy * c + pivot.y + dz * s;
            p.z = dz * c + pivot.z - dy * s;
        }
    }
}
// 405080: translate all four current positions, including depth.
void translate_quad(QuadRecord &quad, const Position3 &offset) {
    for (auto &p : quad.positions) {
        p.x += offset.x; p.y += offset.y; p.z += offset.z;
    }
}
}
