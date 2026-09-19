int32_t function_472c90(int32_t path_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data) {
    int32_t input_object[3] = { object_vtable, object_type, object_data };
    HANDLE file_handle = INVALID_HANDLE_VALUE;
    unsigned char *encoded = NULL;
    unsigned char *decoded = NULL;
    DWORD encoded_size = 0;
    DWORD bytes_read = 0;
    int32_t stream[16] = { 0 };
    int32_t table_object[3];
    int32_t result = 0;

    retdec_trace("savedata:load-begin");
    retdec_trace((const char *)(intptr_t)path_ptr);
    file_handle = CreateFileA(
        (LPCSTR)(intptr_t)path_ptr, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
        goto cleanup;

    encoded = (unsigned char *)malloc(0x20000u);
    decoded = (unsigned char *)malloc(0x20000u);
    if (encoded == NULL || decoded == NULL)
        goto cleanup;
    if (!ReadFile(file_handle, &encoded_size, sizeof(encoded_size),
                  &bytes_read, NULL) || bytes_read != sizeof(encoded_size) ||
        encoded_size > 0x20000u)
        goto cleanup;
    if (!ReadFile(file_handle, encoded, encoded_size, &bytes_read, NULL) ||
        bytes_read != encoded_size)
        goto cleanup;

    stream[2] = function_404430((int32_t)(intptr_t)encoded,
                          (int32_t)encoded_size,
                          (int32_t)(intptr_t)decoded, 0x20000);
    retdec_trace_i32("savedata:decoded-size", stream[2]);
    if (stream[2] <= 0)
        goto cleanup;
    stream[0] = (int32_t)(intptr_t)decoded;
    stream[1] = 0;
    if (!retdec_squirrel_object_from_pair(table_object, object_type,
                                          object_data))
        goto cleanup;
    result = function_4722e0(stream, table_object[0], table_object[1], table_object[2]);
    retdec_trace_i32("savedata:decoded-consumed", stream[1]);

cleanup:
    if (file_handle != INVALID_HANDLE_VALUE)
        CloseHandle(file_handle);
    if (decoded != NULL)
        free(decoded);
    if (encoded != NULL)
        free(encoded);
    function_4a9d70_this((int32_t)(intptr_t)input_object);
    retdec_trace_i32("savedata:load-result", result);
    return result;
}