int32_t function_46bbe0(int32_t this_ptr, int32_t device,
                        int32_t field, int32_t value) {
    if (this_ptr == 0 || field < 0 || field >= 12)
        return 0;

    int32_t begin = *(int32_t *)(intptr_t)(this_ptr + 0xb4);
    int32_t end = *(int32_t *)(intptr_t)(this_ptr + 0xb8);
    int32_t count = (begin != 0 && end >= begin)
                        ? (end - begin) / 0xa8
                        : 0;
    int32_t *destination = 0;
    if (device == -1) {
        destination = (int32_t *)(intptr_t)(this_ptr + 0x10);
    } else if (device >= 0 && device < count) {
        destination = (int32_t *)(intptr_t)(begin + device * 0xa8 + 4);
    } else {
        return 0;
    }

    int32_t record[17];
    memcpy(record, destination, sizeof(record));
    record[field + 5] = value;
    if ((int32_t)(int8_t)(record[0] & 0xff) >= g782) {
        memset(record, 0, sizeof(record));
        record[0] = 0xfe;
    }
    memcpy(destination, record, sizeof(record));
    return 0;
/* The generated body below is retained as a reference to the original
 * control flow, but the explicit receiver path above is the live one. */
}