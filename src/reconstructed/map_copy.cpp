#include "kinoko/squirrel_host_compat.h"
#include "kinoko/map_manager_records.hpp"
#include "kinoko/map_containers.h"
#include "kinoko/act_resource.h"
#include "kinoko/legacy_memory.hpp"
#include <cstdlib>

using namespace kinoko::map;
using kinoko::legacy::address;
extern "C" void kinoko_map_manager_assign(KinokoMapManager *destination, KinokoMapManager *source) {
    const ManagerView out(destination), in(source);
    (int32_t)(intptr_t)(kinoko_sqplus_object_assign((void *)(destination), (const void *)(source)));
    out.set(&ManagerRecord::source_act, in.get(&ManagerRecord::source_act));
    out.set(&ManagerRecord::source_holder, in.get(&ManagerRecord::source_holder));
    // 470100 copies the two raw owners but transfers auto_ptr's runtime.
    // Preserve even self-copy: clearing source first prevents destruction.
    auto *player = in.get(&ManagerRecord::player);
    in.set(&ManagerRecord::player, static_cast<KinokoActRuntime *>(nullptr));
    auto *previous = out.get(&ManagerRecord::player);
    if (player != previous && previous) {
        kinoko_act_runtime_dispose(previous);
        std::free(previous);
    }
    out.set(&ManagerRecord::player, player);
    kinoko_map_containers_assign(address(destination), address(source));
    out.set(&ManagerRecord::unknown52, in.get(&ManagerRecord::unknown52));
    out.set(&ManagerRecord::last_id, in.get(&ManagerRecord::last_id));
    out.set(&ManagerRecord::last_bounds, in.get(&ManagerRecord::last_bounds));
    out.set(&ManagerRecord::width, in.get(&ManagerRecord::width));
    out.set(&ManagerRecord::height, in.get(&ManagerRecord::height));
}
// SqPlus's explicit destination/source callback (4701B0 -> 470100).
extern "C" int32_t function_4701b0(int32_t destination, int32_t source) {
    kinoko_map_manager_assign(kinoko::legacy::pointer<KinokoMapManager>(destination),
        kinoko::legacy::pointer<KinokoMapManager>(source));
    return destination;
}
