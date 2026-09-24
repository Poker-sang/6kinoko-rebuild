#include "kinoko/archive_random.h"
#include "kinoko/compat/archive_index.hpp"
#include <random>
namespace {
// Original 404230/404270: standard 32-bit MT19937, default seed 5489.
std::mt19937 &engine() { static std::mt19937 value; return value; }
static_assert((0xff3a58adu << 7)==0x9d2c5680u,"original temper mask");
static_assert((0xffffdf8cu << 15)==0xefc60000u,"original temper mask");
}
extern "C" void kinoko_seed_random(uint32_t seed) { engine().seed(seed); }
extern "C" void kinoko_decode_archive_index(uint8_t *bytes,uint32_t size) {
    // 41066F: retain shared-engine side effects, not just the decoded bytes.
    kinoko::compat::decode_dat_index(bytes,size,engine());
}
