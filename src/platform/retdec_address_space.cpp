#include <windows.h>
#include <cstdint>
#include <cstddef>

// Shared production address validation, independent of CRT emulation and the
// unresolved legacy scanner. Keep its single-region/protection rules intact.
extern "C" int retdec_valid_range(const void *address, size_t size, int writeable)
{
    MEMORY_BASIC_INFORMATION info;
    uintptr_t start = (uintptr_t)address;
    uintptr_t end;
    uintptr_t region_end;
    DWORD protection;

    if (address == nullptr) {
        return size == 0;
    }
    if (VirtualQuery(address, &info, sizeof(info)) == 0 ||
        info.State != MEM_COMMIT) {
        return 0;
    }
    if (size > (uintptr_t)-1 - start) {
        return 0;
    }
    end = start + size;
    region_end = (uintptr_t)info.BaseAddress + info.RegionSize;
    if (end > region_end) {
        return 0;
    }
    protection = info.Protect & 0xffu;
    if (protection == PAGE_NOACCESS || (info.Protect & PAGE_GUARD) != 0) {
        return 0;
    }
    if (writeable) {
        return protection == PAGE_READWRITE ||
               protection == PAGE_WRITECOPY ||
               protection == PAGE_EXECUTE_READWRITE ||
               protection == PAGE_EXECUTE_WRITECOPY;
    }
    return 1;
}
