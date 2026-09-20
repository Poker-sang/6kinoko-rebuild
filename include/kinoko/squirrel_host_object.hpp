#pragma once

#include <squirrel.h>
#include "kinoko/upstream_bindings.hpp"
#include "kinoko/native_record_view.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace kinoko::script {

// The game's SqPlus wrapper owns an EXTERNAL reference. SQObjectPtr instead
// owns an internal reference; it must never be overlaid on this 12-byte record.
struct ObjectStorage {
    std::uint32_t vtable;
    HSQOBJECT value;
};
static_assert(sizeof(void*) == 4 && sizeof(HSQOBJECT) == 8);
static_assert(sizeof(ObjectStorage) == 12 && offsetof(ObjectStorage, value) == 4);

inline int32_t address(const void* pointer) noexcept {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(pointer));
}
template<class T = void> T* pointer(int32_t value) noexcept {
    return reinterpret_cast<T*>(static_cast<uintptr_t>(static_cast<uint32_t>(value)));
}
inline int32_t data_bits(const HSQOBJECT& value) noexcept {
    int32_t bits;
    std::memcpy(&bits, &value._unVal, sizeof(bits));
    return bits;
}
inline HSQOBJECT borrowed_value(int32_t type, int32_t data) noexcept {
    HSQOBJECT result;
    result._type = static_cast<SQObjectType>(type);
    std::memcpy(&result._unVal, &data, sizeof(data));
    return result;
}

// Views also accept the legacy int32_t[3] temporaries, without type-punning
// them as live C++ objects or assuming stronger alignment than their storage.
class ObjectView final {
public:
    explicit ObjectView(void* storage) noexcept : record_(storage) {}
    explicit ObjectView(int32_t storage) noexcept : ObjectView(pointer(storage)) {}
    HSQOBJECT value() const noexcept {
        return record_.get(&ObjectStorage::value);
    }
    void write(const HSQOBJECT& value) const noexcept {
        record_.set(&ObjectStorage::value, value);
    }
    void set_vtable(int32_t vtable) const noexcept {
        record_.set(&ObjectStorage::vtable, static_cast<std::uint32_t>(vtable));
    }
    void initialize(int32_t vtable) const noexcept {
        set_vtable(vtable);
        reset();
    }
    void reset() const noexcept {
        HSQOBJECT empty;
        sq_resetobject(&empty);
        write(empty);
    }
    void push(HSQUIRRELVM vm) const { sq_pushobject(vm, value()); }
    void retain(HSQUIRRELVM vm) const {
        upstream::sqplus_retain(vm, value());
    }
    void release(HSQUIRRELVM vm) const {
        upstream::sqplus_release(vm, value());
    }
    void assign(HSQUIRRELVM vm, HSQOBJECT incoming) const {
        // Acquire first: source and destination may be identical, or the old
        // destination may own the source. Keep the borrowed value in a local.
        write(upstream::sqplus_assign(vm, value(), incoming));
    }
    SQObjectType capture(HSQUIRRELVM vm, SQInteger index) const {
        HSQOBJECT incoming;
        sq_resetobject(&incoming);
        // Match the legacy scratch-pair initialization; the caller supplies
        // a valid index, as required by the Squirrel 2.2.2 API.
        sq_getstackobj(vm, index, &incoming);
        assign(vm, incoming);
        return incoming._type;
    }
    int32_t payload_address() const noexcept {
        return address(record_.bytes(&ObjectStorage::value));
    }
private:
    kinoko::native::RecordView<ObjectStorage> record_;
};

// Only use for balanced helper operations. BeginIteration intentionally leaves
// two values on the VM stack and therefore must NOT use this guard.
class StackTop final {
public:
    explicit StackTop(HSQUIRRELVM vm) : vm_(vm), top_(sq_gettop(vm)) {}
    ~StackTop() { sq_settop(vm_, top_); }
    StackTop(const StackTop&) = delete;
    StackTop& operator=(const StackTop&) = delete;
    SQInteger top() const noexcept { return top_; }
private:
    HSQUIRRELVM vm_;
    SQInteger top_;
};

} // namespace kinoko::script
