#include "kinoko/integer_map.h"
#include "kinoko/native_record_view.hpp"
#include <map>
struct KinokoIntegerMap { std::map<int32_t,int32_t> values; };
extern "C" KinokoIntegerMap* kinoko_integer_map_create() { return new KinokoIntegerMap; }
extern "C" void kinoko_integer_map_destroy(KinokoIntegerMap* map) { delete map; }
extern "C" void kinoko_integer_map_clear(KinokoIntegerMap* map) { if(map) map->values.clear(); }
extern "C" uint32_t kinoko_integer_map_size(const KinokoIntegerMap* map) {
    return map?static_cast<uint32_t>(map->values.size()):0;
}
extern "C" int32_t* kinoko_integer_map_put(KinokoIntegerMap* map,int32_t key,int32_t value) {
    auto& slot=map->values[key];slot=value;return &slot;
}
extern "C" int32_t* kinoko_integer_map_find(KinokoIntegerMap* map,int32_t key) {
    if(!map) return nullptr;
    const auto found=map->values.find(key);
    return found==map->values.end()?nullptr:&found->second;
}
// 4706C0: out-iterator is returned by address. Only this legacy boundary
// converts nodes/sentinel to integer slots; the native lookup uses nullptr.
extern "C" int32_t* kinoko_integer_map_lookup_index(const KinokoIntegerMapIndex* index,int32_t* entry,const int32_t* key) {
    const kinoko::native::RecordView<KinokoIntegerMapIndex> view(const_cast<KinokoIntegerMapIndex*>(index));
    auto* owner=view.get(&KinokoIntegerMapIndex::owner);
    auto* found=kinoko_integer_map_find(owner,*key);
    *entry=static_cast<int32_t>(reinterpret_cast<intptr_t>(found?static_cast<void*>(found):owner));
    return entry;
}
