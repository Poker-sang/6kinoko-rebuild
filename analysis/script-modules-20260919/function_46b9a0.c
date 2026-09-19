int32_t function_46b9a0(int32_t this_ptr) {
    // 0x46b9a0
    int32_t v2 = this_ptr;
    int32_t begin;
    int32_t end;
    int32_t device_count;
    int32_t index;

    if (v2 == 0)
        return 0;

    /* The original dispatches Update through each physical-device vtable
       before updating the default keyboard record and the input cluster. */
    begin = *(int32_t *)(intptr_t)(v2 + 180);
    end = *(int32_t *)(intptr_t)(v2 + 184);
    device_count = (begin != 0 && end >= begin &&
                    ((end - begin) % 168) == 0)
        ? (end - begin) / 168 : 0;
    for (index = 0; index < device_count; ++index) {
        int32_t record = begin + index * 168;
        int32_t *vtable = *(int32_t **)(intptr_t)record;
        if (vtable != NULL && vtable[1] != 0)
            retdec_call_thiscall0((void *)(intptr_t)record,
                                  (void *)(intptr_t)vtable[1]);
    }

    {
        int32_t record = v2 + 12;
        int32_t *vtable = *(int32_t **)(intptr_t)record;
        if (vtable != NULL && vtable[1] != 0)
            retdec_call_thiscall0((void *)(intptr_t)record,
                                  (void *)(intptr_t)vtable[1]);
    }
    retdec_update_input_cluster(v2 + 196);
    function_408320(v2 + 392);

    *(int32_t *)(intptr_t)(v2 + 1444) =
        *(int32_t *)(intptr_t)(v2 + 276);
    *(int32_t *)(intptr_t)(v2 + 1436) =
        *(int32_t *)(intptr_t)(v2 + 268);
    *(int32_t *)(intptr_t)(v2 + 1440) =
        *(int32_t *)(intptr_t)(v2 + 272);
    *(int32_t *)(intptr_t)(v2 + 1456) =
        *(int32_t *)(intptr_t)(v2 + 288);
    *(int32_t *)(intptr_t)(v2 + 1448) =
        *(int32_t *)(intptr_t)(v2 + 280);
    *(int32_t *)(intptr_t)(v2 + 1452) =
        *(int32_t *)(intptr_t)(v2 + 284);
    *(int32_t *)(intptr_t)(v2 + 1460) =
        *(int32_t *)(intptr_t)(v2 + 292);
    *(unsigned char *)(intptr_t)(v2 + 1468) =
        *(unsigned char *)(intptr_t)(v2 + 326);
    *(int32_t *)(intptr_t)(v2 + 1464) =
        *(int32_t *)(intptr_t)(v2 + 296);
    *(unsigned char *)(intptr_t)(v2 + 1469) =
        *(unsigned char *)(intptr_t)(v2 + 327);
    *(unsigned char *)(intptr_t)(v2 + 1470) =
        *(unsigned char *)(intptr_t)(v2 + 328);
    *(unsigned char *)(intptr_t)(v2 + 1471) =
        *(unsigned char *)(intptr_t)(v2 + 329);
    *(int32_t *)(intptr_t)(v2 + 1472) =
        function_4083e0(v2 + 392, 11, 0, 0, 0);
    *(int32_t *)(intptr_t)(v2 + 1476) =
        function_4083e0(v2 + 392, 2, 0, 0, 0);
    *(int32_t *)(intptr_t)(v2 + 1480) =
        function_4083e0(v2 + 392, 3, 0, 0, 0);
    *(int32_t *)(intptr_t)(v2 + 1484) =
        function_4083e0(v2 + 392, 4, 0, 0, 0);
    *(int32_t *)(intptr_t)(v2 + 1488) =
        function_4083e0(v2 + 392, 5, 0, 0, 0);
    *(int32_t *)(intptr_t)(v2 + 1492) =
        function_4083e0(v2 + 392, 6, 0, 0, 0);
    *(int32_t *)(intptr_t)(v2 + 1496) =
        function_4083e0(v2 + 392, 7, 0, 0, 0);
    *(int32_t *)(intptr_t)(v2 + 1500) =
        function_4083e0(v2 + 392, 8, 0, 0, 0);
    *(int32_t *)(intptr_t)(v2 + 1504) =
        function_4083e0(v2 + 392, 9, 0, 0, 0);
    *(int32_t *)(intptr_t)(v2 + 1508) =
        function_4083e0(v2 + 392, 10, 0, 0, 0);
    return *(int32_t *)(intptr_t)(v2 + 1508);
}