#include "kinoko/actor_priority.h"
#include "kinoko/actor_records.hpp"
#include <map>
#include <memory>
namespace {
struct Entry;
using Tree=std::multimap<int32_t,std::unique_ptr<Entry>>;
struct Entry { KinokoActor *actor; Tree::iterator position; };
struct Index { void *policy; Tree *storage; int32_t count; };
static_assert(sizeof(Index)==sizeof(kinoko::actor::TreeIndex));
using IndexView=kinoko::native::RecordView<Index>;
Tree *storage(void *index) { return IndexView(index).get(&Index::storage); }
void count(void *index) { IndexView(index).set(&Index::count,static_cast<int32_t>(storage(index)->size())); }
void *token(void *index,Tree::iterator position) {
    return position==storage(index)->end()?static_cast<void *>(storage(index)):position->second.get();
}
}
extern "C" void kinoko_priority_construct(void *index) { IndexView(index).set(&Index::storage,new Tree); count(index); }
extern "C" void kinoko_priority_clear(void *index) { if(storage(index)) { storage(index)->clear();count(index); } }
extern "C" void kinoko_priority_destroy(void *index) { delete storage(index);IndexView(index).set(&Index::storage,static_cast<Tree *>(nullptr));IndexView(index).set(&Index::count,int32_t{0}); }
extern "C" void *kinoko_actor_priority_insert_ordered(void *index,KinokoActor *actor,int32_t insert_left) {
    if (!storage(index)) kinoko_priority_construct(index);
    auto entry=std::make_unique<Entry>(); entry->actor=actor;
    const int32_t key=actor?kinoko::actor::ActorView(actor).get(&kinoko::actor::ActorRecord::priority):0;
    auto& items=*storage(index);
    const auto hint=insert_left?items.lower_bound(key):items.upper_bound(key);
    const auto position=items.emplace_hint(hint,key,std::move(entry));
    position->second->position=position;count(index);
    return position->second.get();
}
extern "C" void *kinoko_actor_priority_insert(void *index,KinokoActor *actor) {
    return kinoko_actor_priority_insert_ordered(index,actor,0);
}
extern "C" void *kinoko_actor_priority_erase_next(void *index,void *entry) {
    if (!storage(index) || entry==storage(index)) return nullptr;
    auto position=static_cast<Entry *>(entry)->position;
    auto *next=token(index,std::next(position));
    storage(index)->erase(position);count(index);return next;
}
extern "C" void kinoko_actor_priority_erase(void *index,void *entry) { kinoko_actor_priority_erase_next(index,entry); }
extern "C" void *kinoko_actor_priority_first(void *index) { return storage(index)?token(index,storage(index)->begin()):nullptr; }
extern "C" void *kinoko_actor_priority_next(void *index,void *entry) { return token(index,std::next(static_cast<Entry *>(entry)->position)); }
extern "C" KinokoActor *kinoko_actor_priority_value(void *entry) { return static_cast<Entry *>(entry)->actor; }
