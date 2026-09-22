#include "kinoko/actor_priority.h"
#include "kinoko/legacy_memory.hpp"
#include "kinoko/actor_records.hpp"
#include <map>
#include <memory>
namespace {
using namespace kinoko::legacy;
struct Entry;
using Tree=std::multimap<int32_t,std::unique_ptr<Entry>>;
struct Entry { KinokoActor *actor;Tree::iterator position; };
struct Index { void *policy; Tree *storage; int32_t count; };
static_assert(sizeof(Index)==sizeof(kinoko::actor::TreeIndex));
using IndexView=kinoko::native::RecordView<Index>;
IndexView index_view(int32_t tree) { return IndexView(pointer<void>(tree)); }
Tree *storage(int32_t tree) { return index_view(tree).get(&Index::storage); }
void count(int32_t tree) { index_view(tree).set(&Index::count,static_cast<int32_t>(storage(tree)->size())); }
int32_t token(int32_t tree,Tree::iterator position) {
    return position==storage(tree)->end()?address(storage(tree)):address(position->second.get());
}
}
extern "C" void kinoko_priority_construct(int32_t tree) { index_view(tree).set(&Index::storage,new Tree);count(tree); }
extern "C" void kinoko_priority_clear(int32_t tree) { if(storage(tree)) { storage(tree)->clear();count(tree); } }
extern "C" void kinoko_priority_destroy(int32_t tree) { delete storage(tree);index_view(tree).set(&Index::storage,static_cast<Tree *>(nullptr));index_view(tree).set(&Index::count,int32_t{0}); }
extern "C" int32_t kinoko_priority_first(int32_t tree) { return storage(tree)?token(tree,storage(tree)->begin()):0; }
extern "C" int32_t kinoko_priority_next(int32_t tree,int32_t value) { return token(tree,std::next(pointer<Entry>(value)->position)); }
extern "C" int32_t kinoko_priority_value(int32_t value) { return address(pointer<Entry>(value)->actor); }
extern "C" int32_t function_463210_this(int32_t tree,int32_t source) {
    if(!storage(tree)) kinoko_priority_construct(tree);
    return address(new Entry{pointer<KinokoActor>(field<int32_t>(source)),{}});
}
extern "C" int32_t function_463610_this(int32_t tree,int32_t output,int32_t value,int32_t left) {
    auto entry=std::unique_ptr<Entry>(pointer<Entry>(value));
    const int32_t key=entry->actor?kinoko::actor::ActorView(entry->actor).get(&kinoko::actor::ActorRecord::priority):0;
    auto& items=*storage(tree);
    const auto hint=left?items.lower_bound(key):items.upper_bound(key);
    const auto position=items.emplace_hint(hint,key,std::move(entry));
    position->second->position=position;count(tree);
    field<int32_t>(output)=value;field<int32_t>(output+4)=1;return output;
}
extern "C" void *kinoko_actor_priority_insert(void *index, KinokoActor *actor) {
    int32_t result[2]{};
    const auto node=function_463210_this(address(index),address(&actor));
    if (!node || !function_463610_this(address(index),address(result),node,0)) return nullptr;
    return pointer<void>(result[0]);
}
extern "C" void kinoko_actor_priority_erase(void *index, void *entry) {
    int32_t next{}; function_463280_this(address(index),address(&next),address(entry));
}
extern "C" void *kinoko_actor_priority_first(void *index) { return pointer<void>(kinoko_priority_first(address(index))); }
extern "C" void *kinoko_actor_priority_next(void *index, void *entry) { return pointer<void>(kinoko_priority_next(address(index),address(entry))); }
extern "C" KinokoActor *kinoko_actor_priority_value(void *entry) { return pointer<Entry>(address(entry))->actor; }
extern "C" int32_t function_463280_this(int32_t tree,int32_t output,int32_t value) {
    if(!storage(tree) || value==address(storage(tree))) return 0;
    auto position=pointer<Entry>(value)->position;
    field<int32_t>(output)=token(tree,std::next(position));
    storage(tree)->erase(position);count(tree);return output;
}
