int32_t function_466090(void) {
    static volatile LONG trace_count;
    LONG trace_index = InterlockedIncrement(&trace_count);
    if (trace_index == 1) {
        retdec_trace_i32("render:g603", g603);
        retdec_trace_i32("render:g603-first",
                         g603 != 0 ? *(int32_t *)(uintptr_t)g603 : 0);
    }
    if (g603 == 0)
        return 0;
    int32_t v1 = *(int32_t *)g603; // 0x466096
    if (v1 == g603) {
        // 0x4660b5
        return g603;
    }
    int32_t object_list_node = *(int32_t *)(intptr_t)(v1 + 8);
    int32_t resource = object_list_node != 0
        ? *(int32_t *)(intptr_t)(object_list_node + 8) : 0;
    int32_t v2 = v1; // 0x46609a
    int32_t result = resource != 0 ? function_4522f0(resource) : 0;
    v2 = *(int32_t *)v2;
    while (v2 != g603) {
        // 0x4660a0
        object_list_node = *(int32_t *)(intptr_t)(v2 + 8);
        resource = object_list_node != 0
            ? *(int32_t *)(intptr_t)(object_list_node + 8) : 0;
        result = resource != 0 ? function_4522f0(resource) : 0;
        v2 = *(int32_t *)v2;
    }
    // 0x4660b5
    return result;
}