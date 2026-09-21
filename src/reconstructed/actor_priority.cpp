#include "kinoko/actor_priority.h"
#include "kinoko/legacy_memory.hpp"
#include <map>
#include <memory>
namespace {
using namespace kinoko::legacy;
struct Entry;
using Tree=std::multimap<int32_t,std::unique_ptr<Entry>>;
struct Entry { int32_t actor;Tree::iterator position; };
Tree*& storage(int32_t tree) { return field<Tree*>(tree+4); }
void count(int32_t tree) { field<int32_t>(tree+8)=static_cast<int32_t>(storage(tree)->size()); }
int32_t token(int32_t tree,Tree::iterator position) {
    return position==storage(tree)->end()?address(storage(tree)):address(position->second.get());
}
}
extern "C" void kinoko_priority_construct(int32_t tree) { storage(tree)=new Tree;count(tree); }
extern "C" void kinoko_priority_clear(int32_t tree) { if(storage(tree)) { storage(tree)->clear();count(tree); } }
extern "C" void kinoko_priority_destroy(int32_t tree) { delete storage(tree);storage(tree)=nullptr;field<int32_t>(tree+8)=0; }
extern "C" int32_t kinoko_priority_first(int32_t tree) { return storage(tree)?token(tree,storage(tree)->begin()):0; }
extern "C" int32_t kinoko_priority_next(int32_t tree,int32_t value) { return token(tree,std::next(pointer<Entry>(value)->position)); }
extern "C" int32_t kinoko_priority_value(int32_t value) { return pointer<Entry>(value)->actor; }
extern "C" int32_t function_463210_this(int32_t tree,int32_t source) {
    if(!storage(tree)) kinoko_priority_construct(tree);
    return address(new Entry{field<int32_t>(source),{}});
}
extern "C" int32_t function_463610_this(int32_t tree,int32_t output,int32_t value,int32_t left) {
    auto entry=std::unique_ptr<Entry>(pointer<Entry>(value));
    const int32_t key=entry->actor?field<int32_t>(entry->actor+228):0;
    auto& items=*storage(tree);
    const auto hint=left?items.lower_bound(key):items.upper_bound(key);
    const auto position=items.emplace_hint(hint,key,std::move(entry));
    position->second->position=position;count(tree);
    field<int32_t>(output)=value;field<int32_t>(output+4)=1;return output;
}
extern "C" int32_t function_463280_this(int32_t tree,int32_t output,int32_t value) {
    if(!storage(tree) || value==address(storage(tree))) return 0;
    auto position=pointer<Entry>(value)->position;
    field<int32_t>(output)=token(tree,std::next(position));
    storage(tree)->erase(position);count(tree);return output;
}
