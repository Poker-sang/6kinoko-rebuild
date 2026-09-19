int32_t function_472e50(int32_t path_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data) {
    int32_t input_object[3] = { object_vtable, object_type, object_data };
    int32_t table_object[3];
    int32_t input_stream[16] = { 0 };
    unsigned char *raw = NULL;
    unsigned char *encoded = NULL;
    HANDLE file_handle = INVALID_HANDLE_VALUE;
    DWORD encoded_size;
    DWORD bytes_written;
    int32_t result = 0;

    retdec_trace("savedata:save-begin");
    retdec_trace((const char *)(intptr_t)path_ptr);
    file_handle = CreateFileA(
        (LPCSTR)(intptr_t)path_ptr, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
        goto cleanup;
    raw = (unsigned char *)malloc(0x20000u);
    encoded = (unsigned char *)malloc(0x20000u);
    if (raw == NULL || encoded == NULL)
        goto cleanup;

    input_stream[0] = (int32_t)(intptr_t)raw;
    input_stream[1] = 0;
    input_stream[2] = 0x20000;
    if (!retdec_squirrel_object_from_pair(table_object, object_type, object_data) ||
        !function_472820(input_stream, table_object[0], table_object[1],
                         table_object[2]))
        goto cleanup;
    retdec_trace_i32("savedata:raw-size", input_stream[1]);
    encoded_size = (DWORD)function_404390(
        (int32_t)(intptr_t)raw, input_stream[1],
        (int32_t *)(intptr_t)encoded, 0x20000);
    retdec_trace_i32("savedata:encoded-size", (int32_t)encoded_size);
    if (encoded_size == 0 || encoded_size > 0x20000)
        goto cleanup;
    if (!WriteFile(file_handle, &encoded_size, sizeof(encoded_size),
                   &bytes_written, NULL) ||
        bytes_written != sizeof(encoded_size) ||
        !WriteFile(file_handle, encoded, encoded_size, &bytes_written, NULL) ||
        bytes_written != encoded_size)
        goto cleanup;
    result = 1;

cleanup:
    if (file_handle != INVALID_HANDLE_VALUE)
        CloseHandle(file_handle);
    if (encoded != NULL)
        free(encoded);
    if (raw != NULL)
        free(raw);
    function_4a9d70_this((int32_t)(intptr_t)input_object);
    retdec_trace_i32("savedata:save-result", result);
    return result;
}