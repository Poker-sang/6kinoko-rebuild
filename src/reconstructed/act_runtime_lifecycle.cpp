#include "kinoko/legacy_string.hpp"
#include "kinoko/act_frame.h"
#include "kinoko/act_resource.h"
#include "kinoko/act_resource_records.hpp"
#include "kinoko/stage_records.hpp"
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/squirrel_pair.hpp"
#include <cstdlib>
#include <new>
#include <map>
#include <memory>

namespace kinoko::act {
struct FindEntry {
    HANDLE handle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA data{};
    ~FindEntry() { if (handle != INVALID_HANDLE_VALUE) FindClose(handle); }
};
struct FindState {
    std::map<int32_t, std::unique_ptr<FindEntry>> entries;
};
}

namespace {
using namespace kinoko::act;
using kinoko::native::RecordView;
using kinoko::legacy::pointer;
using kinoko::legacy::address;

RecordView<RuntimeRecord> runtime(KinokoActRuntime *value) { return RecordView<RuntimeRecord>(value); }
int32_t* object_bytes(const RecordView<RuntimeRecord>& view) {
    return reinterpret_cast<int32_t*>(view.bytes(&RuntimeRecord::environment));
}
using FindMap = decltype(FindState::entries);
FindMap *finds(KinokoActRuntime *storage) {
    const auto state = storage ? runtime(storage).get(&RuntimeRecord::find_state) : nullptr;
    return state ? &state->entries : nullptr;
}
}

// Original 452150..4522C0. IDs count successful starts, independently of the
// number of live handles. FindNext writes the same per-search WIN32 data.
extern "C" int32_t kinoko_act_find_first(KinokoActRuntime *storage, const char* pattern) {
    auto* entries=finds(storage);
    if (!entries || !pattern) return 0;
    try {
        auto entry=std::make_unique<FindEntry>();
        entry->handle=FindFirstFileA(pattern,&entry->data);
        if (entry->handle==INVALID_HANDLE_VALUE) return 0;
        const auto view=runtime(storage);
        const auto id=view.get(&RuntimeRecord::next_find_id)+uint32_t{1};
        view.set(&RuntimeRecord::next_find_id,id);
        // Original map assignment replaces an entry if the 32-bit ID wraps.
        (*entries)[static_cast<int32_t>(id)]=std::move(entry);
        view.set(&RuntimeRecord::find_count,static_cast<uint32_t>(entries->size()));
        return static_cast<int32_t>(id);
    } catch (...) { return 0; }
}
extern "C" int32_t kinoko_act_find_next(KinokoActRuntime *storage, int32_t id) {
    auto* entries=finds(storage);
    if (!entries) return 0;
    const auto found=entries->find(id);
    return found!=entries->end() && FindNextFileA(found->second->handle,&found->second->data);
}
extern "C" int32_t kinoko_act_find_close(KinokoActRuntime *storage, int32_t id) {
    auto* entries=finds(storage);
    if (!entries) return 0;
    const auto found=entries->find(id);
    if (found==entries->end()) return 0;
    const auto handle=found->second->handle;
    found->second->handle=INVALID_HANDLE_VALUE;
    entries->erase(found); // Original erases even if the OS close fails.
    runtime(storage).set(&RuntimeRecord::find_count,static_cast<uint32_t>(entries->size()));
    return FindClose(handle)!=0;
}
extern "C" const char* kinoko_act_find_name(KinokoActRuntime *storage, int32_t id) {
    auto* entries=finds(storage);
    if (!entries) return nullptr;
    const auto found=entries->find(id);
    return found==entries->end() ? nullptr : found->second->data.cFileName;
}

// Original 44FDE0 writes selected members, not all 192 bytes. Keep unknown
// bytes untouched and let the C++ compiler implement bad_alloc unwinding.
extern "C" KinokoActRuntime *kinoko_act_runtime_initialize(
    KinokoActRuntime *storage, KinokoActSourceHolder *source_holder) {
    const RecordView<RuntimeRecord> view(storage);
    view.set(&RuntimeRecord::source_holder, source_holder);
    view.set(&RuntimeRecord::active_document, static_cast<KinokoActDocument *>(nullptr));
    view.set(&RuntimeRecord::active_holder, static_cast<KinokoActSourceHolder *>(nullptr));
    view.view(&RuntimeRecord::draw_commands).clear();
    view.view(&RuntimeRecord::draw_sprites).clear();
    view.set(&RuntimeRecord::find_count, uint32_t{0});
    view.set(&RuntimeRecord::find_state, new FindState);
    const auto name = view.view(&RuntimeRecord::name);
    name.set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
    name.set(&kinoko::legacy::StringRecord::length, uint32_t{0});
    *name.bytes(&kinoko::legacy::StringRecord::characters) = 0;
    view.set(&RuntimeRecord::current_time, int32_t{0});
    view.set(&RuntimeRecord::vm, static_cast<HSQUIRRELVM>(nullptr));
    view.set(&RuntimeRecord::stage_active, uint8_t{0});
    kinoko::script::pair::reset(object_bytes(view));
    view.set(&RuntimeRecord::render_target, uint32_t{0});
    view.set(&RuntimeRecord::next_find_id, uint32_t{0});
    view.set(&RuntimeRecord::wake_time, uint32_t{0});
    view.set(&RuntimeRecord::hidden, uint8_t{0});
    view.set(&RuntimeRecord::stage_state, std::array<uint32_t, 11>{});
    InitializeCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(view.bytes(&RuntimeRecord::lock)));
    return storage;
}

// Preserve the recovered 450020/4513F0 order: stop, unregister, find handles,
// lock, name, vectors, storage, cloned ACT, then the external VM reference.
extern "C" void kinoko_act_runtime_dispose(KinokoActRuntime *storage) {
    if (!storage) return;
    const auto view = runtime(storage);
    const auto vm = view.get(&RuntimeRecord::vm);
    const auto holder = view.get(&RuntimeRecord::source_holder);
    const auto source = holder
        ? RecordView<kinoko::stage::SourceHolderRecord>(holder).get(&kinoko::stage::SourceHolderRecord::document)
        : nullptr;
    kinoko_act_end_stage(storage, nullptr);
    auto environment = kinoko::script::pair::read(object_bytes(view));
    const kinoko::legacy::StringView name(view.bytes(&RuntimeRecord::name));
    if (vm && sq_type(environment) == OT_TABLE && name.length()) {
        const auto top = sq_gettop(vm);
        sq_pushobject(vm, environment);
        sq_pushstring(vm, name.data(), -1);
        sq_deleteslot(vm, -2, SQFalse);
        sq_settop(vm, top);
    }
    delete view.get(&RuntimeRecord::find_state);
    view.set(&RuntimeRecord::find_state, static_cast<FindState *>(nullptr));
    view.set(&RuntimeRecord::find_count, uint32_t{0});
    DeleteCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(view.bytes(&RuntimeRecord::lock)));
    name.destroy();
    // The existing recovered cleanup clears the first word, not the whole SSO buffer.
    std::memset(view.bytes(&RuntimeRecord::name), 0, sizeof(Address));
    view.view(&RuntimeRecord::name).set(&kinoko::legacy::StringRecord::length, uint32_t{0});
    view.view(&RuntimeRecord::name).set(&kinoko::legacy::StringRecord::capacity, uint32_t{15});
    kinoko_act_draw_storage_destroy(address(storage));
    std::free(view.get(&RuntimeRecord::active_holder));
    view.set(&RuntimeRecord::active_holder, static_cast<KinokoActSourceHolder *>(nullptr));
    const auto act = view.get(&RuntimeRecord::active_document);
    if (act && act != source) retdec_destroy_cact_with_flags(address(act), 1);
    view.set(&RuntimeRecord::active_document, static_cast<KinokoActDocument *>(nullptr));
    environment = kinoko::script::pair::read(object_bytes(view));
    if (vm && (sq_type(environment) & SQOBJECT_REF_COUNTED))
        kinoko::script::pair::release(vm, object_bytes(view));
    kinoko::script::pair::reset(object_bytes(view));
    view.set(&RuntimeRecord::vm, static_cast<HSQUIRRELVM>(nullptr));
}
