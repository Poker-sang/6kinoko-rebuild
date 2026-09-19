int32_t function_466050(void) {
    int32_t v1 = *(int32_t *)g603; // 0x466056
    static volatile LONG trace_count;
    LONG trace_index = InterlockedIncrement(&trace_count);

    if (trace_index <= 8) {
        retdec_trace("466050:entry");
        retdec_trace_i32("466050:g603", g603);
        retdec_trace_i32("466050:first", v1);
        retdec_trace_i32("466050:update-mask", g459);
    }
    if (v1 == g603) {
        // 0x466080
        if (trace_index <= 8)
            retdec_trace("466050:empty");
        return g603;
    }
    int32_t v2 = v1; // 0x46605a
    int32_t result = g603;
    while (v2 != g603) {
        int32_t object_list_node = *(int32_t *)(intptr_t)(v2 + 8);
        int32_t resource = object_list_node != 0
            ? *(int32_t *)(intptr_t)(object_list_node + 8) : 0;
        if (resource != 0) {
            if (trace_index <= 8) {
                int32_t act = *(int32_t *)(intptr_t)(resource + 12);
                retdec_trace_i32("466050:resource", resource);
                retdec_trace_i32("466050:active",
                                 *(int32_t *)(intptr_t)(resource + 8));
                retdec_trace_i32("466050:suspend",
                                 *(int32_t *)(intptr_t)(resource + 104));
                retdec_trace_i32("466050:time",
                                 *(int32_t *)(intptr_t)(resource + 100));
                retdec_trace_i32("466050:act", act);
                if (act != 0)
                    retdec_trace_squirrel_name(
                        "466050:act-name",
                        (int32_t)(intptr_t)retdec_std_string_data(act + 16));
            }
            function_451620(resource);
            result = function_451640(resource); // 0x466071
            if (trace_index <= 8)
                retdec_trace_i32("466050:update-result", result);
        }
        // 0x466060
        v2 = *(int32_t *)(intptr_t)v2;
    }
    // 0x466080
    return result;
}