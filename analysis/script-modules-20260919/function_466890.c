int32_t function_466890(int32_t a1) {
    static int32_t camera_native_trace_count;
    int32_t result[3] = { 0, 0, 0 };
    int32_t arguments[3] = { 0, 0, 0 };
    int32_t native_instance;
    int32_t type_info;
    int32_t method;

    // 0x466890: sub_4665E0(&result, a1)
    function_4665e0_this((int32_t)(intptr_t)result, a1);
    native_instance = result[0];
    type_info = result[1];
    if (native_instance == 0 || type_info == 0) {
        // 0x4668de
        return sq_throwerror(kinoko_vm(a1), "Invalid Instance Type");
    }
    method = *(int32_t *)(intptr_t)type_info;
    if (method == 0)
        return sq_throwerror(kinoko_vm(a1), "Invalid Instance Type");

    // 0x4668b3: the native callback is __thiscall with three SQ arguments.
    function_45f5e0(arguments, 0, a1);
    if (camera_native_trace_count < 16) {
        retdec_trace_i32("466890:vm", a1);
        retdec_trace_i32("466890:native-instance", native_instance);
        retdec_trace_i32("466890:type-info", type_info);
        retdec_trace_i32("466890:method", method);
        retdec_trace_i32("466890:arg0", arguments[0]);
        retdec_trace_i32("466890:arg1", arguments[1]);
        retdec_trace_i32("466890:arg2", arguments[2]);
        ++camera_native_trace_count;
    }
    retdec_call_thiscall3_result(
        (void *)(intptr_t)native_instance,
        (void *)(intptr_t)method,
        arguments[0], arguments[1], arguments[2]);
    return 0;
}