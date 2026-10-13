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
struct SQVM *kinoko_primary_vm = nullptr;
const void* kinoko_squirrel_object_vtable(void) { return reinterpret_cast<const void*>(0x12345678); }
void* kinoko_native_void_type(void) { return reinterpret_cast<void*>(0x13572468); }
void kinoko_trace(const char*) {}
void kinoko_trace_i32(const char*, int32_t) {}
void kinoko_trace_squirrel_name(const char*, int32_t) {}
void kinoko_host_free_allocation(int32_t* p) { std::free(p); }
}
namespace {
using Bytes = std::vector<unsigned char>;
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
class Machine {
public:
    HSQUIRRELVM vm = sq_open(128);
    Machine() {
        require(vm != nullptr, "open VM"); kinoko_primary_vm = reinterpret_cast<SQVM*>(vm);
        sq_setcompilererrorhandler(vm, [](HSQUIRRELVM, const SQChar* error,
            const SQChar* source, SQInteger line, SQInteger column) {
            std::fprintf(stderr,"%s:%lld:%lld: %s\n",source,
                static_cast<long long>(line),static_cast<long long>(column),error);
        });
    }
    ~Machine() { sq_close(vm); kinoko_primary_vm = nullptr; }
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
    ~Files() { std::printf("Retained savedata fixtures: %s\n", directory.c_str()); }
};
void evaluate(HSQUIRRELVM vm, const char* source) {
    const auto top=sq_gettop(vm);
    if (SQ_FAILED(sq_compilebuffer(vm,source,static_cast<SQInteger>(std::strlen(source)),
        "savedata-contract",SQTrue))) {
        sq_getlasterror(vm); const SQChar* error=nullptr;
        sq_getstring(vm,-1,&error);
        std::fprintf(stderr,"compile: %s\nsource: %s\n",error?error:"<unknown>",source);
        sq_settop(vm,top); throw std::runtime_error("compile fixture");
    }
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
        ? kinoko_savedata_save_file_entry(path.c_str(), (const void*)(uintptr_t)(kinoko_squirrel_object_vtable()), value._type, kinoko::script::data_bits(value))
        : kinoko_savedata_load_file_entry(path.c_str(), (const void*)(uintptr_t)(kinoko_squirrel_object_vtable()), value._type, kinoko::script::data_bits(value));
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
    Bytes plain(0x20000);
    z_stream stream{};
    require(inflateInit(&stream)==Z_OK,"independent inflater initialization");
    stream.next_in=const_cast<Bytef*>(file.data()+4); stream.avail_in=length;
    stream.next_out=plain.data(); stream.avail_out=static_cast<uInt>(plain.size());
    const int status=inflate(&stream,Z_FINISH);
    const auto size=stream.total_out;
    const bool consumed_all=stream.avail_in==0;
    inflateEnd(&stream);
    // Unlike the production compatibility wrapper, require an entire zlib stream.
    require(status==Z_STREAM_END && consumed_all,"saved complete zlib stream");
    plain.resize(size); require(plain==expected,"saved tags/key/scalar/container terminator bytes");
}
Bytes nested_table_wire(unsigned depth) {
    Bytes raw;
    for(unsigned level=1;level<depth;++level) field(raw,OT_TABLE,"child");
    field(raw,OT_INTEGER,"value"); word(raw,uint32_t(-2468));
    for(unsigned level=0;level<depth;++level) word(raw,OT_NULL);
    return raw;
}
void check_serialization_guards(HSQUIRRELVM vm,Files& files) {
    const auto deep=files.path("nesting-limit.dat"), saved=files.path("guarded-save.dat");
    const auto too_deep=files.path("excessive-nesting-wire.dat");
    const auto shared=files.path("shared-child.dat");
    evaluate(vm,R"sq(
        depth_limit <- {};
        depth_loaded <- {};
        {
            local node=depth_limit;
            for(local depth=1;depth<256;++depth) {
                local child={}; node.child <- child; node=child;
            }
            node.value <- -2468;
        }
    )sq");
    // This must be the fresh 128-slot VM's first save: shallow saves would
    // already reserve enough stack space and hide the deep-iteration overflow.
    // Keep a caller-owned value below every native call as well as checking top.
    const auto top=sq_gettop(vm);
    sq_pushinteger(vm,0x1234567);
    require(file_call(vm,deep,"depth_limit",true),"save 256 nested tables with small initial VM stack");
    check_saved_wire(deep,nested_table_wire(256));
    require(file_call(vm,deep,"depth_loaded",false),"load 256 nested tables");
    evaluate(vm,R"sq(
        local node=depth_loaded;
        for(local depth=1;depth<256;++depth) {
            assert(typeof node=="table" && node.len()==1 && "child" in node);
            node=node.child;
        }
        assert(typeof node=="table" && node.len()==1 && node.value==-2468);
    )sq");

    evaluate(vm,R"sq(
        cycle_table <- {};
        cycle_table.self <- cycle_table;
        cycle_array <- []; cycle_array.append(cycle_array);
        cycle_indirect <- {};
        cycle_indirect.children <- [cycle_indirect];
        excess_depth <- {};
        {
            local node=excess_depth;
            for(local depth=1;depth<257;++depth) {
                local child={}; node.child <- child; node=child;
            }
            node.value <- -2468;
        }
        scalar_integer <- 17; scalar_float <- 1.25; scalar_bool <- true;
        scalar_string <- "unsupported root"; scalar_null <- null;
        recovery_source <- { value=73 };
        recovery_loaded <- {};
    )sq");
    require(file_call(vm,saved,"recovery_source",true),"establish prior save for failure guards");
    const auto recover=[&]() {
        // Change the payload so a stale prior file cannot pass recovery checks.
        evaluate(vm,"recovery_source.value+=1; recovery_loaded={};");
        require(file_call(vm,saved,"recovery_source",true),"normal save succeeds after serialization failure");
        require(file_call(vm,saved,"recovery_loaded",false),"normal load succeeds after serialization failure");
        evaluate(vm,"assert(recovery_loaded.len()==1 && recovery_loaded.value==recovery_source.value);");
    };
    const auto rejected_save=[&](const char* source,const char* message) {
        const auto previous=read_bytes(saved);
        require(!file_call(vm,saved,source,true),message);
        require(read_bytes(saved)==previous,"rejected save preserves every prior file byte");
        recover();
    };
    rejected_save("cycle_table","self-referential table save fails");
    rejected_save("cycle_array","self-referential array save fails");
    rejected_save("cycle_indirect","indirect table-array cycle save fails");
    rejected_save("excess_depth","257 nested tables exceed save depth limit");
    rejected_save("scalar_integer","integer root save fails");
    rejected_save("scalar_float","float root save fails");
    rejected_save("scalar_bool","boolean root save fails");
    rejected_save("scalar_string","string root save fails");
    rejected_save("scalar_null","null root save fails");

    // A valid compressed tree built independently of the writer reaches the
    // reader's own depth guard, well below the file buffer's byte limit.
    encoded_fixture(too_deep,nested_table_wire(257));
    evaluate(vm,"excess_loaded <- {};");
    require(!file_call(vm,too_deep,"excess_loaded",false),"257 nested wire tables exceed load depth limit");
    recover();

    evaluate(vm,R"sq(
        shared_table <- { value=19 };
        shared_array <- [shared_table,7];
        shared_source <- { first=shared_table, second=shared_table,
                           arrays=[shared_array,shared_array] };
        shared_loaded <- {};
    )sq");
    require(file_call(vm,shared,"shared_source",true),"shared acyclic tables and arrays save successfully");
    require(file_call(vm,shared,"shared_loaded",false),"shared acyclic children load successfully");
    evaluate(vm,R"sq(
        assert(shared_loaded.first.value==19 && shared_loaded.second.value==19);
        assert(shared_loaded.arrays.len()==2);
        foreach(child in shared_loaded.arrays)
            assert(child.len()==2 && child[0].value==19 && child[1]==7);
        // The original wire is a tree: aliases become independent value copies.
        shared_loaded.first.value=99;
        shared_loaded.arrays[0][1]=8;
        assert(shared_loaded.second.value==19 && shared_loaded.arrays[0][0].value==19);
        assert(shared_loaded.arrays[1][0].value==19 && shared_loaded.arrays[1][1]==7);
        cycle_table.self=null; cycle_array[0]=null; cycle_indirect.children[0]=null;
    )sq");
    SQInteger sentinel=0;
    require(sq_gettop(vm)==top+1 && SQ_SUCCEEDED(sq_getinteger(vm,-1,&sentinel)) &&
        sentinel==0x1234567,"serialization guards preserve caller stack contents");
    sq_pop(vm,1);
}
}
int main() {
    try {
        Machine machine; auto* vm=machine.vm; Files files;
        check_serialization_guards(vm,files);
        const auto saved=files.path("roundtrip.dat"), golden=files.path("golden.dat");
        const auto bad=files.path("bad.dat"), missing=files.path("missing.dat");
        evaluate(vm,"source <- { integer=-1234567, real=1.25, yes=true, no=false, text=\"hello\", empty=\"\", nested={value=9}, array=[2,null,\"three\"], omitted=null }\n destination <- {}\n golden <- {}\n simple <- { value=-7 }\n blank <- {}; ");
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
        // Buffer exhaustion must leave the prior save intact.
        const auto previous_save = read_bytes(saved);
        evaluate(vm,"too_large <- { text = \"x\" }\n for(local i=0;i<18;++i) too_large.text += too_large.text;");
        require(!file_call(vm,saved,"too_large",true),"oversized table save fails");
        require(read_bytes(saved)==previous_save,"serialization failure preserves existing save");
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
