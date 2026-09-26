#include "kinoko/squirrel_value_bridge.h"

#include <cstddef>
#include <cstring>
#include "sqpcheader.h"
#include "sqvm.h"

// Original 4916A0 matches Squirrel 2.2.2 SQVM::Remove. Its assignments must
// release the OLD slot's type/data, including the final slot reset to null.
extern "C" void kinoko_sq_stack_remove(SQVM* vm, int32_t index) {
    vm->Remove(index);
}
#include "sqclosure.h"
#include "sqtable.h"
#include "sqclass.h"

namespace {
static_assert(sizeof(void *) == 4, "The reconstructed VM requires Win32.");
static_assert(sizeof(SQObject) == 8 && sizeof(SQObjectPtr) == 8,
              "The original VM stores 8-byte type/value pairs.");
static_assert(sizeof(SQVM) == 168 && offsetof(SQVM, _lasterror) == 64 &&
              offsetof(SQVM, _sharedstate) == 140,
              "Squirrel 2.2.2 VM layout must match the original.");
static_assert(offsetof(SQRefCounted, _uiRef) == 4,
              "The original virtual objects store their reference count at +4.");

template <typename T>
T &at(int32_t address) {
    return *reinterpret_cast<T *>(static_cast<uintptr_t>(
        static_cast<uint32_t>(address)));
}
}

extern "C" void kinoko_sq_set_error_string(SQVM* vm, SQString* interned_string) {
    // The temporary owns one reference until assignment has acquired its own.
    // Original 48AC00/499A20 and sqapi.cpp::sq_throwerror use this sequence.
    SQObjectPtr error(&(*interned_string));
    (*vm)._lasterror = error;
}

extern "C" void kinoko_sq_set_error_value(SQVM* vm, const int32_t value[2]) {
    SQObject borrowed;
    std::memcpy(&borrowed, value, sizeof(borrowed));
    (*vm)._lasterror = borrowed;
}

extern "C" void kinoko_sq_reset_error(SQVM* vm) {
    (*vm)._lasterror.Null();
}

extern "C" void kinoko_sq_assign_integer(SQObjectPtr* object, int32_t value) {
    (*object) = static_cast<SQInteger>(value);
}

extern "C" void kinoko_sq_assign_float(SQObjectPtr* object, float value) {
    (*object) = static_cast<SQFloat>(value);
}

// Original 489F50/489F30 are SQObjectPtr assignment and destruction.
// Use the supplied 2.2.2 operations, including acquire-before-release and
// dispatch through the value's own virtual Release slot.
extern "C" SQObjectPtr* kinoko_sq_pair_assign(SQObjectPtr* destination, SQObjectPtr* source) {
    (*destination) = (*source);
    return destination;
}

extern "C" void kinoko_sq_pair_destroy(SQObjectPtr* object) {
    (*object).~SQObjectPtr();
}

// These destroy members and unlink the GC node, but do not free the outer
// allocation. Its existing scalar-deleting entry owns that final operation.
extern "C" void kinoko_sq_closure_destroy(SQClosure* closure) {
    static_assert(sizeof(SQClosure) == 64 && offsetof(SQClosure, _env) == 24);
    (*closure).SQClosure::~SQClosure();
}

extern "C" void kinoko_sq_class_destroy(SQClass* klass) {
    static_assert(sizeof(SQClass) == 92 && offsetof(SQClass, _attributes) == 68);
    (*klass).SQClass::~SQClass();
}

// 48C580 / sq_setnativeclosurename: SQObjectPtr retains the new name and
// releases the previous name, including repeated registration on a child VM.
extern "C" int32_t kinoko_sq_set_native_name(SQVM* vm, int32_t index, const char *name) {
    return sq_setnativeclosurename(&(*vm), index, name);
}
