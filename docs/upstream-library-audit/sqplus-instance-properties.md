# Declaring-base property receiver regression

The unmodified original function `4AA750–4AA96D` tests Static/Constant first,
compares the instance typetag with the property declaring type, and on a
mismatch reads `__ot[declaring_type]` before adding the field offset. A missing
mapping takes the `Invalid Instance Type` error path, not the primary pointer.
The 20080713 snapshot `getInstanceVarInfo` contains the same branch.

The reconstruction at `bdca65f8` always uses the primary `sq_getinstanceup`
pointer. The regression installs a different declaring-base address and
checks exact pointers, getter/setter results, guard words, null primary
pointer, missing mappings and the Static/Constant bypass. It is committed
before the fix to demonstrate the old behavior fails without real game data.

The correction calls the original SqPlus receiver-selection body, factored
within its source translation unit. The host must still keep checked metadata,
32-bit modulo field-address calculation and its null-primary-pointer guard.
The snapshot native exception is converted at the host boundary to the same
Squirrel error text; this does not restore every old MSVC exception unwind.
