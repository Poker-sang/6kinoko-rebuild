#pragma once
#include "kinoko/act_types.h"
#include "kinoko/legacy_memory.hpp"
#include <cstddef>

namespace kinoko::act {
// ISerializable::Dispose (+12) and CAct's deleting destructor (+16) are
// different contracts. Layout slot +16 is RTTI, NOT a deleting destructor.
template<class T> void dispose_owned(T *value) {
    if (!value) return;
    using Dispose = void (__thiscall *)(T *);
    const auto *methods = legacy::load<const unsigned char *>(value);
    legacy::load<Dispose>(methods + 3 * sizeof(void *))(value);
}
inline void delete_document(KinokoActDocument *value) {
    if (!value) return;
    using Delete = void (__thiscall *)(KinokoActDocument *, int);
    const auto *methods = legacy::load<const unsigned char *>(value);
    legacy::load<Delete>(methods + 4 * sizeof(void *))(value, 1);
}
struct DocumentDeleter { void operator()(KinokoActDocument *p) const { delete_document(p); } };
template<class T> struct OwnedDeleter { void operator()(T *p) const { dispose_owned(p); } };
}
