int32_t function_46be40(int32_t this_ptr, int32_t device, int32_t field) {
    if (this_ptr == 0 || field < 0 || field >= 12)
        return -1;

    int32_t begin = *(int32_t *)(intptr_t)(this_ptr + 0xb4);
    int32_t end = *(int32_t *)(intptr_t)(this_ptr + 0xb8);
    int32_t count = (begin != 0 && end >= begin)
                        ? (end - begin) / 0xa8
                        : 0;
    int32_t *record = 0;
    if (device == -1) {
        record = (int32_t *)(intptr_t)(this_ptr + 0x10);
    } else if (device >= 0 && device < count) {
        record = (int32_t *)(intptr_t)(begin + device * 0xa8 + 4);
    } else {
        return -1;
    }
    /* 46BE83: keyboard indexes include the four direction assignments. */
    return record[field + (device == -1 ? 1 : 5)];
}