// Real map loader; fault-controlled ports verify original failure policy.
#include "kinoko/map_manager_records.hpp"
#include "kinoko/act_document.h"
#include "kinoko/act_source.h"
#include "kinoko/act_resource.h"
#include "kinoko/act_runtime.h"
#include "kinoko/act_layer_access.h"
#include "kinoko/stage_records.hpp"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_host_compat.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"map load line %d: %s\n",__LINE__,#x); std::abort(); } } while(0)
using namespace kinoko::map;
using namespace kinoko::script;
namespace {
ManagerRecord record{};
auto *manager=reinterpret_cast<KinokoMapManager *>(&record);
auto *vm=reinterpret_cast<SQVM *>(1);
bool load_ok=true, throw_instance=false;
int clears=0, deletes=0;
std::vector<int> calls;
void __fastcall delete_document(KinokoActDocument *p,void *,int flags) {CHECK(flags==1);++deletes;std::free(p);}
const void *methods[]={nullptr,nullptr,nullptr,nullptr,reinterpret_cast<void *>(delete_document)};
}
extern "C" {
void retdec_trace(const char *) {}
void retdec_trace_i32(const char *,int32_t) {}
void retdec_trace_squirrel_name(const char *,int32_t) {}
int32_t kinoko_squirrel_object_vtable() {return 0;}
void sq_resetobject(HSQOBJECT *o) {o->_type=OT_NULL;o->_unVal.pUserPointer=nullptr;}
void sq_pushstring(HSQUIRRELVM,const SQChar *,SQInteger) {}
void sq_pop(HSQUIRRELVM,SQInteger) {}
void kinoko_map_manager_clear(KinokoMapManager *) {
    ++clears;std::free(record.player);std::free(record.source_holder);std::free(record.source_act);
    record.player=nullptr;record.source_holder=nullptr;record.source_act=nullptr;
}
KinokoActDocument *kinoko_act_document_create() {
    auto *p=static_cast<const void **>(std::calloc(1,240));*p=methods;
    return reinterpret_cast<KinokoActDocument *>(p);
}
int32_t kinoko_act_document_load(KinokoActDocument *,const char *) {return load_ok;}
int32_t kinoko_act_document_load_resources(KinokoActDocument *,const char *) {calls.push_back(1);return 0;}
const char *kinoko_act_document_name(const KinokoActDocument *) {return "fixture";}
int32_t kinoko_act_document_screen_width(const KinokoActDocument *) {return 320;}
int32_t kinoko_act_document_screen_height(const KinokoActDocument *) {return 240;}
KinokoActSourceHolder *kinoko_act_source_initialize(KinokoActSourceHolder *h,KinokoActDocument *d) {h->document=d;return h;}
KinokoActRuntime *kinoko_act_source_create_runtime(KinokoActSourceHolder *) {return static_cast<KinokoActRuntime *>(std::malloc(192));}
int32_t kinoko_act_source_layer_count(const KinokoActSourceHolder *) {return 0;}
void kinoko_act_runtime_dispose(KinokoActRuntime *) {}
KinokoActLayout *kinoko_act_layer_layout(KinokoActRuntime *,int32_t) {return nullptr;}
int32_t retdec_root_table_construct_this(int32_t p,int32_t,int32_t) {CHECK(p==address(record.player));calls.push_back(2);return -1;}
int32_t retdec_begin_stage_this(int32_t p,int32_t) {CHECK(p==address(record.player));calls.push_back(3);return -2;}
void *kinoko_sqplus_object_assign(void *p,const void *v) {ObjectView(p).write(ObjectView(v).value());return p;}
int32_t kinoko_sqplus_object_set_instance(void *,void *) {return 1;}
int32_t kinoko_sqplus_object_raw_set_name(void *,const char *key,const void *) {
    if(!std::strcmp(key,"map"))calls.push_back(5);
    if(!std::strcmp(key,"currentMap"))calls.push_back(6);
    return 1;
}
void *kinoko_sqplus_object_new_array(void *p,int32_t) {return p;}
SQVM *kinoko_sqplus_object_append(void *,const void *) {return vm;}
int32_t kinoko_sqplus_object_reverse(void *) {return 1;}
void *kinoko_sqplus_object_get_value(void *,void *p,const char *) {return p;}
}
namespace kinoko::script::upstream {
HSQOBJECT sqplus_create_instance(HSQUIRRELVM,HSQOBJECT) {
    calls.push_back(4);if(throw_instance)throw std::runtime_error("fixture factory failure");
    return borrowed_value(OT_INSTANCE,0);
}
void sqplus_release(HSQUIRRELVM,HSQOBJECT) {}
HSQOBJECT sqplus_capture(HSQUIRRELVM,HSQOBJECT,int) {return borrowed_value(OT_NULL,0);}
}
int main() {
    ObjectStorage klass{},root{};
    load_ok=false;
    CHECK(!kinoko_map_manager_load(manager,"map.act",vm,&klass,&root));
    CHECK(clears==1 && deletes==1 && !record.source_act && calls.empty());
    load_ok=true;
    CHECK(kinoko_map_manager_load(manager,"map.act",vm,&klass,&root)==1);
    CHECK((calls==std::vector<int>{1,2,3,4,5,6}));
    CHECK(clears==2 && record.source_act && record.source_holder && record.player);
    CHECK(record.width==320 && record.height==240);
    calls.clear();throw_instance=true;
    bool caught=false;
    try {kinoko_map_manager_load(manager,"map.act",vm,&klass,&root);}
    catch(const std::runtime_error &) {caught=true;}
    CHECK(caught && clears==3 && record.source_act && record.source_holder && record.player);
    CHECK((calls==std::vector<int>{1,2,3,4}));
    kinoko_map_manager_clear(manager);
}
