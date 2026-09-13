# Blue crystal countdown

Scope: restore original countdown cadence without changing original audio/assets/timing. Binary identity/imports carry forward from map-visibility-20260913. IDA MCP ec2d2f0e survey confirms the same original native x86 SHA256.

The effective stage.cv4 comes from archive 2 (the archive probe applies original override order). SetSwitchBlue initializes a 900-frame counter and 44 timestamps. UpdateStage compares the last timestamp against elapsed milliseconds, plays SE 120, then pops the timestamp. Other SE cues (42/43/119) remain separate. This is a scripted SE cadence, not BGM playback frequency.

Original IDA 48DD10 / 48DAC0 and supplied Squirrel 2.2.2 sqapi.cpp::sq_arraypop, sqarray.h::Pop show the explicit array receiver, value push and vector pop/shrink. The generated 48DAC0 has an uninitialized receiver. Native 4A25A0 also returns 1 unconditionally instead of SQ_ERROR on pop failure. Native top still relies on the ambient VM helper instead of its supplied receiver.

The new offline --crystal-countdown probe executes the original packaged stage script and captures all SE-120 calls over 900 frames. It independently checks them against the original script's generated timestamp array, without substituting a new timer or sorting/fixing any source timestamp.
