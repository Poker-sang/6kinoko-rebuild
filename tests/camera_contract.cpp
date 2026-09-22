#include "kinoko/camera_records.hpp"
#include "kinoko/quad_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include <cstdio>
#include <cstring>
#include <vector>
using namespace kinoko::camera;
using namespace kinoko::render;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
static std::vector<int> calls;
extern "C" {
int32_t g611[3]{},g722[3]{};
int32_t  kinoko_sqplus_new_instance_adapter(int32_t *out, int32_t *) { calls.push_back(1);return address(out); }
void * kinoko_sqplus_object_assign(void * out, const void * in) { calls.push_back(2);std::memcpy(static_cast<void *>(out),in,12);return out; }
int32_t  kinoko_sqplus_object_destroy(void * ) { calls.push_back(3);return 0; }
int32_t  kinoko_sqplus_object_set_instance(void * , void * ) { calls.push_back(4);return 0; }
int32_t  kinoko_sqplus_object_raw_set_name(void * , const char *, const void * ) { calls.push_back(5);return 77; }
void retdec_trace_i32(const char *,int32_t) {}
}
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"camera line %d\n",__LINE__);return 1; } } while(0)
int main() {
    Record camera{};camera.width=640;camera.height=480;
    camera.update_vm=reinterpret_cast<SQVM *>(1234);camera.update_function[5]=17;
    auto *receiver=reinterpret_cast<KinokoCamera *>(&camera);
    CHECK(kinoko_camera_initialize(receiver)==77);
    CHECK((calls==std::vector<int>{1,2,3,4,5}));
    CHECK(camera.width==640 && camera.height==480 && camera.update_function[5]==17);
    camera.x=10.25f;camera.y=20.25f;camera.center_x=4;camera.center_y=8;
    camera.offset_x=0.5f;camera.offset_y=-0.5f;camera.bounds={1,2,3,4};
    QuadRecord quad{};quad.positions[0]={-0.5f,0.5f,7};
    kinoko_camera_project(receiver,reinterpret_cast<KinokoQuad *>(&quad));
    CHECK(quad.positions[0].x==-8 && quad.positions[0].y==-11 && quad.positions[0].z==7);
    Record copy{};calls.clear();
    CHECK(kinoko_camera_copy(reinterpret_cast<KinokoCamera *>(&copy),receiver)==reinterpret_cast<KinokoCamera *>(&copy));
    CHECK((calls==std::vector<int>{2,2,2}));
    CHECK(copy.update_vm==camera.update_vm && copy.update_function==camera.update_function);
    CHECK(copy.width==640 && copy.x==camera.x && copy.bounds.bottom==4);
    return 0;
}
