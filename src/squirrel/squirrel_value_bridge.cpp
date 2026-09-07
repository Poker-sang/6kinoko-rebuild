#include "kinoko/squirrel_value_bridge.h"

#include <cstddef>
#include <cstring>
#include "sqpcheader.h"
#include "sqvm.h"

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

extern "C" void kinoko_sq_set_error_string(int32_t vm, int32_t interned_string) {
    // The temporary owns one reference until assignment has acquired its own.
    // Original 48AC00/499A20 and sqapi.cpp::sq_throwerror use this sequence.
    SQObjectPtr error(&at<SQString>(interned_string));
    at<SQVM>(vm)._lasterror = error;
}

extern "C" void kinoko_sq_set_error_value(int32_t vm, const int32_t value[2]) {
    SQObject borrowed;
    std::memcpy(&borrowed, value, sizeof(borrowed));
    at<SQVM>(vm)._lasterror = borrowed;
}

extern "C" void kinoko_sq_reset_error(int32_t vm) {
    at<SQVM>(vm)._lasterror.Null();
}

extern "C" void kinoko_sq_assign_integer(int32_t object, int32_t value) {
    at<SQObjectPtr>(object) = static_cast<SQInteger>(value);
}

extern "C" void kinoko_sq_assign_float(int32_t object, float value) {
    at<SQObjectPtr>(object) = static_cast<SQFloat>(value);
}
