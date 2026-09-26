#include "kinoko/actor_manager.h"
#include "kinoko/actor_records.hpp"
#include "kinoko/actor_animation.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_host_object.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>
using namespace kinoko::actor;
using kinoko::script::address;
namespace {
ActorRecord fixture_actor{};
KinokoOwnedObjectWords klass{reinterpret_cast<const void*>(1),OT_CLASS,9};
std::array<int,16> refs{};
std::vector<int> releases,copies;
bool factory_fails=false,call_throws=false,replace_inputs=false, copy_throws=false;
ControlRecord control{};
int strong=0,advanced_take=0;
KinokoActor *self() { return reinterpret_cast<KinokoActor *>(&fixture_actor); }
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"actor init line %d: %s\n",__LINE__,#x); std::abort(); } } while (0)
KinokoOwnedObjectWords read(const void *p) { KinokoOwnedObjectWords o; std::memcpy(&o,p,sizeof(o)); return o; }
void write(void *p,KinokoOwnedObjectWords o) { std::memcpy(p,&o,sizeof(o)); }
void retain(KinokoOwnedObjectWords o) { if(o.value) ++refs[o.value]; }
void drop(KinokoOwnedObjectWords o) { if(o.value) { CHECK(refs[o.value]>0); --refs[o.value]; releases.push_back(o.value); } }
void clear() {
    for(auto member:script_members) kinoko_sqplus_object_destroy((fixture_actor.*member).data());
    if(fixture_actor.owner) { std::free(fixture_actor.owner); fixture_actor.owner=nullptr; }
    fixture_actor={}; refs.fill(0); copies.clear(); releases.clear(); strong=0;
}
}
namespace kinoko::script::upstream {
HSQOBJECT sqplus_create_instance(HSQUIRRELVM,HSQOBJECT type) {
    CHECK(type._type==OT_CLASS);
    CHECK(refs[1]>=2 && refs[2]>=2); // copies precede all field replacement
    if(factory_fails) throw std::runtime_error("factory");
    if(replace_inputs) { // factory/release hooks may replace the caller's slots
        kinoko_sqplus_object_destroy(fixture_actor.initial_function.data());
        kinoko_sqplus_object_destroy(fixture_actor.initial_argument.data());
    }
    ++refs[3]; return borrowed_value(OT_INSTANCE,3);
}
}
extern "C" {
const void* kinoko_squirrel_object_vtable() { return reinterpret_cast<const void*>(1); }
SQVM *kinoko_actor_default_vm() { return reinterpret_cast<SQVM *>(1); }
void *kinoko_actor_class_object() { return &klass; }
void *kinoko_sqplus_object_initialize(void *p) { write(p,{reinterpret_cast<const void*>(1),OT_NULL,0}); return p; }
void *kinoko_sqplus_object_copy_construct(void *out,const void *in) {
    auto o=read(in);
    if(copy_throws && o.value==1 && copies.size()>2) throw std::runtime_error("copy");
    retain(o); write(out,o); copies.push_back(o.value); return out;
}
void *kinoko_sqplus_object_construct_value(void *out,int32_t type,int32_t value) {
    write(out,{reinterpret_cast<const void*>(1),type,value}); retain(read(out)); return out;
}
void *kinoko_sqplus_object_assign(void *out,const void *in) {
    auto o=read(in); retain(o); drop(read(out)); write(out,o); return out;
}
void *kinoko_sqplus_object_destroy(void *p) { drop(read(p)); write(p,{reinterpret_cast<const void*>(1),OT_NULL,0}); return p; }
int32_t kinoko_sqplus_object_type(void *p) { return read(p).type; }
int32_t kinoko_sqplus_object_set_instance(void *p,void *native) { CHECK(read(p).value==3 && native==self()); return 1; }
void* kinoko_native_control_create(void* out,void*) {
    *reinterpret_cast<int32_t *>(out)=address(&control); strong=1; return out;
}
void kinoko_native_add_strong(void* p) { CHECK(p==&control); ++strong; }
void kinoko_native_release_strong(void* p) { if(p) { CHECK(p==&control); --strong; } }
KinokoScriptCallback *kinoko_script_callback_construct(KinokoScriptCallback *out,const char *name) {
    CHECK(!name); out->vm=kinoko_actor_default_vm();
    kinoko_sqplus_object_initialize(&out->environment); kinoko_sqplus_object_initialize(&out->closure); return out;
}
int32_t kinoko_script_callback_invoke_owned(KinokoScriptCallback *call,KinokoOwnedObjectWords *arg,int32_t,int32_t) {
    CHECK(call->closure.value==1 && call->environment.value==3 && arg->value==2);
    CHECK(strong==1 && fixture_actor.owner && *fixture_actor.owner==self());
    // Init is allowed to replace its own saved arguments during the callback.
    kinoko_sqplus_object_destroy(fixture_actor.initial_function.data());
    kinoko_sqplus_object_destroy(fixture_actor.initial_argument.data());
    CHECK(refs[1]>=2 && refs[2]>=2);
    kinoko_sqplus_object_destroy(arg); // consuming callee owns argument on error too
    if(call_throws) throw std::runtime_error("callback");
    fixture_actor.x=30; fixture_actor.y=40; fixture_actor.local_bounds={-2,-3,5,7};
    return -1; // original ignores a negative script result
}
int32_t kinoko_actor_trace_step_begin(KinokoActor *,int32_t) { return 0; }
void kinoko_actor_trace_step_end(KinokoActor *,int32_t,int32_t) {}
int32_t kinoko_actor_step_callback(KinokoActor *) { fixture_actor.take=99; return 0; }
void kinoko_actor_advance_animation(KinokoActor *,int32_t take) { advanced_take=take; }
KinokoActor *kinoko_actor_manager_create(KinokoActorManager *,const KinokoOwnedObjectWords *,float,float,float,const KinokoOwnedObjectWords *,const void *) { return nullptr; }
}
int main() {
    const KinokoOwnedObjectWords fn{reinterpret_cast<const void*>(1),OT_CLOSURE,1},arg{reinterpret_cast<const void*>(1),OT_TABLE,2};
    for(int mode=0;mode<4;++mode) {
        clear(); refs[1]=refs[2]=1;
        write(fixture_actor.initial_function.data(),fn); write(fixture_actor.initial_argument.data(),arg);
        fixture_actor.pool_handle=0x20042;
        factory_fails=mode==1; call_throws=mode==2; replace_inputs=mode==0; copy_throws=mode==3;
        bool threw=false;
        try {
            CHECK(kinoko_actor_initialize(self(),nullptr,
                reinterpret_cast<const KinokoOwnedObjectWords *>(fixture_actor.initial_function.data()),1,2,-1,
                reinterpret_cast<const KinokoOwnedObjectWords *>(fixture_actor.initial_argument.data()))==1);
        } catch(const std::runtime_error&) { threw=true; }
        CHECK(threw==(mode!=0)); CHECK(copies[0]==2 && copies[1]==1);
        CHECK(releases[releases.size()-2]==1 && releases.back()==2);
        CHECK(fixture_actor.id==0x42);
        if(mode==1) { CHECK(!fixture_actor.owner && refs[1]==1 && refs[2]==1); }
        else if(mode==3) { CHECK(refs[1]==1 && refs[2]==1 && refs[3]==1); }
        else {
            CHECK(refs[1]==0 && refs[2]==0 && refs[3]==1);
            if(mode==0) CHECK(fixture_actor.world_bounds.left==28 && fixture_actor.world_bounds.top==37 && fixture_actor.world_bounds.right==35 && fixture_actor.previous_y==40);
        }
    }
    clear(); fixture_actor.take=7; write(fixture_actor.update_function.data(),{reinterpret_cast<const void*>(1),OT_CLOSURE,0});
    kinoko_actor_tick(self()); CHECK(fixture_actor.take==99 && advanced_take==7);
    clear();
}
