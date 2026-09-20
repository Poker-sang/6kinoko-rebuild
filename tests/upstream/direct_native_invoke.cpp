#include <squirrel.h>
// Only the recovered game receiver hook is absent in this isolated source VM.
// This invokes the real native closure; compiler, GC and VM are not mocked.
SQInteger kinoko_squirrel_invoke_native(HSQUIRRELVM vm, SQFUNCTION function) {
    return function(vm);
}
