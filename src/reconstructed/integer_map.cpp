#include "kinoko/integer_map.h"
#include "kinoko/legacy_memory.hpp"
#include <map>
namespace { using Map=std::map<int32_t,int32_t>;using namespace kinoko::legacy; }
extern "C" int32_t kinoko_integer_map_create() { return address(new Map); }
extern "C" void kinoko_integer_map_destroy(int32_t map) { delete pointer<Map>(map); }
extern "C" void kinoko_integer_map_clear(int32_t map) { if(map) pointer<Map>(map)->clear(); }
extern "C" uint32_t kinoko_integer_map_size(int32_t map) { return map?static_cast<uint32_t>(pointer<Map>(map)->size()):0; }
extern "C" int32_t kinoko_integer_map_put(int32_t map,int32_t key,int32_t value) { auto& result=(*pointer<Map>(map))[key];result=value;return address(&result); }
extern "C" int32_t kinoko_integer_map_find(int32_t map,int32_t key) {
    if(!map) return 0;
    auto& items=*pointer<Map>(map);const auto found=items.find(key);
    return found==items.end()?map:address(&found->second);
}
extern "C" int32_t function_4706c0_this(int32_t tree,int32_t* entry,int32_t* key) {
    *entry=kinoko_integer_map_find(field<int32_t>(tree+4),*key);return address(entry);
}
