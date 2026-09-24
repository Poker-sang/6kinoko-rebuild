#include "kinoko/file_io.h"
#include "kinoko/compat/resource_rules.hpp"
#include "kinoko/archive_random.h"
#include <windows.h>
#include <zlib.h>
#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <utility>
extern "C" { int32_t kinoko_archive_count = 0; }
namespace {
struct Entry {
    std::string path;
    uint32_t archive,offset,size;
};
// Original 410750 stores unsigned CRC keys and an insertion-ordered collision
// chain. Path spelling stays unchanged; only the hash copy is lowercased.
std::map<uint32_t,std::list<Entry>> entries;
std::vector<std::string> archives;
class File {
    HANDLE value_;
public:
    explicit File(const char *path):value_(CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,
        nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr)) {}
    ~File() { if(valid()) CloseHandle(value_); }
    File(const File&)=delete;
    File& operator=(const File&)=delete;
    bool valid() const { return value_!=INVALID_HANDLE_VALUE; }
    HANDLE get() const { return value_; }
    void close() { if(valid()) CloseHandle(detach()); }
    HANDLE detach() { return std::exchange(value_,INVALID_HANDLE_VALUE); }
    bool read(void *data,DWORD size) {
        DWORD read=0;
        return ReadFile(value_,data,size,&read,nullptr) && read==size;
    }
};
uint32_t path_hash(const std::string &path) {
    std::vector<char> lowered(path.begin(),path.end());lowered.push_back('\0');
    CharLowerBuffA(lowered.data(),static_cast<DWORD>(lowered.size()));
    // 4107B0 passes strlen+1 in EAX; the terminating NUL is part of CRC32.
    return static_cast<uint32_t>(crc32(0,reinterpret_cast<const Bytef*>(lowered.data()),
                                     static_cast<uInt>(lowered.size())));
}
void insert(const char *path,uint32_t archive,uint32_t offset,uint32_t size) {
    auto &chain=entries[path_hash(path)];
    for(auto &entry:chain) {
        if(_stricmp(entry.path.c_str(),path)==0) {
            entry.archive=archive;entry.offset=offset;entry.size=size;return;
        }
    }
    chain.push_back(Entry{path,archive,offset,size});
}
}
extern "C" int32_t kinoko_archive_insert(const char *path,uint32_t archive,uint32_t offset,uint32_t size) {
    if(!path) return 0;
    insert(path,static_cast<uint32_t>(archive),static_cast<uint32_t>(offset),static_cast<uint32_t>(size));
    return 1;
}
extern "C" int32_t kinoko_archive_mount(const char *path) {
    if(!path) return 0;
    File file(path);if(!file.valid()) return 0;
    uint16_t count=0;uint32_t size=0;
    // Retain the rebuilt loader's malformed-file checks. Normal DAT loading
    // has no artificial 64-archive or 8192-entry container limit anymore.
    if(!file.read(&count,sizeof(count)) || !file.read(&size,sizeof(size)) || size>256u*1024u*1024u) return 0;
    std::vector<uint8_t> bytes(size);
    if(size && !file.read(bytes.data(),size)) return 0;
    file.close();
    kinoko_decode_archive_index(bytes.data(),size);
    const auto archive=static_cast<uint32_t>(archives.size());
    archives.emplace_back(path);kinoko_archive_count=static_cast<int32_t>(archives.size());
    uint32_t cursor=0;
    for(uint32_t i=0;i<count;++i) {
        if(size-cursor<9u) return 0;
        const auto offset=kinoko::compat::read_le32(bytes.data()+cursor);
        const auto length=kinoko::compat::read_le32(bytes.data()+cursor+4);
        const auto path_size=uint32_t(bytes[cursor+8]);cursor+=9;
        if(size-cursor<path_size) return 0;
        std::string name(reinterpret_cast<const char*>(bytes.data()+cursor),path_size);
        cursor+=path_size;
        insert(name.c_str(),archive,offset,length);
    }
    return 1;
}
extern "C" HANDLE kinoko_archive_open_entry(const char *path,uint32_t *offset,uint32_t *size) {
    if(!path || !offset || !size) return 0;
    *offset=*size=0;
    const auto normalized=kinoko::compat::runtime_archive_lookup_path(path);
    const auto found=entries.find(path_hash(normalized));
    if(found==entries.end()) return 0;
    const auto &chain=found->second;
    for(const auto &entry:chain) {
        // Original bypasses the name comparison if there is no collision
        // successor. Keep that subtle single-entry CRC behavior.
        if(chain.size()!=1 && _stricmp(entry.path.c_str(),normalized.c_str())!=0) continue;
        if(entry.archive>=archives.size()) return 0;
        *offset=entry.offset;*size=entry.size;
        File file(archives[entry.archive].c_str());if(!file.valid()) return 0;
        SetFilePointer(file.get(),static_cast<LONG>(entry.offset),nullptr,FILE_BEGIN);
        return file.detach();
    }
    return 0;
}

extern "C" void kinoko_archive_initialize(void) {
    entries.clear(); archives.clear(); kinoko_archive_count = 0;
}
