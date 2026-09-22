#pragma once
#ifdef __cplusplus
extern "C" {
#endif
/* Original 4040D0/404130: degrees, nearest tenth after absolute value;
   sine shifts the scaled angle by -900 before rounding. */
float kinoko_sin_degrees(float degrees);
float kinoko_cos_degrees(float degrees);
#ifdef __cplusplus
}
#endif
