#include "kinoko/script_callbacks.h"
#include "kinoko/native_record_view.hpp"
#include "kinoko/squirrel_binding_detail.hpp"
#include "kinoko/squirrel_source_runtime.h"
extern "C" void retdec_trace_i32(const char *,int32_t);
namespace {
using namespace kinoko::script;
using namespace kinoko::script::binding;
using CallbackView=kinoko::native::RecordView<KinokoScriptCallback>;
}
// Shared invocation has no dependency on the game Actor class or bootstrap.
extern "C" int32_t kinoko_script_callback_invoke(KinokoScriptCallback *callback) {
    if (!callback) return -1;
    const CallbackView view(callback);
    auto *vm=view.get(&KinokoScriptCallback::vm);
    if (!vm) return -1;
    const auto base=sq_gettop(vm);
    ObjectView(view.bytes(&KinokoScriptCallback::closure)).push(vm);
    ObjectView(view.bytes(&KinokoScriptCallback::environment)).push(vm);
    const auto result=kinoko_sq_call(address(vm),1,1,1);
    if (result<0) {
        retdec_trace_i32("actor:step-call-failed",result);
        sq_settop(vm,base);
        return result;
    }
    return kinoko_sq_pop(address(vm),2);
}
extern "C" int32_t kinoko_script_callback_invoke_owned(KinokoScriptCallback *callback,KinokoOwnedObjectWords *argument,int32_t type,int32_t data) {
    if (!callback || !argument) return -1;
    const CallbackView view(callback);
    auto *vm=view.get(&KinokoScriptCallback::vm);
    if (!vm) return -1;
    ObjectView(view.bytes(&KinokoScriptCallback::closure)).push(vm);
    ObjectView(view.bytes(&KinokoScriptCallback::environment)).push(vm);
    sq_pushobject(vm,borrowed_value(type,data));
    const auto result=kinoko_sq_call(address(vm),2,SQTrue,SQTrue);
    if (SQ_SUCCEEDED(result)) sq_pop(vm,2);
    // Preserve failed-call stack contents and the current-VM release boundary.
    function_4a9d70_this(address(argument));
    return result;
}
