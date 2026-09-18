/* Register-only compatibility boundaries. These remain C on purpose:
 * _memcpy2 receives an implicit caller frame; __ftol consumes x87 ST(0).
 * Recover every caller before replacing these with explicit C++ arguments.
 * Ordinary CRT/Windows wrappers now live in platform/retdec_runtime_compat.cpp.
 */
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Shared Windows memory-query helper, implemented in C++. */
int retdec_valid_range(const void *address, size_t size, int writeable);

static int32_t retdec_memcpy2_from_frame(uint32_t *caller_frame)
{
    int best_score = -1;
    void *best_destination = NULL;
    const void *best_source = NULL;
    size_t best_size = 0;
    int offset;

    if (caller_frame == NULL) {
        return 0;
    }

    /* RetDec writes [destination, source, count] into adjacent words and
       emits a zero-argument call. Search the caller frame for that pattern. */
    for (offset = -0x800; offset <= 0x100; offset += 4) {
        uint32_t *slot = (uint32_t *)((unsigned char *)caller_frame + offset);
        uintptr_t destination_value;
        uintptr_t source_value;
        size_t count;
        int score;

        if (!retdec_valid_range(slot, 12, 0)) {
            continue;
        }
        destination_value = slot[0];
        source_value = slot[1];
        count = (size_t)slot[2];
        if (destination_value == 0 || source_value == 0 || count > 0x04000000u) {
            continue;
        }
        if (!retdec_valid_range((const void *)source_value, count == 0 ? 1 : count, 0) ||
            !retdec_valid_range((void *)destination_value, count == 0 ? 1 : count, 1)) {
            continue;
        }
        score = 1000 - abs(offset + 0x14);
        if (count <= 0x1000u) {
            score += 30;
        }
        if (destination_value == source_value) {
            score -= 10;
        }
        if (score > best_score) {
            best_score = score;
            best_destination = (void *)destination_value;
            best_source = (const void *)source_value;
            best_size = count;
        }
    }

    if (best_destination == NULL) {
        return 0;
    }
    if (best_size != 0) {
        memmove(best_destination, best_source, best_size);
    }
    return (int32_t)(uintptr_t)best_destination;
}

#if defined(_M_IX86)
__declspec(naked) int32_t _memcpy2(void)
{
    __asm {
        push ebp
        call retdec_memcpy2_from_frame
        add esp, 4
        ret
    }
}
#else
int32_t _memcpy2(void)
{
    return 0;
}
#endif

int32_t __ftol(void)
{
#if defined(_M_IX86)
    int32_t result = 0;
    __asm {
        fistp result
    }
    return result;
#else
    return 0;
#endif
}
