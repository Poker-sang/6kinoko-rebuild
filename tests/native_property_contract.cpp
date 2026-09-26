#include "kinoko/sqrat_object_bridge.h"
#include "kinoko/native_property_bridge.h"
#include "kinoko/native_property_callbacks.h"
#include "kinoko/legacy_string.hpp"
#include "squirrel_bridge_test_support.hpp"
#include <limits>

extern "C" {
char kinoko_sqrat_trace_enabled = 0;
int32_t kinoko_sqrat_object_vtable(void) { return 0x12121212; }
int32_t kinoko_sqrat_root_vtable(void) { return 0x34343434; }
void kinoko_trace_i32(const char*, int32_t) {}
void kinoko_trace_squirrel_name(const char*, int32_t) {}

}
namespace {
using namespace bridge_test;
using Callback = int32_t(*)(int32_t);
int32_t callback_address(Callback cb) { return address(reinterpret_cast<void*>(cb)); }
class Fixture final {
public:
    explicit Fixture(HSQUIRRELVM vm) : vm(vm), instance(vm), methods(vm) {
        native.fill(0xa7); alias.fill(0xb6);
        require(SQ_SUCCEEDED(sq_newclass(vm, SQFalse)), "create actual source class");
        require(SQ_SUCCEEDED(sq_createinstance(vm, -1)), "source native instance");
        sq_setinstanceup(vm, -1, object()); instance.capture(); sq_pop(vm, 1);
        require(kinoko_sqrat_new_table((struct SQVM *)(vm), methods.data()), "property callback table");
    }
    unsigned char* object() { return native.data() + 1; } // Unaligned on purpose.
    unsigned char* aliased() { return alias.data() + 1; }
    void bind(const char* name, int32_t offset, Callback cb) {
        require(kinoko_sqrat_set_offset_closure((struct SQVM *)(vm), methods.data(), name, offset, (void *)(intptr_t)(callback_address(cb))), "bind actual captured descriptor");
    }
    void bind(const char* name, int32_t offset, SQFUNCTION cb) {
        require(kinoko_sqrat_set_offset_closure((struct SQVM *)(vm), methods.data(), name, offset, (void *)(intptr_t)(address(reinterpret_cast<void*>(cb)))), "bind typed SQFUNCTION without an original code address");
    }
    void prepare(const char* name) {
        methods.push(); sq_pushstring(vm, name, -1);
        require(SQ_SUCCEEDED(sq_get(vm, -2)), "find property native closure");
        sq_remove(vm, -2); instance.push();
    }
    void call(SQInteger arguments, bool result) {
        const auto previous = receiver;
        const auto status = kinoko_sq_call((SQVM*)(uintptr_t)(address(vm)), arguments, result, SQFalse);
        if (SQ_FAILED(status)) last_error(vm);
        require(SQ_SUCCEEDED(status), "property call succeeds");
        require(receiver == previous, "property call restores receiver");
        // sq_call leaves the closure below the optional return value.
        if (result) sq_remove(vm, -2); else sq_pop(vm, 1);
    }
    void set_int(const char* name, SQInteger value) { prepare(name); sq_pushinteger(vm, value); call(2, false); }
    void set_float(const char* name, SQFloat value) { prepare(name); sq_pushfloat(vm, value); call(2, false); }
    void set_string(const char* name, const char* value) { prepare(name); sq_pushstring(vm, value, -1); call(2, false); }
    SQInteger get_int(const char* name) { prepare(name); call(1, true); const auto value = get_integer(vm); sq_pop(vm, 1); return value; }
    SQFloat get_float(const char* name) { prepare(name); call(1, true); const auto value = bridge_test::get_float(vm); sq_pop(vm, 1); return value; }
    bool get_bool(const char* name) { prepare(name); call(1, true); SQBool value; require(SQ_SUCCEEDED(sq_getbool(vm, -1, &value)), "bool result"); sq_pop(vm, 1); return value != 0; }
    std::string get_string(const char* name) { prepare(name); call(1, true); const auto value = bridge_test::get_string(vm); sq_pop(vm, 1); return value; }
    void require_canaries() {
        require(native.front() == 0xa7 && native.back() == 0xa7 && alias.front() == 0xb6 && alias.back() == 0xb6, "native layout canaries");
    }
    HSQUIRRELVM vm;
    Pair instance, methods;
    std::array<unsigned char, 258> native;
    std::array<unsigned char, 66> alias;
};
void scalar_fields(HSQUIRRELVM vm) {
    Top restore(vm); const auto base = sq_gettop(vm); Fixture f(vm);
    f.bind("get_i", 21, kinoko_cact_layer_get_int); f.bind("set_i", 21, kinoko_cact_layer_set_int);
    f.bind("get_f", 29, kinoko_cact_layer_get_float); f.bind("set_f", 29, kinoko_cact_layer_set_float);
    f.bind("get_b", 0x8c, kinoko_cact_layer_get_bool); f.bind("set_b", 0x8c, kinoko_cact_layer_set_bool);
    f.set_int("set_i", -123456789); require(f.get_int("get_i") == -123456789, "unaligned integer round trip");
    f.set_float("set_i", 12.75f); require(f.get_int("get_i") == 12, "property integers retain source numeric coercion");
    f.set_string("set_i", "wrong"); require(f.get_int("get_i") == 12, "wrong integer type does not overwrite");
    f.set_float("set_f", -1.25f); require(f.get_float("get_f") == -1.25f, "unaligned float round trip");
    f.set_int("set_f", 27); require(f.get_float("get_f") == 27, "integer to float conversion");
    f.set_string("set_f", "wrong"); require(f.get_float("get_f") == 27, "wrong float type does not overwrite");
    f.set_float("set_f", -0.0f); require(load<uint32_t>(f.object() + 29) == 0x80000000u, "negative-zero float bits preserved");
    store<uint8_t>(f.object() + 0x8c, 0xff); require(f.get_bool("get_b"), "boolean getter normalizes any nonzero byte");
    f.prepare("set_b"); sq_pushnull(vm); f.call(2, false); require(!f.get_bool("get_b"), "null truthiness");
    f.set_string("set_b", ""); require(f.get_bool("get_b"), "empty string truthiness follows 2.2.2");
    require(load<uint8_t>(f.object() + 0x8b) == 0xa7 && load<uint8_t>(f.object() + 0x8d) == 0xa7, "boolean touches one byte");
    f.bind("get_l_i", 21, kinoko_c2dlayout_get_int); f.bind("set_l_i", 21, kinoko_c2dlayout_set_int);
    f.bind("get_l_f", 29, kinoko_c2dlayout_get_float); f.bind("set_l_f", 29, kinoko_c2dlayout_set_float);
    f.bind("color", 21, kinoko_c2dlayout_set_color);
    f.set_int("set_l_i", -20); require(f.get_int("get_l_i") == -20, "layout integer");
    f.set_float("set_l_f", 4.5f); require(f.get_float("get_l_f") == 4.5f, "layout float");
    f.set_int("color", -12); require(f.get_int("get_l_i") == 0, "color lower clamp");
    f.set_int("color", 300); require(f.get_int("get_l_i") == 255, "color upper clamp");
    f.set_int("color", 127); require(f.get_int("get_l_i") == 127, "color middle");
    f.set_string("color", "wrong"); require(f.get_int("get_l_i") == 127, "invalid color unchanged");
    f.bind("short_get", 41, kinoko_native_view_get_short); f.bind("short_set", 41, kinoko_native_view_set_short);
    for (const auto value : {0, 32767, 32768, -1, 65537, -32769}) {
        f.set_int("short_set", value);
        const auto bits = static_cast<uint16_t>(value);
        int16_t expected; std::memcpy(&expected, &bits, 2);
        require(f.get_int("short_get") == expected, "signed-short low bits and sign extension");
    }
    require(f.object()[40] == 0xa7 && f.object()[43] == 0xa7, "short touches two bytes");
    f.require_canaries(); top(vm, base, "scalar callback stack");
}
void indirect_fields(HSQUIRRELVM vm) {
    Top restore(vm); const auto base = sq_gettop(vm); Fixture f(vm);
    store(f.object() + 0x34, f.aliased()); store(f.object() + 0x38, f.aliased() + 8);
    f.bind("pf_get", 0x34, kinoko_cact_layer_get_pointer_float); f.bind("pf_set", 0x34, kinoko_cact_layer_set_pointer_float);
    f.bind("pi_get", 0x38, kinoko_cact_layer_get_pointer_int); f.bind("pi_set", 0x38, kinoko_cact_layer_set_pointer_int);
    f.set_float("pf_set", 12.5f); require(f.get_float("pf_get") == 12.5f, "aliased float round trip");
    f.set_int("pi_set", -200); require(f.get_int("pi_get") == -200, "aliased integer round trip");
    f.set_string("pi_set", "invalid"); require(f.get_int("pi_get") == -200, "alias type mismatch does not mutate");
    const auto bytes = f.alias;
    store<void*>(f.object() + 0x34, nullptr); store<void*>(f.object() + 0x38, nullptr);
    f.set_float("pf_set", 99.0f); f.set_int("pi_set", 99);
    require(f.alias == bytes, "null alias setters do nothing");
    f.prepare("pf_get"); f.call(1, true); require(sq_gettype(vm, -1) == OT_NULL, "null alias getter returns no value"); sq_pop(vm, 1);
    f.prepare("pi_get"); f.call(1, true); require(sq_gettype(vm, -1) == OT_NULL, "null integer alias getter"); sq_pop(vm, 1);
    f.require_canaries(); top(vm, base, "indirect callback stack");
}
void player_and_strings(HSQUIRRELVM vm) {
    Top restore(vm); const auto base = sq_gettop(vm); Fixture f(vm);
    f.bind("staging_get", 8, kinoko_acting_player_get_property); f.bind("staging_set", 8, kinoko_acting_player_set_property);
    f.bind("visible_get", 132, kinoko_acting_player_get_property); f.bind("visible_set", 132, kinoko_acting_player_set_property);
    f.bind("float_get", 124, kinoko_acting_player_get_property); f.bind("float_set", 124, kinoko_acting_player_set_property);
    f.bind("int_get", 108, kinoko_acting_player_get_property); f.bind("int_set", 108, kinoko_acting_player_set_property);
    f.bind("name_get", 148, kinoko_acting_player_get_property); f.bind("name_set", 148, kinoko_acting_player_set_property);
    store(f.object() + 132, f.aliased()); store(f.object() + 124, f.aliased() + 4);
    store(f.object() + 108, f.aliased() + 8); store(f.object() + 148, f.aliased() + 16);
    f.prepare("staging_set"); sq_pushbool(vm, SQFalse); f.call(2, false); require(!f.get_bool("staging_get"), "staging is direct byte not pointer");
    f.prepare("visible_set"); sq_pushbool(vm, SQTrue); f.call(2, false); require(f.get_bool("visible_get"), "visible is aliased byte");
    f.set_float("float_set", -7.5f); require(f.get_float("float_get") == -7.5f, "player aliased float");
    f.set_int("int_set", 800); require(f.get_int("int_get") == 800, "player aliased integer");
    auto name = f.aliased() + 16;
    std::memcpy(name, "inline", 7); store<uint32_t>(name + 16, 6); store<uint32_t>(name + 20, 15);
    require(f.get_string("name_get") == "inline", "legacy inline string field");
    const char* heap = "long legacy heap string";
    store(name, heap); store<uint32_t>(name + 16, static_cast<uint32_t>(std::strlen(heap))); store<uint32_t>(name + 20, 31);
    require(f.get_string("name_get") == heap, "legacy heap string field");
    // The borrowed heap string above is a getter fixture, not writable owned
    // storage. Reset to an initialized inline record before real assignments.
    std::memset(name, 0, 24); store<uint32_t>(name + 20, 15);
    f.set_string("name_set", "new value");
    require(f.get_string("name_get") == "new value", "player string uses real native storage");
    const std::string long_name(180, 'p');
    f.set_string("name_set", long_name.c_str());
    require(f.get_string("name_get") == long_name, "player inline to heap assignment");
    const auto owned_name = kinoko::legacy::StringView(name);
    const auto allocation = owned_name.data();
    f.set_string("name_set", "short");
    require(f.get_string("name_get") == "short" && owned_name.data() == allocation,
        "short assignment retains the original heap allocation");
    f.set_int("name_set", 5);
    require(f.get_string("name_get") == "short", "string setter rejects wrong type");
    owned_name.destroy();
    f.bind("layer_name_get", 177, kinoko_cact_layer_get_string); f.bind("layer_name_set", 177, kinoko_cact_layer_set_string);
    std::memcpy(f.object() + 177, "layer", 6);
    store<uint32_t>(f.object() + 193, 5); store<uint32_t>(f.object() + 197, 15);
    require(f.get_string("layer_name_get") == "layer", "layer string inline layout");
    f.set_string("layer_name_set", "updated");
    require(f.get_string("layer_name_get") == "updated", "layer string real assignment");
    f.set_string("layer_name_set", long_name.c_str());
    require(f.get_string("layer_name_get") == long_name, "unaligned layer heap assignment");
    f.set_string("layer_name_set", "");
    require(f.get_string("layer_name_get").empty(), "layer empty assignment");
    kinoko::legacy::StringView(f.object() + 177).destroy();
    store<void*>(f.object() + 177, nullptr); store<uint32_t>(f.object() + 197, 31);
    require(f.get_string("layer_name_get").empty(), "layer null heap pointer becomes empty text");
    store<void*>(name, nullptr); store<uint32_t>(name + 20, 31);
    require(kinoko_string_data((const void*)(name)) == nullptr, "string view does not invent heap buffer");
    require(kinoko_string_data(0) == nullptr, "null legacy string view");
    f.require_canaries(); top(vm, base, "player/string stack");
}
void typed_source_callbacks(HSQUIRRELVM vm) {
    Top restore(vm); const auto base = sq_gettop(vm); Fixture f(vm);
    f.bind("i", 17, kinoko_sqrat_get_int); f.bind("set_i", 17, kinoko_sqrat_set_int);
    f.bind("f", 25, kinoko_sqrat_get_float); f.bind("set_f", 25, kinoko_sqrat_set_float);
    f.bind("b", 33, kinoko_sqrat_get_bool); f.bind("set_b", 33, kinoko_sqrat_set_bool);
    f.bind("s", 37, kinoko_sqrat_get_short); f.bind("set_s", 37, kinoko_sqrat_set_short);
    f.bind("pi", 45, kinoko_sqrat_get_pointer_int); f.bind("set_pi", 45, kinoko_sqrat_set_pointer_int);
    f.bind("pf", 53, kinoko_sqrat_get_pointer_float); f.bind("set_pf", 53, kinoko_sqrat_set_pointer_float);
    f.bind("noop", 0, kinoko_sqrat_noop);
    store(f.object() + 45, f.aliased()); store(f.object() + 53, f.aliased() + 8);
    for (const auto value : {std::numeric_limits<SQInteger>::min(), SQInteger{-17}, SQInteger{0},
                             std::numeric_limits<SQInteger>::max()}) {
        f.set_int("set_i", value); f.set_int("set_pi", value);
        require(f.get_int("i") == value && f.get_int("pi") == value, "typed signed integer and indirect fields");
    }
    for (uint32_t bits : {0u, 0x80000000u, 0x3fc00000u, 0xc0600000u, 0x7f800000u, 0xff800000u, 0x7fc01234u}) {
        SQFloat value; std::memcpy(&value, &bits, sizeof(value));
        f.set_float("set_f", value); f.set_float("set_pf", value);
        const auto direct = f.get_float("f"), indirect = f.get_float("pf");
        require(load<uint32_t>(&direct) == bits && load<uint32_t>(&indirect) == bits,
                "typed floats preserve fractions, signed zero, infinity and quiet NaN payloads");
    }
    f.set_float("set_i", -12.75f); require(f.get_int("i") == -12, "typed integer uses source numeric conversion");
    f.set_int("set_f", 27); require(f.get_float("f") == 27, "typed float accepts a source integer");
    f.set_string("set_i", "invalid"); f.set_string("set_f", "invalid");
    require(f.get_int("i") == -12 && f.get_float("f") == 27, "typed invalid numbers leave fields unchanged");
    f.prepare("set_b"); sq_pushnull(vm); f.call(2, false); require(!f.get_bool("b"), "typed null is false");
    f.set_string("set_b", ""); require(f.get_bool("b"), "typed empty string follows Squirrel truthiness");
    require(f.object()[32] == 0xa7 && f.object()[34] == 0xa7, "typed bool changes one byte");
    f.set_int("set_s", -32769); require(f.get_int("s") == 32767, "typed short truncates to signed low 16 bits");
    require(f.object()[36] == 0xa7 && f.object()[39] == 0xa7, "typed short changes two bytes");
    store<void*>(f.object() + 45, nullptr); store<void*>(f.object() + 53, nullptr);
    const auto old_alias = f.alias;
    f.set_int("set_pi", 900); f.set_float("set_pf", 5.5f);
    require(f.alias == old_alias, "typed null indirect setters do not write");
    for (const auto name : {"pi", "pf", "noop"}) {
        f.prepare(name); f.call(1, true);
        require(sq_gettype(vm, -1) == OT_NULL, "typed absent result maps to VM null"); sq_pop(vm, 1);
    }
    f.require_canaries(); top(vm, base, "typed callback stack");
}
int conversion_calls = 0;
SQInteger conversion_meta(HSQUIRRELVM vm) {
    ++conversion_calls;
    SQInteger mode = 0; sq_getinteger(vm, -1, &mode);
    if (mode == 2) return sq_throwerror(vm, "conversion sentinel");
    if (mode == 1) sq_pushinteger(vm, 777); // Invalid metamethod result: fallback.
    else sq_pushstring(vm, "converted\0ignored", 17);
    return 1;
}
void push_conversion_table(HSQUIRRELVM vm, SQInteger mode) {
    sq_newtable(vm); sq_newtable(vm);
    sq_pushstring(vm, "_tostring", -1); sq_pushinteger(vm, mode);
    sq_newclosure(vm, conversion_meta, 1);
    require(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)), "conversion metamethod slot");
    require(SQ_SUCCEEDED(sq_setdelegate(vm, -2)), "conversion table delegate");
}
void coercing_string_fields(HSQUIRRELVM vm) {
    Top restore(vm); const auto base = sq_gettop(vm); Fixture f(vm);
    auto* storage = f.object() + 101;
    std::memset(storage, 0, sizeof(kinoko::legacy::StringRecord));
    store<uint32_t>(storage + 20, 15);
    struct Cleanup { kinoko::legacy::StringView value; ~Cleanup() { value.destroy(); } } cleanup{kinoko::legacy::StringView(storage)};
    f.bind("string_get", 101, kinoko_sqrat_get_string);
    f.bind("string_set", 101, kinoko_sqrat_set_string);
    f.set_int("string_set", -2147483647);
    require(f.get_string("string_get") == "-2147483647", "Sqrat string setter converts integers rather than rejecting them");
    f.set_float("string_set", 1.25f);
    require(f.get_string("string_get") == "1.25", "source float formatting");
    f.prepare("string_set"); sq_pushbool(vm, SQTrue); f.call(2, false);
    require(f.get_string("string_get") == "true", "source bool formatting");
    f.prepare("string_set"); sq_pushstring(vm, "part\0tail", 9); f.call(2, false);
    require(f.get_string("string_get") == "part" && cleanup.value.length() == 4,
            "first-NUL truncation on converted input");
    cleanup.value.assign("get\0tail", 8);
    require(f.get_string("string_get") == "get", "getter retains NUL-terminated semantics even for sized storage");
    // A source-owned string is bounded by Squirrel, not by the transitional
    // address probe used for unrelated missing-length decompiler callers.
    const std::string large(0x100008, 'x');
    f.set_string("string_set", large.c_str());
    require(f.get_string("string_get") == large, "known source string is not truncated by the old address-scan limit");
    for (SQInteger mode : {SQInteger{-1}, SQInteger{0}, SQInteger{1}, SQInteger{2}}) {
        Pair value(vm);
        if (mode == -1) sq_pushnull(vm); else push_conversion_table(vm, mode);
        value.capture();
        value.push(); sq_reseterror(vm); sq_tostring(vm, -1);
        const auto expected = bridge_test::get_string(vm);
        sq_pop(vm, 2);
        sq_getlasterror(vm); HSQOBJECT expected_error; sq_getstackobj(vm, -1, &expected_error);
        sq_addref(vm, &expected_error); sq_pop(vm, 1);
        conversion_calls = 0; sq_reseterror(vm);
        f.prepare("string_set"); value.push(); f.call(2, false);
        require(conversion_calls == (mode == -1 ? 0 : 1), "setter invokes _tostring exactly once");
        sq_getlasterror(vm); HSQOBJECT actual_error; sq_getstackobj(vm, -1, &actual_error); sq_pop(vm, 1);
        require(actual_error._type == expected_error._type && std::memcmp(&actual_error._unVal, &expected_error._unVal, sizeof(SQObjectValue)) == 0,
                "source conversion fallback and last-error are not replaced with a new policy");
        sq_release(vm, &expected_error);
        require(f.get_string("string_get") == expected, "null, string metamethod, nonstring return and failure fallback match source VM");
    }
    sq_reseterror(vm);
    require(f.object()[100] == 0xa7 && f.object()[125] == 0xa7, "string record canaries");
    f.require_canaries(); top(vm, base, "coercing string callback stack");
}
void malformed_descriptors(HSQUIRRELVM vm) {
    Top restore(vm); const auto base = sq_gettop(vm); Fixture f(vm);
    // Directly exercise malformed ABI inputs that source APIs do not bounds-check.
    require(!kinoko_cact_layer_get_int(0), "null VM guard");
    require(!kinoko_c2dlayout_get_int(address(vm)), "empty stack guard");
    int32_t offset = -999;
    f.instance.push(); require(!kinoko_c2dlayout_property_offset(address(vm), &offset), "missing descriptor guard");
    require(offset == -999, "failed descriptor leaves output unchanged");
    sq_newuserdata(vm, 1);
    require(!kinoko_cact_layer_property_offset(address(vm), &offset), "short descriptor cannot read four bytes");
    require(offset == -999, "short descriptor leaves output unchanged");
    sq_pop(vm, 1); sq_pushinteger(vm, 123);
    require(!kinoko_c2dlayout_get_float(address(vm)), "wrong descriptor type rejected");
    sq_pop(vm, 1); auto descriptor = sq_newuserdata(vm, 4); store<int32_t>(descriptor, 21);
    const auto before = f.native;
    require(!kinoko_cact_layer_set_bool(address(vm)), "missing setter value rejected");
    require(f.native == before, "malformed setter did not write native memory");
    sq_settop(vm, base);
    sq_pushnull(vm); sq_newuserdata(vm, 4);
    require(!kinoko_c2dlayout_property_offset(address(vm), &offset), "wrong receiver type rejected");
    sq_settop(vm, base);
    f.instance.push(); sq_setinstanceup(vm, -1, nullptr); sq_newuserdata(vm, 4);
    require(!kinoko_c2dlayout_property_offset(address(vm), &offset), "null native pointer rejected");
    sq_settop(vm, base); f.require_canaries();
}
}
int main() {
    try {
        bridge_test::Machine machine;
        for (int repeat = 0; repeat < 16; ++repeat) {
            scalar_fields(machine.get()); indirect_fields(machine.get());
            player_and_strings(machine.get()); malformed_descriptors(machine.get());
            typed_source_callbacks(machine.get()); coercing_string_fields(machine.get());
        }
        std::puts("Native property source-VM contracts passed (16 repetitions)");
    } catch (const std::exception& e) { std::fprintf(stderr, "%s\n", e.what()); return 1; }
}
