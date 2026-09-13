# Win32 resources

Original ../6kinoko/6kinoko.exe contains only RT_ICON 1 and RT_GROUP_ICON 101, language 1041 (Japanese). The icon is a 32x32 8-bit DIB. It contains no VERSIONINFO, menu, dialog, bitmap or manifest resources. PE resource inventory and hashes are in original-resources.json. Identity/import evidence carries forward from preceding reports; this is resource reconstruction, with no game behavior change.

The current window class already calls LoadIconA(instance,101) for its large and small icons, but the rebuilt EXE contains only the toolchain asInvoker manifest. Reconstruct kinoko.ico from the original group entry and image payload, preserving image bytes; compile it with the original group ID/language through Windows RC. Share IDI_KINOKO with the existing window calls instead of numeric literals.

CMake enables RC and limits C/C++-specific compiler flags to their languages, preserving those flags for game code and avoiding passing them to rc.exe. Include the resource directory for reliable icon lookup independent of the build directory. Do not invent original version/copyright fields or add DPI/compatibility declarations. Keep the existing toolchain manifest.

Commit before producing independent diagnostic/quiet builds. Verify resource payload hashes, group ID/language, unchanged manifest and actual Windows icon loading without launching the game.
