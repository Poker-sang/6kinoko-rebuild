#pragma once

#include "kinoko/squirrel_binding.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_host_object.hpp"
#include <array>

extern "C" {
extern char* g644;
int32_t function_4a8cc0(void);
int32_t function_4a8db0(int32_t vm);
}

namespace kinoko::script::binding {

// Fixed-width views of the original records, not replacement C++ object
// layouts. Read/write through memcpy: generated callers can be unaligned.
struct Variable {
    int32_t offset;
    int32_t category;
    int32_t instance_type;
    int32_t value_type;
    uint16_t size;
    uint16_t flags;
};
static_assert(sizeof(Variable) == 20 && offsetof(Variable, flags) == 18);
enum VariableFlags : uint16_t { ReadOnly = 1, Constant = 2, Static = 4 };
struct Method { int32_t function; int32_t receiver_offset; };
static_assert(sizeof(Method) == 8);

template<class T> T load(const void* data) noexcept {
    T result;
    std::memcpy(&result, data, sizeof(result));
    return result;
}
template<class T> T load(int32_t data) noexcept { return load<T>(pointer(data)); }
template<class T> void store(void* data, const T& value) noexcept {
    std::memcpy(data, &value, sizeof(value));
}
template<class T> void store(int32_t data, const T& value) noexcept {
    store(pointer(data), value);
}
inline HSQUIRRELVM current_vm() noexcept { return reinterpret_cast<HSQUIRRELVM>(g644); }
inline int32_t add_address(int32_t base, int32_t offset) noexcept {
    return static_cast<int32_t>(static_cast<uint32_t>(base) + static_cast<uint32_t>(offset));
}
inline bool valid_index(HSQUIRRELVM vm, SQInteger index) noexcept {
    if (!vm || index == 0) return false;
    const auto top = sq_gettop(vm);
    return index > 0 ? index <= top : index >= -top;
}
inline std::array<char, 258> variable_key(const char* name) noexcept {
    std::array<char, 258> key{};
    key[0] = '_'; key[1] = 'v';
    for (size_t i = 0; name && i < 255 && name[i]; ++i) key[i + 2] = name[i];
    return key;
}

// A scoped EXTERNAL reference for locally owned SquirrelObject temporaries.
// It cannot be used for by-value parameters whose ownership passes to a
// native callee (see function_460b50).
class Object final {
public:
    explicit Object(HSQUIRRELVM vm) : vm_(vm) {
        view().initialize(kinoko_squirrel_object_vtable());
    }
    ~Object() { view().release(vm_); }
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;
    ObjectView view() noexcept { return ObjectView(&storage_); }
    int32_t location() noexcept { return address(&storage_); }
private:
    HSQUIRRELVM vm_;
    ObjectStorage storage_{};
};

inline bool get_slot(HSQUIRRELVM vm, ObjectView object, const char* key, ObjectView output) {
    StackTop stack(vm);
    object.push(vm); sq_pushstring(vm, key, -1);
    if (SQ_FAILED(sq_get(vm, -2))) return false;
    output.capture(vm, -1);
    return true;
}
inline void raw_store(HSQUIRRELVM vm, ObjectView object, const char* key, ObjectView value) {
    StackTop stack(vm);
    object.push(vm); sq_pushstring(vm, key, -1); value.push(vm);
    sq_rawset(vm, -3);
}
inline bool has_slot(HSQUIRRELVM vm, ObjectView object, const char* key) {
    StackTop stack(vm);
    object.push(vm); sq_pushstring(vm, key, -1);
    return SQ_SUCCEEDED(sq_get(vm, -2));
}
inline void new_table(HSQUIRRELVM vm, ObjectView output) {
    StackTop stack(vm);
    sq_newtable(vm); output.capture(vm, -1);
}

} // namespace kinoko::script::binding
