from pathlib import Path
import re, hashlib
r = Path.cwd()
p=r/'src/reconstructed/actor_methods.cpp';s=p.read_text()
s=s.replace('#include "kinoko/actor_animation.h"','#include "kinoko/actor_animation.h"\n#include "kinoko/actor_records.hpp"\n#include "kinoko/legacy_memory.hpp"')
s=s.replace('class ActorView {','using namespace kinoko::actor;\nusing kinoko::legacy::pointer;\nusing kinoko::legacy::address;\n\nclass ActorMethods {')
s=s.replace('explicit ActorView(int32_t address) : address_(address) {}','explicit ActorMethods(int32_t address) : address_(address), view_(pointer(address)) {}')
s=s.replace('write(chip_flags_offset, extended);','view_.view(&ActorRecord::initial).set(&InitialData::chip_flags, extended);')
s=s.replace('write(chip_bound_type_offset, shape);','view_.view(&ActorRecord::initial).set(&InitialData::chip_bound_type, shape);')
s=s.replace('read<float>(x_offset)','view_.get(&ActorRecord::x)').replace('read<float>(y_offset)','view_.get(&ActorRecord::y)')
s=s.replace('address_ + chip_ids_offset + 4 * layer','address(view_.bytes(&ActorRecord::chip_cache_storage) + sizeof(int32_t) * layer)')
s=s.replace('write(priority_offset, priority);','view_.set(&ActorRecord::priority, priority);')
s=s.replace('write<uint8_t>(release_pending_offset, 1);\n        ActorView(read<int32_t>(manager_offset)).write<uint8_t>(\n            manager_cleanup_pending_offset, 1);','view_.set(&ActorRecord::release_pending, uint8_t{1});\n        ManagerView(pointer(static_cast<int32_t>(view_.get(&ActorRecord::manager))))\n            .set(&ManagerPrefix::cleanup_pending, uint8_t{1});')
s=s.replace('std::memcpy(bytes() + init_data_offset, ActorView(source).bytes(), 48);','std::memcpy(view_.bytes(&ActorRecord::initial), pointer(source), sizeof(InitialData));')
s=s.replace('read<int32_t>(animation_offset)','view_.get(&ActorRecord::animation)')
a=s.index('    enum Offset {');b=s.index('    int32_t address_;',a)
s=s[:a]+s[b:];s=s.replace('    int32_t address_;','    int32_t address_;\n    ActorView view_;')
s=s.replace('return ActorView(actor).','return ActorMethods(actor).')
p.write_text(s)
p=r/'src/squirrel/actor_lifecycle.cpp';s=p.read_text()
s=s.replace('#include "kinoko/legacy_abi.h"','#include "kinoko/actor_records.hpp"')
a=s.index('// Views do not turn legacy');b=s.index('void raw_set_step',a)
s=s[:a]+'''using namespace kinoko::actor;

// Boost-style native strong/weak counts remain separate from external VM refs.
// Count slots have the original Win32 alignment required by Interlocked APIs.
class ControlView final {
    kinoko::native::RecordView<ControlRecord> view_;
public:
    explicit ControlView(int32_t control) : view_(pointer(control)) {}
    LONG add_strong(LONG delta) const noexcept {
        return InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(
            view_.bytes(&ControlRecord::strong)), delta);
    }
    LONG add_weak(LONG delta) const noexcept {
        return InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(
            view_.bytes(&ControlRecord::weak)), delta);
    }
    Address get(Address ControlRecord::* member) const noexcept {
        return view_.get(member);
    }
    void set(Address ControlRecord::* member, Address value) const noexcept {
        view_.set(member, value);
    }
};

void call_control_method(int32_t control, Address table,
                         Address ControlTable::* member) {
    if (!table) return;
    const kinoko::native::RecordView<ControlTable> methods(pointer(static_cast<int32_t>(table)));
    const auto entry = methods.get(member);
    if (!entry) return;
    // Exact zero-argument x86 member ABI, not a C register-context simulator.
    using Method = void (__thiscall*)(void*);
    reinterpret_cast<Method>(pointer(static_cast<int32_t>(entry)))(pointer(control));
}

''' + s[b:]
s=re.sub(r'(?:const )?ActorView (view|target)\((actor|receiver)\);',lambda m: 'const ActorView '+m[1]+'(pointer('+m[2]+'));',s)
s=s.replace('ObjectView(actor.at(ActorView::script_object))','ObjectView(actor.bytes(&ActorRecord::script_object))')
s=s.replace('view.set(ActorView::vtable, kinoko_actor_vtable());','view.set(&ActorRecord::vtable, static_cast<Address>(kinoko_actor_vtable()));')
s=s.replace('view.set(ActorView::type, 1);','view.set(&ActorRecord::type, int32_t{1});')
s=s.replace('constexpr std::array<size_t, 7> objects{44, 56, 68, 96, 108, 124, 136};\n    for (auto offset : objects)\n        ObjectView(view.at(offset)).initialize(kinoko_squirrel_object_vtable());', 'for (auto member : script_members)\n        ObjectView(view.bytes(member)).initialize(kinoko_squirrel_object_vtable());')
s=s.replace('view.set(ActorView::collision_slots, view.at(ActorView::inline_slots));','view.set(&ActorRecord::collision_slots, static_cast<Address>(address(view.bytes(&ActorRecord::inline_slots))));')
s=s.replace('view.set(ActorView::collision_records, view.at(ActorView::inline_records));','view.set(&ActorRecord::collision_records, static_cast<Address>(address(view.bytes(&ActorRecord::initial))));')
s=s.replace('view.get(0)','view.get(&ControlRecord::vtable)').replace('view.get(12)','view.get(&ControlRecord::allocation)').replace('view.set(12, 0)','view.set(&ControlRecord::allocation, 0)')
s=s.replace('call_control_method(control, table, 8)','call_control_method(control, table, &ControlTable::destroy)').replace('call_control_method(control, table, 4)','call_control_method(control, table, &ControlTable::dispose)')
s=re.sub(r'view.at\(ActorView::(\w+)\)', r'address(view.bytes(&ActorRecord::\1))',s)
s=s.replace('ActorView::','&ActorRecord::')
s=re.sub(r'view.set\((&ActorRecord::\w+), 0\)',r'view.set(\1, Address{0})',s)
s=s.replace('constexpr std::array<size_t, 7> objects{136, 124, 108, 96, 68, 56, 44};\n    for (auto offset : objects)\n        kinoko_squirrel_object_destroy(view.at(offset), address(current_vm()),', 'for (auto member = script_members.rbegin(); member != script_members.rend(); ++member)\n        kinoko_squirrel_object_destroy(address(view.bytes(*member)), address(current_vm()),')
p.write_text(s)
p=r/'src/reconstructed/actor_cleanup.cpp';s=p.read_text()
s=s.replace('#include "kinoko/actor_cleanup.h"','#include "kinoko/actor_cleanup.h"\n#include "kinoko/actor_records.hpp"')
s=s.replace('namespace {','namespace {\nusing namespace kinoko::actor;\nusing kinoko::native::RecordView;\n',1)
s=s.replace('frame += 248','frame += sizeof(FrameRecord)')
s=s.replace('std::free(pointer<void>(*pointer<int32_t>(static_cast<int32_t>(frame + 244))));\n                *pointer<int32_t>(static_cast<int32_t>(frame)) = g23;', '''const RecordView<FrameRecord> item(pointer<void>(static_cast<int32_t>(frame)));
                std::free(pointer<void>(static_cast<int32_t>(item.get(&FrameRecord::owned_payload))));
                item.set(&FrameRecord::vtable, static_cast<Address>(g23));''')
a=s.index('    auto *words = pointer<int32_t>(manager);')
s=s[:a]+'''    const ManagerView state(pointer<void>(manager));
    const auto textures = state.view(&ManagerPrefix::textures);
    const auto first = textures.get(&VectorIndex::begin);
    const auto last = textures.get(&VectorIndex::end);
    const auto count = static_cast<int32_t>(last - first) / static_cast<int32_t>(sizeof(int32_t));
    for (int32_t index = 0; index < count; ++index)
        function_405d60(pointer<int32_t>(static_cast<int32_t>(first))[index]);
    textures.set(&VectorIndex::end, first);
    // Actor destruction precedes releasing animations that actors only borrow.
    const auto actors = state.view(&ManagerPrefix::actors);
    function_463730(address(actors.data()));
    const auto animations = state.view(&ManagerPrefix::animation_lookup);
    auto animation_count = animations.get(&TreeIndex::count);
    clear_tree(pointer<AnimationTreeNode>(static_cast<int32_t>(animations.get(&TreeIndex::head))), animation_count);
    animations.set(&TreeIndex::count, animation_count);
    kinoko_clear_animation_list(address(state.bytes(&ManagerPrefix::animations)));
    auto actor_count = actors.get(&TreeIndex::count);
    clear_tree(pointer<PriorityTreeNode>(static_cast<int32_t>(actors.get(&TreeIndex::head))), actor_count);
    actors.set(&TreeIndex::count, actor_count);
    const auto iteration = state.view(&ManagerPrefix::iteration);
    const auto iteration_begin = iteration.get(&VectorIndex::begin);
    iteration.set(&VectorIndex::end, iteration_begin);
    state.set(&ManagerPrefix::iteration_state, int32_t{0});
    state.set(&ManagerPrefix::cleanup_pending, uint8_t{0});
    return static_cast<int32_t>(iteration_begin);
}
''';p.write_text(s)
p=r/'CMakeLists.txt';s=p.read_text();at='add_executable(kinoko_com_owner_contract'
s=s.replace(at,'''add_executable(kinoko_native_record_view_contract tests/native_record_view_contract.cpp)
target_include_directories(kinoko_native_record_view_contract PRIVATE include)
add_test(NAME native_record_view_contract COMMAND kinoko_native_record_view_contract)

add_executable(kinoko_actor_records_contract tests/actor_records_contract.cpp)
target_include_directories(kinoko_actor_records_contract PRIVATE include)
target_link_libraries(kinoko_actor_records_contract PRIVATE kinoko_native_methods)
add_test(NAME actor_records_contract COMMAND kinoko_actor_records_contract)

'''+at,1)
s=s.replace('set(KINOKO_TOOL_TARGETS\n','set(KINOKO_TOOL_TARGETS\n    kinoko_native_record_view_contract\n    kinoko_actor_records_contract\n',1);p.write_text(s)
Path(__file__).unlink()
