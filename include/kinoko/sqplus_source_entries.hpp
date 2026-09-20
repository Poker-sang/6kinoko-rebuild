#pragma once
// Narrow build integration with the historical SqPlus translation unit.
// Definitions delegate to its original getVar/setVar scalar switch bodies.
struct StackHandler;
namespace SqPlus {
struct VarRef;
int ReadScalarForHost(StackHandler& stack, VarRef& metadata, void* aligned_value);
using ScalarCommit = void (*)(void*) noexcept;
int WriteScalarForHost(StackHandler& stack, VarRef& metadata, void* aligned_value,
                       ScalarCommit commit, void* context);
}
