#include <windows.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <squirrel.h>
#include "kinoko/squirrel_game_objects.h"
#include "kinoko/squirrel_host_compat.h"

extern "C" {
extern char* g644;
void retdec_trace(const char*);
void retdec_trace_i32(const char*, int32_t);
int32_t function_404390(int32_t, int32_t, int32_t*, int32_t);
int32_t function_404430(int32_t, int32_t, int32_t, int32_t);
int32_t function_4722e0(int32_t*, int32_t, int32_t, int32_t);
int32_t function_472820(int32_t*, int32_t, int32_t, int32_t);
}
// Original wire format: 32-bit Squirrel tags, counted strings, one-byte bools,
// recursive arrays/tables and an OT_NULL terminator. Arguments own references.
// Existing stream bounds and failure behavior are retained unchanged.
static int32_t retdec_table_stream_write(int32_t *stream, const void *data,
                                         uint32_t size) {
    if (stream == NULL || stream[0] == 0 || (data == NULL && size != 0) ||
        stream[1] < 0 || stream[1] > stream[2] ||
        size > (uint32_t)(stream[2] - stream[1]))
        return 0;
    if (size != 0) {
        memcpy((void *)(intptr_t)(stream[0] + stream[1]), data, size);
        stream[1] += (int32_t)size;
    }
    return 1;
}

static int32_t retdec_table_stream_read(int32_t *stream, void *data,
                                        uint32_t size) {
    if (stream == NULL || stream[0] == 0 || (data == NULL && size != 0) ||
        stream[1] < 0 || stream[1] > stream[2] ||
        size > (uint32_t)(stream[2] - stream[1]))
        return 0;
    if (size != 0) {
        memcpy(data, (const void *)(intptr_t)(stream[0] + stream[1]), size);
        stream[1] += (int32_t)size;
    }
    return 1;
}


static int32_t retdec_table_stream_read_string(int32_t *stream,
                                               int32_t *object) {
    uint32_t length;
    char *data;
    int32_t result;

    if (!retdec_table_stream_read(stream, &length, sizeof(length)) ||
        length > 0x1000000u)
        return 0;
    data = (char *)malloc((size_t)length + 1);
    if (data == NULL)
        return 0;
    if (!retdec_table_stream_read(stream, data, length)) {
        free(data);
        return 0;
    }
    data[length] = 0;
    result = retdec_squirrel_object_from_string(object, data, length);
    free(data);
    return result;
}

static int32_t retdec_table_stream_write_object_string(int32_t *stream,
                                                       int32_t *object) {
    const char *data;
    uint32_t length;

    if (!retdec_squirrel_object_string(object, &data))
        return 0;
    length = (uint32_t)strlen(data);
    return retdec_table_stream_write(stream, &length, sizeof(length)) &&
           retdec_table_stream_write(stream, data, length);
}

extern "C" int32_t function_4722e0(int32_t *stream_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data) {
    int32_t parent[3] = { object_vtable, object_type, object_data };
    int32_t key[3];
    int32_t value[3];
    int32_t ok = 1;

    if (stream_ptr == NULL || stream_ptr[0] == 0 || g644 == 0)
        ok = 0;
    (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(intptr_t)((int32_t)(intptr_t)key)));
    (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(intptr_t)((int32_t)(intptr_t)value)));

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
        if (key_type == OT_INTEGER) {
            uint32_t raw_key;
            if (!retdec_table_stream_read(stream_ptr, &raw_key,
                                          sizeof(raw_key)) ||
                !retdec_squirrel_object_from_pair(
                    key, (int32_t)key_type, (int32_t)raw_key)) {
                ok = 0;
                break;
            }
        } else if (key_type == OT_STRING) {
            if (!retdec_table_stream_read_string(stream_ptr, key)) {
                ok = 0;
                break;
            }
        } else {
            ok = 0;
            break;
        }

        if (value_type == OT_INTEGER || value_type == OT_FLOAT) {
            uint32_t raw_value;
            if (!retdec_table_stream_read(stream_ptr, &raw_value,
                                          sizeof(raw_value)) ||
                !retdec_squirrel_object_from_pair(
                    value, (int32_t)value_type, (int32_t)raw_value))
                ok = 0;
        } else if (value_type == OT_BOOL) {
            unsigned char bool_value;
            if (!retdec_table_stream_read(stream_ptr, &bool_value, 1) ||
                !retdec_squirrel_object_from_pair(
                    value, (int32_t)value_type, (int32_t)bool_value))
                ok = 0;
        } else if (value_type == OT_STRING) {
            if (!retdec_table_stream_read_string(stream_ptr, value))
                ok = 0;
        } else if (value_type == OT_ARRAY) {
            uint32_t count;
            int32_t nested[3];

            if (!retdec_table_stream_read(stream_ptr, &count,
                                          sizeof(count)) || count > 0x100000u ||
                !(int32_t*)(intptr_t)(kinoko_sqplus_object_new_array((void *)(intptr_t)(value), (int32_t)count)) ||
                !kinoko_sqplus_object_raw_set_object((void *)(intptr_t)((int32_t)(intptr_t)parent), (const void *)(intptr_t)((int32_t)(intptr_t)key), (const void *)(intptr_t)((int32_t)(intptr_t)value)) ||
                !retdec_squirrel_object_copy(nested, value)) {
                ok = 0;
            } else {
                ok = function_4722e0(stream_ptr, nested[0], nested[1], nested[2]);
            }
        } else if (value_type == OT_TABLE) {
            int32_t nested[3];

            if (!(int32_t*)(intptr_t)(kinoko_sqplus_object_new_table((void *)(intptr_t)(value))) ||
                !kinoko_sqplus_object_raw_set_object((void *)(intptr_t)((int32_t)(intptr_t)parent), (const void *)(intptr_t)((int32_t)(intptr_t)key), (const void *)(intptr_t)((int32_t)(intptr_t)value)) ||
                !retdec_squirrel_object_copy(nested, value)) {
                ok = 0;
            } else {
                ok = function_4722e0(stream_ptr, nested[0], nested[1], nested[2]);
            }
        } else {
            ok = 0;
        }

        if (ok && value_type != OT_ARRAY &&
            value_type != OT_TABLE) {
            if (!kinoko_sqplus_object_raw_set_object((void *)(intptr_t)((int32_t)(intptr_t)parent), (const void *)(intptr_t)((int32_t)(intptr_t)key), (const void *)(intptr_t)((int32_t)(intptr_t)value)))
                ok = 0;
        }
        kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)value));
        kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)key));
    }

    kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)value));
    kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)key));
    kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)parent));
    return ok;
}

extern "C" int32_t function_472820(int32_t *stream_ptr, int32_t object_vtable,
                        int32_t object_type, int32_t object_data) {
    int32_t input_object[3] = { object_vtable, object_type, object_data };
    int32_t container[3];
    int32_t key[3];
    int32_t value[3];
    int32_t iterator_started = 0;
    int32_t ok = 1;

    if (stream_ptr == NULL || stream_ptr[0] == 0 || g644 == 0)
        ok = 0;
    (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(intptr_t)((int32_t)(intptr_t)container)));
    (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(intptr_t)((int32_t)(intptr_t)key)));
    (int32_t)(intptr_t)(kinoko_sqplus_object_initialize((void *)(intptr_t)((int32_t)(intptr_t)value)));

    if (ok && !retdec_squirrel_object_from_pair(
                   container, object_type, object_data))
        ok = 0;
    if (ok && kinoko_sqplus_object_begin_iteration((void *)(intptr_t)((int32_t)(intptr_t)container)))
        iterator_started = 1;

    while (ok && iterator_started &&
           kinoko_sqplus_object_next((int32_t *)(intptr_t)key, (int32_t *)(intptr_t)value)) {
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
        if (key_type == OT_INTEGER) {
            if (!retdec_table_stream_write(stream_ptr, &key_type,
                                           sizeof(key_type)) ||
                !retdec_table_stream_write(stream_ptr, &key[2],
                                           sizeof(key[2]))) {
                ok = 0;
                break;
            }
        } else if (key_type == OT_STRING) {
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

        if (value_type == OT_INTEGER || value_type == OT_FLOAT) {
            if (!retdec_table_stream_write(stream_ptr, &value[2],
                                           sizeof(value[2])))
                ok = 0;
        } else if (value_type == OT_BOOL) {
            unsigned char bool_value = (unsigned char)value[2];
            if (!retdec_table_stream_write(stream_ptr, &bool_value, 1))
                ok = 0;
        } else if (value_type == OT_STRING) {
            if (!retdec_table_stream_write_object_string(stream_ptr, value))
                ok = 0;
        } else if (value_type == OT_ARRAY) {
            int32_t nested[3];
            int32_t count = kinoko_sqplus_object_size((void *)(intptr_t)((int32_t)(intptr_t)value));
            if (count < 0 ||
                !retdec_table_stream_write(stream_ptr, &count,
                                            sizeof(count)) ||
                !retdec_squirrel_object_copy(nested, value)) {
                ok = 0;
            } else {
                ok = function_472820(stream_ptr, nested[0], nested[1], nested[2]);
            }
        } else if (value_type == OT_TABLE) {
            int32_t nested[3];
            if (!retdec_squirrel_object_copy(nested, value)) {
                ok = 0;
            } else {
                ok = function_472820(stream_ptr, nested[0], nested[1], nested[2]);
            }
        }

        kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)value));
        kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)key));
    }

    if (iterator_started) {
        uint32_t null_type = OT_NULL;
        if (ok && !retdec_table_stream_write(stream_ptr, &null_type,
                                       sizeof(null_type)))
            ok = 0;
        kinoko_sqplus_object_end_iteration();
    }
    kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)value));
    kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)key));
    kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)container));
    kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)input_object));
    return ok;
}

extern "C" int32_t function_472c90(int32_t path_ptr, int32_t object_vtable,
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
    kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)input_object));
    retdec_trace_i32("savedata:load-result", result);
    return result;
}

extern "C" int32_t function_472e50(int32_t path_ptr, int32_t object_vtable,
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
    kinoko_sqplus_object_destroy((void *)(intptr_t)((int32_t)(intptr_t)input_object));
    retdec_trace_i32("savedata:save-result", result);
    return result;
}

