/* Compile this contract as C, just like the remaining recovered host. */
#include "kinoko/squirrel_api_types.h"
#include <limits.h>
#include <stdio.h>

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "C API contract failed at line %d: %s\n", __LINE__, #test); \
    sq_close(vm); return 1; } } while (0)

int kinoko_test_squirrel_c_api(void) {
    HSQUIRRELVM vm = sq_open(64);
    SQInteger number = 0;
    SQUserPointer data = NULL, tag = NULL;
    HSQOBJECT owned, value;
    const SQChar* text = NULL;
    const int32_t samples[] = {0, INT32_MIN, 0x3f800000, 0x7f800000, 0x7fc01234};
    unsigned index;
    if (!vm) return 1;
    CHECK(kinoko_vm((int32_t)(uintptr_t)vm) == vm);
    CHECK(kinoko_pointer((int32_t)0x87654321u) == (void*)(uintptr_t)0x87654321u);
    for (index = 0; index < sizeof(samples) / sizeof(samples[0]); ++index) {
        SQFloat real = kinoko_float_bits(samples[index]);
        int32_t bits = 0;
        memcpy(&bits, &real, sizeof(bits));
        CHECK(bits == samples[index]);
        value = kinoko_borrowed_object(OT_INTEGER, samples[index]);
        sq_pushobject(vm, value);
        CHECK(SQ_SUCCEEDED(sq_getinteger(vm, -1, &number)));
        CHECK(number == samples[index]);
        sq_pop(vm, 1);
    }
    sq_newtable(vm);
    sq_pushstring(vm, "counter", -1);
    sq_pushinteger(vm, 41);
    CHECK(SQ_SUCCEEDED(sq_newslot(vm, -3, SQFalse)));
    CHECK(sq_gettop(vm) == 1);
    sq_pushstring(vm, "counter", -1);
    CHECK(SQ_SUCCEEDED(sq_get(vm, -2)));
    CHECK(SQ_SUCCEEDED(sq_getinteger(vm, -1, &number)) && number == 41);
    sq_pop(vm, 1);
    sq_pushstring(vm, "counter", -1);
    sq_pushinteger(vm, 42);
    CHECK(SQ_SUCCEEDED(sq_rawset(vm, -3)));
    sq_pushstring(vm, "counter", -1);
    CHECK(SQ_SUCCEEDED(sq_get(vm, -2)));
    CHECK(SQ_SUCCEEDED(sq_getinteger(vm, -1, &number)) && number == 42);
    sq_pop(vm, 1);
    CHECK(sq_gettop(vm) == 1);

    /* An external retained object survives stack clearing and source GC. */
    sq_pushstring(vm, "owned", -1);
    CHECK(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &owned)));
    sq_addref(vm, &owned);
    sq_settop(vm, 0);
    sq_collectgarbage(vm);
    sq_pushobject(vm, owned);
    CHECK(SQ_SUCCEEDED(sq_getstring(vm, -1, &text)) && strcmp(text, "owned") == 0);
    sq_pop(vm, 1);
    sq_release(vm, &owned);
    sq_resetobject(&owned);
    CHECK(owned._type == OT_NULL);

    data = sq_newuserdata(vm, sizeof(int32_t));
    CHECK(data != NULL);
    *(int32_t*)data = 12345;
    CHECK(SQ_SUCCEEDED(sq_settypetag(vm, -1, (SQUserPointer)(uintptr_t)0x1234)));
    data = NULL;
    CHECK(SQ_SUCCEEDED(sq_getuserdata(vm, -1, &data, &tag)));
    CHECK(*(int32_t*)data == 12345 && tag == (SQUserPointer)(uintptr_t)0x1234);
    sq_settop(vm, 0);

    CHECK(SQ_SUCCEEDED(sq_compilebuffer(vm, "return 6 * 7;", (SQInteger)strlen("return 6 * 7;"), "direct-c-api", SQTrue)));
    sq_pushroottable(vm);
    CHECK(SQ_SUCCEEDED(sq_call(vm, 1, SQTrue, SQTrue)));
    CHECK(SQ_SUCCEEDED(sq_getinteger(vm, -1, &number)) && number == 42);
    sq_settop(vm, 0);
    sq_close(vm);
    puts("PASS: direct Squirrel 2.2.2 C API, address/value bits, ownership and stack contracts");
    return 0;
}
