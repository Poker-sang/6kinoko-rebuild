#include "kinoko/squirrel_binding.h"
#include <sqplus.h>
#include <cstdint>

namespace {
template<int Id> struct NativeTag {};
using CopyEntry = int32_t (*)(int32_t, int32_t);
// The descriptor is a normally constructed upstream ClassType. Only its game
// copy operation is supplied by the embedding; naming/base/cache methods and
// the actual C++ vtable come from the source library.
template<int Id> class NativeDescriptor final : public SqPlus::ClassType<NativeTag<Id>> {
public:
    explicit NativeDescriptor(CopyEntry copy) { entry = copy; }
    SqPlus::CopyVarFunc vgetCopyFunc() override { return &copy; }
private:
    static inline CopyEntry entry = nullptr;
    static void copy(void* destination, void* source) {
        entry(static_cast<int32_t>(reinterpret_cast<intptr_t>(destination)),
              static_cast<int32_t>(reinterpret_cast<intptr_t>(source)));
    }
};
template<int Id> int32_t* native_type(CopyEntry copy) {
    static NativeDescriptor<Id> type(copy);
    return reinterpret_cast<int32_t*>(&type);
}
}
extern "C" int32_t* kinoko_sqplus_scalar_type(int32_t category) {
    switch (category) {
    case -1: return reinterpret_cast<int32_t*>(SqPlus::ClassType<void>::Get());
    case 0: return reinterpret_cast<int32_t*>(SqPlus::ClassType<INT>::Get());
    case 2: return reinterpret_cast<int32_t*>(SqPlus::ClassType<FLOAT>::Get());
    case 3: return reinterpret_cast<int32_t*>(SqPlus::ClassType<bool>::Get());
    default: return nullptr;
    }
}
extern "C" int32_t* kinoko_sqplus_game_type(int32_t kind, CopyEntry copy) {
    switch (kind) {
    case 0: return native_type<0>(copy); // Actor
    case 1: return native_type<1>(copy); // Camera
    case 2: return native_type<2>(copy); // Input
    case 3: return native_type<3>(copy); // MapManager
    default: return nullptr;
    }
}
