#include "kinoko/squirrel_host_compat.h"
#include "kinoko/map_manager_records.hpp"
#include "kinoko/map_containers.h"
#include "kinoko/act_resource.h"
#include <cstdlib>

using namespace kinoko::map;
extern "C" KinokoMapManager* kinoko_map_manager_assign(KinokoMapManager *destination, KinokoMapManager *source) {
    const ManagerView out(destination), in(source);
    kinoko_sqplus_object_assign((void *)(destination), (const void *)(source));
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
    kinoko_map_containers_assign(destination, source);
    out.set(&ManagerRecord::unknown52, in.get(&ManagerRecord::unknown52));
    out.set(&ManagerRecord::last_id, in.get(&ManagerRecord::last_id));
    out.set(&ManagerRecord::last_bounds, in.get(&ManagerRecord::last_bounds));
    out.set(&ManagerRecord::width, in.get(&ManagerRecord::width));
    out.set(&ManagerRecord::height, in.get(&ManagerRecord::height));
    return destination;
}
