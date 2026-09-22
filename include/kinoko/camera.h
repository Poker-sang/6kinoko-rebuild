#pragma once
#include <stdint.h>
typedef struct KinokoCamera KinokoCamera;
typedef struct KinokoQuad KinokoQuad;
#ifdef __cplusplus
extern "C" {
#endif
int32_t kinoko_camera_initialize(KinokoCamera *camera);
KinokoCamera *kinoko_camera_copy(KinokoCamera *destination, KinokoCamera *source);
void kinoko_camera_project(KinokoCamera *camera, KinokoQuad *quad);
#ifdef __cplusplus
}
#endif
