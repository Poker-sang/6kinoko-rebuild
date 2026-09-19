int32_t function_46b7c0(int32_t this_ptr, int32_t lpFileName) {
    retdec_trace_i32("46b7c0:this", this_ptr);
    retdec_trace_i32("46b7c0:path", lpFileName);
    HANDLE file_handle = CreateFileA((LPCSTR)(intptr_t)lpFileName,
                                      GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE)
        return 0;

    DWORD transferred = 0;
    WriteFile(file_handle, (LPCVOID)(intptr_t)(this_ptr + 0x10), 0x44,
              &transferred, NULL);
    int32_t begin = *(int32_t *)(intptr_t)(this_ptr + 0xb4);
    int32_t end = *(int32_t *)(intptr_t)(this_ptr + 0xb8);
    if (begin != end)
        WriteFile(file_handle, (LPCVOID)(intptr_t)(begin + 4), 0x44,
                  &transferred, NULL);
    CloseHandle(file_handle);
    retdec_trace_squirrel_name("input:config-saved", lpFileName);
    return 0;
}