int32_t function_4660c0(void) {
    static volatile LONG trace_count;
    LONG trace_index = InterlockedIncrement(&trace_count);
    if (trace_index == 1) {
        retdec_trace_i32("render:g603-float", g603);
        retdec_trace_i32("render:g603-float-first",
                         g603 != 0 ? *(int32_t *)(uintptr_t)g603 : 0);
    }
    if (g603 == 0)
        return 0;
    int32_t node = *(int32_t *)g603; // 0x4660c6
    int32_t result = g603;
    int32_t node_index = 0;
    while (node != g603) {
        int32_t object_list_node = *(int32_t *)(intptr_t)(node + 8);
        int32_t resource = object_list_node != 0
            ? *(int32_t *)(intptr_t)(object_list_node + 8) : 0;
        if (resource != 0) {
            if (trace_index <= 3) {
                int32_t act = *(int32_t *)(intptr_t)(resource + 12);
                retdec_trace_i32("4660c0:index", node_index);
                retdec_trace_i32("4660c0:resource", resource);
                retdec_trace_i32("4660c0:active",
                                 *(int32_t *)(intptr_t)(resource + 8));
                retdec_trace_i32("4660c0:suspend",
                                 *(int32_t *)(intptr_t)(resource + 104));
                retdec_trace_i32("4660c0:act", act);
                if (act != 0)
                    retdec_trace_squirrel_name(
                        "4660c0:act-name",
                        (int32_t)(intptr_t)retdec_std_string_data(act + 16));
            }
            result = function_4525d0(resource, 0.0f, 0.0f);
        }
        ++node_index;
        node = *(int32_t *)(intptr_t)node;
    }
    // 0x4660f1
    return result;
}