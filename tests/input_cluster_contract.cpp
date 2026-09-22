#include "kinoko/input_cluster.h"
#include <cstring>
#include <limits>
#include <cstdio>
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"cluster line %d\n",__LINE__);return 1; } } while(0)
extern "C" int32_t g35[2]{};
int main() {
    KinokoInputCluster cluster{},copy{};
    KinokoInputDevice first{},second{},third{};
    first.assignment.id=10;second.assignment.id=20;third.assignment.id=30;
    first.state.counts[0]=-5;second.state.counts[0]=5;third.state.counts[0]=3;
    first.state.counts[1]=2;second.state.counts[1]=-6;
    first.state.counts[2]=9;second.state.counts[2]=9;second.state.released[2]=1;
    first.state.released[3]=1;third.state.counts[13]=7;
    first.state.axes[0]=-0.75f;second.state.axes[0]=0.75f;third.state.axes[5]=-0.9f;
    kinoko_input_cluster_construct(&cluster);kinoko_input_cluster_construct(&copy);
    kinoko_input_cluster_append(&cluster,&first);kinoko_input_cluster_append(&cluster,&second);
    kinoko_input_cluster_append(&cluster,&third);
    CHECK(kinoko_input_cluster_update(&cluster,nullptr)==3);
    const auto& state=cluster.device.state;
    CHECK(state.counts[0]==-5 && state.counts[1]==-6 && state.counts[2]==9 && state.counts[13]==7);
    CHECK(!state.released[2] && state.released[3] && cluster.active_device==30);
    CHECK(state.axes[0]==-0.75f && state.axes[5]==-0.9f);
    kinoko_input_cluster_assign(&copy,&cluster);
    CHECK(copy.devices!=cluster.devices && kinoko_input_cluster_at(&copy,0)==&first);
    kinoko_input_cluster_clear(&cluster);
    CHECK(kinoko_input_cluster_size(&copy)==3);
    kinoko_input_cluster_update(&cluster,nullptr);
    KinokoInputState zero{};CHECK(!std::memcmp(&state,&zero,sizeof(zero)) && cluster.active_device==30);
    first.state.counts[0]=INT32_MIN;second.state.counts[0]=0;third.state.counts[0]=0;
    first.state.axes[0]=std::numeric_limits<float>::quiet_NaN();second.state.axes[0]=0;third.state.axes[0]=0;
    kinoko_input_cluster_update(&copy,nullptr);
    CHECK(copy.device.state.counts[0]==0 && copy.device.state.axes[0]==0);
    kinoko_input_cluster_delete(&cluster,nullptr,0);kinoko_input_cluster_delete(&copy,nullptr,0);
    CHECK(!cluster.devices && !copy.devices && first.assignment.id==10);
    return 0;
}
