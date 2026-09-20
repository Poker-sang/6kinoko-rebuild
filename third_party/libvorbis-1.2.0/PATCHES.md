# Local defined-arithmetic and encoder-bound adaptation to libvorbis 1.2.0

The original archive/member hashes remain in `UPSTREAM.json`. This is an
altered source version; `PATCHES.json` pins the modified files. The decoder,
codec version, sample conversion, FPU setup, seek algorithms and Vorbisfile
API have not been upgraded to a different release.

* `sharedbook.c`: form the high-bit codeword in `ogg_uint32_t` before shifting;
  the old signed-int shift could overflow before conversion to the same type.
* `floor1.c`: encode a negative deviation as `-1-val*2` instead of left-shifting
  a negative signed value. Post deviations are bounded by the floor quantizer.
* `psy.c`: use bounded multiplication to pack a signed Bark lower bound instead
  of shifting a negative value. When interpolation clamps to the last band,
  use that band as both endpoints rather than reading `noiseoff[j][P_BANDS]`
  out of bounds even though its multiplier is zero.

The latter two files are encoder paths used to generate the non-proprietary
contract fixtures, not a new game encode path. UBSan reproduced these sites
on `2cc8a985`. Before/after encoded bytes and decoded PCM must be compared in
the same Release toolchain. This is a narrow correctness patch, separate from
host/library refactors. It is **not** a comprehensive security backport for
libvorbis 1.2.0; untrusted/malformed-stream hardening remains a separate scope.
