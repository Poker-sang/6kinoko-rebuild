#include "kinoko/integer_vector.h"
#include <vector>
struct KinokoIntegerVectorStorage { std::vector<int32_t> values; };
extern "C" void kinoko_integer_vector_construct(KinokoIntegerVector* slot) { slot->owner=new KinokoIntegerVectorStorage; }
extern "C" void kinoko_integer_vector_destroy(KinokoIntegerVector* slot) { delete slot->owner;slot->owner=nullptr; }
extern "C" void kinoko_integer_vector_clear(KinokoIntegerVector* slot) { if(slot->owner) slot->owner->values.clear(); }
extern "C" uint32_t kinoko_integer_vector_size(const KinokoIntegerVector* slot) {
    return slot->owner?static_cast<uint32_t>(slot->owner->values.size()):0;
}
extern "C" const int32_t* kinoko_integer_vector_data(const KinokoIntegerVector* slot) {
    return slot->owner?slot->owner->values.data():nullptr;
}
extern "C" void kinoko_integer_vector_append(KinokoIntegerVector* slot,int32_t value) {
    if(!slot->owner) kinoko_integer_vector_construct(slot);
    slot->owner->values.push_back(value);
}
