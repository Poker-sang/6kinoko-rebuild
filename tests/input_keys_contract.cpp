#include "kinoko/input_keys.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"keys line %d: %s\n",__LINE__,#x);return 1; } } while(0)
extern "C" unsigned char g_retdec_keyboard_state[256]{};
int main() {
    KinokoKeyTracker first{},copy{};
    first.reserved1043=0xab;
    kinoko_input_keys_construct(&first);kinoko_input_keys_construct(&copy);
    kinoko_input_keys_add(&first,0xff);kinoko_input_keys_add(&first,0xff);
    CHECK(kinoko_input_keys_size(&first)==1);
    g_retdec_keyboard_state[0xff]=0x80;g_retdec_keyboard_state[0x9d]=0x80;
    g_retdec_keyboard_state[0x36]=0x80;g_retdec_keyboard_state[0xb8]=0x80;
    first.counts[42]=17;
    CHECK(kinoko_input_keys_update(&first)==1);
    CHECK(first.shift && first.alt && first.control && first.counts[42]==17);
    CHECK(kinoko_input_key_pressed(&first,0x1ff,1,1,1)==1);
    CHECK(first.reserved1043==0xab);
    kinoko_input_keys_update(&first);
    CHECK(!kinoko_input_key_pressed(&first,255,0,0,0));
    first.counts[255]=INT32_MAX;kinoko_input_keys_update(&first);
    CHECK(first.counts[255]==INT32_MIN);
    auto* copied_owner=copy.keys;
    kinoko_input_keys_assign(&copy,&first);
    CHECK(copy.keys==copied_owner && copy.keys!=first.keys && copy.counts[255]==INT32_MIN);
    kinoko_input_keys_clear(&first);
    CHECK(kinoko_input_keys_size(&copy)==1 && kinoko_input_keys_size(&first)==0);
    std::memset(g_retdec_keyboard_state,0,256);
    CHECK(kinoko_input_keys_update(&copy)==0 && copy.counts[255]==0 && !copy.shift && !copy.alt);
    copy.counts[255]=1;
    CHECK(kinoko_input_key_pressed(&copy,255,256,256,256)==1);
    kinoko_input_keys_assign(&copy,&copy);
    kinoko_input_keys_destroy(&first);kinoko_input_keys_destroy(&copy);
    CHECK(!first.keys && !copy.keys);
    return 0;
}
