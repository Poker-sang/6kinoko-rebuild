#include "kinoko/camera_records.hpp"
#include "kinoko/legacy_memory.hpp"
extern "C" {
extern int32_t g611[3],g722[3];
void* kinoko_sqplus_object_new_instance(void*, const void*);
void * kinoko_sqplus_object_assign(void * , const void * );
void*  kinoko_sqplus_object_destroy(void *);
int32_t  kinoko_sqplus_object_set_instance(void * , void * );
int32_t  kinoko_sqplus_object_raw_set_name(void * , const char *, const void * );
void retdec_trace_i32(const char *,int32_t);
}
namespace {
using namespace kinoko::camera;
using kinoko::legacy::address;
}
extern "C" int32_t kinoko_camera_initialize(KinokoCamera *camera) {
    if (!camera) return 0;
    const View state(camera);
    int32_t temporary[3]{};
    auto* object=kinoko_sqplus_object_new_instance(temporary, g611);
    kinoko_sqplus_object_assign((void *)(state.bytes(&Record::script_object)), object);
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(temporary)));
    kinoko_sqplus_object_set_instance((void *)(state.bytes(&Record::script_object)), (void *)(camera));
    const auto result=kinoko_sqplus_object_raw_set_name((void *)(g722), "camera", (const void *)(camera));
    // 466270 resets only these fields. Width/height and callback remain intact;
    // the global backing storage is already zero-initialized at process startup.
    state.set(&Record::y,0.0f);state.set(&Record::x,0.0f);
    state.set(&Record::center_y,0.0f);state.set(&Record::center_x,0.0f);
    state.set(&Record::bounds,Bounds{});
    state.set(&Record::offset_x,0.0f);state.set(&Record::offset_y,0.0f);
    retdec_trace_i32("actor:camera-init-left",0);
    retdec_trace_i32("actor:camera-init-right",0);
    return result;
}
extern "C" KinokoCamera *kinoko_camera_copy(KinokoCamera *destination,KinokoCamera *source) {
    const View out(destination),in(source);
    kinoko_sqplus_object_assign((void *)(out.bytes(&Record::script_object)), (const void *)(in.bytes(&Record::script_object)));
    out.set(&Record::update_vm,in.get(&Record::update_vm));
    kinoko_sqplus_object_assign((void *)(out.bytes(&Record::update_environment)), (const void *)(in.bytes(&Record::update_environment)));
    kinoko_sqplus_object_assign((void *)(out.bytes(&Record::update_function)), (const void *)(in.bytes(&Record::update_function)));
    for(auto field:{&Record::x,&Record::y,&Record::center_x,&Record::center_y,
        &Record::offset_x,&Record::offset_y,&Record::width,&Record::height}) out.set(field,in.get(field));
    out.set(&Record::bounds,in.get(&Record::bounds));
    return destination;
}
