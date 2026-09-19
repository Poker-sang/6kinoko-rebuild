int32_t function_46bc90(int32_t this_ptr, int32_t device, int32_t field) {
    int32_t record[17];
    int32_t begin, end;
    if (this_ptr == 0 || field < 0 || field >= 12)
        return 0;
    if (device == -1) {
        for (int32_t scan = 0; scan < 256; ++scan) {
            /* The original excludes Kanji, Caps Lock and Kana. */
            if (scan == 148 || scan == 58 || scan == 112 || !function_408e60(scan))
                continue;
            memcpy(record, (const void *)(intptr_t)(this_ptr + 16), sizeof(record));
            record[field + 1] = scan;
            memcpy((void *)(intptr_t)(this_ptr + 16), record, sizeof(record));
            retdec_trace_i32("input:assign-keyboard-field", field);
            retdec_trace_i32("input:assign-keyboard-scan", scan);
            return 1;
        }
        return 0;
    }
    begin = *(int32_t *)(intptr_t)(this_ptr + 180);
    end = *(int32_t *)(intptr_t)(this_ptr + 184);
    if (device >= 0 && device < (end - begin) / 168) {
        for (int32_t index = 0; index < (end - begin) / 168; ++index) {
            int32_t state = function_408e80(index);
            if (state == 0) continue;
            for (int32_t button = 0; button < 32; ++button) {
                if (*(uint8_t *)(intptr_t)(state + 48 + button) == 0) continue;
                memcpy(record, (const void *)(intptr_t)(begin + 4), sizeof(record));
                record[field + 5] = button;
                for (int32_t target = begin; target < end; target += 168)
                    memcpy((void *)(intptr_t)(target + 4), record, sizeof(record));
                return 1;
            }
        }
    }
    return 0;
}