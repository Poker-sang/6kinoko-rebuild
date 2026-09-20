#pragma once
#include <squirrel.h>
// Declarations already present in sqstate.h, needed earlier for standard
// two-phase lookup of squtils.h templates by current non-MSVC compilers.
// This is a compile-only preinclude; it replaces no allocator or VM code.
void* sq_vm_malloc(SQUnsignedInteger size);
void* sq_vm_realloc(void* pointer, SQUnsignedInteger oldsize, SQUnsignedInteger size);
void sq_vm_free(void* pointer, SQUnsignedInteger size);
