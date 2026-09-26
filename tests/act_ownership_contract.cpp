// Production destructors with observable virtual callbacks. Compile-only handoff.
#include "kinoko/act_document.h"
#include "kinoko/act_document_records.hpp"
#include "kinoko/act_ownership.hpp"
#include "kinoko/act_layer_lifecycle.h"
#include "kinoko/act_layer_storage.hpp"
#include "kinoko/act_resource_records.hpp"
#include "kinoko/act_resource.h"
#include "kinoko/act_runtime.h"
#include "kinoko/act_key_records.hpp"
#include "kinoko/act_array.h"
#include "kinoko/act_list.h"
#include "kinoko/act_host.h"
#include "kinoko/stage_records.hpp"
#include <cstdlib>
#include <cstdio>
#include <vector>
using namespace kinoko::act;
using kinoko::legacy::address;
#define REQUIRE(x) do { if (!(x)) { std::fprintf(stderr,"ownership line %d: %s\n",__LINE__,#x); std::abort(); } } while(0)
namespace {
std::vector<int> events;
DocumentRecord *observed_document;
LayerStorageRecord *observed_layer;
struct Payload { const void *methods; int id; };
void __fastcall dispose(Payload *value, void *) {
    events.push_back(value->id);
    if (observed_document) {
        REQUIRE(observed_document->layers.storage);
        REQUIRE(observed_document->resources.storage);
    }
    if (observed_layer) {
        REQUIRE(observed_layer->keys.head && observed_layer->timelines.head);
        REQUIRE(observed_layer->script.bytes); // embedded script still alive
    }
}
void __fastcall destroy_clone(KinokoActDocument *, void *, int flags) {
    REQUIRE(flags == 1); events.push_back(9);
}
KinokoActDocument *__fastcall clone_document(KinokoActDocument *,void *) {
    events.push_back(8);return kinoko_act_document_create();
}
const void *source_methods[] = {nullptr,nullptr,nullptr,nullptr,nullptr,reinterpret_cast<void *>(clone_document)};
const void *payload_methods[] = {nullptr,nullptr,nullptr,reinterpret_cast<void *>(dispose)};
const void *clone_methods[] = {nullptr,nullptr,nullptr,nullptr,reinterpret_cast<void *>(destroy_clone)};
}
extern "C" int kinoko_test_act_ownership_chain() {
    auto *document = kinoko_act_document_create();
    REQUIRE(document);
    observed_document = reinterpret_cast<DocumentRecord *>(document);
    Payload layer{payload_methods,1}, resource{payload_methods,2};
    kinoko_act_array_append((void*)(uintptr_t)(address(&observed_document->layers)), (void*)(uintptr_t)(address(&layer)));
    kinoko_act_array_append((void*)(uintptr_t)(address(&observed_document->resources)), (void*)(uintptr_t)(address(&resource)));
    delete_document(document);
    observed_document=nullptr;
    REQUIRE((events==std::vector<int>{1,2}));

    // Reverse array destruction, independently observable per document.
    auto *allocation=static_cast<unsigned char *>(std::malloc(4+2*sizeof(DocumentRecord)));
    *reinterpret_cast<uint32_t *>(allocation)=2;
    auto *documents=reinterpret_cast<DocumentRecord *>(allocation+4);
    Payload first{payload_methods,3},last{payload_methods,4};
    for(int i=0;i<2;++i) {
        kinoko_act_document_initialize(reinterpret_cast<KinokoActDocument *>(documents+i));
        kinoko_act_array_append((void*)(uintptr_t)(address(&documents[i].layers)), (void*)(uintptr_t)(address(i?&last:&first)));
    }
    (int32_t)(intptr_t)kinoko_destroy_cact_with_flags((KinokoActDocument*)(uintptr_t)(address(documents)), 3);
    REQUIRE((events==std::vector<int>{1,2,4,3}));

    // Key owns a layout whose slot 4 is deliberately absent (not a CAct dtor).
    auto *key=static_cast<KeyRecord *>(std::calloc(1,sizeof(KeyRecord)));
    key->methods=kinoko_act_host_symbols()->key_vtable;
    key->script_name.capacity=15;
    Payload layout{payload_methods,5};
    key->layout=reinterpret_cast<KinokoActLayout *>(&layout);
    dispose_owned(reinterpret_cast<KinokoActKey *>(key));
    REQUIRE(events.back()==5);

    auto *storage=static_cast<LayerStorageRecord *>(std::malloc(sizeof(LayerStorageRecord)));
    kinoko_act_layer_initialize(reinterpret_cast<KinokoActLayer *>(storage),nullptr);
    observed_layer=storage;
    storage->script.bytes=std::malloc(8);storage->script.size=8;
    Payload key_payload{payload_methods,6},timeline{payload_methods,7};
    REQUIRE(kinoko_act_append_list((void*)(uintptr_t)(address(&storage->keys.head)), (void*)(uintptr_t)(address(&key_payload))));
    REQUIRE(kinoko_act_append_list((void*)(uintptr_t)(address(&storage->timelines.head)), (void*)(uintptr_t)(address(&timeline))));
    dispose_owned(reinterpret_cast<KinokoActLayer *>(storage));
    observed_layer=nullptr;
    REQUIRE((events==std::vector<int>{1,2,4,3,5,6,7}));

    RuntimeRecord runtime{};
    kinoko_act_runtime_initialize(reinterpret_cast<KinokoActRuntime *>(&runtime),nullptr);
    Payload clone{clone_methods,0};
    runtime.active_document=reinterpret_cast<KinokoActDocument *>(&clone);
    kinoko_act_runtime_dispose(reinterpret_cast<KinokoActRuntime *>(&runtime));
    REQUIRE(events.back()==9 && !runtime.active_document);
    kinoko_act_runtime_initialize(reinterpret_cast<KinokoActRuntime *>(&runtime),nullptr);
    Payload source{source_methods,0};
    KinokoActSourceHolder holder{reinterpret_cast<KinokoActDocument *>(&source)};
    runtime.source_holder=&holder;
    runtime.active_document=reinterpret_cast<KinokoActDocument *>(&clone);
    REQUIRE(kinoko_bind_act_resource_object((KinokoActRuntime*)(uintptr_t)(address(&runtime))));
    REQUIRE(events[events.size()-2]==8 && events.back()==9);
    REQUIRE(runtime.active_document && runtime.active_document != holder.document);
    REQUIRE(runtime.active_holder->document==runtime.active_document);
    kinoko_act_runtime_dispose(reinterpret_cast<KinokoActRuntime *>(&runtime));
    return 0;
}
