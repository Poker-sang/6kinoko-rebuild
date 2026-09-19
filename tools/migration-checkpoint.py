from pathlib import Path
import subprocess

def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text()
    assert text.count(old) == 1, (path, "anchor count", text.count(old))
    p.write_text(text.replace(old, new, 1))

native = Path("src/squirrel/squirrel_native_calls.cpp")
text = native.read_text()
assert 'extern "C" int32_t function_470df0' not in text
text = text.rstrip() + r'''

extern "C" int32_t function_470df0(int32_t id, int32_t index) {
    auto vm = pointer<SQVM>(id);
    if (!index_exists(vm, index)) return error(vm, argument_error);
    switch (sq_gettype(vm, index)) {
    case OT_NULL:
        return 0;
    case OT_BOOL: {
        SQBool value = SQFalse;
        if (SQ_FAILED(sq_getbool(vm, index, &value))) return error(vm, conversion_error);
        return value != SQFalse;
    }
    case OT_INTEGER: {
        SQInteger value = 0;
        if (SQ_FAILED(sq_getinteger(vm, index, &value))) return error(vm, conversion_error);
        return value != 0;
    }
    case OT_FLOAT: {
        SQFloat value = 0;
        if (SQ_FAILED(sq_getfloat(vm, index, &value))) return error(vm, conversion_error);
        return value != 0;
    }
    default:
        return 1;
    }
}
extern "C" int32_t function_470ee0(int32_t id) {
    auto vm = pointer<SQVM>(id);
    const auto callback = target(vm);
    if (callback) reinterpret_cast<int32_t (__cdecl *)(void)>(pointer(callback))();
    return 0;
}
extern "C" int32_t function_471160(int32_t callback, int32_t id, int32_t index) {
    auto vm = pointer<SQVM>(id);
    if (!strict_type(vm, index, OT_STRING)) return error(vm, argument_error);
    HSQOBJECT object{};
    if (!pair_argument(vm, static_cast<int64_t>(index) + 1, object))
        return error(vm, argument_error);
    auto argument = transfer(vm, object);
    const SQChar* value = nullptr;
    if (SQ_FAILED(sq_getstring(vm, index, &value))) {
        sq_release(vm, &argument.value);
        return error(vm, conversion_error);
    }
    if (!callback) {
        sq_release(vm, &argument.value);
        return 0;
    }
    using Function = int32_t (__cdecl *)(int32_t, int32_t, int32_t, int32_t);
    const auto result = reinterpret_cast<Function>(pointer(callback))(
        address(value), static_cast<int32_t>(argument.vtable),
        static_cast<int32_t>(argument.value._type), data_bits(argument.value));
    sq_pushbool(vm, static_cast<unsigned char>(result) != 0);
    return 1;
}
extern "C" int32_t function_471330(int32_t callback, int32_t id, int32_t index) {
    auto vm = pointer<SQVM>(id);
    const SQChar* value = nullptr;
    if (SQ_FAILED(string_argument(vm, index, value))) return -1;
    if (callback) {
        using Function = int32_t (__cdecl *)(int32_t);
        const auto result = reinterpret_cast<Function>(pointer(callback))(address(value));
        sq_pushbool(vm, static_cast<unsigned char>(result) != 0);
    }
    return 1;
}
extern "C" int32_t function_471880(int32_t callback, int32_t id, int32_t index) {
    auto vm = pointer<SQVM>(id);
    const int64_t second_index = static_cast<int64_t>(index) + 1;
    const int64_t third_index = static_cast<int64_t>(index) + 2;
    const int64_t fourth_index = static_cast<int64_t>(index) + 3;
    if (!strict_type(vm, index, OT_STRING) || !strict_type(vm, second_index, OT_INTEGER) ||
        !strict_type(vm, third_index, OT_INTEGER) || !index_exists(vm, fourth_index))
        return error(vm, argument_error);
    const auto fourth = function_470df0(id, static_cast<int32_t>(fourth_index));
    if (fourth < 0) return fourth;
    SQInteger third = 0, second = 0;
    const SQChar* first = nullptr;
    if (SQ_FAILED(sq_getinteger(vm, static_cast<SQInteger>(third_index), &third)) ||
        SQ_FAILED(sq_getinteger(vm, static_cast<SQInteger>(second_index), &second)) ||
        SQ_FAILED(sq_getstring(vm, index, &first))) return error(vm, conversion_error);
    if (callback) {
        using Function = void (__cdecl *)(int32_t, int32_t, int32_t, int32_t);
        reinterpret_cast<Function>(pointer(callback))(address(first), second, third, fourth);
    }
    return 0;
}
extern "C" int32_t function_471960(int32_t callback, int32_t id, int32_t index) {
    auto vm = pointer<SQVM>(id);
    const int64_t second_index = static_cast<int64_t>(index) + 1;
    const int64_t third_index = static_cast<int64_t>(index) + 2;
    const int64_t fourth_index = static_cast<int64_t>(index) + 3;
    const int64_t fifth_index = static_cast<int64_t>(index) + 4;
    if (!strict_type(vm, index, OT_STRING) || !strict_type(vm, second_index, OT_INTEGER) ||
        !strict_type(vm, third_index, OT_INTEGER) || !strict_type(vm, fourth_index, OT_INTEGER) ||
        !index_exists(vm, fifth_index)) return error(vm, argument_error);
    const auto fifth = function_470df0(id, static_cast<int32_t>(fifth_index));
    if (fifth < 0) return fifth;
    SQInteger fourth = 0, third = 0, second = 0;
    const SQChar* first = nullptr;
    if (SQ_FAILED(sq_getinteger(vm, static_cast<SQInteger>(fourth_index), &fourth)) ||
        SQ_FAILED(sq_getinteger(vm, static_cast<SQInteger>(third_index), &third)) ||
        SQ_FAILED(sq_getinteger(vm, static_cast<SQInteger>(second_index), &second)) ||
        SQ_FAILED(sq_getstring(vm, index, &first))) return error(vm, conversion_error);
    if (callback) {
        using Function = void (__cdecl *)(int32_t, int32_t, int32_t, int32_t, int32_t);
        reinterpret_cast<Function>(pointer(callback))(address(first), second, third, fourth, fifth);
    }
    return 0;
}
extern "C" int32_t function_471bc0(int32_t id) { return function_470ee0(id); }
extern "C" int32_t function_471c10(int32_t id) {
    auto vm = pointer<SQVM>(id);
    return function_471160(target(vm), id, 2);
}
extern "C" int32_t function_471d90(int32_t id) {
    auto vm = pointer<SQVM>(id);
    return function_471330(target(vm), id, 2);
}
extern "C" int32_t function_471eb0(int32_t id) {
    auto vm = pointer<SQVM>(id);
    const auto callback = target(vm);
    if (!callback) return 0;
    if (!strict_type(vm, 2, OT_FLOAT) || !strict_type(vm, 3, OT_FLOAT)) {
        error(vm, argument_error);
        return 0;
    }
    SQFloat first = 0, second = 0;
    if (SQ_FAILED(sq_getfloat(vm, 2, &first)) || SQ_FAILED(sq_getfloat(vm, 3, &second))) {
        error(vm, conversion_error);
        return 0;
    }
    reinterpret_cast<int32_t (__cdecl *)(SQFloat, SQFloat)>(pointer(callback))(first, second);
    return 0;
}
extern "C" int32_t function_471f10(int32_t id) {
    auto vm = pointer<SQVM>(id);
    return function_4716b0(target(vm), id, 2);
}
extern "C" int32_t function_471fd0(int32_t id) {
    auto vm = pointer<SQVM>(id);
    const auto callback = target(vm);
    if (!callback) return 0;
    if (!strict_type(vm, 2, OT_INTEGER)) {
        error(vm, argument_error);
        return 0;
    }
    SQInteger value = 0;
    if (SQ_FAILED(sq_getinteger(vm, 2, &value))) {
        error(vm, conversion_error);
        return 0;
    }
    reinterpret_cast<int32_t (__cdecl *)(int32_t)>(pointer(callback))(value);
    return 0;
}
extern "C" int32_t function_472080(int32_t id) {
    auto vm = pointer<SQVM>(id);
    return function_471880(target(vm), id, 2);
}
extern "C" int32_t function_4720e0(int32_t id) {
    auto vm = pointer<SQVM>(id);
    return function_471960(target(vm), id, 2);
}
extern "C" int32_t function_472140(int32_t id) {
    auto vm = pointer<SQVM>(id);
    return function_471a60(target(vm), id, 2);
}
'''
native.write_text(text)

replace_once(
    "include/kinoko/squirrel_native_calls.h",
    "int32_t function_4716b0(int32_t callback, int32_t vm, int32_t index);\n",
    """int32_t function_470df0(int32_t vm, int32_t index);
int32_t function_470ee0(int32_t vm);
int32_t function_471160(int32_t callback, int32_t vm, int32_t index);
int32_t function_471330(int32_t callback, int32_t vm, int32_t index);
int32_t function_471880(int32_t callback, int32_t vm, int32_t index);
int32_t function_471960(int32_t callback, int32_t vm, int32_t index);
int32_t function_4716b0(int32_t callback, int32_t vm, int32_t index);
""",
)
replace_once(
    "include/kinoko/squirrel_native_calls.h",
    "int32_t function_471d30(int32_t vm);\n",
    """int32_t function_471d30(int32_t vm);
int32_t function_471bc0(int32_t vm);
int32_t function_471c10(int32_t vm);
int32_t function_471d90(int32_t vm);
int32_t function_471eb0(int32_t vm);
int32_t function_471f10(int32_t vm);
int32_t function_471fd0(int32_t vm);
int32_t function_472080(int32_t vm);
int32_t function_4720e0(int32_t vm);
int32_t function_472140(int32_t vm);
""",
)

rebuilt = Path("src/decompiled/6kinoko_rebuilt.c")
text = rebuilt.read_text()
def remove_definition(text, name):
    signature = "int32_t " + name + "("
    start = text.rfind(signature)
    assert start >= 0, name
    brace = text.find("{", start)
    semi = text.find(";", start)
    assert brace >= 0 and (semi < 0 or brace < semi), ("prototype selected", name)
    depth = 0
    for i in range(brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[:start] + "/* " + name + " is implemented in native C++ (squirrel_native_calls.cpp). */" + text[i + 1:]
    raise AssertionError(("unbalanced", name))

for name in [
    "function_46b450", "function_470df0", "function_470ee0",
    "function_471160", "function_471330", "function_471880",
    "function_471960", "function_471bc0", "function_471c10",
    "function_471d90", "function_471eb0", "function_471f10",
    "function_471fd0", "function_472080", "function_4720e0",
    "function_472140",
]:
    text = remove_definition(text, name)
rebuilt.write_text(text)

test = Path("tests/squirrel_native_calls_contract.cpp")
text = test.read_text()
assert "bool_name(int32_t text)" not in text
text = text.replace(
    "using kinoko::script::data_bits;\n",
    "using kinoko::script::data_bits;\nusing kinoko::script::borrowed_value;\n",
    1,
)
anchor = 'void __cdecl optional_two(int32_t a, int32_t b) { require(a == -5 && b == 13, "optional two integer order"); ++native_calls; }\n'
addition = r'''int32_t __cdecl bool_name(int32_t text) {
    require(std::string(pointer<char>(text)) == "loaded", "single string callback value");
    ++native_calls; return 0x100;
}
int32_t __cdecl string_object(int32_t text, int32_t vtable, int32_t type, int32_t data) {
    require(std::string(pointer<char>(text)) == "table", "string/object callback value");
    ObjectStorage object{static_cast<uint32_t>(vtable), borrowed_value(type, data)};
    require(object.vtable == static_cast<uint32_t>(kinoko_squirrel_object_vtable()), "string/object wrapper vtable");
    require(sq_release(active_vm, &object.value) == SQTrue, "string/object callee consumes external handle");
    ++consumed; return 1;
}
void __cdecl play_four(int32_t text, int32_t a, int32_t b, int32_t truth) {
    require(std::string(pointer<char>(text)) == "bgm" && a == 2 && b == 3 && truth == 1, "four argument callback");
    ++native_calls;
}
void __cdecl play_five(int32_t text, int32_t a, int32_t b, int32_t c, int32_t truth) {
    require(std::string(pointer<char>(text)) == "se" && a == 4 && b == 5 && c == 6 && truth == 0, "five argument callback");
    ++native_calls;
}
int32_t __cdecl integer_callback(int32_t value) { require(value == -44, "integer callback"); ++native_calls; return 0; }
int32_t __cdecl float_callback(float a, float b) { require(a == 1.5f && b == -2.25f, "float callback"); ++native_calls; return 0; }
'''
assert text.count(anchor) == 1
text = text.replace(anchor, anchor + addition, 1)
anchor = "void invalid_and_optional(HSQUIRRELVM vm) {\n"
addition = r'''void migrated_adapters(HSQUIRRELVM vm) {
    Top restore(vm);
    const auto base = sq_gettop(vm);
    auto truth = [&](auto push, int expected) {
        sq_settop(vm, base); push();
        require(function_470df0(address(vm), base + 1) == expected, "source truth conversion");
    };
    truth([&]{ sq_pushnull(vm); }, 0);
    truth([&]{ sq_pushbool(vm, SQFalse); }, 0);
    truth([&]{ sq_pushbool(vm, SQTrue); }, 1);
    truth([&]{ sq_pushinteger(vm, 0); }, 0);
    truth([&]{ sq_pushinteger(vm, -7); }, 1);
    truth([&]{ sq_pushfloat(vm, 0.0f); }, 0);
    truth([&]{ sq_pushfloat(vm, -0.5f); }, 1);
    truth([&]{ sq_pushstring(vm, "value", -1); }, 1);
    truth([&]{ sq_newtable(vm); }, 1);

    sq_settop(vm, base); sq_pushstring(vm, "loaded", -1);
    const auto before = native_calls;
    require(function_471330(address(reinterpret_cast<void*>(bool_name)), address(vm), base + 1) == 1,
        "single string adapter return count");
    SQBool boolean = SQTrue; require(SQ_SUCCEEDED(sq_getbool(vm, -1, &boolean)) && boolean == SQFalse,
        "single string adapter preserves low-byte BOOL conversion");
    require(native_calls == before + 1, "single string callback called");

    sq_settop(vm, base); sq_pushstring(vm, "table", -1); push_owned_userdata(vm);
    const auto consumed_before = consumed, released_before = released;
    require(function_471160(address(reinterpret_cast<void*>(string_object)), address(vm), base + 1) == 1,
        "string/object adapter return count");
    require(consumed == consumed_before + 1, "string/object callback consumes one external handle");
    sq_settop(vm, base); sq_collectgarbage(vm);
    require(released == released_before + 1, "string/object stack ownership releases exactly once");

    sq_pushstring(vm, "table", -1); push_owned_userdata(vm);
    require(function_471160(0, address(vm), base + 1) == 0, "null string/object callback");
    sq_settop(vm, base); sq_collectgarbage(vm);
    require(released == released_before + 2, "null callback balances temporary external handle");

    sq_pushstring(vm, "bgm", -1); sq_pushinteger(vm, 2); sq_pushinteger(vm, 3); sq_pushstring(vm, "truthy", -1);
    require(function_471880(address(reinterpret_cast<void*>(play_four)), address(vm), base + 1) == 0,
        "four argument source adapter");
    sq_settop(vm, base); sq_pushstring(vm, "se", -1); sq_pushinteger(vm, 4); sq_pushinteger(vm, 5);
    sq_pushinteger(vm, 6); sq_pushnull(vm);
    require(function_471960(address(reinterpret_cast<void*>(play_five)), address(vm), base + 1) == 0,
        "five argument source adapter");

    sq_settop(vm, base); captured_closure(vm, function_471fd0, address(reinterpret_cast<void*>(integer_callback)));
    sq_pushroottable(vm); sq_pushinteger(vm, -44);
    require(SQ_SUCCEEDED(kinoko_sq_call(address(vm), 2, SQFalse, SQFalse)), "captured integer wrapper");
    sq_settop(vm, base); captured_closure(vm, function_471eb0, address(reinterpret_cast<void*>(float_callback)));
    sq_pushroottable(vm); sq_pushfloat(vm, 1.5f); sq_pushfloat(vm, -2.25f);
    require(SQ_SUCCEEDED(kinoko_sq_call(address(vm), 3, SQFalse, SQFalse)), "captured float wrapper");
    require(native_calls == before + 5, "all migrated callbacks invoked");
    sq_settop(vm, base);
}
'''
assert text.count(anchor) == 1
text = text.replace(anchor, addition + anchor, 1)
old = "methods(vm); ownership(vm); properties(vm); invalid_and_optional(vm);"
assert text.count(old) == 1
text = text.replace(old, "methods(vm); ownership(vm); properties(vm); migrated_adapters(vm); invalid_and_optional(vm);", 1)
test.write_text(text)

subprocess.run(["python3", "tools/check_migration_boundaries.py"], check=True)
Path(__file__).unlink()
