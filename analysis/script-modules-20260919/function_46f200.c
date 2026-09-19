int32_t function_46f200(int32_t * a1, int32_t a2, int32_t a3, int32_t a4) {
    int32_t v1 = __readfsdword(0); // bp-16, 0x46f210
    int32_t v2; // bp-4, 0x46f200
    int32_t v3 = g507 ^ (int32_t)&v2; // bp-44, 0x46f21e
    __writefsdword(0, (int32_t)&v1);
    int32_t v4 = a2; // bp-48, 0x46f22d
    int32_t v5 = sq_gettop(kinoko_vm(a2)); // 0x46f234
    retdec_msvc_0_Init_locks_std__QAE_XZ5();
    if ((g635 & 1) == 0) {
        // 0x46f25c
        g635 |= 1;
        g631 = 0;
        g633 = 0;
        g634 = -1;
        g630 = (int32_t)&g38;
        g632 = 0;
    }
    int32_t result = (int32_t)a1;
    v4 = a4;
    int32_t v6 = a3; // bp-52, 0x46f28f
    int32_t v7 = function_4aa540(a2, result, &g630, a3, a4); // 0x46f297
    int32_t * v8 = &v3; // 0x46f2a1
    if (v7 != 0) {
        // 0x46f2a3
        int32_t root_object[3];
        function_4a94e0_this((int32_t)(intptr_t)root_object);
        int32_t v9 = *(int32_t *)(result + 4); // 0x46f2bd
        sq_pushobject(kinoko_vm((int32_t)g644), kinoko_borrowed_object(v9, *(int32_t *)(result + 8)));
        function_4a9660_this((int32_t)(intptr_t)root_object, -1);
        kinoko_sq_pop((int32_t)g644, 1);
        function_45f640(root_object);
        v8 = &v4;
    }
    int32_t v10 = (int32_t)v8;
    *(int32_t *)(v10 - 4) = v5;
    *(int32_t *)(v10 - 8) = a2;
    sq_settop(kinoko_vm(a2), (SQInteger)((uint32_t)v5));
    __writefsdword(0, v1);
    return result;
}