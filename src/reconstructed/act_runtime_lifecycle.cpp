#include "kinoko/act_resource.h"
#include "kinoko/act_resource_records.hpp"
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/squirrel_pair.hpp"
#include <cstdlib>
#include <new>

extern "C" int32_t _3f__3f_2_40_YAPAXI_40_Z(int32_t size);

namespace {
using namespace kinoko::act;
using kinoko::native::RecordView;
using kinoko::legacy::pointer;
using kinoko::legacy::address;

RecordView<RuntimeRecord> runtime(int32_t value) { return RecordView<RuntimeRecord>(pointer<void>(value)); }
int32_t* object_bytes(const RecordView<RuntimeRecord>& view) {
    return reinterpret_cast<int32_t*>(view.bytes(&RuntimeRecord::environment));
}
void release_find_tree(Address node) {
    for (;;) {
        const RecordView<FindNode> item(pointer<void>(node));
        if (item.get(&FindNode::sentinel)) return;
        const auto left = item.get(&FindNode::left);
        release_find_tree(item.get(&FindNode::right));
        FindClose(item.get(&FindNode::handle));
        std::free(item.data());
        node = left;
    }
}
void release_vector(const RecordView<VectorStorage>& vector) {
    std::free(pointer<void>(vector.get(&VectorStorage::begin)));
    vector.clear();
}
}

// Original 44FDE0 writes selected members, not all 192 bytes. Keep unknown
// bytes untouched and let the C++ compiler implement bad_alloc unwinding.
extern "C" int32_t function_44fde0(int32_t storage, int32_t source_holder) {
    const auto view = runtime(storage);
    view.set(&RuntimeRecord::source_holder, static_cast<Address>(source_holder));
    view.set(&RuntimeRecord::act, Address{0});
    view.set(&RuntimeRecord::owned_storage, Address{0});
    view.view(&RuntimeRecord::draw_commands).clear();
    view.view(&RuntimeRecord::draw_sprites).clear();
    view.set(&RuntimeRecord::find_count, uint32_t{0});
    const int32_t head = _3f__3f_2_40_YAPAXI_40_Z(sizeof(FindNode));
    if (!head) throw std::bad_alloc();
    view.set(&RuntimeRecord::find_head, static_cast<Address>(head));
    const RecordView<FindNode> sentinel(pointer<void>(head));
    sentinel.set(&FindNode::left, static_cast<Address>(head));
    sentinel.set(&FindNode::parent, static_cast<Address>(head));
    sentinel.set(&FindNode::right, static_cast<Address>(head));
    sentinel.set(&FindNode::color, uint8_t{1});
    sentinel.set(&FindNode::sentinel, uint8_t{1});
    view.set(&RuntimeRecord::name_capacity, uint32_t{15});
    view.set(&RuntimeRecord::name_length, uint32_t{0});
    *view.bytes(&RuntimeRecord::name_storage) = 0;
    view.set(&RuntimeRecord::current_time, int32_t{0});
    view.set(&RuntimeRecord::vm, Address{0});
    view.set(&RuntimeRecord::stage_active, uint8_t{0});
    kinoko::script::pair::reset(object_bytes(view));
    view.set(&RuntimeRecord::field76, uint32_t{0});
    view.set(&RuntimeRecord::field96, uint32_t{0});
    view.set(&RuntimeRecord::wake_time, uint32_t{0});
    view.set(&RuntimeRecord::hidden, uint8_t{0});
    view.set(&RuntimeRecord::stage_state, std::array<uint32_t, 11>{});
    InitializeCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(view.bytes(&RuntimeRecord::lock)));
    return storage;
}

// Preserve the recovered 450020/4513F0 order: stop, unregister, find handles,
// lock, name, vectors, storage, cloned ACT, then the external VM reference.
extern "C" void retdec_destroy_act_runtime(int32_t storage) {
    const auto view = runtime(storage);
    const auto vm = pointer<SQVM>(view.get(&RuntimeRecord::vm));
    const auto holder = view.get(&RuntimeRecord::source_holder);
    const Address source = holder ? *pointer<Address>(holder) : 0;
    kinoko_act_end_stage(storage, nullptr);
    auto environment = kinoko::script::pair::read(object_bytes(view));
    if (vm && sq_type(environment) == OT_TABLE && view.get(&RuntimeRecord::name_length)) {
        const auto top = sq_gettop(vm);
        sq_pushobject(vm, environment);
        const auto* name = view.bytes(&RuntimeRecord::name_storage);
        Address heap_name;
        if (view.get(&RuntimeRecord::name_capacity) >= 16) {
            std::memcpy(&heap_name, name, sizeof(heap_name));
            name = pointer<unsigned char>(heap_name);
        }
        sq_pushstring(vm, reinterpret_cast<const SQChar*>(name), -1);
        sq_deleteslot(vm, -2, SQFalse);
        sq_settop(vm, top);
    }
    if (const auto head = view.get(&RuntimeRecord::find_head)) {
        const RecordView<FindNode> sentinel(pointer<void>(head));
        release_find_tree(sentinel.get(&FindNode::parent));
        std::free(sentinel.data());
        view.set(&RuntimeRecord::find_head, Address{0});
        view.set(&RuntimeRecord::find_count, uint32_t{0});
    }
    DeleteCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(view.bytes(&RuntimeRecord::lock)));
    if (view.get(&RuntimeRecord::name_capacity) >= 16) {
        Address heap_name;
        std::memcpy(&heap_name, view.bytes(&RuntimeRecord::name_storage), sizeof(heap_name));
        std::free(pointer<void>(heap_name));
    }
    // The existing recovered cleanup clears the first word, not the whole SSO buffer.
    std::memset(view.bytes(&RuntimeRecord::name_storage), 0, sizeof(Address));
    view.set(&RuntimeRecord::name_length, uint32_t{0});
    view.set(&RuntimeRecord::name_capacity, uint32_t{15});
    release_vector(view.view(&RuntimeRecord::draw_sprites));
    release_vector(view.view(&RuntimeRecord::draw_commands));
    std::free(pointer<void>(view.get(&RuntimeRecord::owned_storage)));
    view.set(&RuntimeRecord::owned_storage, Address{0});
    const auto act = view.get(&RuntimeRecord::act);
    if (act && act != source) retdec_destroy_cact_with_flags(act, 1);
    view.set(&RuntimeRecord::act, Address{0});
    environment = kinoko::script::pair::read(object_bytes(view));
    if (vm && (sq_type(environment) & SQOBJECT_REF_COUNTED))
        kinoko::script::pair::release(vm, object_bytes(view));
    kinoko::script::pair::reset(object_bytes(view));
    view.set(&RuntimeRecord::vm, Address{0});
}

extern "C" int32_t function_450020(int32_t resource) {
    if (resource) retdec_destroy_act_runtime(resource);
    return resource;
}
