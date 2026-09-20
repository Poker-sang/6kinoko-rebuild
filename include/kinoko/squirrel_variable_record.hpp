#pragma once
#include <cstddef>
#include <cstdint>

namespace kinoko::script::binding {
// A serialized/recovered Win32 record, not a constructed SqPlus::VarRef.
// Descriptors are borrowed identity tokens, not native ClassTypeBase objects.
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
} // namespace kinoko::script::binding
