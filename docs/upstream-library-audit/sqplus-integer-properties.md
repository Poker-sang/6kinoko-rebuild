# SqPlus integer-property return/width correction

This is a behavior fix, not a structural migration. The preceding regression
commit intentionally fails against the old reconstructed setter.

## Independent source evidence

* Unmodified reference `src/decompiled/6kinoko.exe.c`, original function
  `4AAC40–4AAF05`: signed-char store at `4AAD07` returns the sign-extended low
  byte (`0x1000000 * value >> 24`); signed-short store at `4AACF5` returns the
  sign-extended low word (`0x10000 * value >> 16`). The UINT branch at `4AAD1B`
  always writes four bytes and returns the same bits in the integer register.
* The 20080713 snapshot's `sqplus/SqPlus.cpp`, `setVar`: the assignment result
  is stored back into `v` (`v = (*(char*)val = (char)v)` / short equivalent)
  before `sa.Return(v)`. Its unsigned case directly accesses `unsigned*`,
  without the signed-size switch. `getVar` has the same signed/unsigned split.

The reconstructed compatibility setter instead returned the pre-truncation
input and treated unsigned metadata as signed-small-integer metadata. Its
old test asserted that reconstructed behavior; the new test uses the two
independent source expressions above as the expectation. It does not alter
Squirrel's assignment-expression bytecode or claim all property behavior is
now identical to the snapshot.

## Scope and regression

Only signed integer return narrowing and unsigned32 width selection change.
Metadata and data are still read/written with memcpy, including unaligned
addresses. Test values straddle 8/16-bit signed boundaries and preserve buffer
canaries. Constant values, invalid-conversion policy, float/bool/string cases,
metadata lookup and original EXE-relative asset loading remain unchanged.
No original EXE/DAT gameplay parity claim is made.
