#pragma once
#include <squirrel.h>
// Narrow build integration with the historical SqPlus translation unit.
// Definitions delegate to its original getVar/setVar scalar switch bodies.
struct StackHandler;
class SquirrelObject;
namespace SqPlus {
struct VarRef;
int ReadVariableInfoForHost(StackHandler& stack, void*& output);
SQUserPointer ReadInstanceBaseForHost(SquirrelObject& instance, const VarRef& metadata);
int ReadInstanceStorageForHost(SquirrelObject& instance, const VarRef& metadata, void*& output);
int ReadScalarForHost(StackHandler& stack, VarRef& metadata, void* aligned_value);
using ScalarCommit = void (*)(void*) noexcept;
int WriteScalarForHost(StackHandler& stack, VarRef& metadata, void* aligned_value,
                       ScalarCommit commit, void* context);
}

int CreateNativeClassInstanceForHost(HSQUIRRELVM vm, const SQChar* name,
                                     SQUserPointer native, SQRELEASEHOOK hook,
                                     SQUserPointer native_type);
