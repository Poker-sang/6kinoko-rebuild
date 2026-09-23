#include "kinoko/quad_transform.hpp"
#include <cstdio>
#include <cstring>
using namespace kinoko::render;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"quad line %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main() {
    QuadRecord quad{};
    quad.texture = 7; quad.texture_width = 128; quad.source_u_extent = .25f;
    for (auto &v : quad.vertices) { v.color = 0x12345678; v.rhw = 1; v.u = .5f; }
    quad.base_positions = {{{2,3,4},{5,6,7},{8,9,10},{11,12,13}}};
    quad.positions = quad.base_positions;
    const auto original = quad;
    rotate_quad(quad, {90,90,90}, {1,1,1});
    // Z -> Y -> X with original signs maps (dx,dy,dz) to (dz,dy,-dx).
    for (int i = 0; i < 4; ++i) {
        CHECK(quad.positions[i].x == original.positions[i].z);
        CHECK(quad.positions[i].y == original.positions[i].y);
        CHECK(quad.positions[i].z == 2 - original.positions[i].x);
    }
    scale_quad(quad, {-2,0,.5f}, {1,2,3});
    CHECK(quad.positions[0].x == -5 && quad.positions[0].y == 2 && quad.positions[0].z == 1.5f);
    const auto scaled = quad.positions;
    translate_quad(quad, {10,-20,30});
    for (int i = 0; i < 4; ++i) {
        CHECK(quad.positions[i].x == scaled[i].x + 10);
        CHECK(quad.positions[i].y == scaled[i].y - 20);
        CHECK(quad.positions[i].z == scaled[i].z + 30);
    }
    const auto translated = quad.positions;
    rotate_quad(quad, {0,0,0}, {100,100,100});
    CHECK(std::memcmp(&translated, &quad.positions, sizeof(translated)) == 0);
    CHECK(std::memcmp(&original, &quad, offsetof(QuadRecord,positions)) == 0);
    CHECK(quad.source_u_extent == original.source_u_extent && quad.source_v_extent == original.source_v_extent);
    std::puts("PASS: typed quad transforms, Z/Y/X order, pivots, depth and preserved frame data");
}
