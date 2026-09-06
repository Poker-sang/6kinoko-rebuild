# Upstream source

Unmodified codec sources from https://zlib.net/fossils/zlib-1.2.3.tar.gz.
Archive SHA256: 1795c7d067a43174113fdf03447532f373e1c6c57c08d61d9e4e9be5e244b05e.
The upstream README and source headers contain the zlib license.

The original 6kinoko.exe embeds version 1.2.3 and calls deflate with default
compression / Z_FINISH and inflate with Z_NO_FLUSH. The reconstructed wrappers
404390 and 404430 use these same source routines. No DLL is needed at runtime.
