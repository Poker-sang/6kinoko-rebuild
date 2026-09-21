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
        CHECK(!runtime.get(&RuntimeRecord::source_holder));
        CHECK(!runtime.get(&RuntimeRecord::act));
    }
    ++state.deletes;
    // The real cleanup must re-read owner.runtime after this virtual callback.
    if (state.replace_runtime) {
        auto *replacement = static_cast<KinokoActRuntime *>(std::calloc(1, sizeof(RuntimeRecord)));
        CHECK(replacement && state.owner);
        const RuntimeView runtime(replacement);
        runtime.set(&RuntimeRecord::source_holder, state.holder);
        runtime.set(&RuntimeRecord::act, static_cast<uint32_t>(address(document)));
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
int32_t g603 = 0, g604 = 0, g638 = 0, g639 = 0, g459 = 0;
char *g644 = nullptr;
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
const char *kinoko_act_document_name(const KinokoActDocument *) { return "fixture"; }
KinokoActRuntime *kinoko_act_runtime_initialize(KinokoActRuntime *storage, KinokoActSourceHolder *holder) {
    if (state.throw_runtime) throw std::bad_alloc();
    state.holder = holder;
    state.latest_runtime = storage;
    const RuntimeView runtime(state.latest_runtime);
    runtime.clear();
    runtime.set(&RuntimeRecord::source_holder, holder);
    runtime.set(&RuntimeRecord::act, static_cast<uint32_t>(address(state.holder->document)));
    ++state.runtimes;
    return storage;
}
int32_t function_450020(int32_t storage) {
    CHECK(pointer<KinokoActRuntime>(storage) == state.latest_runtime);
    const RuntimeView runtime(state.latest_runtime);
    CHECK(!runtime.get(&RuntimeRecord::source_holder));
    CHECK(!runtime.get(&RuntimeRecord::act));
    CHECK(state.deletes == 1); // source document before runtime destruction
    ++state.runtime_deletes;
    state.latest_runtime = nullptr;
    return storage; // production stage cleanup owns the raw runtime allocation
}
int32_t retdec_root_table_construct_this(int32_t, int32_t, int32_t) {
    if (state.throw_publish) fail_next_new = true;
    return 1;
}
void retdec_trace(const char *) {}
void retdec_trace_i32(const char *, int32_t) {}
void retdec_trace_squirrel_name(const char *, int32_t) {}
int32_t __fastcall kinoko_act_increment_frame(int32_t, void *) { return 0; }
int32_t kinoko_act_update_frame(int32_t) { return 0; }
int32_t kinoko_act_prepare_draw(int32_t) { return 0; }
int32_t kinoko_act_draw(int32_t, float, float) { return 0; }
int32_t kinoko_integer_map_create() { return 0; }
void kinoko_integer_map_destroy(int32_t) {}
void kinoko_integer_map_clear(int32_t) {}
void kinoko_initialize_render_queue() {}
int32_t kinoko_clear_render_queue() { return 0; }
int32_t function_40b3a0() { return 0; }
}
int main() {
    kinoko_stage_owner_destroy(nullptr);
    reset(); state.fail_document = true;
    CHECK(!kinoko_stage_load("allocation-failure"));
    CHECK(!state.documents && !state.deletes);
    reset(); state.reject = true;
    CHECK(!kinoko_stage_load("rejected"));
    CHECK(state.documents == 1 && state.deletes == 1 && !state.runtimes);
    for (int fault = 0; fault < 2; ++fault) {
        reset(); state.throw_load = fault == 0; state.throw_runtime = fault == 1;
        bool caught = false;
        try { kinoko_stage_load("unwind"); } catch (const std::bad_alloc &) { caught = true; }
        CHECK(caught && state.documents == 1 && state.deletes == 1 && !state.runtime_deletes);
    }
    reset();
    state.owner = kinoko_stage_load("caller-owned");
    CHECK(state.owner && !g603 && state.deletes == 0 && state.runtimes == 1);
    CHECK(kinoko_act_source_layer_count(OwnerView(state.owner).get(&OwnerRecord::holder)) == 0);
    kinoko_stage_owner_destroy(state.owner);
    CHECK(state.deletes == 1 && state.runtime_deletes == 1);
    reset(); state.replace_runtime = true;
    state.owner = kinoko_stage_load("virtual-callback");
    CHECK(state.owner);
    kinoko_stage_owner_destroy(state.owner);
    CHECK(state.deletes == 1 && state.runtime_deletes == 1);
    kinoko_stage_list_construct();
    reset(); g644 = reinterpret_cast<char *>(&state);
    state.throw_publish = true;
    bool caught = false;
    try { kinoko_stage_load("list-allocation-failure"); } catch (const std::bad_alloc &) { caught = true; }
    CHECK(caught && !fail_next_new && g604 == 0);
    CHECK(state.deletes == 1 && state.runtime_deletes == 1);
    g644 = nullptr;
    reset();
    state.owner = kinoko_stage_load("published");
    CHECK(state.owner && g604 == 1 && state.deletes == 0);
    CHECK(kinoko_stage_list_value(kinoko_stage_list_first()) == state.owner);
    kinoko_clear_global_stages();
    CHECK(g604 == 0 && state.deletes == 1 && state.runtime_deletes == 1);
    kinoko_clear_global_stages();
    CHECK(state.deletes == 1);
    kinoko_stage_list_destroy();
    std::puts("PASS: rejected ACT, loader/runtime/list exceptions, borrowed pointers, virtual callback, caller/list ownership");
    return 0;
}
