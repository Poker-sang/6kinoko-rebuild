int32_t function_46fac0(void) {
    int32_t v1 = __readfsdword(0); // bp-16, 0x46fad0
    __writefsdword(0, (int32_t)&v1);
    int32_t map_state[12] = { 0 };
    int32_t *map_object = map_state + 2;
    int32_t layer_object[3] = { 0, 0, 0 };

    function_46f9f0_this((int32_t)(intptr_t)map_state, "Map", 0);
    function_4a95c0_this((int32_t)(intptr_t)g636,
                         (int32_t)(intptr_t)map_object);
    char v3 = g635; // 0x46fb0c
    if ((v3 & 1) == 0) {
        // 0x46fb1a
        g635 = v3 | 1;
        g631 = 0;
        g633 = 0;
        g634 = -1;
        g630 = (int32_t)&g38;
        g632 = 0;
    }
    // 0x46fb3f
    function_460920(map_object, &g630, 76, "width", 0);
    char v4 = g635; // 0x46fb58
    if ((v4 & 1) == 0) {
        // 0x46fb61
        g635 = v4 | 1;
        g631 = 0;
        g633 = 0;
        g634 = -1;
        g630 = (int32_t)&g38;
        g632 = 0;
    }
    // 0x46fb86
    function_460920(map_object, &g630, 80, "height", 0);
    char v5 = g635; // 0x46fb9f
    if ((v5 & 1) == 0) {
        // 0x46fba8
        g635 = v5 | 1;
        g631 = 0;
        g633 = 0;
        g634 = -1;
        g630 = (int32_t)&g38;
        g632 = 0;
    }
    // 0x46fbcd
    function_460920(map_object, &g630, 56, "last_id", 0);
    char v6 = g635; // 0x46fbe6
    if ((v6 & 1) == 0) {
        // 0x46fbef
        g635 = v6 | 1;
        g631 = 0;
        g633 = 0;
        g634 = -1;
        g630 = (int32_t)&g38;
        g632 = 0;
    }
    // 0x46fc14
    function_4609c0(map_object, &g630, 60, "last_left", 0);
    char v7 = g635; // 0x46fc2d
    if ((v7 & 1) == 0) {
        // 0x46fc36
        g635 = v7 | 1;
        g631 = 0;
        g633 = 0;
        g634 = -1;
        g630 = (int32_t)&g38;
        g632 = 0;
    }
    // 0x46fc5b
    function_4609c0(map_object, &g630, 64, "last_top", 0);
    char v8 = g635; // 0x46fc74
    if ((v8 & 1) == 0) {
        // 0x46fc7d
        g635 = v8 | 1;
        g631 = 0;
        g633 = 0;
        g634 = -1;
        g630 = (int32_t)&g38;
        g632 = 0;
    }
    // 0x46fca2
    function_4609c0(map_object, &g630, 68, "last_right", 0);
    if ((g635 & 1) == 0) {
        // 0x46fcc4
        g635 |= 1;
        g631 = 0;
        g633 = 0;
        g634 = -1;
        g630 = (int32_t)&g38;
        g632 = 0;
    }
    // 0x46fce9
    function_4609c0(map_object, &g630, 72, "last_bottom", 0);
    retdec_msvc_0_Init_locks_std__QAE_XZ5();
    function_4a94e0_this((int32_t)(intptr_t)layer_object);
    function_4a9840_this((int32_t)(intptr_t)g636, "layer_name",
                         (int32_t)(intptr_t)layer_object);
    function_4a9d70_this((int32_t)(intptr_t)layer_object);
    function_4a9d70_this((int32_t)(intptr_t)(map_state + 9));
    function_4a9d70_this((int32_t)(intptr_t)(map_state + 6));
    int32_t result = function_4a9d70_this(
        (int32_t)(intptr_t)map_object); // 0x46fd4d
    __writefsdword(0, v1);
    return result;
}