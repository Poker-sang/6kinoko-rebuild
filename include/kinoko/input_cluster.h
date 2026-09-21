#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void kinoko_input_cluster_construct(int32_t cluster);
void kinoko_input_cluster_clear(int32_t cluster);
void kinoko_input_cluster_append(int32_t cluster, int32_t device);
void kinoko_input_cluster_assign(int32_t destination, int32_t source);
uint32_t kinoko_input_cluster_size(int32_t cluster);
int32_t kinoko_input_cluster_at(int32_t cluster, uint32_t index);
int32_t __fastcall kinoko_input_cluster_delete(int32_t cluster, void *unused, unsigned char flags);
int32_t __fastcall function_4077c0(int32_t cluster);
#ifdef __cplusplus
}
#endif
