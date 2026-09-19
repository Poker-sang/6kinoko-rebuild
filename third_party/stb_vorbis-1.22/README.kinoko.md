# stb_vorbis 1.22

Ogg Vorbis decoder by Sean Barrett, from the stb project:
https://github.com/nothings/stb

This file was relocated from `src/decompiled/stb_vorbis.c` without changing
its bytes. It is third-party source, not decompiler output. The precise
upstream revision of the previously imported copy has not been established.

SHA256 of the relocated source:
`218714ebaac44038635d700934f666d613f99b5eb5dbe480783a11de32073d84`

The MIT / public-domain license alternatives remain at the end of the source.
`src/reconstructed/audio_runtime.cpp` includes the implementation with
`STB_VORBIS_NO_STDIO`; do not also compile it as a separate translation unit.
The audio contract test includes that same runtime implementation.
