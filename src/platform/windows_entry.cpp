#include "kinoko/diagnostics.h"

#include "kinoko/application.h"


int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
                   LPSTR command_line, int show_command) {
    kinoko_diagnostics_initialize();
    int result;
    __try {
        result = kinoko_application_run(instance, show_command);
    } __except (kinoko_report_exception(GetExceptionInformation())) {
        result = -1;
    }
    kinoko_diagnostics_shutdown();
    return result;
}
