#include "kinoko/act_types.h"
/* Real CAct -> resource/layer -> key/layout virtuals, compiled only in R132. */
static char act_clone_events[32];
static int act_clone_event_count;
static int32_t act_clone_bound_resource;
static void act_clone_event(char event) {
    if (act_clone_event_count < (int)sizeof(act_clone_events))
        act_clone_events[act_clone_event_count++] = event;
}
static int32_t __fastcall probe_clone_resource(int32_t self, void *unused) {
    act_clone_event('R');
    return (int32_t)(intptr_t)kinoko_method_clone_texture_resource((KinokoActResource*)(uintptr_t)(self), unused);
}
static int32_t __fastcall probe_clone_layer(int32_t self, void *unused) {
    act_clone_event('L');
    return kinoko_method_clone_act_layer(self, unused);
}
static int32_t __fastcall probe_clone_key(int32_t self, void *unused) {
    act_clone_event('K');
    return (int32_t)(intptr_t)kinoko_method_clone_act_key((KinokoActKey*)(uintptr_t)(self), unused);
}
static int32_t __fastcall probe_clone_layout(int32_t self, void *unused) {
    act_clone_event('O');
    return (int32_t)(intptr_t)kinoko_method_clone_c2d_layout((KinokoActLayout*)(uintptr_t)(self), unused);
}
static int32_t __fastcall probe_clone_bind_layout(int32_t self, void *unused, int32_t layer) {
    (void)unused;
    act_clone_event('B');
    act_clone_bound_resource = *(int32_t *)(intptr_t)(layer + 100);
    return kinoko_c2dlayout_set_layer_impl(self, layer);
}
static int32_t __fastcall probe_clone_set_resource(KinokoActLayer *self, void *unused, KinokoActResource *resource) {
    act_clone_event('A');
    return kinoko_act_layer_set_resource(self, unused, resource);
}

static int test_act_virtual_clone(void) {
    const char text[] = "return 132;";
    const char compiled_text[] = "/* This script is compiled. Can't read this. Don't edit this.*/";
    for (int compiled = 0; compiled != 2; ++compiled) {
        int32_t source[60] = {0};
        CHECK(kinoko_act_document_initialize((KinokoActDocument *)source));
        kinoko_string_assign_cstr(source + 4, "a document clone with a long name");
        kinoko_string_assign_cstr(source + 11, "original resource prefix/");
        kinoko_string_assign_cstr(source + 41, "original script file.nut");
        source[1] = 37; source[2] = 900; source[3] = 600;
        source[18] = 3; source[19] = 5; source[20] = 7; source[21] = 11;
        ((float *)source)[22] = 1.25f; ((float *)source)[23] = -2.5f;
        ((uint8_t *)source)[96] = 0; ((uint8_t *)source)[204] = 1;
        free((void *)(intptr_t)source[48]);
        source[48] = PTR(malloc(sizeof(text))); CHECK(source[48]);
        memcpy((void *)(intptr_t)source[48], text, sizeof(text)); source[49] = sizeof(text);
        ((uint8_t *)source)[201] = (uint8_t)compiled;

        int32_t *resource = (int32_t *)calloc(1, 100); CHECK(resource);
        resource[0] = PTR(&kinoko_texture_resource_methods_storage); resource[1] = 42; resource[7] = resource[15] = 15;
        kinoko_string_assign_cstr(resource + 2, "cloned resource");
        int32_t layer = (int32_t)(intptr_t)kinoko_act_make_layer(); CHECK(layer);
        *(int32_t *)(intptr_t)(layer + 96) = 42;
        *(int32_t *)(intptr_t)(layer + 100) = PTR(resource);
        *(int32_t *)(intptr_t)(layer + 104) = 9;
        *(uint8_t *)(intptr_t)(layer + 305) = 1;
        int32_t *key = (int32_t *)calloc(1, 36); CHECK(key);
        key[0] = PTR(&kinoko_act_key_methods_storage); key[7] = 15;
        key[1] = PTR(calloc(1, 316)); CHECK(key[1]);
        CHECK((int32_t)(intptr_t)kinoko_construct_c2dlayout((KinokoActLayout*)(uintptr_t)(key[1])));
        CHECK(kinoko_act_append_list(layer + 180, PTR(key)));
        *(int32_t *)(intptr_t)(layer + 184) = 1;
        kinoko_act_array_append((void*)(uintptr_t)(PTR(source) + 224), (void*)(uintptr_t)(0));
        kinoko_act_array_append((void*)(uintptr_t)(PTR(source) + 224), (void*)(uintptr_t)(PTR(resource)));
        kinoko_act_array_append((void*)(uintptr_t)(PTR(source) + 208), (void*)(uintptr_t)(0));
        kinoko_act_array_append((void*)(uintptr_t)(PTR(source) + 208), (void*)(uintptr_t)(layer));

        const struct ActLayoutMethods layout_table = kinoko_act_layout_methods_storage;
        const struct ActKeyMethods key_table = kinoko_act_key_methods_storage;
        /* Preserve actual table types without reproducing their layouts. */
        int32_t (__fastcall *resource_clone)(int32_t, void *) = kinoko_texture_resource_methods_storage.clone;
        int32_t (__fastcall *layer_clone)(int32_t, void *) = kinoko_act_layer_methods_storage.clone;
        int32_t (__fastcall *set_resource)(KinokoActLayer *, void *, KinokoActResource *) = kinoko_act_layer_methods_storage.associate;
        kinoko_texture_resource_methods_storage.clone = probe_clone_resource;
        kinoko_act_layer_methods_storage.clone = probe_clone_layer;
        kinoko_act_layer_methods_storage.associate = probe_clone_set_resource;
        kinoko_act_key_methods_storage.clone = probe_clone_key;
        kinoko_act_layout_methods_storage.clone = probe_clone_layout;
        kinoko_act_layout_methods_storage.associate = probe_clone_bind_layout;
        act_clone_event_count = 0; act_clone_bound_resource = 0;
        KinokoActDocument *copy = kinoko_act_clone((KinokoActDocument *)source, NULL);
        kinoko_texture_resource_methods_storage.clone = resource_clone;
        kinoko_act_layer_methods_storage.clone = layer_clone;
        kinoko_act_layer_methods_storage.associate = set_resource;
        kinoko_act_key_methods_storage = key_table; kinoko_act_layout_methods_storage = layout_table;

        CHECK(copy);
        const int32_t *cloned = (const int32_t *)copy;
        CHECK(act_clone_event_count == 6 && memcmp(act_clone_events, "RLKOBA", 6) == 0);
        CHECK(act_clone_bound_resource == PTR(resource)); // bind before resource reassociation
        CHECK(cloned[53] - cloned[52] == 4 && cloned[57] - cloned[56] == 4);
        CHECK(cloned[0] == PTR(&kinoko_act_document_methods_storage) && cloned[1] == 37 && cloned[2] == 900 && cloned[3] == 600);
        CHECK(strcmp(kinoko_string_data((const void*)(intptr_t)(PTR(copy) + 16)), kinoko_string_data((const void*)(intptr_t)(PTR(source) + 16))) == 0);
        CHECK(kinoko_string_data((const void*)(intptr_t)(PTR(copy) + 16)) != kinoko_string_data((const void*)(intptr_t)(PTR(source) + 16)));
        CHECK(kinoko_string_data((const void*)(intptr_t)(PTR(copy) + 44))[0] == 0);
        CHECK(kinoko_string_data((const void*)(intptr_t)(PTR(copy) + 164))[0] == 0);
        CHECK(memcmp(cloned + 18, source + 18, 24) == 0);
        CHECK(((const uint8_t *)copy)[96] == 0 && ((const uint8_t *)copy)[204] == 0);
        CHECK(((const uint8_t *)copy)[200] == 1 && ((const uint8_t *)copy)[201] == 0);
        CHECK(strcmp((const char *)(intptr_t)cloned[48], compiled ? compiled_text : text) == 0);
        const int32_t copied_layer = *(int32_t *)(intptr_t)cloned[52];
        const int32_t copied_resource = *(int32_t *)(intptr_t)cloned[56];
        CHECK(copied_layer != layer && copied_resource != PTR(resource));
        CHECK(*(int32_t *)(intptr_t)(copied_layer + 100) == copied_resource);
        CHECK(*(uint8_t *)(intptr_t)(copied_resource + 36) == 1);
        CHECK(*(uint8_t *)(intptr_t)(copied_layer + 305) == 1);
        CHECK(strcmp(*(const char **)(intptr_t)(copied_layer + 296), compiled_text) == 0);
        kinoko_destroy_cact_with_flags(PTR(copy), 1);
        CHECK(strcmp((const char *)(intptr_t)source[48], text) == 0);
        kinoko_destroy_cact_object(PTR(source));
    }
    puts("PASS: original document constructor, resource/layer/key/layout virtual order, one early layout bind, compact null slots and script text semantics");
    return 0;
}
