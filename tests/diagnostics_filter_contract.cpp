#include "kinoko/diagnostics_filter.hpp"
#include <cstdio>
#include <initializer_list>
#include <stdexcept>
using kinoko::diagnostics::TraceFilter;
using kinoko::diagnostics::accepts;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void contracts() {
    for (bool verbose : {false, true}) {
        for (auto error : {"stagevm:failure:case", "stagevm:compile-error:case",
                          "actor:update-failed:case", "seh:case", "veh:case"})
            require(accepts(error, verbose, TraceFilter::errors_only), "errors-only lost an error");
        for (auto noise : {"game:frame", "scene:load", "map:path", "savedata:read",
                          "actor:star", "bgm:write", "random:trace", ""})
            require(!accepts(noise, verbose, TraceFilter::errors_only), "errors-only leaked normal logs");
    }
    for (auto mode : {TraceFilter::all, TraceFilter::graphics_audio_input, TraceFilter::star}) {
        require(accepts("scene:load", false, mode), "nonverbose scene policy changed");
        require(accepts("stagevm:compile-error", false, mode), "nonverbose error policy changed");
        require(!accepts("draw:frame", false, mode), "nonverbose frame policy changed");
    }
    require(accepts("arbitrary diagnostic", true, TraceFilter::all), "verbose all");
    require(accepts("actor:star:spawn", true, TraceFilter::star), "star filter");
    require(!accepts("actor:walk", true, TraceFilter::star), "star filter scope");
    for (auto selected : {"draw:frame", "bgm:write", "audio:buffer", "input:poll", "stagevm:failure"})
        require(accepts(selected, true, TraceFilter::graphics_audio_input), "graphics/audio/input filter");
    require(!accepts("unselected:trace", true, TraceFilter::graphics_audio_input), "filter scope");
    require(!accepts("se", true, TraceFilter::errors_only), "short prefix");
}
}
int main() {
    try {
        contracts();
        std::puts("PASS: diagnostic filter priority and retained trace selection");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Diagnostic filter failed: %s\n", error.what());
        return 1;
    }
}
