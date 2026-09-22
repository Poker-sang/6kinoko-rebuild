#pragma once
#include <stdint.h>
/* Native vector owns integer handles. Releasing the referenced textures is
   the manager's responsibility, before clear/destroy. Reserved words retain
   the original 12-byte embedded slot without pretending to be end/capacity. */
typedef struct KinokoIntegerVectorStorage KinokoIntegerVectorStorage;
typedef struct KinokoIntegerVector {
    KinokoIntegerVectorStorage* owner;
    uint32_t reserved[2];
} KinokoIntegerVector;
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_integer_vector_construct(KinokoIntegerVector* slot);
void kinoko_integer_vector_destroy(KinokoIntegerVector* slot);
void kinoko_integer_vector_clear(KinokoIntegerVector* slot);
uint32_t kinoko_integer_vector_size(const KinokoIntegerVector* slot);
const int32_t* kinoko_integer_vector_data(const KinokoIntegerVector* slot);
void kinoko_integer_vector_append(KinokoIntegerVector* slot,int32_t value);
#ifdef __cplusplus
}
static_assert(sizeof(KinokoIntegerVector)==12);
#endif
