# Win32 resources

Original ../6kinoko/6kinoko.exe contains only RT_ICON 1 and RT_GROUP_ICON 101, language 1041 (Japanese). The icon is a 32x32 8-bit DIB. It contains no VERSIONINFO, menu, dialog, bitmap or manifest resources. PE resource inventory and hashes are in original-resources.json. Identity/import evidence carries forward from preceding reports; this is resource reconstruction, with no game behavior change.

The current window class already calls LoadIconA(instance,101) for its large and small icons, but the rebuilt EXE contains only the toolchain asInvoker manifest. Reconstruct kinoko.ico from the original group entry and image payload, preserving image bytes; compile it with the original group ID/language through Windows RC. Share IDI_KINOKO with the existing window calls instead of numeric literals.

CMake enables RC and limits C/C++-specific compiler flags to their languages, preserving those flags for game code and avoiding passing them to rc.exe. Include the resource directory for reliable icon lookup independent of the build directory. Do not invent original version/copyright fields or add DPI/compatibility declarations. Keep the existing toolchain manifest.

Commit before producing independent diagnostic/quiet builds. Verify resource payload hashes, group ID/language, unchanged manifest and actual Windows icon loading without launching the game.

## Verification and delivery

Source checkpoint e83c7c4. Both win32-resources-20260913-quiet and -diag build successfully and pass CTest 8/8. Resource verification compares raw RT_ICON and RT_GROUP_ICON payloads against the original EXE byte-for-byte, including group ID 101, image ID 1 and Japanese language 1041. The only additional rebuilt resource is the pre-existing toolchain asInvoker manifest, whose payload also matches the prior build exactly.

Using LoadLibraryExW(LOAD_LIBRARY_AS_DATAFILE), Windows LoadIconW(instance,101) succeeds, and LoadImageW loads 16x16 and 32x32 icons successfully. This tests the resource loader without executing either game. Details/hashes are in verification.json; verify_resources.py reproduces the checks with pefile and Windows APIs. No automated gameplay run was needed for this resource-only change.

All three original DATs are staged beside both final game EXEs and hash-verified. The prior orange-platform quiet save is copied to both new directories; originals and old products are retained. Each runtime validation.json records source revision, executable hash, resource validation and test result. No version metadata is fabricated: the original contains none.

Checklist: original resource inventory complete; original icon reconstructed with identical payload; RC included in CMake; language-specific compiler flags preserved; Windows resource load verified; manifest unchanged; both regression suites pass; DAT/save staging complete; all artifacts retained; source committed before builds.
