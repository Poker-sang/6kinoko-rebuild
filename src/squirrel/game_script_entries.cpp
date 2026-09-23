#include "kinoko/game_script_api.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_native_arguments.h"
#include "kinoko/squirrel_game_objects.h"
extern "C" void retdec_trace_i32(const char*,int32_t);
namespace {
using namespace kinoko::script;
inline auto commit_actor_result = function_4029b0;
inline int32_t destroy_object_result(void* object) {
    return address(kinoko_sqplus_object_destroy(object));
}
KinokoOwnedObjectWords transfer(HSQUIRRELVM vm,const HSQOBJECT& borrowed) {
    auto value=borrowed;
    sq_addref(vm,&value);
    return {kinoko_squirrel_object_vtable(),static_cast<int32_t>(value._type),data_bits(value)};
}
bool read(HSQUIRRELVM vm,int index,HSQOBJECT& result) {
    return vm && index>0 && index<=sq_gettop(vm) && SQ_SUCCEEDED(sq_getstackobj(vm,index,&result));
}
}
extern "C" int32_t kinoko_script_global_update_entry(SQVM* vm) {
    auto* target=kinoko_native_target_from_userdata(vm);
    HSQOBJECT environment{},closure{};
    if(!target || !read(vm,3,environment) || !read(vm,2,closure)) return 0;
    // 471CE4 constructs slot 3 first, but passes closure(slot 2) before it.
    const auto env=transfer(vm,environment),fn=transfer(vm,closure);
    using Function=int32_t (__cdecl *)(KinokoOwnedObjectWords,KinokoOwnedObjectWords);
    reinterpret_cast<Function>(target)(fn,env);
    return 0;
}
extern "C" int32_t kinoko_script_create_actor_entry(SQVM* vm) {
    auto* target=kinoko_native_target_from_userdata(vm);
    HSQOBJECT closure{},argument{};
    float x{},y{},z{};
    const auto top=vm ? sq_gettop(vm) : 0;
    retdec_trace_i32("native-471df0:top",top);
    for(int i=2;i<=5;++i) {
        const char* labels[]={"native-471df0:type2","native-471df0:type3","native-471df0:type4","native-471df0:type5"};
        retdec_trace_i32(labels[i-2],vm && i<=top ? sq_gettype(vm,i) : OT_NULL);
    }
    if(!target || top<6 || !read(vm,2,closure) ||
       !kinoko_native_float_arg(vm, 3, &x) || !kinoko_native_float_arg(vm, 4, &y) ||
       !kinoko_native_float_arg(vm, 5, &z) || !read(vm,6,argument)) return 0;
    // Restore owning by-value arguments; the native manager below only borrows.
    const auto arg=transfer(vm,argument),fn=transfer(vm,closure);
    KinokoOwnedObjectWords result{kinoko_squirrel_object_vtable(),OT_NULL,0};
    using Function=KinokoOwnedObjectWords* (__cdecl *)(KinokoOwnedObjectWords*,KinokoOwnedObjectWords,float,float,float,KinokoOwnedObjectWords);
    reinterpret_cast<Function>(target)(&result,fn,x,y,z,arg);
    commit_actor_result(address(vm),reinterpret_cast<int32_t*>(&result));
    destroy_object_result(&result);
    return 1;
}
