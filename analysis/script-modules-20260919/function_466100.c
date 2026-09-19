int32_t function_466100(int32_t a1) {
    static volatile LONG trace_count;
    LONG trace_index = InterlockedIncrement(&trace_count);
    int32_t *object_list_node;
    int32_t act;
    int32_t holder;
    int32_t resource = 0;
    int32_t list_value;
    int32_t list_node;

    if (trace_index <= 8) {
        retdec_trace("466100:entry");
        retdec_trace_squirrel_name("466100:file", a1);
    }

    object_list_node = (int32_t *)(intptr_t)
        _3f__3f_2_40_YAPAXI_40_Z(12);
    if (object_list_node == NULL)
        return 0;
    object_list_node[0] = 0;
    object_list_node[1] = 0;
    object_list_node[2] = 0;

    act = _3f__3f_2_40_YAPAXI_40_Z(240);
    if (act == 0) {
        free(object_list_node);
        return 0;
    }
    act = function_427530(act);
    object_list_node[0] = act;
    if (act == 0) {
        free(object_list_node);
        return 0;
    }

    if (!function_428000(act, (const char *)(intptr_t)a1)) {
        retdec_trace("466100:act-header-failed");
        /* Do not publish or execute callbacks from a partially parsed CAct.
           The current loader can reject a layout after it has initialized
           the outer object; keeping that object detached is safer than
           passing invalid vectors into the renderer. */
        retdec_trace("466100:skip-invalid-act");
        free(object_list_node);
        return 0;
    } else {
        retdec_trace("466100:act-header-ok");
    }

    /* 455880/455E40 form the holder/resource pair owned by the list node.
       Their original ECX receivers are explicit here. */
    holder = _3f__3f_2_40_YAPAXI_40_Z(4);
    if (holder != 0) {
        function_455880(holder, act);
        object_list_node[1] = holder;
        function_455e40(holder, (int32_t)(intptr_t)&resource, 0);
        object_list_node[2] = resource;
    }
    retdec_trace_i32("466100:act", act);
    retdec_trace_i32("466100:holder", holder);
    retdec_trace_i32("466100:resource", resource);

    /* Original 466100 calls 450E30 before linking the CAct node. */
    if (resource != 0 && g644 != NULL) {
        int32_t register_result = retdec_call_thiscall2_result(
            (void *)(intptr_t)resource,
            (void *)(intptr_t)kinoko_method_root_table_construct,
            (int32_t)(intptr_t)g644, 0);
        retdec_trace_i32("466100:450e30-result", register_result);
    }

    if (g603 == 0)
        return (int32_t)(intptr_t)object_list_node;
    if (trace_index <= 8) {
        retdec_trace_i32("466100:list-before", g603);
        retdec_trace_i32("466100:list-tail",
                         *(int32_t *)(intptr_t)(g603 + 4));
    }
    list_value = (int32_t)(intptr_t)object_list_node;
    list_node = function_4214a0(
        g603, *(int32_t *)(intptr_t)(g603 + 4), &list_value);
    if (trace_index <= 8)
        retdec_trace_i32("466100:list-node", list_node);
    if (list_node != 0) {
        *(int32_t *)(intptr_t)(*(int32_t *)(intptr_t)(list_node + 4)) =
            list_node;
        *(int32_t *)(intptr_t)(g603 + 4) = list_node;
        ++g604;
        if (trace_index <= 8) {
            retdec_trace_i32("466100:list-first",
                             *(int32_t *)(intptr_t)g603);
            retdec_trace_i32("466100:list-tail-after",
                             *(int32_t *)(intptr_t)(g603 + 4));
            retdec_trace_i32("466100:list-count", g604);
        }
    }
    return (int32_t)(intptr_t)object_list_node;
}