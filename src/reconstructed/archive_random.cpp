#include "kinoko/archive_random.h"
#include <random>
namespace {
// Original 404230/404270: standard 32-bit MT19937, default seed 5489.
std::mt19937 &engine() { static std::mt19937 value; return value; }
static_assert((0xff3a58adu << 7)==0x9d2c5680u,"original temper mask");
static_assert((0xffffdf8cu << 15)==0xefc60000u,"original temper mask");
}
extern "C" void kinoko_seed_random(uint32_t seed) { engine().seed(seed); }
extern "C" void kinoko_decode_archive_index(uint8_t *bytes,uint32_t size) {
    // 41066F seeds EAX with index length + six header bytes. One full
    // engine result per byte, truncated to AL; no distribution adaptor.
    auto &random=engine();random.seed(size+6u);
    for(uint32_t i=0;i<size;++i) bytes[i]^=static_cast<uint8_t>(random());
    uint8_t key=0xc5u,step=0x89u;
    for(uint32_t i=0;i<size;++i) {
        bytes[i]^=key;key=static_cast<uint8_t>(key+step);
        step=static_cast<uint8_t>(step+0x49u);
    }
}
