#include <stdint.h>
#include <windows.h>

extern int32_t _WinMain_40_16(int32_t instance, int32_t previous,
                              int32_t command_line, int32_t show_command);

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show_command)
{
    (void)previous;
    (void)command_line;
    (void)show_command;
    return (int)_WinMain_40_16((int32_t)(uintptr_t)instance,
                               (int32_t)(uintptr_t)previous,
                               (int32_t)(uintptr_t)command_line,
                               show_command);
}
