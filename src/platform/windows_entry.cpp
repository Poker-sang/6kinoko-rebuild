#include "kinoko/diagnostics.h"

#include <cstdint>

extern "C" int32_t _WinMain_40_16(int32_t instance, int32_t previous,
    int32_t command_line, int32_t show_command);

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
                   LPSTR command_line, int show_command) {
    kinoko_diagnostics_initialize();
    int result;
    __try {
        result = _WinMain_40_16(reinterpret_cast<int32_t>(instance),
            reinterpret_cast<int32_t>(previous),
            reinterpret_cast<int32_t>(command_line), show_command);
    } __except (kinoko_report_exception(GetExceptionInformation())) {
        result = -1;
    }
    kinoko_diagnostics_shutdown();
    return result;
}
