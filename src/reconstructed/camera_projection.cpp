#include "kinoko/camera_records.hpp"
#include "kinoko/quad_records.hpp"
#include <cmath>

// 466320: independent translation store followed by asymmetric pixel rounding.
extern "C" void kinoko_camera_project(KinokoCamera *camera,KinokoQuad *storage) {
    float x=0.0f,y=0.0f;
    if (camera) { // inherited null-camera projection boundary
        const auto view=kinoko::camera::View(camera).load();
        x=view.center_x-view.x-view.offset_x;
        y=view.center_y-view.y-view.offset_y;
    }
    const kinoko::render::QuadView quad(storage);
    auto positions=quad.get(&kinoko::render::QuadRecord::positions);
    for(auto &position:positions) {
        position.x+=x;position.y+=y;
        position.x=std::floor(position.x);position.y=std::ceil(position.y);
    }
    quad.set(&kinoko::render::QuadRecord::positions,positions);
}
