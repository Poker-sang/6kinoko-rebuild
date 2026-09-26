#pragma once

#include "kinoko/squirrel_binding.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_vm_bootstrap.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_variable_record.hpp"

extern "C" { extern struct SQVM *kinoko_primary_vm; }

namespace kinoko::script::binding {

// Read/write all recovered records through memcpy; callers can be unaligned.
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
inline HSQUIRRELVM current_vm() noexcept { return reinterpret_cast<HSQUIRRELVM>(kinoko_primary_vm); }
inline int32_t add_address(int32_t base, int32_t offset) noexcept {
    return static_cast<int32_t>(static_cast<uint32_t>(base) + static_cast<uint32_t>(offset));
}
inline bool valid_index(HSQUIRRELVM vm, SQInteger index) noexcept {
    if (!vm || index == 0) return false;
    const auto top = sq_gettop(vm);
    return index > 0 ? index <= top : index >= -top;
}
inline std::array<char, 258> variable_key(const char* name) noexcept {
    return upstream::sqplus_variable_key(name);
}

// A scoped EXTERNAL reference for locally owned SquirrelObject temporaries.
// It cannot be used for by-value parameters whose ownership passes to a
// native callee (see kinoko_sqplus_object_method).
class Object final {
public:
    explicit Object(HSQUIRRELVM vm) : vm_(vm) {
        view().initialize(kinoko_squirrel_object_vtable());
    }
    ~Object() { view().release(vm_); }
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;
    ObjectView view() noexcept { return ObjectView(&storage_); }
    void *data() noexcept { return &storage_; }
    int32_t location() noexcept { return address(data()); }
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
    upstream::sqplus_raw_set(vm, object.value(), key, value.value());
}
inline bool has_slot(HSQUIRRELVM vm, ObjectView object, const char* key) {
    return upstream::sqplus_exists(vm, object.value(), key);
}
inline void new_table(HSQUIRRELVM vm, ObjectView output) {
    StackTop stack(vm);
    const auto value = upstream::sqplus_new_table(vm);
    // Transfer the factory's one external ref. During the old-value release,
    // retain the same stack root and external count as AttachToStackObject.
    sq_pushobject(vm, value);
    output.release(vm);
    output.write(value);
}

} // namespace kinoko::script::binding
