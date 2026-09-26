#include "kinoko/integer_map.h"
#include "kinoko/game_runtime.h"
extern "C" { KinokoGameMasks kinoko_game_masks{}; }
#include "kinoko/stage_cleanup.h"
#include "kinoko/stage_runtime.h"
#include "kinoko/stage_records.hpp"
#include "kinoko/act_document.h"
#include "kinoko/act_document_records.hpp"
#include "kinoko/act_resource_records.hpp"
#include "kinoko/act_source.h"
#include "kinoko/act_resource.h"
#include <type_traits>
#include "kinoko/legacy_memory.hpp"
#include <cstdio>
#include <cstdlib>
#include <new>

using namespace kinoko::stage;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
using kinoko::act::DocumentRecord;
using kinoko::act::DocumentView;
using kinoko::act::RuntimeRecord;
using RuntimeView = kinoko::native::RecordView<RuntimeRecord>;
static_assert(std::is_same_v<decltype(RuntimeRecord::source_holder), KinokoActSourceHolder *>);
static_assert(std::is_same_v<decltype(&kinoko_act_runtime_initialize),
    KinokoActRuntime *(*)(KinokoActRuntime *, KinokoActSourceHolder *)>);
namespace {
bool fail_next_new = false;
struct State {
    bool reject = false, throw_load = false, throw_runtime = false, throw_publish = false;
    bool fail_document = false, replace_runtime = false;
    int documents = 0, deletes = 0, runtimes = 0, runtime_deletes = 0;
    KinokoStageOwner *owner = nullptr;
    KinokoActRuntime *latest_runtime = nullptr;
    KinokoActSourceHolder *holder = nullptr;
} state;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "stage owner line %d: %s\n", __LINE__, #x); std::abort(); } } while (0)
int32_t __fastcall delete_document(KinokoActDocument *document, void *, int32_t flags) {
    CHECK(flags == 1);
    if (state.owner) CHECK(!OwnerView(state.owner).get(&OwnerRecord::holder));
    if (state.latest_runtime) {
        const RuntimeView runtime(state.latest_runtime);
        CHECK(runtime.get(&RuntimeRecord::source_holder) == state.holder);
        CHECK(!runtime.get(&RuntimeRecord::active_document));
    }
    ++state.deletes;
    // The real cleanup must re-read owner.runtime after this virtual callback.
    if (state.replace_runtime) {
        auto *replacement = static_cast<KinokoActRuntime *>(std::calloc(1, sizeof(RuntimeRecord)));
        CHECK(replacement && state.owner);
        const RuntimeView runtime(replacement);
        runtime.set(&RuntimeRecord::source_holder, state.holder);
        runtime.set(&RuntimeRecord::active_document, static_cast<KinokoActDocument *>(nullptr));
        std::free(state.latest_runtime); // fixture contains no runtime-owned objects
        state.latest_runtime = replacement;
        OwnerView(state.owner).set(&OwnerRecord::runtime, replacement);
    }
    std::free(document);
    return 1;
}
DocumentVirtuals virtuals{{}, reinterpret_cast<DeleteDocument>(delete_document)};
void reset() { CHECK(!fail_next_new); state = {}; }
}
// Deterministically fail the actual std::list node allocation, not a fake list.
void *operator new(std::size_t size) {
    if (fail_next_new) { fail_next_new = false; throw std::bad_alloc(); }
    if (auto *p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
extern "C" {
int32_t kinoko_stage_list_slot = 0, kinoko_stage_count = 0, kinoko_sound_lookup_count = 0;
KinokoIntegerMap* kinoko_sound_lookup = nullptr;
struct SQVM *kinoko_primary_vm = nullptr;
KinokoActDocument *kinoko_act_document_create() {
    if (state.fail_document) return nullptr;
    auto *document = static_cast<KinokoActDocument *>(std::calloc(1, sizeof(DocumentRecord)));
    CHECK(document);
    DocumentView(document).set(&DocumentRecord::vtable, static_cast<const void *>(&virtuals));
    ++state.documents;
    return document;
}
int32_t kinoko_act_document_load(KinokoActDocument *, const char *) {
    if (state.throw_load) throw std::bad_alloc();
    return state.reject ? 0 : 1;
}
int32_t kinoko_act_document_load_resources(KinokoActDocument *document, const char *prefix) { CHECK(document && prefix && !*prefix); return 0; }
const char *kinoko_act_document_name(const KinokoActDocument *) { return "fixture"; }
KinokoActRuntime *kinoko_act_runtime_initialize(KinokoActRuntime *storage, KinokoActSourceHolder *holder) {
    if (state.throw_runtime) throw std::bad_alloc();
    state.holder = holder;
    state.latest_runtime = storage;
    const RuntimeView runtime(state.latest_runtime);
    runtime.clear();
    runtime.set(&RuntimeRecord::source_holder, holder);
    runtime.set(&RuntimeRecord::active_document, static_cast<KinokoActDocument *>(nullptr));
    ++state.runtimes;
    return storage;
}
void kinoko_act_runtime_dispose(KinokoActRuntime *storage) {
    CHECK(storage == state.latest_runtime);
    const RuntimeView runtime(state.latest_runtime);
    CHECK(runtime.get(&RuntimeRecord::source_holder) == state.holder);
    CHECK(!runtime.get(&RuntimeRecord::active_document));
    CHECK(state.deletes == 1); // source document before runtime destruction
    ++state.runtime_deletes;
    state.latest_runtime = nullptr;
    // production stage cleanup owns the raw runtime allocation
}
int32_t kinoko_root_table_construct_this(KinokoActRuntime*, struct SQVM*, void*) {
    if (state.throw_publish) fail_next_new = true;
    return 1;
}
void kinoko_trace(const char *) {}
void kinoko_trace_i32(const char *, int32_t) {}
void kinoko_trace_squirrel_name(const char *, int32_t) {}
int32_t __fastcall kinoko_act_increment_frame(KinokoActRuntime *, void *) { return 0; }
int32_t kinoko_act_update_frame(int32_t) { return 0; }
int32_t kinoko_act_prepare_draw(int32_t) { return 0; }
int32_t kinoko_act_draw(int32_t, float, float) { return 0; }
KinokoIntegerMap* kinoko_integer_map_create() { return 0; }
void kinoko_integer_map_destroy(KinokoIntegerMap*) {}
void kinoko_integer_map_clear(KinokoIntegerMap*) {}
void kinoko_initialize_render_queue() {}
int32_t kinoko_clear_render_queue() { return 0; }
int32_t kinoko_audio_shutdown_resources() { return 0; }
}
int main() {
    kinoko_stage_owner_destroy(nullptr);
    // 466179 ignores the load status and continues to the resource pass.
    reset(); state.reject = true;
    state.owner = kinoko_stage_load("rejected");
    CHECK(state.owner && state.documents == 1 && !state.deletes && state.runtimes == 1);
    kinoko_stage_owner_destroy(state.owner);
    reset();
    state.owner = kinoko_stage_load("caller-owned");
    CHECK(state.owner && !kinoko_stage_list_slot && state.deletes == 0 && state.runtimes == 1);
    CHECK(kinoko_act_source_layer_count(OwnerView(state.owner).get(&OwnerRecord::holder)) == 0);
    kinoko_stage_owner_destroy(state.owner);
    CHECK(state.deletes == 1 && state.runtime_deletes == 1);
    reset(); state.replace_runtime = true;
    state.owner = kinoko_stage_load("virtual-callback");
    CHECK(state.owner);
    kinoko_stage_owner_destroy(state.owner);
    CHECK(state.deletes == 1 && state.runtime_deletes == 1);
    kinoko_stage_list_construct();
    reset();
    state.owner = kinoko_stage_load("published");
    CHECK(state.owner && kinoko_stage_count == 1 && state.deletes == 0);
    CHECK(kinoko_stage_list_value(kinoko_stage_list_first()) == state.owner);
    kinoko_clear_global_stages();
    CHECK(kinoko_stage_count == 0 && state.deletes == 1 && state.runtime_deletes == 1);
    kinoko_clear_global_stages();
    CHECK(state.deletes == 1);
    kinoko_stage_list_destroy();
    std::puts("PASS: ignored ACT load result, unchanged source borrow, virtual callback and caller/list ownership");
    return 0;
}
