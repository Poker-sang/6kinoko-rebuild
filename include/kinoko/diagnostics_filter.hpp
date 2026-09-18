#pragma once
#include <string_view>

namespace kinoko::diagnostics {
enum class TraceFilter { all, graphics_audio_input, star, errors_only };

inline bool starts_with(std::string_view message, std::string_view prefix) noexcept {
    return message.size() >= prefix.size() && message.compare(0, prefix.size(), prefix) == 0;
}
template <std::size_t N>
inline bool matches(std::string_view message, const std::string_view (&prefixes)[N]) noexcept {
    for (auto prefix : prefixes)
        if (starts_with(message, prefix)) return true;
    return false;
}
inline bool accepts(std::string_view message, bool verbose, TraceFilter filter) noexcept {
    // An explicit errors-only policy must win over the normal nonverbose
    // scene/frame selection, including when KINOKO_TRACE_VERBOSE is unset.
    if (filter == TraceFilter::errors_only) {
        constexpr std::string_view prefixes[] = {"stagevm:failure", "stagevm:compile-error",
            "actor:update-failed", "seh:", "veh:"};
        return matches(message, prefixes);
    }
    if (!verbose) {
        constexpr std::string_view prefixes[] = {"stagevm:failure", "stagevm:compile-error",
            "seh:", "veh:", "actor:update-failed", "game:", "scene:", "map:path", "savedata:"};
        return matches(message, prefixes);
    }
    if (filter == TraceFilter::star) {
        constexpr std::string_view prefixes[] = {"actor:star", "actor:invalid", "actor:update-failed",
            "stagevm:failure", "map:path", "veh:", "seh:"};
        return matches(message, prefixes);
    }
    if (filter == TraceFilter::graphics_audio_input) {
        constexpr std::string_view prefixes[] = {"seh:", "veh:", "4011b0:", "4017b0:",
            "render-target:", "map:", "mcd:", "draw:", "act:", "actor:",
            "actor-create:", "actor-update:", "actor-manager:", "native-471df0:",
            "native-userdata:", "c2d:", "4525d0:", "40d790:", "408b30:",
            "411d80:", "40b520:", "4701e0:", "470220:", "470290:", "470300:",
            "470320:", "470360:", "470980:", "game:", "scene:", "stagevm:",
            "savedata:", "prepcall-beginstage-", "call-initstage-", "bgm:",
            "audio:", "input:"};
        return matches(message, prefixes);
    }
    return true;
}
} // namespace kinoko::diagnostics
