#include "kinoko/upstream_bindings.hpp"
#include "kinoko/sqplus_source_entries.hpp"
#include <sqplus.h>
#include <cstring>
#include <limits>
#include <type_traits>

namespace kinoko::script::upstream {
namespace {
using binding::Variable;
static_assert(sizeof(INT) == 4 && sizeof(unsigned) == 4 && sizeof(FLOAT) == 4);
static_assert(sizeof(bool) == 1 && std::numeric_limits<char>::is_signed);
static_assert(SqPlus::VAR_TYPE_INT == 0 && SqPlus::VAR_TYPE_UINT == 1 &&
              SqPlus::VAR_TYPE_FLOAT == 2 && SqPlus::VAR_TYPE_BOOL == 3);

// Select storage, not a second implementation of the source conversion switch.
// Constant signed values occupy the complete integer word, regardless of size.
template<class Operation>
SQInteger with_scalar(const Variable& info, Operation&& operation) {
    switch (info.category) {
    case SqPlus::VAR_TYPE_INT:
        if (!(info.flags & binding::Constant)) {
            if (info.size == 1) return operation(char{});
            if (info.size == 2) return operation(short{});
        }
        return operation(INT{});
    case SqPlus::VAR_TYPE_UINT: return operation(unsigned{});
    case SqPlus::VAR_TYPE_FLOAT: return operation(FLOAT{});
    case SqPlus::VAR_TYPE_BOOL: return operation(bool{});
    default: return SQ_ERROR;
    }
}
SqPlus::VarRef scalar_metadata(const Variable& info, size_t size) {
    // Do not call the registry-populating VarRef constructor or overlay one
    // on C byte storage. These are aligned, automatic source-library values.
    SqPlus::VarRef result;
    result.varType = nullptr;
    result.m_type = static_cast<SqPlus::ScriptVarType>(info.category);
    result.m_size = static_cast<short>(size);
    result.m_access = SqPlus::VAR_ACCESS_READ_WRITE;
    return result;
}
template<class T>
struct Writeback {
    T* value;
    void* destination;
    static void commit(void* context) noexcept {
        auto& self = *static_cast<Writeback*>(context);
        std::memcpy(self.destination, self.value, sizeof(T));
    }
};
}

SQInteger sqplus_read_scalar(HSQUIRRELVM vm, const Variable& info,
                            const void* storage, int32_t immediate_value) {
    const bool immediate = (info.flags & binding::Constant) != 0;
    if (!vm || (!immediate && !storage)) return SQ_ERROR;
    StackHandler stack(vm);
    return with_scalar(info, [&](auto value) -> SQInteger {
        using T = decltype(value);
        if (immediate) {
            // Host constant float is integer-to-float; bool tests the complete
            // word. Resolve this representation before the source dereference.
            value = static_cast<T>(immediate_value);
        } else if constexpr (std::is_same_v<T, bool>) {
            unsigned char byte;
            std::memcpy(&byte, storage, sizeof(byte));
            value = byte != 0; // never overlay a C++ bool on arbitrary byte data
        } else std::memcpy(&value, storage, sizeof(value));
        auto metadata = scalar_metadata(info, sizeof(value));
        return SqPlus::ReadScalarForHost(stack, metadata, &value);
    });
}

SQInteger sqplus_write_scalar(HSQUIRRELVM vm, const Variable& info, void* storage) {
    if (!vm || !storage || sq_gettop(vm) < 3 ||
        (info.flags & (binding::ReadOnly | binding::Constant))) return SQ_ERROR;
    // The established host rejects a nonnumeric float value and preserves the
    // destination. Snapshot StackHandler::GetFloat instead supplies zero.
    // Keep the host policy and its actual sq_getfloat last-error side effect.
    if (info.category == SqPlus::VAR_TYPE_FLOAT &&
        sq_gettype(vm, 3) != OT_INTEGER && sq_gettype(vm, 3) != OT_FLOAT) {
        SQFloat unused;
        return sq_getfloat(vm, 3, &unused);
    }
    StackHandler stack(vm);
    return with_scalar(info, [&](auto value) -> SQInteger {
        using T = decltype(value);
        auto metadata = scalar_metadata(info, sizeof(value));
        Writeback<T> output{&value, storage};
        // Source setVar performs conversion/narrowing. Commit the byte record
        // immediately before source Return pushes its result, not afterwards.
        return SqPlus::WriteScalarForHost(stack, metadata, &value,
            &Writeback<T>::commit, &output);
    });
}
} // namespace kinoko::script::upstream
