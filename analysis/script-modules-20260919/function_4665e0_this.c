static int32_t function_4665e0_this(int32_t result, int32_t a1) {
    static int32_t camera_instance_trace_count;
    int32_t v1 = a1;
    int32_t v2 = __readfsdword(0); // bp-16, 0x4665f0
    __writefsdword(0, (int32_t)&v2);
    int32_t v3 = sq_gettop(kinoko_vm(a1)); // 0x46660f
    int32_t v4 = sq_getinstanceup(kinoko_vm(a1), 1, (SQUserPointer*)(&v1), kinoko_pointer(0)) > -1 ? v1 : 0; // 0x466635
    int32_t * v5 = (int32_t *)result; // 0x466638
    *v5 = v4;
    int32_t v6; // bp-24, 0x4665e0
    int32_t v7; // bp-28, 0x4665e0
    int32_t pair_status = 0;
    if (v3 < 1) {
        // 0x466660
        *(int32_t *)(result + 4) = 0;
    } else {
        pair_status = sq_getuserdata(kinoko_vm(a1), v3, (SQUserPointer*)(&v6), (SQUserPointer*)kinoko_pointer((int32_t)&v1)); // 0x466648
        int32_t v9 = 0; // 0x466652
        if (pair_status >= 0) {
            // 0x466654
            v9 = v1 != 0 ? 0 : v6;
        }
        // 0x46666b
        *(int32_t *)(result + 4) = v9;
        sq_resetobject((HSQOBJECT*)kinoko_pointer((int32_t)&v7));
        sq_getstackobj(kinoko_vm(a1), 1, (HSQOBJECT*)(&v7));
    }
    if (camera_instance_trace_count < 16) {
        retdec_trace_i32("4665e0:vm", a1);
        retdec_trace_i32("4665e0:result", result);
        retdec_trace_i32("4665e0:stack-count", v3);
        retdec_trace_i32("4665e0:instance-up", v4);
        retdec_trace_i32("4665e0:userdata-status", pair_status);
        retdec_trace_i32("4665e0:userdata-payload", v3 >= 1 ? v6 : 0);
        retdec_trace_i32("4665e0:userdata-tag", v1);
        retdec_trace_i32("4665e0:instance-type", v3 >= 1 ? v7 : 0);
        retdec_trace_i32("4665e0:instance-data",
                         v3 >= 1 ? *(int32_t *)((intptr_t)&v7 + 4) : 0);
        ++camera_instance_trace_count;
    }
    // 0x466683
    int32_t v9[3] = { 0, 0, 0 };
    /* The temporary object is the script receiver.  The userdata payload
       remains the native type-info returned through result[1], but it is not
       the SQInstance pointer used by SquirrelObject::GetType. */
    function_4a9540_this(
        (int32_t)(intptr_t)v9, v7,
        *(int32_t *)((intptr_t)&v7 + 4));
    int32_t v10; // bp-32, 0x4665e0
    function_4a9d30_this((int32_t)(intptr_t)v9, &v10);
    char v11 = g610; // 0x4666a5
    if ((v11 & 1) == 0) {
        // 0x4666b3
        g610 = v11 | 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x4666d9
    if (v10 == (int32_t)&g605) {
        // 0x466743
        function_4a9d70_this((int32_t)(intptr_t)v9);
        __writefsdword(0, v2);
        return result;
    }
    // 0x4666e2
    int32_t v12[3] = { 0, 0, 0 }; // bp-56, 0x4665e0
    function_4aa3a0_this((int32_t)(intptr_t)v9,
                         (int32_t)(intptr_t)v12, "__ot");
    if ((g610 & 1) == 0) {
        // 0x466700
        g610 |= 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466726
    *v5 = function_4aa000_this((int32_t)(intptr_t)v12,
                               (int32_t)(intptr_t)&g605);
    function_4a9d70_this((int32_t)(intptr_t)v12);
    // 0x466743
    function_4a9d70_this((int32_t)(intptr_t)v9);
    __writefsdword(0, v2);
    return result;
}