#pragma once
#include <stdint.h>
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
/* C ABI entry points only; state and resources are owned by audio_runtime.cpp. */
void retdec_bgm_update_fade(void);
HANDLE kinoko_audio_start_workers(void);
int32_t kinoko_audio_stop_workers(void);
int32_t kinoko_audio_set_bgm_volume(float gain);
int32_t kinoko_audio_shutdown_resources(void);
int32_t kinoko_audio_initialize_sound_pool(void);
int32_t kinoko_audio_set_sound_volume(float gain);
int32_t kinoko_audio_initialize_device(HWND hwnd, int32_t options);
int32_t kinoko_audio_shutdown_device(void);
int32_t kinoko_audio_initialize_playback(void);
int32_t kinoko_audio_play_bgm(const char* path, int32_t delay_ms, int32_t unused, int32_t looping);
int32_t kinoko_audio_play_bgm_margin(const char* path, int32_t delay_ms, int32_t fade_delay_ms, int32_t unused,
                        int32_t looping);
int32_t kinoko_audio_pause_bgm(void);
int32_t kinoko_audio_fade_bgm(int32_t duration_ms, int32_t volume_percent);
int32_t kinoko_audio_stop_bgm(void);
int32_t kinoko_audio_play_sound(int32_t id);
int32_t kinoko_audio_load_sound_table(const char* path);
#ifdef __cplusplus
}
#endif
