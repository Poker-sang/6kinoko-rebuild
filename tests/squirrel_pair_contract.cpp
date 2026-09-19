#include "kinoko/squirrel_pair.hpp"
#include <cstdio>
#include <cstdlib>

using namespace kinoko::script;
namespace {
int released = 0;
void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
SQInteger release_probe(SQUserPointer user, SQInteger) {
    require(user == &released, "original instance user pointer reaches release hook");
    ++released;
    return 0;
}
void contract(HSQUIRRELVM vm) {
    const int before = released;
    int32_t bytes[2]; pair::reset(bytes);
    require(sq_type(pair::read(bytes)) == OT_NULL && data_bits(pair::read(bytes)) == 0, "pair reset");
    require(SQ_SUCCEEDED(sq_newclass(vm, SQFalse)), "source class");
    HSQOBJECT class_value; sq_getstackobj(vm, -1, &class_value);
    require(SQ_SUCCEEDED(sq_createinstance(vm, -1)), "source instance");
    int32_t user = -1;
    require(SQ_SUCCEEDED(pair::instance_address(vm, -1, &user)) && user == 0, "null native pointer");
    sq_setinstanceup(vm, -1, &released); sq_setreleasehook(vm, -1, release_probe);
    require(SQ_SUCCEEDED(pair::capture(vm, -1, bytes)), "capture does not type-pun legacy storage");
    pair::retain(vm, bytes); pair::retain(vm, bytes);
    sq_weakref(vm, -1); sq_remove(vm, -2); sq_remove(vm, -2);
    const auto top = sq_gettop(vm);
    for (unsigned i = 0; i < 32; ++i) {
        const auto info = pair::inspect_instance(vm, pair::read(bytes));
        require(info.class_address == data_bits(class_value) && info.user_address == address(&released), "source class and user diagnostics");
        require(sq_gettop(vm) == top, "diagnostics leave the source VM stack balanced");
    }
    const auto saved = pair::read(bytes);
    pair::release(vm, bytes); sq_collectgarbage(vm);
    require(released == before, "one remaining EXTERNAL reference keeps instance alive");
    require(std::memcmp(&saved, bytes, sizeof saved) == 0, "release does not reset or mutate pair");
    pair::release(vm, bytes); pair::reset(bytes); sq_collectgarbage(vm);
    require(released == before + 1, "last external reference releases instance exactly once");
    require(SQ_SUCCEEDED(sq_getweakrefval(vm, -1)) && sq_gettype(vm, -1) == OT_NULL, "weak reference invalidated by real source VM");
    sq_settop(vm, 0);
    sq_pushinteger(vm, 42); user = 123;
    require(SQ_FAILED(pair::instance_address(vm, -1, &user)) && user == 123, "type mismatch preserves caller output");
    sq_reseterror(vm);
    pair::capture(vm, -1, bytes); pair::retain(vm, bytes); pair::release(vm, bytes);
    const auto non_instance = pair::inspect_instance(vm, pair::read(bytes));
    require(!non_instance.class_address && !non_instance.user_address && sq_gettop(vm) == 1, "non-instance inspection is stack-neutral");
    sq_settop(vm, 0);
}
}
int main() {
    const auto vm = sq_open(64); require(vm != nullptr, "real vendored Squirrel VM");
    for (unsigned i = 0; i < 128; ++i) contract(vm);
    sq_close(vm);
    require(released == 128, "all instance ownership released");
    std::puts("PASS: source VM pair ownership, balanced diagnostics, class/user access, errors and weak references");
}
