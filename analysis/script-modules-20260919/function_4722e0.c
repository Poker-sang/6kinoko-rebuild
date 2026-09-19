int32_t function_4722e0(int32_t *stream_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data) {
    int32_t parent[3] = { object_vtable, object_type, object_data };
    int32_t key[3];
    int32_t value[3];
    int32_t ok = 1;

    if (stream_ptr == NULL || stream_ptr[0] == 0 || g644 == 0)
        ok = 0;
    function_4a94e0_this((int32_t)(intptr_t)key);
    function_4a94e0_this((int32_t)(intptr_t)value);

    while (ok) {
        uint32_t value_type;
        uint32_t key_type;

        if (!retdec_table_stream_read(stream_ptr, &value_type,
                                      sizeof(value_type))) {
            ok = 0;
            break;
        }
        if ((value_type & 0x7eu) == 0)
            break;

        if (!retdec_table_stream_read(stream_ptr, &key_type,
                                      sizeof(key_type))) {
            ok = 0;
            break;
        }
        if (key_type == 0x05000002u) {
            uint32_t raw_key;
            if (!retdec_table_stream_read(stream_ptr, &raw_key,
                                          sizeof(raw_key)) ||
                !retdec_squirrel_object_from_pair(
                    key, (int32_t)key_type, (int32_t)raw_key)) {
                ok = 0;
                break;
            }
        } else if (key_type == 0x08000010u) {
            if (!retdec_table_stream_read_string(stream_ptr, key)) {
                ok = 0;
                break;
            }
        } else {
            ok = 0;
            break;
        }

        if (value_type == 0x05000002u) {
            uint32_t raw_value;
            if (!retdec_table_stream_read(stream_ptr, &raw_value,
                                          sizeof(raw_value)) ||
                !retdec_squirrel_object_from_pair(
                    value, (int32_t)value_type, (int32_t)raw_value))
                ok = 0;
        } else if (value_type == 0x05000004u) {
            uint32_t raw_value;
            if (!retdec_table_stream_read(stream_ptr, &raw_value,
                                          sizeof(raw_value)) ||
                !retdec_squirrel_object_from_pair(
                    value, (int32_t)value_type, (int32_t)raw_value))
                ok = 0;
        } else if (value_type == 0x01000008u) {
            unsigned char bool_value;
            if (!retdec_table_stream_read(stream_ptr, &bool_value, 1) ||
                !retdec_squirrel_object_from_pair(
                    value, (int32_t)value_type, (int32_t)bool_value))
                ok = 0;
        } else if (value_type == 0x08000010u) {
            if (!retdec_table_stream_read_string(stream_ptr, value))
                ok = 0;
        } else if (value_type == 0x08000040u) {
            uint32_t count;
            int32_t nested[3];

            if (!retdec_table_stream_read(stream_ptr, &count,
                                          sizeof(count)) || count > 0x100000u ||
                !function_4a92e0_this(value, (int32_t)count) ||
                !function_4a97b0_this(
                    (int32_t)(intptr_t)parent,
                    (int32_t)(intptr_t)key,
                    (int32_t)(intptr_t)value) ||
                !retdec_squirrel_object_copy(nested, value)) {
                ok = 0;
            } else {
                ok = function_4722e0(stream_ptr, nested[0], nested[1], nested[2]);
            }
        } else if (value_type == 0x0A000020u) {
            int32_t nested[3];

            if (!function_4a91c0_this(value) ||
                !function_4a97b0_this(
                    (int32_t)(intptr_t)parent,
                    (int32_t)(intptr_t)key,
                    (int32_t)(intptr_t)value) ||
                !retdec_squirrel_object_copy(nested, value)) {
                ok = 0;
            } else {
                ok = function_4722e0(stream_ptr, nested[0], nested[1], nested[2]);
            }
        } else {
            ok = 0;
        }

        if (ok && value_type != 0x08000040u &&
            value_type != 0x0A000020u) {
            if (!function_4a97b0_this(
                    (int32_t)(intptr_t)parent,
                    (int32_t)(intptr_t)key,
                    (int32_t)(intptr_t)value))
                ok = 0;
        }
        function_4a9d70_this((int32_t)(intptr_t)value);
        function_4a9d70_this((int32_t)(intptr_t)key);
    }

    function_4a9d70_this((int32_t)(intptr_t)value);
    function_4a9d70_this((int32_t)(intptr_t)key);
    function_4a9d70_this((int32_t)(intptr_t)parent);
    return ok;
}