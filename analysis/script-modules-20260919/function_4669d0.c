int32_t function_4669d0(void) {
    int32_t v1 = __readfsdword(0); // bp-16, 0x4669e0
    __writefsdword(0, (int32_t)&v1);
    int32_t root_object[3] = { 0, 0, 0 };
    int32_t camera_class[12] = { 0 };
    int32_t vm;

    function_4a9500_this(root_object, function_4a8cc0());
    function_466900_this((int32_t)(intptr_t)camera_class, "Camera", 0);
    function_4a95c0_this((int32_t)(intptr_t)g611,
                         (int32_t)(intptr_t)(camera_class + 2));
    vm = camera_class[0];
    sq_pushobject(kinoko_vm(vm), kinoko_borrowed_object(camera_class[3], camera_class[4]));
    sq_pushstring(kinoko_vm(vm), (const SQChar*)kinoko_pointer((int32_t)"SetUpdateFunction"), -1);
    *(int32_t *)(int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(vm), 4)) =
        (int32_t)(intptr_t)kinoko_camera_set_update_callback;
    sq_newclosure(kinoko_vm(vm), (SQFUNCTION)kinoko_pointer((int32_t)(intptr_t)function_466890), 1);
    sq_newslot(kinoko_vm(vm), -3, ((0) != 0));
    kinoko_sq_pop(vm, 1);
    char v4 = g610; // 0x466a78
    if ((v4 & 1) == 0) {
        // 0x466a86
        g610 = v4 | 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466aab
    function_4609c0(camera_class + 2, &g605, 40, "x", 0);
    char v5 = g610; // 0x466ac4
    if ((v5 & 1) == 0) {
        // 0x466acd
        g610 = v5 | 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466af2
    function_4609c0(camera_class + 2, &g605, 44, "y", 0);
    char v6 = g610; // 0x466b0b
    if ((v6 & 1) == 0) {
        // 0x466b14
        g610 = v6 | 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466b39
    function_4609c0(camera_class + 2, &g605, 48, "cx", 0);
    char v7 = g610; // 0x466b52
    if ((v7 & 1) == 0) {
        // 0x466b5b
        g610 = v7 | 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466b80
    function_4609c0(camera_class + 2, &g605, 52, "cy", 0);
    char v8 = g610; // 0x466b99
    if ((v8 & 1) == 0) {
        // 0x466ba2
        g610 = v8 | 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466bc7
    function_4609c0(camera_class + 2, &g605, 56, "offset_x", 0);
    char v9 = g610; // 0x466be0
    if ((v9 & 1) == 0) {
        // 0x466be9
        g610 = v9 | 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466c0e
    function_4609c0(camera_class + 2, &g605, 60, "offset_y", 0);
    char v10 = g610; // 0x466c27
    if ((v10 & 1) == 0) {
        // 0x466c30
        g610 = v10 | 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466c55
    function_4609c0(camera_class + 2, &g605, 72, "left", 0);
    char v11 = g610; // 0x466c6e
    if ((v11 & 1) == 0) {
        // 0x466c77
        g610 = v11 | 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466c9c
    function_4609c0(camera_class + 2, &g605, 76, "top", 0);
    char v12 = g610; // 0x466cb5
    if ((v12 & 1) == 0) {
        // 0x466cbe
        g610 = v12 | 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466ce3
    function_4609c0(camera_class + 2, &g605, 80, "right", 0);
    char v13 = g610; // 0x466cfc
    if ((v13 & 1) == 0) {
        // 0x466d05
        g610 = v13 | 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466d2a
    function_4609c0(camera_class + 2, &g605, 84, "bottom", 0);
    char v14 = g610; // 0x466d43
    if ((v14 & 1) == 0) {
        // 0x466d4c
        g610 = v14 | 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466d71
    function_4609c0(camera_class + 2, &g605, 64, "width", 0);
    if ((g610 & 1) == 0) {
        // 0x466d93
        g610 |= 1;
        g606 = 0;
        g608 = 0;
        g609 = -1;
        g605 = (int32_t)&g26;
        g607 = 0;
    }
    // 0x466db8
    function_4609c0(camera_class + 2, &g605, 68, "height", 0);
    if (camera_class[4] != 0) {
        retdec_trace_i32("camera-class:type", camera_class[3]);
        retdec_trace_i32("camera-class:data", camera_class[4]);
        retdec_trace_squirrel_table_entries(
            "camera-class-members",
            *(int32_t *)(intptr_t)(camera_class[4] + 24));
    }
    function_4a9d70_this((int32_t)(intptr_t)(camera_class + 9));
    function_4a9d70_this((int32_t)(intptr_t)(camera_class + 6));
    function_4a9d70_this((int32_t)(intptr_t)(camera_class + 2));
    function_4a9d70_this((int32_t)(intptr_t)root_object);
    __writefsdword(0, v1);
    return 0;
}