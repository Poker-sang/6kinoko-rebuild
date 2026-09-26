#include "kinoko/act_frame.h"
#include "kinoko/act_draw_records.hpp"
#include "kinoko/act_host.h"
#include "kinoko/legacy_memory.hpp"
#include <vector>
#include <type_traits>
#include <new>
extern "C" void kinoko_trace_i32(const char*, int32_t);
namespace {
using namespace kinoko::act;
using namespace kinoko::legacy;
using kinoko::native::RecordView;
struct Sprite {
    BlitSprite value{};
    Sprite() { value.sprite.vtable=const_cast<void*>(kinoko_act_host_symbols()->sprite_vtable); }
    ~Sprite() { value.sprite.vtable=const_cast<void*>(kinoko_act_host_symbols()->color_vtable); }
};
static_assert(sizeof(Sprite)==sizeof(BlitSprite));
}
struct KinokoActCommandOwner { std::vector<BlitCommand> values; };
struct KinokoActSpriteOwner { std::vector<Sprite> values; };
namespace {
template<class Storage> auto owner(void* storage) { return RecordView<Storage>(storage).get(&Storage::owner); }
template<class Storage> auto ensure(void* storage) {
    const RecordView<Storage> view(storage);
    auto* value=view.get(&Storage::owner);
    if(!value) {
        using Owner=std::remove_pointer_t<decltype(value)>;
        value=new Owner;
        view.set(&Storage::owner,value);
    }
    return value;
}
template<class Owner> KinokoDrawSpan span(Owner* owner) {
    if(!owner) return {};
    auto& values=owner->values;
    auto* begin=reinterpret_cast<unsigned char*>(values.data());
    if(!begin) return {};
    return {begin,begin+values.size()*sizeof(typename decltype(owner->values)::value_type),
        begin+values.capacity()*sizeof(typename decltype(owner->values)::value_type)};
}
void* command_storage(KinokoActRuntime* runtime) { return RecordView<RuntimeRecord>(runtime).bytes(&RuntimeRecord::draw_commands); }
void* sprite_storage(KinokoActRuntime* runtime) { return RecordView<RuntimeRecord>(runtime).bytes(&RuntimeRecord::draw_sprites); }
}
extern "C" int32_t kinoko_act_append_blit(KinokoActRuntime* self, int32_t x, int32_t y,
    int32_t width, int32_t height, KinokoActResource* texture_resource, int32_t sx, int32_t sy,
    int32_t blend, float alpha) {
    if (!self || !texture_resource) return E_FAIL;
    const RecordView<TextureResourcePrefix> texture(texture_resource);
    const auto* symbols=kinoko_act_host_symbols();
    const auto type=texture.get(&TextureResourcePrefix::vtable);
    if(type!=symbols->texture_resource_vtable &&
       type!=symbols->render_target_vtable) return E_FAIL;
    const BlitCommand command{blend,alpha<0?0:alpha>1?1:alpha,
        static_cast<float>(x),static_cast<float>(y),sx,sy,width,height,
        texture.get(&TextureResourcePrefix::texture)};
    try { ensure<KinokoActCommandStorage>(command_storage(self))->values.push_back(command); }
    catch(const std::bad_alloc&) { return E_OUTOFMEMORY; }
    static volatile LONG trace_count;
    if(InterlockedIncrement(&trace_count)<=12) {
        kinoko_trace_i32("act:bitblt-texture",command.texture);
        kinoko_trace_i32("act:bitblt-x",x);kinoko_trace_i32("act:bitblt-y",y);
    }
    return 0;
}
extern "C" int32_t kinoko_act_resize_sprites(KinokoActSpriteStorage* storage,uint32_t requested) {
    if(!storage || requested>0x1642c85u) return 0;
    try {
        auto& values=ensure<KinokoActSpriteStorage>(storage)->values;
        const auto capacity=static_cast<int32_t>(values.capacity());
        values.resize(requested);return capacity;
    } catch(const std::bad_alloc&) { return 0; }
}
// 455230 assigns elements without overwriting destination virtual identity.
extern "C" KinokoBlitSprite* kinoko_act_copy_blit_sprites(const KinokoBlitSprite* first,
    const KinokoBlitSprite* last,KinokoBlitSprite* output) {
    auto* source=reinterpret_cast<const unsigned char*>(first);
    auto* end=reinterpret_cast<const unsigned char*>(last);
    auto* target=reinterpret_cast<unsigned char*>(output);
    while(source!=end) {
        const RecordView<BlitSprite> destination(target);
        auto value=load<BlitSprite>(source);
        value.sprite.vtable=destination.view(&BlitSprite::sprite).get(&KinokoSprite::vtable);
        store(destination.data(),value);
        source+=sizeof(BlitSprite);target+=sizeof(BlitSprite);
    }
    return reinterpret_cast<KinokoBlitSprite*>(target);
}
extern "C" unsigned char* kinoko_act_clear_sprites(KinokoActSpriteStorage* storage) {
    if(!storage) return nullptr;
    auto* value=owner<KinokoActSpriteStorage>(storage);
    if(!value) return nullptr;
    auto* begin=reinterpret_cast<unsigned char*>(value->values.data());
    value->values.clear();return begin;
}
extern "C" KinokoDrawSpan kinoko_act_command_span(KinokoActRuntime* runtime) { return span(owner<KinokoActCommandStorage>(command_storage(runtime))); }
extern "C" KinokoDrawSpan kinoko_act_sprite_span(KinokoActRuntime* runtime) { return span(owner<KinokoActSpriteStorage>(sprite_storage(runtime))); }
extern "C" void kinoko_act_commands_clear(KinokoActRuntime* runtime) {
    if(auto* value=owner<KinokoActCommandStorage>(command_storage(runtime))) value->values.clear();
}
extern "C" void kinoko_act_draw_storage_destroy(KinokoActRuntime* runtime) {
    const auto sprites=RecordView<RuntimeRecord>(runtime).view(&RuntimeRecord::draw_sprites);
    delete sprites.get(&KinokoActSpriteStorage::owner);sprites.set(&KinokoActSpriteStorage::owner,static_cast<KinokoActSpriteOwner*>(nullptr));
    const auto commands=RecordView<RuntimeRecord>(runtime).view(&RuntimeRecord::draw_commands);
    delete commands.get(&KinokoActCommandStorage::owner);commands.set(&KinokoActCommandStorage::owner,static_cast<KinokoActCommandOwner*>(nullptr));
}
