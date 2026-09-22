#include "kinoko/input_device.h"
#include "kinoko/direct_input.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"device line %d\n",__LINE__);return 1; } } while(0)
static KinokoControllerState controller{};
static bool available=true;
extern "C" {
unsigned char g_retdec_keyboard_state[256]{};
const KinokoControllerState* kinoko_input_controller_state(int32_t index) {
    return index==0 && available?&controller:nullptr;
}
}
int main() {
    KinokoInputDevice device{};
    auto& a=device.assignment;auto& s=device.state;
    a.id=255;a.left=203;a.right=205;a.up=200;a.down=208;
    std::fill_n(a.buttons,12,-1);a.buttons[0]=44;
    s.counts[3]=37;s.reserved70[0]=0xab;
    g_retdec_keyboard_state[203]=g_retdec_keyboard_state[205]=g_retdec_keyboard_state[44]=0x80;
    kinoko_input_device_update(&device,nullptr);
    CHECK(s.counts[0]==-1 && s.counts[2]==1 && s.counts[3]==37 && s.axes[0]==-1);
    s.counts[2]=INT32_MAX;kinoko_input_device_update(&device,nullptr);
    CHECK(s.counts[2]==INT32_MIN && s.reserved70[0]==0xab);
    std::memset(g_retdec_keyboard_state,0,256);kinoko_input_device_update(&device,nullptr);
    CHECK(s.released[0]==1 && s.released[2]==0 && s.counts[2]==0);
    kinoko_input_device_update(&device,nullptr);CHECK(s.released[0]==0);
    a.id=0;a.buttons[0]=0;controller.axes[0]=-501;controller.axes[1]=501;
    controller.axes[2]=250;controller.buttons[0]=1;
    kinoko_input_device_update(&device,nullptr);
    CHECK(s.counts[0]==-1 && s.counts[1]==1 && s.counts[2]==1 && s.axes[2]==0.25f);
    controller.axes[0]=-500;controller.axes[1]=500;controller.buttons[0]=0;
    kinoko_input_device_update(&device,nullptr);
    CHECK(s.counts[0]==0 && s.counts[1]==0 && s.released[0] && s.released[1] && s.released[2]);
    auto before=s;available=false;kinoko_input_device_update(&device,nullptr);
    CHECK(std::memcmp(&before,&s,sizeof(s))==0);
    a.id=254;kinoko_input_device_update(&device,nullptr);
    KinokoInputState zero{};CHECK(std::memcmp(&zero,&s,sizeof(s))==0 && a.id==254);
    return 0;
}
