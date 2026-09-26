#pragma once
#include <cstddef>
#include <cstdint>

namespace kinoko::script::binding {
// Recovered Win32 VarRef layout. Descriptor identities are borrowed pointers;
// offset is deliberately numeric: it also stores constants and byte offsets.
struct Variable {
    int32_t offset;
    int32_t category;
    void* instance_type;
    void* value_type;
    uint16_t size;
    uint16_t flags;
};
static_assert(sizeof(Variable) == 20 && offsetof(Variable, flags) == 18);
enum VariableFlags : uint16_t { ReadOnly = 1, Constant = 2, Static = 4 };
} // namespace kinoko::script::binding
