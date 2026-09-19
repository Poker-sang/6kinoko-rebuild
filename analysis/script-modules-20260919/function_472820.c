int32_t function_472820(int32_t *stream_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data) {
    int32_t input_object[3] = { object_vtable, object_type, object_data };
    int32_t container[3];
    int32_t key[3];
    int32_t value[3];
    int32_t iterator_started = 0;
    int32_t ok = 1;

    if (stream_ptr == NULL || stream_ptr[0] == 0 || g644 == 0)
        ok = 0;
    function_4a94e0_this((int32_t)(intptr_t)container);
    function_4a94e0_this((int32_t)(intptr_t)key);
    function_4a94e0_this((int32_t)(intptr_t)value);

    if (ok && !retdec_squirrel_object_from_pair(
                   container, object_type, object_data))
        ok = 0;
    if (ok && function_4a9c10_this((int32_t)(intptr_t)container))
        iterator_started = 1;

    while (ok && iterator_started &&
           function_4a9c60((int32_t *)(intptr_t)key,
                           (int32_t *)(intptr_t)value)) {
        uint32_t value_type = (uint32_t)value[1];
        uint32_t key_type = (uint32_t)key[1];

        /* Original 4728E9 skips null/non-serializable values before the tag. */
        if ((value_type & 0x7eu) == 0)
            continue;
        if (!retdec_table_stream_write(stream_ptr, &value_type,
                                       sizeof(value_type))) {
            ok = 0;
            break;
        }
        if (key_type == 0x05000002u) {
            if (!retdec_table_stream_write(stream_ptr, &key_type,
                                           sizeof(key_type)) ||
                !retdec_table_stream_write(stream_ptr, &key[2],
                                           sizeof(key[2]))) {
                ok = 0;
                break;
            }
        } else if (key_type == 0x08000010u) {
            if (!retdec_table_stream_write(stream_ptr, &key_type,
                                           sizeof(key_type)) ||
                !retdec_table_stream_write_object_string(stream_ptr, key)) {
                ok = 0;
                break;
            }
        } else {
            ok = 0;
            break;
        }

        if (value_type == 0x05000002u || value_type == 0x05000004u) {
            if (!retdec_table_stream_write(stream_ptr, &value[2],
                                           sizeof(value[2])))
                ok = 0;
        } else if (value_type == 0x01000008u) {
            unsigned char bool_value = (unsigned char)value[2];
            if (!retdec_table_stream_write(stream_ptr, &bool_value, 1))
                ok = 0;
        } else if (value_type == 0x08000010u) {
            if (!retdec_table_stream_write_object_string(stream_ptr, value))
                ok = 0;
        } else if (value_type == 0x08000040u) {
            int32_t nested[3];
            int32_t count = function_4a96d0(
                (int32_t)(intptr_t)value);
            if (count < 0 ||
                !retdec_table_stream_write(stream_ptr, &count,
                                            sizeof(count)) ||
                !retdec_squirrel_object_copy(nested, value)) {
                ok = 0;
            } else {
                ok = function_472820(stream_ptr, nested[0], nested[1], nested[2]);
            }
        } else if (value_type == 0x0A000020u) {
            int32_t nested[3];
            if (!retdec_squirrel_object_copy(nested, value)) {
                ok = 0;
            } else {
                ok = function_472820(stream_ptr, nested[0], nested[1], nested[2]);
            }
        }

        function_4a9d70_this((int32_t)(intptr_t)value);
        function_4a9d70_this((int32_t)(intptr_t)key);
    }

    if (iterator_started) {
        uint32_t null_type = 0x01000001u;
        if (ok && !retdec_table_stream_write(stream_ptr, &null_type,
                                       sizeof(null_type)))
            ok = 0;
        function_4a9d50();
    }
    function_4a9d70_this((int32_t)(intptr_t)value);
    function_4a9d70_this((int32_t)(intptr_t)key);
    function_4a9d70_this((int32_t)(intptr_t)container);
    function_4a9d70_this((int32_t)(intptr_t)input_object);
    return ok;
}