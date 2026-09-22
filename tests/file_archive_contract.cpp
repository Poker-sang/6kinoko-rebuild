#include "kinoko/file_io_layout.h"
#include "kinoko/archive_random.h"
#include "kinoko/legacy_string.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"file/archive line %d: %s\n",__LINE__,#x); return 1; } } while(0)
namespace {
struct TempFile {
    char path[MAX_PATH]{};
    TempFile() { char dir[MAX_PATH]{}; GetTempPathA(MAX_PATH,dir); GetTempFileNameA(dir,"kio",0,path); }
    ~TempFile() { DeleteFileA(path); }
};
void word(std::vector<uint8_t>& bytes,uint32_t value) {
    for (int i=0;i<4;++i) bytes.push_back(static_cast<uint8_t>(value>>(8*i)));
}
}
int main() {
    kinoko_archive_initialize();
    TempFile ordinary, dat;
    KinokoArchiveReader *writer=nullptr, *plain=nullptr, *reader=nullptr;
    CHECK(kinoko_writer_open(&writer,ordinary.path));
    CHECK(kinoko_writer_write(writer,"ABCDE",5));
    CHECK(writer->methods->transferred(writer)==5);
    CHECK(kinoko_reader_seek(writer,2,FILE_BEGIN)==2);
    CHECK(kinoko_writer_write(writer,"x",1));
    kinoko_reader_close(writer); writer=nullptr;
    CHECK(kinoko_reader_open(&plain,ordinary.path));
    CHECK(kinoko_reader_size(plain)==5);
    char bytes[8]{};
    CHECK(kinoko_reader_read(plain,bytes,8) && plain->transferred==5);
    CHECK(std::memcmp(bytes,"ABxDE",5)==0);
    // File ReadFile returns success with zero bytes at EOF; package Read does not.
    CHECK(kinoko_reader_read(plain,bytes,1) && plain->transferred==0);
    CHECK(kinoko_reader_seek(plain,0,FILE_BEGIN)==0);
    // Exercise the recovered string overload via the actual thiscall table.
    KinokoArchiveReader string_reader{&kinoko_file_reader_methods,nullptr,0};
    kinoko::legacy::StringRecord path_record{};
    char *name=ordinary.path; std::memcpy(path_record.characters,&name,sizeof(name));
    path_record.capacity=MAX_PATH; path_record.length=static_cast<uint32_t>(std::strlen(name));
    CHECK(string_reader.methods->open_string(&string_reader,&path_record));
    CHECK(string_reader.methods->size(&string_reader)==5);
    string_reader.methods->destroy(&string_reader,0);
    CHECK(string_reader.handle==nullptr);

    const std::string entry="data/fixture.bin";
    constexpr uint32_t offset=96, length=5;
    std::vector<uint8_t> index; word(index,offset); word(index,length);
    index.push_back(static_cast<uint8_t>(entry.size()));
    index.insert(index.end(),entry.begin(),entry.end());
    kinoko_decode_archive_index(index.data(),static_cast<uint32_t>(index.size()));
    CHECK(kinoko_writer_open(&writer,dat.path));
    uint16_t count=1; uint32_t index_size=static_cast<uint32_t>(index.size());
    CHECK(kinoko_writer_write(writer,&count,2));
    CHECK(kinoko_writer_write(writer,&index_size,4));
    CHECK(kinoko_writer_write(writer,index.data(),index_size));
    CHECK(kinoko_reader_seek(writer,offset,FILE_BEGIN)==offset);
    std::array<uint8_t,length> payload{'H','E','L','L','O'};
    const auto key=static_cast<uint8_t>((offset>>1)|0x23);
    for(auto& c:payload) c^=key;
    CHECK(kinoko_writer_write(writer,payload.data(),length));
    kinoko_reader_close(writer); writer=nullptr;
    CHECK(kinoko_archive_mount(dat.path) && kinoko_archive_count==1);
    // An existing ordinary stream stays ordinary after a DAT is mounted.
    CHECK(kinoko_reader_read_exact(plain,bytes,5) && std::memcmp(bytes,"ABxDE",5)==0);
    kinoko_reader_close(plain); plain=nullptr;
    CHECK(kinoko_reader_open(&reader,"./DATA\\fixture.bin"));
    CHECK(kinoko_reader_size(reader)==length);
    auto* package=reinterpret_cast<KinokoPackageReader*>(reader);
    CHECK(package->entry_offset==offset && package->read_position==offset && package->xor_key==key);
    kinoko_archive_count=0;
    CHECK(kinoko_reader_read_exact(reader,bytes,2) && std::memcmp(bytes,"HE",2)==0);
    CHECK(!kinoko_reader_read_exact(reader,bytes,4) && package->read_position==offset+2);
    CHECK(kinoko_reader_seek_relative(reader,1));
    CHECK(kinoko_reader_read_exact(reader,bytes,2) && std::memcmp(bytes,"LO",2)==0);
    CHECK(!kinoko_reader_seek_relative(reader,1));
    // Explicitly pin the original virtual seek quirk; guarded loader skips above
    // remain absolute. FILE_END subtracts distance instead of adding it.
    CHECK(kinoko_reader_seek(reader,1,FILE_END)==4 && package->read_position==4);
    CHECK(SetFilePointer(reader->handle,0,nullptr,FILE_CURRENT)==offset+4);
    CHECK(kinoko_reader_seek(reader,2,FILE_BEGIN)==2);
    CHECK(SetFilePointer(reader->handle,0,nullptr,FILE_CURRENT)==offset+2);
    CHECK(kinoko_reader_seek(reader,1,FILE_CURRENT)==3);
    CHECK(package->read_position==3);
    kinoko_archive_count=1;
    CHECK(kinoko_reader_open(&reader,"data/fixture.bin"));
    CHECK(kinoko_reader_read(reader,bytes,8) && reader->transferred==5);
    CHECK(std::memcmp(bytes,"HELLO",5)==0 && package!=nullptr);
    CHECK(!kinoko_reader_read(reader,bytes,1));
    CHECK(!kinoko_reader_open(&reader,"data/missing.bin") && reader==nullptr);
    kinoko_archive_initialize();
    return 0;
}
