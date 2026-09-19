int32_t function_46b880(int32_t this_ptr, int32_t lpFileName) {
    retdec_trace_i32("46b880:this", this_ptr);
    retdec_trace_i32("46b880:path", lpFileName);
    retdec_trace_squirrel_name("input:config-load", lpFileName);
    HANDLE file_handle = CreateFileA((LPCSTR)(intptr_t)lpFileName,
                                      GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL, OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
        return 0;

    unsigned char record[0x44];
    DWORD transferred = 0;
    if (ReadFile(file_handle, record, sizeof(record), &transferred, NULL) &&
        transferred == sizeof(record)) {
        memcpy((void *)(intptr_t)(this_ptr + 0x10), record, sizeof(record));
        retdec_trace("input:config-keyboard-loaded");
        if ((int32_t)(int8_t)record[0] >= g782) {
            memset((void *)(intptr_t)(this_ptr + 0x10), 0, sizeof(record));
            *(unsigned char *)(intptr_t)(this_ptr + 0x10) = 0xfe;
        }

        // The original reads the second record once, then broadcasts that
        // same 0x44-byte value to every registered device record.
        if (ReadFile(file_handle, record, sizeof(record), &transferred,
                     NULL) && transferred == sizeof(record)) {
            int32_t begin = *(int32_t *)(intptr_t)(this_ptr + 0xb4);
            int32_t end = *(int32_t *)(intptr_t)(this_ptr + 0xb8);
            if ((int32_t)(int8_t)record[0] >= g782) {
                memset(record, 0, sizeof(record));
                record[0] = 0xfe;
            }
            while (begin != end) {
                memcpy((void *)(intptr_t)(begin + 4), record,
                       sizeof(record));
                begin += 0xa8;
            }
        }
    }
    CloseHandle(file_handle);
    retdec_trace("46b880:done");
    return 0;
}