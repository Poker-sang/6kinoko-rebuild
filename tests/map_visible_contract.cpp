#include "kinoko/map_query.hpp"
#include <cstdio>
#include <vector>
using namespace kinoko::map;
static retdec_mcd_chip chip{};
static retdec_mcd_data data{};
extern "C" retdec_mcd_data *kinoko_map_query_chip_data(KinokoActLayout *) { return &data; }
extern "C" retdec_mcd_chip *retdec_mcd_find_chip(retdec_mcd_data *,uint32_t) { return &chip; }
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"map query line %d\n",__LINE__); return 1; } } while(0)
int main() {
    LayerRecord layer{}; layer.position_x=0.75f;
    Placement records[5]{};
    for(int i=0;i<5;++i) { records[i].left=i*16; records[i].top=5; }
    LayoutRecord map{};map.owning_layer=reinterpret_cast<KinokoActLayer *>(&layer);
    map.placements={records,records+5,nullptr};map.max_chip_width=map.max_chip_height=16;
    auto *layout=reinterpret_cast<KinokoActLayout *>(&map);
    int32_t cache=2;std::vector<int> output;
    auto emit=[&](retdec_mcd_chip *,Placement *,int index) { output.push_back(index);return true; };
    CHECK(query_visible(layout,&cache,16,0,48,10,emit));
    CHECK((output==std::vector<int>{2,3,1,0}));CHECK(cache==0);
    CHECK(records[2].fractional_left==32.75f);
    output.clear();cache=3;
    CHECK(query_visible(layout,&cache,0,0,16,10,emit));
    CHECK((output==std::vector<int>{1,0}));
    output.clear();cache=0;
    CHECK(query_visible(layout,&cache,0,40,100,50,emit));CHECK(output.empty());
    return 0;
}
