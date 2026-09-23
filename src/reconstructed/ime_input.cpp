#include "kinoko/ime_input.h"
#include <windows.h>
#include <imm.h>
#include <array>
#include <algorithm>
#include <cstring>

extern "C" {
extern int32_t g534, g535, g545, g546, g550;
extern char *g767;
extern char g547, g548, g549;
}
namespace {
// The original IME ABI exposes scalar control slots to the host. This view
// gives those borrowed slots field names while this module owns the text,
// composition and attribute buffers below.
struct ImeControl {
    int32_t& input_context() const { return g534; }
    int32_t& default_window() const { return g535; }
    int32_t& text_limit() const { return g545; }
    int32_t& commit_pending() const { return g546; }
    char& text_changed() const { return g547; }
    char& composition_changed() const { return g548; }
    char& enabled() const { return g549; }
    int32_t& cursor() const { return g550; }
    HWND game_window() const { return reinterpret_cast<HWND>(g767); }
} control;
// Original 50F6F8 text, 50FEF8 composition and 50FFF8 attributes. The
// recovered C declarations represented these buffers as individual scalars.
std::array<char, 1024> text{};
std::array<char, 256> composition{};
std::array<unsigned char, 256> attributes{};
bool lead(unsigned char c) {
    return (c >= 0x81 && c < 0xa0) || (c >= 0xe0 && c < 0xff);
}
void terminate_text() {
    if (lead(static_cast<unsigned char>(text[control.text_limit() - 1]))) text[control.text_limit() - 1] = 0;
    text[control.text_limit()] = 0;
}
HIMC context() { return reinterpret_cast<HIMC>(static_cast<intptr_t>(control.input_context())); }
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
    strcpy_s(tail.data(), tail.size(), text.data() + control.cursor());
    auto available = 1024 - control.cursor();
    strncpy_s(text.data() + control.cursor(), available, composition.data(),
        (std::min)(available, static_cast<int>(std::strlen(composition.data()))));
    control.cursor() += count;
    available = 1024 - control.cursor();
    strncpy_s(text.data() + control.cursor(), available, tail.data(),
        (std::min)(available, static_cast<int>(std::strlen(tail.data()))));
    if (control.cursor() > control.text_limit()) control.cursor() -= count;
    terminate_text();
    composition.fill(0);
    control.text_changed() = 1;
}
void update_composition() {
    if (ImmGetCompositionStringA(context(), GCS_RESULTREADSTR, nullptr, 0) != 0)
        result_string();
    if (read_composition(GCS_COMPSTR) < 0) return;
    ImmGetCompositionStringA(context(), GCS_COMPATTR, attributes.data(), 255);
    ImmGetCompositionStringA(context(), GCS_CURSORPOS, nullptr, 0);
    control.composition_changed() = 1;
}
void insert_character(char value) {
    if (composition[0] != 0) return;
    composition[1] = 0;
    std::array<char, 1024> tail{};
    strcpy_s(tail.data(), tail.size(), text.data() + control.cursor());
    text[control.cursor()++] = value;
    const auto available = 1024 - control.cursor();
    strncpy_s(text.data() + control.cursor(), available, tail.data(),
        (std::min)(available, static_cast<int>(std::strlen(tail.data()))));
    if (control.cursor() > control.text_limit()) --control.cursor();
    terminate_text();
    control.text_changed() = 1;
}
void move_cursor(int direction) {
    // 413000 takes direction in EAX; both dispatcher calls pass -1.
    if (direction < 0) {
        if (control.cursor() < 2) control.cursor() = 0;
        else control.cursor() -= lead(static_cast<unsigned char>(text[control.cursor() - 2])) ? 2 : 1;
    } else if (direction > 0 && text[control.cursor()] != 0) {
        if (control.cursor() >= 1022) control.cursor() = 1023;
        control.cursor() += lead(static_cast<unsigned char>(text[control.cursor()])) ? 2 : 1;
        if (control.cursor() > control.text_limit()) control.cursor() = control.text_limit();
    }
}
void delete_character() {
    if (text[control.cursor()] == 0) return;
    std::array<char, 1024> tail{};
    const auto width = lead(static_cast<unsigned char>(text[control.cursor()])) ? 2 : 1;
    strcpy_s(tail.data(), tail.size(), text.data() + control.cursor() + width);
    text[control.cursor()] = 0;
    strncpy_s(text.data() + control.cursor(), 1024 - control.cursor(), tail.data(),
        (std::min)(1023 - control.cursor(), static_cast<int>(std::strlen(tail.data())) + 2));
    terminate_text();
    control.text_changed() = 1;
}
void commit() {
    if (control.text_changed() == 0 && control.composition_changed() == 0) {
        composition.fill(0);
        control.commit_pending() = 1;
    }
}
}
// 412CA0: initialize the original window's input context and candidate area.
extern "C" int32_t kinoko_ime_initialize(void) {
    const auto window=control.game_window();
    const auto input=ImmGetContext(window);
    control.input_context()=static_cast<int32_t>(reinterpret_cast<intptr_t>(input));
    control.default_window()=static_cast<int32_t>(reinterpret_cast<intptr_t>(ImmGetDefaultIMEWnd(window)));
    RECT bounds{};
    GetWindowRect(window,&bounds);
    CANDIDATEFORM candidate{};
    candidate.dwIndex=0;
    candidate.dwStyle=CFS_EXCLUDE;
    candidate.rcArea={0,0,bounds.right-bounds.left,bounds.bottom-bounds.top};
    return ImmSetCandidateWindow(input,&candidate);
}
extern "C" int32_t kinoko_ime_dispatch(int32_t window, uint32_t message,
                                      uint32_t key, int32_t parameter) {
    if (control.enabled() == 0) return false;
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
            if (control.cursor() > 0) { move_cursor(-1); delete_character(); }
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
        case VK_ESCAPE: control.enabled() = 0; return true;
        default: insert_character(static_cast<char>(key)); return false;
        }
    default: return false;
    }
}
