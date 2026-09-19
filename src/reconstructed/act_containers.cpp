// Native C++ continuation of the recovered ACT path. Original function names
// remain C ABI ports until the surrounding decompiled host is migrated.
#include "kinoko/act_runtime.h"
#include "kinoko/act_host.h"
#include "kinoko/diagnostics.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/sqrat_object_bridge.h"
#include "kinoko/native_property_bridge.h"
#include "kinoko/squirrel_binding.h"
#include "kinoko/squirrel_native_calls.h"
#include "kinoko/squirrel_game_objects.h"
#include "kinoko/squirrel_native_arguments.h"
#include "kinoko/squirrel_host_compat.h"
#include "kinoko/squirrel_legacy_api.h"
#include "kinoko/squirrel_source_runtime.h"
#include "kinoko/squirrel_compile_bridge.h"
#include "kinoko/squirrel_value_bridge.h"
#include "kinoko/actor_methods.h"
#include "kinoko/actor_animation.h"
#include "kinoko/actor_cleanup.h"
#include "kinoko/act_clone.h"
#include "kinoko/act_resource.h"
#include "kinoko/actor_lifecycle.h"
#include "kinoko/legacy_method_entries.h"
#include "kinoko/script_callbacks.h"
#include "kinoko/squirrel_object.h"
#include "kinoko/texture_store.h"
#include "kinoko/map_render.h"
#include "kinoko/sprite.h"
#include "kinoko/game_math.h"
#include <windows.h>
#include <d3d9.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>

using kinoko::legacy::pointer;
using kinoko::legacy::address;
using kinoko::legacy::field;

uint32_t retdec_string_length32(int32_t object)
{
    if (object == 0) {
        return 0;
    }
    return field<uint32_t>(object + 16);
}

const unsigned char *retdec_string_data32(int32_t object)
{
    uint32_t capacity;

    if (object == 0) {
        return nullptr;
    }
    capacity = field<uint32_t>(object + 20);
    if (capacity < 16) {
        return pointer<const unsigned char>(object);
    }
    return pointer<const unsigned char>(field<uint32_t>(object));
}

int32_t retdec_compare_bytes32(const unsigned char *left,
                                      const unsigned char *right,
                                      size_t length)
{
    size_t index;

    for (index = 0; index < length; ++index) {
        if (left[index] != right[index]) {
            return left[index] < right[index] ? -1 : 1;
        }
    }
    return 0;
}

int32_t retdec_compare_strings32(int32_t left_object,
                                        int32_t right_object)
{
    uint32_t left_length = retdec_string_length32(left_object);
    uint32_t right_length = retdec_string_length32(right_object);
    uint32_t common_length = left_length < right_length
        ? left_length : right_length;
    int32_t result = retdec_compare_bytes32(
        retdec_string_data32(left_object),
        retdec_string_data32(right_object),
        common_length);

    if (result != 0) {
        return result;
    }
    if (left_length < right_length) {
        return -1;
    }
    if (left_length > right_length) {
        return 1;
    }
    return 0;
}

uint32_t retdec_string_hash32(int32_t object)
{
    const unsigned char *data = retdec_string_data32(object);
    uint32_t length = retdec_string_length32(object);
    uint32_t hash = 0x811C9DC5u;
    uint32_t index;
    int32_t signed_hash;
    int32_t quotient;
    int32_t remainder;
    int32_t result;

    for (index = 0; index < length; ++index) {
        hash = (hash * 0x01000193u) ^ data[index];
    }

    signed_hash = (int32_t)(hash & 0x7FFFFFFFu);
    quotient = signed_hash / 127773;
    remainder = signed_hash % 127773;
    result = 16807 * remainder - 2836 * quotient;
    if (result < 0) {
        result += 0x7FFFFFFF;
    }
    return (uint32_t)result;
}

int32_t retdec_vector_insert32(uint32_t count,
                                      int32_t *vector,
                                      int32_t position,
                                      int32_t value)
{
    int32_t *begin;
    int32_t *end;
    int32_t *capacity_end;
    int32_t *insert;
    int32_t *new_begin;
    size_t old_count;
    size_t old_capacity;
    size_t offset;
    size_t tail_count;
    size_t new_count;
    size_t new_capacity;
    size_t index;
    int32_t fill_value;

    if (vector == nullptr || count == 0) {
        return address(vector);
    }

    begin = pointer<int32_t>(vector[0]);
    end = pointer<int32_t>(vector[1]);
    capacity_end = pointer<int32_t>(vector[2]);
    if (begin == nullptr) {
        if (end != nullptr || capacity_end != nullptr) {
            return address(vector);
        }
        insert = nullptr;
        old_count = 0;
        old_capacity = 0;
        offset = 0;
    } else {
        insert = pointer<int32_t>(position);
        if (end == nullptr || capacity_end == nullptr || insert == nullptr ||
            insert < begin || insert > end || end < begin ||
            capacity_end < end) {
            return address(vector);
        }
        old_count = (size_t)(end - begin);
        old_capacity = (size_t)(capacity_end - begin);
        offset = (size_t)(insert - begin);
    }

    if ((size_t)count > SIZE_MAX - old_count) {
        return address(vector);
    }
    new_count = old_count + (size_t)count;
    new_capacity = old_capacity;
    if (new_capacity < new_count) {
        size_t growth = new_capacity / 2;
        if (new_capacity > SIZE_MAX - growth) {
            new_capacity = new_count;
        } else {
            new_capacity += growth;
            if (new_capacity < new_count) {
                new_capacity = new_count;
            }
        }
    }
    if (new_capacity > SIZE_MAX / sizeof(int32_t)) {
        return address(vector);
    }
    fill_value = field<int32_t>(value);

    if (new_capacity != old_capacity) {
        new_begin = (int32_t *)std::malloc(new_capacity * sizeof(int32_t));
        if (new_begin == nullptr) {
            return address(vector);
        }
        if (offset != 0) {
            std::memcpy(new_begin, begin, offset * sizeof(int32_t));
        }
        for (index = 0; index < (size_t)count; ++index) {
            new_begin[offset + index] = fill_value;
        }
        tail_count = old_count - offset;
        if (tail_count != 0) {
            std::memcpy(new_begin + offset + count,
                   begin + offset,
                   tail_count * sizeof(int32_t));
        }
        std::free(begin);
        vector[0] = address(new_begin);
        vector[1] = address((new_begin + new_count));
        vector[2] = address((new_begin + new_capacity));
        return address(vector);
    }

    tail_count = old_count - offset;
    if (tail_count != 0) {
        std::memmove(insert + count, insert, tail_count * sizeof(int32_t));
    }
    for (index = 0; index < (size_t)count; ++index) {
        insert[index] = fill_value;
    }
    vector[1] = address((end + count));
    return address(vector);
}

int32_t retdec_erase_node32(int32_t list_base,
                                   int32_t result,
                                   int32_t node)
{
    int32_t next;
    int32_t previous;
    uint32_t capacity;

    if (list_base == 0 || result == 0 || node == 0) {
        return result;
    }
    next = field<int32_t>(node);
    if (node == field<int32_t>(list_base)) {
        field<int32_t>(result) = next;
        return result;
    }
    previous = field<int32_t>(node + 4);
    field<int32_t>(previous) = next;
    field<int32_t>(next + 4) = previous;
    capacity = field<uint32_t>(node + 28);
    if (capacity >= 16) {
        std::free(pointer<void>(field<uint32_t>(node + 8)));
    }
    field<uint32_t>(node + 28) = 15;
    field<uint32_t>(node + 24) = 0;
    field<unsigned char>(node + 8) = 0;
    std::free(pointer<void>(node));
    --field<uint32_t>(list_base + 4);
    field<int32_t>(result) = next;
    return result;
}

int32_t function_44e780(uint32_t count,
                        int32_t *vector,
                        int32_t position,
                        int32_t value)
{
    return retdec_vector_insert32(count, vector, position, value);
}

int32_t function_458090(int32_t a1, int32_t result, int32_t a3, int32_t a4)
{
    uint32_t mask;
    uint32_t bucket;
    uint32_t bucket_count;
    int32_t bucket_array;
    int32_t entry_address;
    int32_t bucket_first;
    int32_t sentinel;
    int32_t current;
    uint32_t key_length;
    const unsigned char *key_data;

    if (a1 == 0 || result == 0 || a3 == 0 || a4 == 0) {
        return result;
    }

    mask = field<uint32_t>(a1 + 32);
    bucket = retdec_string_hash32(a3) & mask;
    bucket_count = field<uint32_t>(a1 + 36);
    if (bucket_count <= bucket) {
        bucket = bucket - 1u - (mask >> 1);
    }
    bucket_array = field<int32_t>(a1 + 16);
    if (bucket_array == 0) {
        return result;
    }
    entry_address = bucket_array + (int32_t)(bucket * 8u);
    bucket_first = field<int32_t>(entry_address);
    sentinel = field<int32_t>(a1 + 4);
    current = bucket_first;
    if (bucket_first != sentinel) {
        int32_t link = field<int32_t>(entry_address + 4);
        current = link != 0
            ? field<int32_t>(link) : sentinel;
    }

    key_length = retdec_string_length32(a3);
    key_data = retdec_string_data32(a3);
    if (current != bucket_first) {
        for (;;) {
            uint32_t node_length;
            uint32_t common_length;
            const unsigned char *node_data;
            int32_t compare;

            current = field<int32_t>(current + 4);
            if (current == 0) {
                current = sentinel;
            }
            node_length = field<uint32_t>(current + 24);
            node_data = retdec_string_data32(current + 8);
            common_length = key_length < node_length
                ? key_length : node_length;
            compare = retdec_compare_bytes32(key_data, node_data,
                                             common_length);
            if (compare != 0) {
                if (compare >= 0) {
                    break;
                }
                if (current == bucket_first) {
                    goto insert_new_node;
                }
                continue;
            }
            if (key_length >= node_length) {
                if (key_length != node_length) {
                    break;
                }
                break;
            }
            if (current == bucket_first) {
                goto insert_new_node;
            }
        }

        if (retdec_compare_strings32(current + 8, a3) < 0) {
            field<int32_t>(result) = sentinel;
            return result;
        }
        field<int32_t>(result) = current;
        return result;
    }

insert_new_node:
    {
        int32_t predecessor = field<int32_t>(a4);
        int32_t *entry = pointer<int32_t>(entry_address);
        int32_t bucket_node;
        int32_t replacement;

        if (current != predecessor) {
            function_458330(current, a4, predecessor);
        }
        bucket_node = entry[0];
        if (bucket_node == sentinel) {
            entry[0] = a4;
            entry[1] = a4;
        } else {
            replacement = a4;
            if (bucket_node != current) {
                int32_t next_link = entry[1] != 0
                    ? field<int32_t>(entry[1]) : sentinel;
                entry[1] = next_link;
                if (next_link != replacement) {
                    entry = &entry[1];
                    replacement = field<int32_t>(entry[0] + 4);
                }
            }
            *entry = replacement;
        }
        function_458200(a1);
        field<int32_t>(result) = a4;
        field<unsigned char>(result + 4) = 1;
        return result;
    }
}

int32_t function_458200(int32_t a1)
{
    int32_t element_count;
    uint32_t bucket_count;
    uint32_t new_bucket_count;
    double load;
    double max_load;
    int32_t value;

    if (a1 == 0) {
        return 0;
    }
    element_count = field<int32_t>(a1 + 8);
    bucket_count = field<uint32_t>(a1 + 36);
    load = (double)(uint32_t)element_count;
    max_load = (double)field<float>(a1 + 40);
    if (bucket_count == 0 || !(load / (double)bucket_count > max_load)) {
        return 0;
    }

    new_bucket_count = bucket_count;
    {
        int32_t index;
        for (index = 0; index < 3 && new_bucket_count < 0x1FFFFFFF;
             ++index) {
            new_bucket_count *= 2;
        }
    }
    if (field<int32_t>(a1 + 20) !=
        field<int32_t>(a1 + 16)) {
        field<int32_t>(a1 + 20) =
            field<int32_t>(a1 + 16);
    }
    value = field<int32_t>(a1 + 4);
    function_44e780(new_bucket_count * 2u,
                    pointer<int32_t>(a1 + 16),
                    field<int32_t>(a1 + 16),
                    address(&value));
    field<uint32_t>(a1 + 32) = new_bucket_count - 1;
    field<uint32_t>(a1 + 36) = new_bucket_count;
    function_4583a0(a1, field<int32_t>(a1 + 4));
    return 0;
}

int32_t function_4583a0(int32_t this_ptr, int32_t result)
{
    int32_t sentinel;
    int32_t stop;
    int32_t pair[2] = { 0, 0 };
    int32_t current;
    int32_t return_value = result;

    if (this_ptr == 0 || result == 0) {
        return result;
    }
    sentinel = field<int32_t>(this_ptr + 4);
    if (sentinel == 0) {
        return result;
    }
    current = field<int32_t>(sentinel);
    if (current == result) {
        return result;
    }
    stop = field<int32_t>(result + 4);
    do {
        bool done = current == stop;
        if (current == 0 || current == sentinel) {
            break;
        }
        return_value = function_458090(
            this_ptr, address(pair), current + 8, current);
        if (done) {
            break;
        }
        current = field<int32_t>(sentinel);
    } while (current != 0);
    return return_value;
}
