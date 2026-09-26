#include "kinoko/savedata.h"
#include "kinoko/squirrel_host_object.hpp"
#include "kinoko/squirrel_source_runtime.h"
#include <windows.h>
#include <zlib.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

// Real source VM, SqPlus references, serializer, Windows file I/O and codec.
// Only unrelated game host/diagnostic ports are supplied by this fixture.
extern "C" {
char* g644 = nullptr;
int32_t kinoko_squirrel_object_vtable(void) { return 0x12345678; }
int32_t kinoko_native_void_type(void) { return 0x13572468; }
void retdec_trace(const char*) {}
void retdec_trace_i32(const char*, int32_t) {}
void retdec_trace_squirrel_name(const char*, int32_t) {}
void _3f__3f_3_40_YAXPAX_40_Z(int32_t* p) { std::free(p); }
}
namespace {
using Bytes = std::vector<unsigned char>;
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
class Machine {
public:
    HSQUIRRELVM vm = sq_open(128);
    Machine() { require(vm != nullptr, "open VM"); g644 = reinterpret_cast<char*>(vm); }
    ~Machine() { sq_close(vm); g644 = nullptr; }
};
// Never touch the original game's marisa[A-C].dat. Each run gets its own directory.
class Files {
    std::string directory;
    std::vector<std::string> paths;
public:
    Files() {
        char temp[MAX_PATH], unique[MAX_PATH];
        require(GetTempPathA(MAX_PATH,temp) > 0, "temp path");
        require(GetTempFileNameA(temp,"ksv",0,unique) != 0, "unique temp name");
        require(DeleteFileA(unique) && CreateDirectoryA(unique,nullptr), "temp directory");
        directory=unique;
    }
    std::string path(const char* name) {
        paths.push_back(directory+"\\"+name); return paths.back();
    }
    ~Files() { for(const auto& p:paths) DeleteFileA(p.c_str()); RemoveDirectoryA(directory.c_str()); }
};
void evaluate(HSQUIRRELVM vm, const char* source) {
    const auto top=sq_gettop(vm);
    require(SQ_SUCCEEDED(sq_compilebuffer(vm,source,static_cast<SQInteger>(std::strlen(source)),
        "savedata-contract",SQTrue)),"compile fixture");
    sq_pushroottable(vm);
    if (SQ_FAILED(sq_call(vm,1,SQFalse,SQTrue))) {
        sq_settop(vm,top); throw std::runtime_error("fixture assertion/call failed");
    }
    sq_settop(vm,top);
}
bool file_call(HSQUIRRELVM vm, const std::string& path, const char* table, bool save) {
    const auto top=sq_gettop(vm);
    sq_pushroottable(vm); sq_pushstring(vm,table,-1);
    require(SQ_SUCCEEDED(sq_get(vm,-2)),"find fixture table");
    HSQOBJECT value; require(SQ_SUCCEEDED(sq_getstackobj(vm,-1,&value)),"get table object");
    // The original by-value SqPlus argument transfers this external reference.
    sq_addref(vm,&value); sq_settop(vm,top);
    const auto result=save
        ? kinoko_savedata_save_file_entry(path.c_str(),kinoko_squirrel_object_vtable(),
            value._type,kinoko::script::data_bits(value))
        : kinoko_savedata_load_file_entry(path.c_str(),kinoko_squirrel_object_vtable(),
            value._type,kinoko::script::data_bits(value));
    require(sq_gettop(vm)==top,"file call stack balance");
    return result!=0;
}
void word(Bytes& b,uint32_t v) { for(unsigned i=0;i<4;++i) b.push_back(static_cast<unsigned char>(v>>(i*8))); }
void text(Bytes& b,const char* s) { word(b,static_cast<uint32_t>(std::strlen(s))); b.insert(b.end(),s,s+std::strlen(s)); }
void field(Bytes& b,uint32_t tag,const char* key) { word(b,tag); word(b,OT_STRING); text(b,key); }
void write_bytes(const std::string& path,const Bytes& b) {
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    out.write(reinterpret_cast<const char*>(b.data()),b.size()); require(out.good(),"fixture file write");
}
Bytes read_bytes(const std::string& path) {
    std::ifstream in(path,std::ios::binary); require(in.good(),"fixture file read");
    return Bytes(std::istreambuf_iterator<char>(in),{});
}
void encoded_fixture(const std::string& path,const Bytes& raw) {
    uLongf size=compressBound(static_cast<uLong>(raw.size())); Bytes encoded(size);
    // Independent zlib entry, not the production compression wrapper/serializer.
    require(compress2(encoded.data(),&size,raw.data(),static_cast<uLong>(raw.size()),Z_DEFAULT_COMPRESSION)==Z_OK,"fixture compression");
    Bytes file; word(file,static_cast<uint32_t>(size)); file.insert(file.end(),encoded.begin(),encoded.begin()+size);
    write_bytes(path,file);
}
void check_saved_wire(const std::string& path,const Bytes& expected) {
    const auto file=read_bytes(path); require(file.size()>=4,"saved length prefix");
    uint32_t length=0; for(unsigned i=0;i<4;++i) length|=uint32_t(file[i])<<(i*8);
    require(length==file.size()-4,"exact encoded file size");
    Bytes plain(0x20000); uLongf size=static_cast<uLongf>(plain.size());
    require(uncompress(plain.data(),&size,file.data()+4,length)==Z_OK,"saved zlib stream");
    plain.resize(size); require(plain==expected,"saved tags/key/scalar/container terminator bytes");
}
}
int main() {
    try {
        Machine machine; auto* vm=machine.vm; Files files;
        const auto saved=files.path("roundtrip.dat"), golden=files.path("golden.dat");
        const auto bad=files.path("bad.dat"), missing=files.path("missing.dat");
        evaluate(vm,"source <- { integer=-1234567, real=1.25, yes=true, no=false, text=\"hello\", empty=\"\", nested={value=9}, array=[2,null,\"three\"], omitted=null }; destination <- {}; golden <- {}; simple <- { value=-7 }; blank <- {}; ");
        for(int pass=0;pass<3;++pass) {
            require(file_call(vm,saved,"source",true),"save nested table");
            require(file_call(vm,saved,"destination",false),"load nested table");
            evaluate(vm,"assert(destination.integer==-1234567 && destination.real==1.25); assert(destination.yes==true && destination.no==false); assert(destination.text==\"hello\" && destination.empty==\"\"); assert(destination.nested.value==9); assert(destination.array.len()==3 && destination.array[0]==2 && destination.array[1]==null && destination.array[2]==\"three\"); assert(!(\"omitted\" in destination));");
        }
        // Writer and reader cannot hide a shared wire-format error here.
        require(file_call(vm,saved,"simple",true),"save scalar table");
        Bytes expected; field(expected,OT_INTEGER,"value"); word(expected,uint32_t(-7)); word(expected,OT_NULL);
        check_saved_wire(saved,expected);
        require(file_call(vm,saved,"blank",true),"save empty table");
        Bytes empty; word(empty,OT_NULL); check_saved_wire(saved,empty);
        Bytes raw;
        field(raw,OT_BOOL,"flag"); raw.push_back(1);
        field(raw,OT_FLOAT,"real"); word(raw,0x3fa00000); // IEEE 1.25
        field(raw,OT_STRING,"text"); text(raw,"fixture");
        field(raw,OT_TABLE,"nested"); field(raw,OT_INTEGER,"n"); word(raw,42); word(raw,OT_NULL);
        field(raw,OT_ARRAY,"array"); word(raw,3);
        word(raw,OT_INTEGER); word(raw,OT_INTEGER); word(raw,0); word(raw,uint32_t(-9));
        word(raw,OT_BOOL); word(raw,OT_INTEGER); word(raw,2); raw.push_back(0);
        word(raw,OT_NULL); word(raw,OT_NULL);
        encoded_fixture(golden,raw);
        require(file_call(vm,golden,"golden",false),"load independent wire fixture");
        evaluate(vm,"assert(golden.flag && golden.real==1.25 && golden.text==\"fixture\"); assert(golden.nested.n==42); assert(golden.array.len()==3 && golden.array[0]==-9 && golden.array[1]==null && golden.array[2]==false);");
        require(!file_call(vm,missing,"blank",false),"missing file fails");
        require(!file_call(vm,missing+"\\child.dat","source",true),"invalid parent save fails");
        write_bytes(bad,{1,2,3}); require(!file_call(vm,bad,"blank",false),"short file header fails");
        Bytes oversized; word(oversized,0x20001); write_bytes(bad,oversized);
        require(!file_call(vm,bad,"blank",false),"oversized encoded length fails");
        Bytes short_payload; word(short_payload,12); short_payload.push_back(0); write_bytes(bad,short_payload);
        require(!file_call(vm,bad,"blank",false),"short encoded payload fails");
        raw.clear(); word(raw,OT_INTEGER); // Missing key tag/value, valid compressed envelope.
        encoded_fixture(bad,raw); require(!file_call(vm,bad,"blank",false),"truncated table fails");
        require(sq_gettop(vm)==0,"final stack balance");
        std::puts("PASS: savedata file roundtrip, independent wire fixtures, scalar widths and file failures");
    } catch(const std::exception& error) {
        std::fprintf(stderr,"savedata: %s\n",error.what()); return 1;
    }
}
