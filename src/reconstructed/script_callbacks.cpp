#include "kinoko/script_callbacks.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/squirrel_source_runtime.h"
#include <windows.h>
extern "C" {
extern int32_t g600[3], g601[3], g602[3];
void retdec_trace(const char *);
void retdec_trace_i32(const char *, int32_t);
}
namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;
using CallbackView = kinoko::native::RecordView<KinokoScriptCallback>;
using namespace kinoko::actor;
static_assert(sizeof(KinokoScriptCallback)==28);
static_assert(offsetof(KinokoScriptCallback, environment)==4 && offsetof(KinokoScriptCallback, closure)==16);
static_assert(offsetof(ActorRecord,update_environment)==offsetof(ActorRecord,update_vm)+offsetof(KinokoScriptCallback,environment));
static_assert(offsetof(ActorRecord,update_function)==offsetof(ActorRecord,update_vm)+offsetof(KinokoScriptCallback,closure));
static_assert(offsetof(ActorRecord,collision_function)==offsetof(ActorRecord,collision_vm)+offsetof(KinokoScriptCallback,closure));
KinokoScriptCallback *update(KinokoActor *actor) {
    return reinterpret_cast<KinokoScriptCallback *>(ActorView(actor).bytes(&ActorRecord::update_vm));
}
KinokoScriptCallback *collision(KinokoActor *actor) {
    return reinterpret_cast<KinokoScriptCallback *>(ActorView(actor).bytes(&ActorRecord::collision_vm));
}
class LocalObject {
    KinokoOwnedObjectWords value_{};
    bool released_ = false;
public:
    LocalObject() = default;
    explicit LocalObject(const void *source) { copy_from(source); }
    LocalObject(int32_t vtable,int32_t type,int32_t value) : value_{vtable,type,value} {}
    LocalObject(const LocalObject&) = delete;
    LocalObject& operator=(const LocalObject&) = delete;
    ~LocalObject() { if (!released_) release(); }
    const void *data() const { return &value_; }
    void copy_from(const void *source) { (int32_t*)(intptr_t)(kinoko_sqplus_object_copy_construct((void *)(intptr_t)(reinterpret_cast<int32_t *>(&value_)), (const void *)(source))); }
    int32_t release() { released_=true; return kinoko_sqplus_object_destroy((void *)(&value_)); }
};
void assign(KinokoScriptCallback *destination,SQVM *vm,const void *environment,const void *closure) {
    const CallbackView view(destination);
    view.set(&KinokoScriptCallback::vm,vm);
    (int32_t)(intptr_t)(kinoko_sqplus_object_assign((void *)(view.bytes(&KinokoScriptCallback::environment)), (const void *)(environment)));
    (int32_t)(intptr_t)(kinoko_sqplus_object_assign((void *)(view.bytes(&KinokoScriptCallback::closure)), (const void *)(closure)));
}
void bind(KinokoScriptCallback *destination,const void *environment,const void *closure) {
    LocalObject saved_environment(environment), saved_function(closure);
    assign(destination,current_vm(),saved_environment.data(),saved_function.data());
}
}
extern "C" KinokoScriptCallback *kinoko_script_callback_construct(KinokoScriptCallback *callback,const char *name) {
    if (!callback) return nullptr;
    const CallbackView view(callback);
    view.set(&KinokoScriptCallback::vm,current_vm());
    (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(view.bytes(&KinokoScriptCallback::environment))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(view.bytes(&KinokoScriptCallback::closure))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_assign((void *)(view.bytes(&KinokoScriptCallback::environment)), (const void *)(intptr_t)((int32_t)(intptr_t)(kinoko_sqplus_root_object()))));
    // Keep the established null-name compatibility path (zero words), including
    // its external-reference cleanup. Named lookup uses the original root.
    KinokoOwnedObjectWords temporary{};
    if (name) (int32_t*)(intptr_t)(kinoko_sqplus_object_get_value((void *)(view.bytes(&KinokoScriptCallback::environment)), (void *)(&temporary), name));
    (int32_t)(intptr_t)(kinoko_sqplus_object_assign((void *)(view.bytes(&KinokoScriptCallback::closure)), (const void *)(&temporary)));
    kinoko_sqplus_object_destroy((void *)(&temporary));
    return callback;
}
extern "C" void kinoko_script_callback_clear(KinokoScriptCallback *callback) {
    KinokoScriptCallback empty{};
    kinoko_script_callback_construct(&empty,nullptr);
    assign(callback,empty.vm,&empty.environment,&empty.closure);
    kinoko_destroy_script_callback(&empty);
}
extern "C" int32_t kinoko_destroy_script_callback(KinokoScriptCallback *callback) {
    const CallbackView view(callback);
    kinoko_sqplus_object_destroy((void *)(view.bytes(&KinokoScriptCallback::closure)));
    return kinoko_sqplus_object_destroy((void *)(view.bytes(&KinokoScriptCallback::environment)));
}
extern "C" int32_t kinoko_actor_step_callback(KinokoActor *actor) {
    auto *callback=update(actor);
    const auto result=kinoko_script_callback_invoke(callback);
    if (result<0) kinoko_script_callback_clear(callback);
    return result;
}
extern "C" int32_t kinoko_actor_clear_script(KinokoActor *actor) {
    const ActorView view(actor);
    auto *instance=view.bytes(&ActorRecord::script_object);
    if (ObjectView(instance).value()._type==OT_INSTANCE) {
        kinoko_script_callback_clear(update(actor));
        kinoko_script_callback_clear(collision(actor));
        KinokoOwnedObjectWords empty{};
        (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(&empty)));
        // 45FC7A/94 target the CLASS defaults, not the outgoing instance.
        kinoko_sqplus_object_raw_set_object((void *)(g602), (const void *)(g601), (const void *)(&empty));
        kinoko_sqplus_object_raw_set_object((void *)(g602), (const void *)(g600), (const void *)(&empty));
        kinoko_sqplus_object_destroy((void *)(&empty));
    }
    return (int32_t)(intptr_t)(kinoko_sqplus_object_reset((void *)(instance)));
}
extern "C" int32_t __fastcall kinoko_actor_set_update_callback(KinokoActor *actor,void *,int32_t vtable,int32_t type,int32_t value) {
    LocalObject incoming(vtable,type,value);
    bind(update(actor),ActorView(actor).bytes(&ActorRecord::script_object),incoming.data());
    return incoming.release();
}
extern "C" int32_t __fastcall kinoko_actor_set_collision_callback(KinokoActor *actor,void *,int32_t vtable,int32_t type,int32_t value) {
    LocalObject incoming(vtable,type,value);
    {
        LocalObject environment, function;
        if (type==OT_CLOSURE) {
            environment.copy_from(ActorView(actor).bytes(&ActorRecord::script_object));
            function.copy_from(incoming.data());
            assign(collision(actor),current_vm(),environment.data(),function.data());
        } else kinoko_script_callback_clear(collision(actor));
        // Preserve cleanup of both empty compatibility temporaries too.
    }
    return incoming.release();
}
extern "C" int32_t __fastcall kinoko_camera_set_update_callback(KinokoCamera *camera,void *,int32_t vtable,int32_t type,int32_t value) {
    if (!camera) return 0;
    LocalObject incoming(vtable,type,value);
    retdec_trace("4663c0:begin");
    retdec_trace_i32("4663c0:this",address(camera));
    retdec_trace_i32("4663c0:argument-type",vtable);
    retdec_trace_i32("4663c0:argument-data",type);
    retdec_trace_i32("4663c0:argument-aux",value);
    const kinoko::camera::View view(camera);
    bind(reinterpret_cast<KinokoScriptCallback *>(view.bytes(&kinoko::camera::Record::update_vm)),
        view.bytes(&kinoko::camera::Record::script_object),incoming.data());
    incoming.release(); retdec_trace("4663c0:end"); return 0;
}
extern "C" int32_t __fastcall kinoko_camera_update(KinokoCamera *camera,void *) {
    const kinoko::camera::View view(camera);
    const auto type=ObjectView(view.bytes(&kinoko::camera::Record::update_function)).value()._type;
    if (type!=OT_CLOSURE) return type;
    return kinoko_script_callback_invoke(reinterpret_cast<KinokoScriptCallback *>(view.bytes(&kinoko::camera::Record::update_vm)));
}

extern "C" void kinoko_actor_clear_failed_collision_callback(KinokoActor *actor) {
    KinokoOwnedObjectWords empty{};
    (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(&empty)));
    kinoko_actor_set_collision_callback(actor,nullptr,empty.vtable,empty.type,empty.value);
}
