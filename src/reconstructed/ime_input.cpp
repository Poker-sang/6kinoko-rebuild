#include "kinoko/ime_input.h"
#include <windows.h>
#include <imm.h>
#include <array>
#include <algorithm>
#include <cstring>

extern "C" {
extern int32_t g534, g545, g546, g550;
extern char g547, g548, g549;
}
namespace {
// Original 50F6F8 text, 50FEF8 composition and 50FFF8 attributes. The
// recovered C declarations represented these buffers as individual scalars.
std::array<char, 1024> text{};
std::array<char, 256> composition{};
std::array<unsigned char, 256> attributes{};
bool lead(unsigned char c) {
    return (c >= 0x81 && c < 0xa0) || (c >= 0xe0 && c < 0xff);
}
void terminate_text() {
    if (lead(static_cast<unsigned char>(text[g545 - 1]))) text[g545 - 1] = 0;
    text[g545] = 0;
}
HIMC context() { return reinterpret_cast<HIMC>(static_cast<intptr_t>(g534)); }
LONG read_composition(DWORD kind) {
    const LONG count = ImmGetCompositionStringA(context(), kind, composition.data(), 255);
    // IMM_ERROR_NODATA/GENERAL are signed errors, never buffer indices.
    if (count < 0 || count > 255) return -1;
    composition[count] = 0;
    return count;
}
void result_string() {
    const LONG count = read_composition(GCS_RESULTSTR);
    if (count < 0) return;
    std::array<char, 1024> tail{};
    strcpy_s(tail.data(), tail.size(), text.data() + g550);
    auto available = 1024 - g550;
    strncpy_s(text.data() + g550, available, composition.data(),
        (std::min)(available, static_cast<int>(std::strlen(composition.data()))));
    g550 += count;
    available = 1024 - g550;
    strncpy_s(text.data() + g550, available, tail.data(),
        (std::min)(available, static_cast<int>(std::strlen(tail.data()))));
    if (g550 > g545) g550 -= count;
    terminate_text();
    composition.fill(0);
    g547 = 1;
}
void update_composition() {
    if (ImmGetCompositionStringA(context(), GCS_RESULTREADSTR, nullptr, 0) != 0)
        result_string();
    if (read_composition(GCS_COMPSTR) < 0) return;
    ImmGetCompositionStringA(context(), GCS_COMPATTR, attributes.data(), 255);
    ImmGetCompositionStringA(context(), GCS_CURSORPOS, nullptr, 0);
    g548 = 1;
}
void insert_character(char value) {
    if (composition[0] != 0) return;
    composition[1] = 0;
    std::array<char, 1024> tail{};
    strcpy_s(tail.data(), tail.size(), text.data() + g550);
    text[g550++] = value;
    const auto available = 1024 - g550;
    strncpy_s(text.data() + g550, available, tail.data(),
        (std::min)(available, static_cast<int>(std::strlen(tail.data()))));
    if (g550 > g545) --g550;
    terminate_text();
    g547 = 1;
}
void move_cursor(int direction) {
    // 413000 takes direction in EAX; both dispatcher calls pass -1.
    if (direction < 0) {
        if (g550 < 2) g550 = 0;
        else g550 -= lead(static_cast<unsigned char>(text[g550 - 2])) ? 2 : 1;
    } else if (direction > 0 && text[g550] != 0) {
        if (g550 >= 1022) g550 = 1023;
        g550 += lead(static_cast<unsigned char>(text[g550])) ? 2 : 1;
        if (g550 > g545) g550 = g545;
    }
}
void delete_character() {
    if (text[g550] == 0) return;
    std::array<char, 1024> tail{};
    const auto width = lead(static_cast<unsigned char>(text[g550])) ? 2 : 1;
    strcpy_s(tail.data(), tail.size(), text.data() + g550 + width);
    text[g550] = 0;
    strncpy_s(text.data() + g550, 1024 - g550, tail.data(),
        (std::min)(1023 - g550, static_cast<int>(std::strlen(tail.data())) + 2));
    terminate_text();
    g547 = 1;
}
void commit() {
    if (g547 == 0 && g548 == 0) {
        composition.fill(0);
        g546 = 1;
    }
}
}
extern "C" int32_t kinoko_ime_dispatch(int32_t window, uint32_t message,
                                      uint32_t key, int32_t parameter) {
    if (g549 == 0) return false;
    switch (message) {
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_COMPOSITION:
        if (parameter & GCS_COMPSTR) update_composition();
        return true;
    case WM_IME_ENDCOMPOSITION: result_string(); return true;
    case WM_IME_SETCONTEXT:
        DefWindowProcA(reinterpret_cast<HWND>(static_cast<intptr_t>(window)),
            message, key, parameter & 0x3ffffff0);
        return true;
    case WM_IME_NOTIFY: return key != 0 && key <= 5;
    case WM_KEYDOWN:
        switch (key) {
        case VK_BACK:
            if (g550 > 0) { move_cursor(-1); delete_character(); }
            return true;
        case VK_RETURN: commit(); return true;
        case VK_LEFT: move_cursor(-1); return true;
        case VK_RIGHT: move_cursor(1); return true;
        case VK_DELETE: delete_character(); return true;
        default: {
            std::array<WORD, 10> translated{};
            return ToAscii(key, parameter >> 16, nullptr, translated.data(), 1) <= 0;
        }
        }
    case WM_CHAR:
        switch (key) {
        case VK_BACK: case VK_TAB: return true;
        case VK_RETURN: commit(); return false;
        case VK_ESCAPE: g549 = 0; return true;
        default: insert_character(static_cast<char>(key)); return false;
        }
    default: return false;
    }
}
