// Layout-only contract source. Original byte offsets below are independent of
// the implementation's member access. This is not game/VM lifetime validation.
#include "kinoko/act_layer_storage.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <type_traits>

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "layer storage %d: %s\n", __LINE__, #c); return 1; } } while (false)
template<class T> T read_at(const unsigned char* p) { T value; std::memcpy(&value,p,sizeof(value)); return value; }

int main() {
    using namespace kinoko::act;
    std::array<unsigned char, 350> raw;
    raw.fill(0xa5);
    LayerStorageView layer(raw.data()+1); // deliberately unaligned
    layer.clear();
    CHECK(raw.front()==0xa5 && raw.back()==0xa5);
    const auto association=layer.view(&LayerStorageRecord::association);
    association.set(&LayerAssociationRecord::layer_id,int32_t{-1});
    layer.set(&LayerStorageRecord::visibility_flags,uint16_t{1});
    layer.view(&LayerStorageRecord::keys).set(&LayerListRecord::count,int32_t{7});
    layer.view(&LayerStorageRecord::timelines).set(&LayerListRecord::count,int32_t{3});
    const auto script=layer.view(&LayerStorageRecord::script);
    script.set(&ScriptStorageRecord::size,uint32_t{17});
    script.set(&ScriptStorageRecord::compiled,uint8_t{1});
    const auto object=layer.view(&LayerStorageRecord::script_object);
    object.set(&LayerObjectRecord::value,std::array<int32_t,2>{42,43});
    object.set(&LayerObjectRecord::owns_reference,uint8_t{1});
    CHECK(read_at<int32_t>(raw.data()+1+104)==-1);
    CHECK(read_at<uint16_t>(raw.data()+1+140)==1);
    CHECK(read_at<int32_t>(raw.data()+1+184)==7);
    CHECK(read_at<int32_t>(raw.data()+1+196)==3);
    CHECK(read_at<uint32_t>(raw.data()+1+300)==17);
    CHECK(raw[1+305]==1 && raw[1+304]==0);
    CHECK(read_at<int32_t>(raw.data()+1+316)==42 && read_at<int32_t>(raw.data()+1+320)==43);
    CHECK(raw[1+324]==1);
    // Selective callback writes must not reset VM slots or padding/unknowns.
    raw.fill(0xa5);
    auto callback=script.view(&ScriptStorageRecord::update);
    callback.set(&ActCallbackRecord::environment,std::array<int32_t,2>{0,0});
    callback.set(&ActCallbackRecord::closure,std::array<int32_t,2>{0,0});
    CHECK(read_at<uint32_t>(raw.data()+1+228)==0xa5a5a5a5u);
    CHECK(read_at<uint32_t>(raw.data()+1+232)==0);
    CHECK(read_at<uint32_t>(raw.data()+1+240)==0);
    CHECK(raw[1+227]==0xa5 && raw[1+248]==0xa5);
    CHECK(raw.front()==0xa5 && raw.back()==0xa5);
    return 0;
}
