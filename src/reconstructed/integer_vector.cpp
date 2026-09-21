#include "kinoko/integer_vector.h"
#include "kinoko/legacy_memory.hpp"
#include <vector>
namespace { using namespace kinoko::legacy;using Values=std::vector<int32_t>;
Values*& storage(int32_t slot) { return field<Values*>(slot); } }
extern "C" void kinoko_integer_vector_construct(int32_t slot) { storage(slot)=new Values; }
extern "C" void kinoko_integer_vector_destroy(int32_t slot) { delete storage(slot);storage(slot)=nullptr; }
extern "C" void kinoko_integer_vector_clear(int32_t slot) { if(storage(slot)) storage(slot)->clear(); }
extern "C" uint32_t kinoko_integer_vector_size(int32_t slot) { return storage(slot)?static_cast<uint32_t>(storage(slot)->size()):0; }
extern "C" int32_t kinoko_integer_vector_data(int32_t slot) { return storage(slot)?address(storage(slot)->data()):0; }
extern "C" void kinoko_integer_vector_append(int32_t slot,int32_t value) {
    if(!storage(slot)) kinoko_integer_vector_construct(slot);storage(slot)->push_back(value);
}
