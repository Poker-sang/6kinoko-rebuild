# SqPlus scalar variable source integration

After the separately tested original integer-width/return correction, the
production `retdec_get_var_value` / `retdec_set_var_value` numeric paths call
**getVar / setVar from the 20080713 SqPlus.cpp source**. The four original
signed-integer, unsigned32, float and bool switch arms are compiled, not copied
into a host reimplementation. Two integration entries expose these static
source helpers. Bootstrap, class-copy and string cases remain separately gated.

The host's 20-byte Variable is not overlaid with a constructed VarRef. The
adapter creates a local source record and stages a correctly typed scalar,
using memcpy across possibly unaligned host byte storage. Bool bytes are
normalized before constructing a bool. A templated stack handler adds only
writeback immediately before forwarding Return, so the native byte record is
updated **before** the source result push. The original conversion/narrowing
switch bodies and standalone StackHandler instantiation are unchanged.

## Deliberately retained host policies

- Read-only or constant writes return SQ_ERROR without introducing C++ unwind.
- Nonnumeric float writes preserve the destination and the existing API error
  path, instead of using the snapshot's default-zero write policy.
- Constants are decoded from the established host representation before source
  dereference: full integer word, integer-to-float, full-word bool. The float
  immediate policy is not asserted to be original-binary parity.
- Metadata lookup, short payload checks, class descriptor identity, source
  address selection and legacy string storage stay at their existing boundary.
  Unsupported categories have not been silently enabled.

The real-source portable contract covers byte canaries and unaligned storage,
8/16/32-bit narrowing, UINT regardless of size, constants, float rejection,
bool normalization and a callback proving commit-before-result-push. The full
Win32 binding contract exercises the same adapter via the production C ABI and
actual Squirrel VM. No source simulation, layout reinterpret_cast to modern
objects, or original EXE/DAT gameplay claim is involved.
