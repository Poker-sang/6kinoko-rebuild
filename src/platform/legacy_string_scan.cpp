// Transitional missing-length adapter. Keep it separate from explicit-size
// string operations: recovering the old callers is NOT the same as strlen().
#include "kinoko/legacy_string.h"
#include <windows.h>
#include <cstddef>
#include <cstdint>

static int page_is_readable(const MEMORY_BASIC_INFORMATION *info)
{
    DWORD protection;

    if (info == NULL || info->State != MEM_COMMIT) {
        return 0;
    }
    protection = info->Protect & 0xffu;
    return protection != PAGE_NOACCESS && protection != PAGE_EXECUTE;
}

/* A large part of the RetDec output lost the third argument of
   std::string::assign(const char *, size_t). Keep those old call sites
   usable while the explicit-length callers are repaired incrementally. */
extern "C" uint32_t retdec_safe_c_string_length(const char *source)
{
    const unsigned char *cursor = (const unsigned char *)source;
    const uintptr_t limit = (uintptr_t)source + 0x100000u;
    uint32_t length = 0;

    if (source == NULL) {
        return 0;
    }
    while ((uintptr_t)cursor < limit && length < 0x100000u) {
        MEMORY_BASIC_INFORMATION info;
        uintptr_t region_end;
        size_t available;
        size_t index;

        if (VirtualQuery(cursor, &info, sizeof(info)) != sizeof(info) ||
            !page_is_readable(&info)) {
            return 0;
        }
        region_end = (uintptr_t)info.BaseAddress + info.RegionSize;
        if (region_end <= (uintptr_t)cursor) {
            return 0;
        }
        available = (size_t)(region_end - (uintptr_t)cursor);
        if (available > (size_t)(limit - (uintptr_t)cursor)) {
            available = (size_t)(limit - (uintptr_t)cursor);
        }
        for (index = 0; index < available; ++index) {
            if (cursor[index] == 0) {
                return length + (uint32_t)index;
            }
        }
        cursor += available;
        length += (uint32_t)available;
    }
    return 0;
}
