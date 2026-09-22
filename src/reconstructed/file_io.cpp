#include "kinoko/file_io_layout.h"
#include "kinoko/legacy_string.hpp"
#include <cstddef>
#include <cstdlib>
#include <cstring>
namespace {
static_assert(sizeof(KinokoArchiveReader) == 12);
static_assert(sizeof(KinokoPackageReader) == 28);
static_assert(offsetof(KinokoPackageReader, entry_size) == 12);
static_assert(offsetof(KinokoPackageReader, entry_offset) == 16);
static_assert(offsetof(KinokoPackageReader, read_position) == 20);
static_assert(offsetof(KinokoPackageReader, xor_key) == 24);
bool valid(const KinokoArchiveReader* reader) { return reader && reader->handle && reader->handle != INVALID_HANDLE_VALUE; }
bool package(const KinokoArchiveReader* reader) { return reader->methods == &kinoko_package_reader_methods; }
KinokoPackageReader& packaged(KinokoArchiveReader* reader) { return *reinterpret_cast<KinokoPackageReader*>(reader); }
KinokoArchiveReader* __fastcall destroy(KinokoArchiveReader* reader, void*, uint8_t flags) {
    if (!reader) return nullptr;
    if (reader->methods != &kinoko_file_writer_methods) reader->methods = &kinoko_file_reader_methods;
    if (valid(reader)) CloseHandle(reader->handle);
    reader->handle = nullptr;
    if (flags & 1) std::free(reader);
    return reader;
}
int32_t open_file(KinokoArchiveReader* reader, const char* path, bool write) {
    // 407270 uses FILE_READ_DATA (1), not GENERIC_READ. Writer creates/truncates.
    reader->handle = CreateFileA(path, write ? GENERIC_WRITE : FILE_READ_DATA,
        write ? 0 : FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        write ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (reader->handle == INVALID_HANDLE_VALUE) reader->handle = nullptr;
    return reader->handle != nullptr;
}
int32_t __fastcall open_read(KinokoArchiveReader* reader, void*, const char* path) { return open_file(reader,path,false); }
int32_t __fastcall open_write(KinokoArchiveReader* reader, void*, const char* path) { return open_file(reader,path,true); }
int32_t __fastcall open_string(KinokoArchiveReader* reader, void*, const void* string) {
    // 4072B0 is the std::string overload, not the byte-count accessor.
    return reader->methods->open_path(reader, kinoko::legacy::StringView(const_cast<void*>(string)).data());
}
uint32_t __fastcall transferred(KinokoArchiveReader* reader, void*) { return reader->transferred; }
int32_t __fastcall read_file(KinokoArchiveReader* reader, void*, void* data, uint32_t size) {
    return ReadFile(reader->handle, data, size, &reader->transferred, nullptr);
}
int32_t __fastcall write_file(KinokoArchiveReader* reader, void*, void* data, uint32_t size) {
    return WriteFile(reader->handle, data, size, &reader->transferred, nullptr);
}
uint32_t __fastcall seek_file(KinokoArchiveReader* reader, void*, int32_t distance, uint32_t origin) {
    return SetFilePointer(reader->handle, distance, nullptr, origin);
}
uint32_t __fastcall size_file(KinokoArchiveReader* reader, void*) { return GetFileSize(reader->handle, nullptr); }
uint32_t __fastcall size_package(KinokoArchiveReader* reader, void*) { return packaged(reader).entry_size; }
int32_t __fastcall read_package(KinokoArchiveReader* reader, void*, void* data, uint32_t size) {
    auto& entry = packaged(reader);
    const uint32_t end = entry.entry_offset + entry.entry_size;
    // 410B90 uses DWORD arithmetic, ignores ReadFile's BOOL, and decodes the
    // clamped request (not transferred count). Keep this original virtual ABI.
    if (end < size + entry.read_position) size = end - entry.read_position;
    ReadFile(reader->handle, data, size, &reader->transferred, nullptr);
    if (!reader->transferred) return 0;
    entry.read_position += reader->transferred;
    for (uint32_t i=0; i<size; ++i) static_cast<uint8_t*>(data)[i] ^= entry.xor_key;
    return 1;
}
uint32_t __fastcall seek_package(KinokoArchiveReader* reader, void*, int32_t distance, uint32_t origin) {
    auto& entry = packaged(reader);
    DWORD position;
    if (origin == FILE_BEGIN) position = SetFilePointer(reader->handle, static_cast<LONG>(entry.entry_offset + uint32_t(distance)), nullptr, FILE_BEGIN);
    else if (origin == FILE_CURRENT) position = SetFilePointer(reader->handle, distance, nullptr, FILE_CURRENT);
    else if (origin == FILE_END) position = SetFilePointer(reader->handle, static_cast<LONG>(entry.entry_offset + entry.entry_size - uint32_t(distance)), nullptr, FILE_BEGIN);
    else return 0;
    // ORIGINAL QUIRK (410C00): Seek stores entry-relative position although
    // Open/Read use absolute positions. Do not silently change virtual behavior.
    return entry.read_position = position - entry.entry_offset;
}
bool seek_absolute(HANDLE file, uint32_t position) {
    SetLastError(NO_ERROR);
    return SetFilePointer(file, static_cast<LONG>(position), nullptr, FILE_BEGIN) != INVALID_SET_FILE_POINTER || GetLastError() == NO_ERROR;
}
}
#define METHOD(name, fn) reinterpret_cast<decltype(KinokoReaderMethods::name)>(fn)
extern "C" const KinokoReaderMethods kinoko_file_reader_methods{
    METHOD(destroy,destroy), METHOD(open_string,open_string), METHOD(open_path,open_read),
    METHOD(transfer,read_file), METHOD(transferred,transferred), METHOD(seek,seek_file), METHOD(size,size_file)};
extern "C" const KinokoReaderMethods kinoko_package_reader_methods{
    METHOD(destroy,destroy), METHOD(open_string,open_string), METHOD(open_path,open_read),
    METHOD(transfer,read_package), METHOD(transferred,transferred), METHOD(seek,seek_package), METHOD(size,size_package)};
extern "C" const KinokoReaderMethods kinoko_file_writer_methods{
    METHOD(destroy,destroy), METHOD(open_string,open_string), METHOD(open_path,open_write),
    METHOD(transfer,write_file), METHOD(transferred,transferred), METHOD(seek,seek_file), nullptr};
#undef METHOD
extern "C" void kinoko_reader_close(KinokoArchiveReader* reader) {
    if (reader) reader->methods->destroy(reader,1);
}
extern "C" int32_t kinoko_reader_open(KinokoArchiveReader** slot, const char* path) {
    if (!slot || !path) return 0;
    kinoko_reader_close(*slot); *slot = nullptr;
    if (kinoko_archive_count) {
        auto* reader = static_cast<KinokoPackageReader*>(std::calloc(1,sizeof(KinokoPackageReader)));
        if (!reader) return 0;
        reader->base.methods = &kinoko_package_reader_methods;
        reader->base.handle = kinoko_archive_open_entry(path,&reader->entry_offset,&reader->entry_size);
        reader->read_position = reader->entry_offset;
        reader->xor_key = static_cast<uint8_t>((reader->entry_offset >> 1) | 0x23u);
        *slot = &reader->base;
    } else {
        *slot = static_cast<KinokoArchiveReader*>(std::calloc(1,sizeof(KinokoArchiveReader)));
        if (!*slot) return 0;
        (*slot)->methods = &kinoko_file_reader_methods;
        open_file(*slot,path,false);
    }
    if (valid(*slot)) return 1;
    kinoko_reader_close(*slot); *slot = nullptr; return 0;
}
extern "C" int32_t kinoko_writer_open(KinokoArchiveReader** slot, const char* path) {
    if (!slot || !path) return 0;
    kinoko_reader_close(*slot);
    *slot = static_cast<KinokoArchiveReader*>(std::calloc(1,sizeof(KinokoArchiveReader)));
    if (!*slot) return 0;
    (*slot)->methods = &kinoko_file_writer_methods;
    if (open_file(*slot,path,true)) return 1;
    kinoko_reader_close(*slot); *slot=nullptr; return 0;
}
extern "C" uint32_t kinoko_reader_size(KinokoArchiveReader* reader) { return valid(reader) && reader->methods->size ? reader->methods->size(reader) : 0; }
extern "C" int32_t kinoko_reader_read(KinokoArchiveReader* reader, void* data, uint32_t size) { return valid(reader) ? reader->methods->transfer(reader,data,size) : 0; }
extern "C" int32_t kinoko_writer_write(KinokoArchiveReader* writer, const void* data, uint32_t size) { return valid(writer) ? writer->methods->transfer(writer,const_cast<void*>(data),size) : 0; }
extern "C" uint32_t kinoko_reader_seek(KinokoArchiveReader* reader, int32_t distance, uint32_t origin) { return valid(reader) ? reader->methods->seek(reader,distance,origin) : 0; }
extern "C" int32_t kinoko_reader_read_exact(KinokoArchiveReader* reader, void* data, uint32_t size) {
    if (!valid(reader) || !data || !size) return 0;
    DWORD count=0;
    if (package(reader)) {
        auto& entry=packaged(reader);
        if (entry.entry_size > UINT32_MAX-entry.entry_offset) return 0;
        const auto end=entry.entry_offset+entry.entry_size;
        if (entry.read_position < entry.entry_offset || entry.read_position > end || size > end-entry.read_position) return 0;
        if (!seek_absolute(reader->handle,entry.read_position)) return 0;
        if (!ReadFile(reader->handle,data,size,&count,nullptr) || count != size) return 0;
        for (uint32_t i=0;i<size;++i) static_cast<uint8_t*>(data)[i]^=entry.xor_key;
        entry.read_position+=count;
    } else if (!ReadFile(reader->handle,data,size,&count,nullptr) || count!=size) return 0;
    reader->transferred=count; return 1;
}
extern "C" int32_t kinoko_reader_seek_relative(KinokoArchiveReader* reader, uint32_t distance) {
    if (!valid(reader)) return 0;
    if (package(reader)) {
        auto& entry=packaged(reader);
        if (entry.entry_size > UINT32_MAX-entry.entry_offset) return 0;
        const auto end=entry.entry_offset+entry.entry_size;
        if (entry.read_position < entry.entry_offset || entry.read_position > end || distance > end-entry.read_position) return 0;
        const auto target=entry.read_position+distance;
        if (!seek_absolute(reader->handle,target)) return 0;
        entry.read_position=target;
    } else {
        SetLastError(NO_ERROR);
        if (SetFilePointer(reader->handle,static_cast<LONG>(distance),nullptr,FILE_CURRENT)==INVALID_SET_FILE_POINTER && GetLastError()!=NO_ERROR) return 0;
    }
    return 1;
}
