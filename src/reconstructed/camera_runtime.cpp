#include "kinoko/camera_records.hpp"
#include "kinoko/legacy_memory.hpp"
extern "C" {
extern int32_t g611[3],g722[3];
int32_t function_4a90c0(int32_t *,int32_t *);
int32_t function_4a95c0_this(int32_t,int32_t);
int32_t function_4a9d70_this(int32_t);
int32_t function_4a9bb0_this(int32_t,int32_t);
int32_t function_4a9840_this(int32_t,const char *,int32_t);
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
    const auto object=function_4a90c0(temporary,g611);
    function_4a95c0_this(address(state.bytes(&Record::script_object)),object);
    function_4a9d70_this(address(temporary));
    function_4a9bb0_this(address(state.bytes(&Record::script_object)),address(camera));
    const auto result=function_4a9840_this(address(g722),"camera",address(camera));
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
    function_4a95c0_this(address(out.bytes(&Record::script_object)),address(in.bytes(&Record::script_object)));
    out.set(&Record::update_vm,in.get(&Record::update_vm));
    function_4a95c0_this(address(out.bytes(&Record::update_environment)),address(in.bytes(&Record::update_environment)));
    function_4a95c0_this(address(out.bytes(&Record::update_function)),address(in.bytes(&Record::update_function)));
    for(auto field:{&Record::x,&Record::y,&Record::center_x,&Record::center_y,
        &Record::offset_x,&Record::offset_y,&Record::width,&Record::height}) out.set(field,in.get(field));
    out.set(&Record::bounds,in.get(&Record::bounds));
    return destination;
}
