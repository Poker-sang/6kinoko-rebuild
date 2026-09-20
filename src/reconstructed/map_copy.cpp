#include "kinoko/squirrel_host_compat.h"
#include "kinoko/map_containers.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>

extern "C" {
extern unsigned char g37;
int32_t function_450020(int32_t player);
}

namespace {
struct Manager {
    unsigned char script_object[12];
    int32_t source_act, source_holder, player;
    int32_t container_state;
    int32_t reserved[6];
    int32_t fields[8];
};
static_assert(offsetof(Manager,container_state)==24);
static_assert(offsetof(Manager,fields)==52 && sizeof(Manager)==84);
}

// SqPlus's explicit destination/source callback (4701B0 -> 470100).
extern "C" int32_t function_4701b0(int32_t destination,int32_t source) {
    auto& out=*reinterpret_cast<Manager*>(destination);
    auto& in=*reinterpret_cast<Manager*>(source);
    function_4a95c0_this(destination,source);
    out.source_act=in.source_act;
    out.source_holder=in.source_holder;
    // Original auto_ptr-style transfer is intentional, including self-copy.
    const int32_t player=in.player;
    in.player=0;
    if(player!=out.player && out.player) {
        function_450020(out.player);
        std::free(reinterpret_cast<void*>(out.player));
    }
    out.player=player;
    kinoko_map_containers_assign(destination,source);
    if(destination!=source) std::copy_n(in.fields,8,out.fields);
    return destination;
}
