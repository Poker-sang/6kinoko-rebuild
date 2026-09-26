#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/savedata.h"
#include "kinoko/base_utilities.h"
#include "kinoko/squirrel_game_objects.h"
#include "kinoko/squirrel_host_compat.h"
#include <squirrel.h>
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

extern "C" struct SQVM *kinoko_primary_vm;
// Savedata's SqPlus references require the live primary VM at each recursive
// container boundary, distinct from the compiled LocalScript VM.
namespace { HSQUIRRELVM serialization_vm() { return reinterpret_cast<HSQUIRRELVM>(kinoko_primary_vm); } }

extern "C" void kinoko_trace(const char *);
extern "C" void kinoko_trace_i32(const char *, int32_t);

namespace kinoko::savedata {
namespace {
constexpr uint32_t kMaximumString = 0x1000000u;
constexpr uint32_t kFileBufferSize = 0x20000u;

// Recovered stream prefix: borrowed buffer followed by cursor and limit.
// memcpy permits the original unaligned callers without integer pointers.
struct StreamPrefix { unsigned char* buffer; int32_t position; int32_t limit; };
static_assert(sizeof(StreamPrefix) == 12);
struct TableStream {
    unsigned char *buffer;
    int32_t position;
    int32_t limit;

    explicit TableStream(int32_t *legacy) noexcept {
        StreamPrefix prefix{};
        if (legacy) std::memcpy(&prefix, legacy, sizeof prefix);
        buffer = prefix.buffer; position = prefix.position; limit = prefix.limit;
    }
    TableStream(unsigned char *data, int32_t size) noexcept
        : buffer(data), position(0), limit(size) {}
    void publish(int32_t *legacy) const noexcept {
        if (legacy) legacy[1] = position;
    }
    bool transfer(void *bytes, uint32_t size, bool write) noexcept {
        if (!buffer || (!bytes && size) || position < 0 || limit < position ||
            size > static_cast<uint32_t>(limit - position)) return false;
        if (size) {
            if (write) std::memcpy(buffer + position, bytes, size);
            else std::memcpy(bytes, buffer + position, size);
            position += static_cast<int32_t>(size);
        }
        return true;
    }
    template<class T> bool read(T &value) noexcept {
        return transfer(&value, sizeof(value), false);
    }
    template<class T> bool write(const T &value) noexcept {
        return transfer(const_cast<T *>(&value), sizeof(value), true);
    }
    bool read_bytes(void *bytes, uint32_t size) noexcept {
        return transfer(bytes, size, false);
    }
    bool write_bytes(const void *bytes, uint32_t size) noexcept {
        return transfer(const_cast<void *>(bytes), size, true);
    }
};

// SqPlus stores a vtable and an externally retained HSQOBJECT in three words.
// The helpers below own and release that reference; nested copies hand their
// new reference to the recursive call, as the original 4722E0/472820 do.
struct Object {
    kinoko::script::ObjectStorage storage{};
    Object() = default;
    Object(const void* vtable, int32_t type, int32_t data) : storage{vtable, kinoko::script::borrowed_value(type, data)} {}
    int32_t *raw() noexcept { return reinterpret_cast<int32_t*>(&storage); }
    int32_t type() const noexcept { return static_cast<int32_t>(storage.value._type); }
    int32_t data() const noexcept { return kinoko::script::data_bits(storage.value); }
    void initialize() noexcept { kinoko_sqplus_object_initialize(raw()); }
    void destroy() noexcept { kinoko_sqplus_object_destroy(raw()); }
    bool assign(int32_t type_tag, int32_t bits) noexcept {
        return kinoko_squirrel_object_from_pair(raw(), type_tag, bits) != 0;
    }
    bool copy_from(Object &source) noexcept {
        return kinoko_squirrel_object_copy(raw(), source.raw()) != 0;
    }
};
static_assert(sizeof(Object) == 12);

bool read_string(TableStream &stream, Object &object) {
    uint32_t length = 0;
    if (!stream.read(length) || length > kMaximumString) return false;
    auto text = std::unique_ptr<char, decltype(&std::free)>(
        static_cast<char *>(std::malloc(static_cast<size_t>(length) + 1)), &std::free);
    if (!text || !stream.read_bytes(text.get(), length)) return false;
    text.get()[length] = 0;
    return kinoko_squirrel_object_from_string(object.raw(), text.get(), length) != 0;
}

bool write_string(TableStream &stream, Object &object) {
    const char *text = nullptr;
    if (!kinoko_squirrel_object_string(object.raw(), &text)) return false;
    const uint32_t length = static_cast<uint32_t>(std::strlen(text));
    return stream.write(length) && stream.write_bytes(text, length);
}

bool read_table(TableStream &stream, Object parent) {
    Object key, value;
    key.initialize();
    value.initialize();
    bool ok = stream.buffer && serialization_vm();
    while (ok) {
        uint32_t value_type = 0, key_type = 0;
        if (!stream.read(value_type)) { ok = false; break; }
        if ((value_type & 0x7eu) == 0) break; // OT_NULL terminates each container.
        if (!stream.read(key_type)) { ok = false; break; }
        if (key_type == OT_INTEGER) {
            uint32_t raw_key = 0;
            if (!stream.read(raw_key) || !key.assign(key_type, raw_key)) {
                ok = false; break;
            }
        } else if (key_type == OT_STRING) {
            if (!read_string(stream, key)) { ok = false; break; }
        } else { ok = false; break; }

        if (value_type == OT_INTEGER || value_type == OT_FLOAT) {
            uint32_t raw_value = 0;
            ok = stream.read(raw_value) && value.assign(value_type, raw_value);
        } else if (value_type == OT_BOOL) {
            uint8_t boolean = 0;
            ok = stream.read(boolean) && value.assign(value_type, boolean);
        } else if (value_type == OT_STRING) {
            ok = read_string(stream, value);
        } else if (value_type == OT_ARRAY || value_type == OT_TABLE) {
            Object nested;
            uint32_t count = 0;
            if (value_type == OT_ARRAY &&
                (!stream.read(count) || count > 0x100000u)) ok = false;
            if (ok) {
                void *created = value_type == OT_ARRAY
                    ? kinoko_sqplus_object_new_array(value.raw(), static_cast<int32_t>(count))
                    : kinoko_sqplus_object_new_table(value.raw());
                ok = created && kinoko_sqplus_object_raw_set_object(
                    parent.raw(), key.raw(), value.raw()) && nested.copy_from(value);
                if (ok) ok = read_table(stream, nested);
            }
        } else { ok = false; }

        if (ok && value_type != OT_ARRAY && value_type != OT_TABLE)
            ok = kinoko_sqplus_object_raw_set_object(
                parent.raw(), key.raw(), value.raw()) != 0;
        value.destroy();
        key.destroy();
    }
    value.destroy();
    key.destroy();
    parent.destroy();
    return ok;
}

bool write_table(TableStream &stream, Object input) {
    Object container, key, value;
    container.initialize();
    key.initialize();
    value.initialize();
    bool ok = stream.buffer && serialization_vm();
    bool iterating = false;
    if (ok) ok = container.assign(input.type(), input.data());
    if (ok && kinoko_sqplus_object_begin_iteration(container.raw())) iterating = true;

    while (ok && iterating && kinoko_sqplus_object_next(key.raw(), value.raw())) {
        const uint32_t value_type = static_cast<uint32_t>(value.type());
        const uint32_t key_type = static_cast<uint32_t>(key.type());
        // 4728E9 skips null and other non-serializable values before the tag.
        if ((value_type & 0x7eu) == 0) continue;
        if (!stream.write(value_type)) { ok = false; break; }
        if (key_type == OT_INTEGER) {
            ok = stream.write(key_type) && stream.write(key.data());
        } else if (key_type == OT_STRING) {
            ok = stream.write(key_type) && write_string(stream, key);
        } else { ok = false; }
        if (!ok) break;

        if (value_type == OT_INTEGER || value_type == OT_FLOAT) {
            ok = stream.write(value.data());
        } else if (value_type == OT_BOOL) {
            const uint8_t boolean = static_cast<uint8_t>(value.data());
            ok = stream.write(boolean);
        } else if (value_type == OT_STRING) {
            ok = write_string(stream, value);
        } else if (value_type == OT_ARRAY || value_type == OT_TABLE) {
            Object nested;
            if (value_type == OT_ARRAY) {
                const int32_t count = kinoko_sqplus_object_size(value.raw());
                ok = count >= 0 && stream.write(count);
            }
            if (ok) ok = nested.copy_from(value) && write_table(stream, nested);
        }
        value.destroy();
        key.destroy();
    }
    if (iterating) {
        if (ok) ok = stream.write(static_cast<uint32_t>(OT_NULL));
        kinoko_sqplus_object_end_iteration();
    }
    value.destroy();
    key.destroy();
    container.destroy();
    input.destroy();
    return ok;
}

struct File {
    HANDLE handle;
    explicit File(const char *path, DWORD access, DWORD share, DWORD disposition)
        : handle(CreateFileA(path, access, share, nullptr, disposition,
                             FILE_ATTRIBUTE_NORMAL, nullptr)) {}
    ~File() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
    explicit operator bool() const noexcept { return handle != INVALID_HANDLE_VALUE; }
};
using Bytes = std::unique_ptr<unsigned char, decltype(&std::free)>;
Bytes allocate_buffer() {
    return Bytes(static_cast<unsigned char *>(std::malloc(kFileBufferSize)), &std::free);
}

int32_t load_file(const char *path, Object input) {
    kinoko_trace("savedata:load-begin");
    kinoko_trace(path);
    int32_t result = 0;
    {
        File file(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
        if (file) {
            auto encoded = allocate_buffer();
            auto decoded = allocate_buffer();
            DWORD encoded_size = 0, bytes_read = 0;
            if (encoded && decoded &&
                ReadFile(file.handle, &encoded_size, sizeof(encoded_size), &bytes_read, nullptr) &&
                bytes_read == sizeof(encoded_size) && encoded_size <= kFileBufferSize &&
                ReadFile(file.handle, encoded.get(), encoded_size, &bytes_read, nullptr) &&
                bytes_read == encoded_size) {
                const int32_t decoded_size = kinoko_decompress_buffer(
                    encoded.get(), static_cast<int32_t>(encoded_size),
                    decoded.get(), kFileBufferSize);
                kinoko_trace_i32("savedata:decoded-size", decoded_size);
                if (decoded_size > 0) {
                    TableStream stream(decoded.get(), decoded_size);
                    Object table;
                    if (table.assign(input.type(), input.data())) {
                        result = read_table(stream, table);
                        kinoko_trace_i32("savedata:decoded-consumed", stream.position);
                    }
                }
            }
        }
    }
    input.destroy();
    kinoko_trace_i32("savedata:load-result", result);
    return result;
}

int32_t save_file(const char *path, Object input) {
    kinoko_trace("savedata:save-begin");
    kinoko_trace(path);
    int32_t result = 0;
    {
        File file(path, GENERIC_WRITE, 0, CREATE_ALWAYS);
        if (file) {
            auto raw = allocate_buffer();
            auto encoded = allocate_buffer();
            if (raw && encoded) {
                TableStream stream(raw.get(), kFileBufferSize);
                Object table;
                if (table.assign(input.type(), input.data()) && write_table(stream, table)) {
                    kinoko_trace_i32("savedata:raw-size", stream.position);
                    const DWORD encoded_size = static_cast<DWORD>(kinoko_compress_buffer(
                        raw.get(), stream.position, encoded.get(), kFileBufferSize));
                    kinoko_trace_i32("savedata:encoded-size", static_cast<int32_t>(encoded_size));
                    DWORD bytes_written = 0;
                    if (encoded_size && encoded_size <= kFileBufferSize &&
                        WriteFile(file.handle, &encoded_size, sizeof(encoded_size), &bytes_written, nullptr) &&
                        bytes_written == sizeof(encoded_size) &&
                        WriteFile(file.handle, encoded.get(), encoded_size, &bytes_written, nullptr) &&
                        bytes_written == encoded_size) result = 1;
                }
            }
        }
    }
    input.destroy();
    kinoko_trace_i32("savedata:save-result", result);
    return result;
}
} // namespace
} // namespace kinoko::savedata

// Registration uses the named path entry; by-value object ownership is preserved.
// All recursion, byte transfer and ownership live in named C++ routines above.
int32_t kinoko_savedata_read_table_entry(int32_t *stream, const void* vtable,
                                      int32_t type, int32_t data) {
    kinoko::savedata::TableStream view(stream);
    const auto result = kinoko::savedata::read_table(view, {vtable, type, data});
    view.publish(stream);
    return result;
}
int32_t kinoko_savedata_write_table_entry(int32_t *stream, const void* vtable,
                                      int32_t type, int32_t data) {
    kinoko::savedata::TableStream view(stream);
    const auto result = kinoko::savedata::write_table(view, {vtable, type, data});
    view.publish(stream);
    return result;
}
int32_t kinoko_savedata_load_file_entry(const char* path, const void* vtable, int32_t type, int32_t data) {
    return kinoko::savedata::load_file(path, {vtable, type, data});
}
int32_t kinoko_savedata_save_file_entry(const char* path, const void* vtable, int32_t type, int32_t data) {
    return kinoko::savedata::save_file(path, {vtable, type, data});
}
