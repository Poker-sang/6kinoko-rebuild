/* Exercise the actual reconstructed functions without WinMain, graphics or DAT startup. */
#include "../src/decompiled/6kinoko_rebuilt.c"

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)
#define PTR(value) ((int32_t)(intptr_t)(value))

void retdec_trace(const char *message) {
    if (message && (strstr(message, "stagevm:compile-error") ||
                    strstr(message, "stagevm:failure")))
        fprintf(stderr, "%s\n", message);
}

void retdec_trace_hresult(const char *label, long value) {
    (void)label;
    (void)value;
}

static int execute_source(int32_t vm, const int32_t *environment, const char *source) {
    unsigned char *bytes = NULL;
    int32_t size = 0, script[26] = {0};
    int result;
    if (!retdec_squirrel_compile_source(source, (int32_t)strlen(source),
            "stage contract", &bytes, &size))
        return 0;
    script[23] = PTR(bytes);
    script[24] = size;
    result = retdec_execute_embedded_act_script(vm, PTR(script), environment);
    if (!result && *(int32_t *)(intptr_t)(vm + 64) == 0x08000010)
        fprintf(stderr, "VM: %s\n", (char *)(intptr_t)(*(int32_t *)(intptr_t)(vm + 68) + 28));
    free(bytes);
    return result;
}

static int execute_file(int32_t vm, const int32_t *environment, const char *path) {
    FILE *file = NULL;
    unsigned char *bytes;
    long size;
    int32_t script[26] = {0};
    int result;
    if (fopen_s(&file, path, "rb") != 0)
        return 0;
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);
    if (size <= 1) { fclose(file); return 0; }
    bytes = (unsigned char *)malloc((size_t)size);
    if (bytes == NULL) { fclose(file); return 0; }
    result = fread(bytes, 1, (size_t)size, file) == (size_t)size;
    fclose(file);
    if (result) {
        unsigned char key = bytes[0] ^ 0xFA;
        for (long i = 0; i < size; ++i) bytes[i] ^= key;
        script[23] = PTR(bytes);
        script[24] = (int32_t)size;
        result = retdec_execute_embedded_act_script(vm, PTR(script), environment);
    }
    free(bytes);
    return result;
}

int main(int argc, char **argv) {
    int32_t vm = function_48a170(1024);
    int32_t root[5], environment[3], closure[3];
    int32_t target = PTR(function_470fa0);
    int32_t manager = PTR(g_retdec_actor_manager_state);
    int32_t layout[100] = {0}, layer[80] = {0}, resource[20] = {0};
    int32_t act[60] = {0}, act_resource[48] = {0}, holder[1], layer_vector[1];
    int32_t sentinel[3] = {0}, node[3] = {0}, layout_key[2] = {0};
    int32_t records[4][8] = {
        {0x443, 100, 200}, {0xc8a, 300, 400},
        {0xc9a, -40, -80}, {0xffff, 0, 0}
    };
    struct retdec_mcd_chip chips[2] = {{0}};
    struct retdec_mcd_data data = {2, chips, 0, NULL};
    int32_t top, created;
    int32_t *actors;

    CHECK(vm != 0);
    g644 = (char *)(intptr_t)vm;
    CHECK(retdec_sqrat_root_construct(PTR(root), vm));
    CHECK(function_415550_this(PTR(root), PTR("SetInitFunctionByID"),
        PTR(&target), 4, PTR(function_471d30), 0) >= 0);
    top = function_48aa20(vm);
    CHECK(execute_source(vm, root + 2,
        "registry <- {};\nother <- {};\nseen <- [];\n"
        "function Spawn(id) { ::seen.append(id); }\n"
        "SetInitFunctionByID(0x443, Spawn, registry);\n"
        "registry.Init0443 = Spawn;\n"
        "SetInitFunctionByID(0xc8a, Spawn, registry);\n"
        "SetInitFunctionByID(0xc9a, Spawn, registry);\n"
        "SetInitFunctionByID(0x443, null, other);\n"
        "if ((\"Init0443\" in other) || (\"Init0443\" in this)) throw \"wrong table\";"));
    CHECK(function_48aa20(vm) == top);
    function_4aa3a0_this(PTR(root + 1), PTR(environment), "registry");
    /* root is Sqrat::Object [vtable, vm, type, value, owner], not SquirrelObject. */
    CHECK(environment[1] == 0x0A000020);
    function_4aa3a0_this(PTR(environment), PTR(closure), "Init0443");
    CHECK(closure[1] == 0x08000100);
    function_4a9d70_this(PTR(closure));

    if (argc > 1) {
        CHECK(execute_file(vm, root + 2, argv[1]));
        CHECK(execute_source(vm, root + 2,
            "if (typeof Init0443 != \"function\" || typeof Init0e4c != \"function\") "
            "throw \"block initialization incomplete\";"));
    }
    CHECK(retdec_construct_actor_manager(manager));
    function_460e00();
    layout[0] = PTR(&g327);
    layout[66] = PTR(records);
    layout[67] = PTR(records + 4);
    layout[78] = PTR(layer);
    layout[79] = PTR(resource);
    resource[16] = PTR(&data);
    chips[0].chip_id = 0x443;
    chips[1].chip_id = 0xc8a;
    *(int16_t *)(chips[0].bytes + 12) = 32;
    *(int16_t *)(chips[0].bytes + 14) = 48;
    *(int16_t *)(chips[1].bytes + 12) = 31;
    *(int16_t *)(chips[1].bytes + 14) = 47;
    *(uint32_t *)(chips[1].bytes + 16) = 0x10000;
    holder[0] = PTR(act);
    layer_vector[0] = PTR(layer);
    act[52] = PTR(layer_vector);
    act[53] = PTR(layer_vector + 1);
    act_resource[2] = 1;
    act_resource[4] = PTR(holder);
    memcpy(layer + 28, "en", 3);
    layer[32] = 2;
    layer[33] = 15;
    layer[45] = PTR(sentinel);
    layer[46] = 1;
    sentinel[0] = PTR(node);
    node[0] = PTR(sentinel);
    node[2] = PTR(layout_key);
    layout_key[1] = PTR(layout);
    *(int32_t *)(g_retdec_map_manager_state + 12) = PTR(act);
    *(int32_t *)(g_retdec_map_manager_state + 16) = PTR(holder);
    *(int32_t *)(g_retdec_map_manager_state + 20) = PTR(act_resource);
    CHECK(function_46f140(PTR("en")) == PTR(layout));
    CHECK(function_46f140(PTR("e")) == 0);
    CHECK(function_46f140(PTR("missing")) == 0);
    target = PTR(function_469d10);
    CHECK(function_415550_this(PTR(root), PTR("CreateActorFromMap"),
        PTR(&target), 4, PTR(function_471e50), 0) >= 0);
    CHECK(execute_source(vm, root + 2,
        "CreateActorFromMap(\"missing\", registry);\n"
        "CreateActorFromMap(\"en\", other);\n"
        "CreateActorFromMap(\"en\", registry);\n"));
    created = *(int32_t *)(intptr_t)(manager + 92);
    CHECK(created == 3);
    CHECK(function_48aa20(vm) == top);
    CHECK(execute_source(vm, root + 2,
        "if (seen.len() != 3 || seen[0] != 0x443 || seen[1] != 0xc8a || "
        "seen[2] != 0xc9a) throw \"spawn order/id mismatch\";"));
    CHECK(retdec_actor_manager_refresh(manager) == 3);
    actors = *(int32_t **)(intptr_t)(manager + 100);
    for (int i = 0; i < 3; ++i) {
        int32_t actor = actors[i];
        int32_t id = *(int32_t *)(intptr_t)(actor + 76);
        CHECK(**(int32_t **)(intptr_t)(actor + 24) == actor);
        CHECK(*(float *)(intptr_t)(actor + 88) == -1.0f);
        if (id == 0x443) {
            unsigned char expected[48];
            memcpy(expected, chips[0].bytes, sizeof(expected));
            /* Actor::Init resets its collision index within the copied data. */
            *(int16_t *)(expected + 34) = -1;
            CHECK(*(float *)(intptr_t)(actor + 80) == 117.0f);
            CHECK(*(float *)(intptr_t)(actor + 84) == 248.0f);
            CHECK(memcmp((void *)(intptr_t)(actor + 376), expected, 48) == 0);
        } else if (id == 0xc8a) {
            CHECK(*(float *)(intptr_t)(actor + 80) == 316.5f);
            CHECK(*(float *)(intptr_t)(actor + 84) == 423.5f);
        } else {
            CHECK(id == 0xc9a);
            CHECK(*(float *)(intptr_t)(actor + 80) == -40.0f);
            CHECK(*(float *)(intptr_t)(actor + 84) == -80.0f);
        }
    }
    CHECK(function_468950_this(PTR(g_514300_storage), manager));
    CHECK(function_4693a0(PTR(layout)) != 0);
    CHECK(function_4693a0(PTR(layout)) != 0);
    CHECK(g_514300_storage[2] - g_514300_storage[1] == 8);
    CHECK(g_514300_storage[14] - g_514300_storage[13] == 16);
    CHECK(retdec_actor_manager_refresh(manager) == 5);
    function_468620_this(PTR(g_514300_storage));
    CHECK(function_468950_this(PTR(g_514300_storage), manager));
    CHECK(g_514300_storage[2] == g_514300_storage[1]);
    CHECK(g_514300_storage[14] == g_514300_storage[13]);
    target = PTR(function_469dd0);
    CHECK(function_415550_this(PTR(root), PTR("CreateEvent"),
        PTR(&target), 4, PTR(function_471f70), 0) >= 0);
    layout[67] = PTR(records + 2);
    CHECK(execute_source(vm, root + 2,
        "events <- { bounds = [] };\n"
        "function Event(id, x1, y1, x2, y2) { bounds.append([id,x1,y1,x2,y2]); }\n"
        "CreateEvent(\"en\", Event, events);\n"
        "if (events.bounds.len() != 2 || events.bounds[0][0] != 0x443 || "
        "events.bounds[0][3] != 132 || events.bounds[0][4] != 248 || "
        "events.bounds[1][0] != 0xc8a || events.bounds[1][1] != 300 || "
        "events.bounds[1][2] != 400 || events.bounds[1][3] != 331 || "
        "events.bounds[1][4] != 447) throw \"event bounds/environment mismatch\";"));
    CHECK(function_48aa20(vm) == top);
    {
        int32_t (*many)[8] = (int32_t (*)[8])calloc(600, 32);
        CHECK(many != NULL);
        for (int i = 0; i < 600; ++i) {
            many[i][0] = 0x443;
            many[i][1] = 32 * i;
        }
        layout[66] = PTR(many);
        layout[67] = PTR(many + 600);
        CHECK(function_463e60(manager, PTR(layout), PTR(environment)) == 600);
        CHECK(retdec_actor_manager_refresh(manager) == 605);
        CHECK(function_48aa20(vm) == top);
        CHECK(execute_source(vm, root + 2,
            "if (seen.len() != 603) throw \"lost actors in large map\";"));
        free(many);
    }
    {
        int32_t *retired_layout = (int32_t *)VirtualAlloc(NULL, 4096,
            MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        int32_t proxy, control, pair[2];
        DWORD old_protection;
        CHECK(retired_layout != NULL);
        memcpy(retired_layout, layout, sizeof(layout));
        retired_layout[66] = PTR(records);
        retired_layout[67] = PTR(records + 2);
        proxy = function_4693a0(PTR(retired_layout));
        CHECK(proxy != 0);
        control = *(int32_t *)(intptr_t)(proxy + 28);
        CHECK(*(int32_t *)(intptr_t)(control + 4) == 1);
        function_469700();
        CHECK(*(int32_t *)(intptr_t)(manager + 92) == 0);
        CHECK(*(int32_t *)(intptr_t)(control + 4) == 0);
        function_45e410_this(g_514300_storage[13], pair);
        CHECK(pair[0] == 0 && pair[1] == 0);
        CHECK(VirtualProtect(retired_layout, 4096, PAGE_NOACCESS, &old_protection));
        function_468620_this(PTR(g_514300_storage));
        {
            int32_t pool = *(int32_t *)(intptr_t)(manager + 4);
            int32_t pool_bytes = *(int32_t *)(intptr_t)(pool + 8) -
                                 *(int32_t *)(intptr_t)(pool + 4);
            int32_t (*reused)[8] = (int32_t (*)[8])calloc(600, 32);
            CHECK(reused != NULL);
            for (int i = 0; i < 600; ++i)
                reused[i][0] = 0x443;
            layout[66] = PTR(reused);
            layout[67] = PTR(reused + 600);
            for (int round = 0; round < 4; ++round) {
                CHECK(function_463e60(manager, PTR(layout), PTR(environment)) == 600);
                CHECK(retdec_actor_manager_refresh(manager) == 600);
                CHECK(*(int32_t *)(intptr_t)(pool + 8) -
                      *(int32_t *)(intptr_t)(pool + 4) == pool_bytes);
                CHECK(*(int32_t *)(intptr_t)(pool + 24) -
                      *(int32_t *)(intptr_t)(pool + 20) == pool_bytes);
                function_45e410_this(g_514300_storage[13], pair);
                CHECK(pair[0] == 0 && pair[1] == 0);
                function_468620_this(PTR(g_514300_storage));
                function_469700();
                CHECK(retdec_actor_manager_refresh(manager) == 0);
                CHECK(function_48aa20(vm) == top);
            }
            free(reused);
        }
        CHECK(function_468950_this(PTR(g_514300_storage), manager));
        CHECK(VirtualFree(retired_layout, 0, MEM_RELEASE));
    }
    CHECK(execute_source(vm, root + 2,
        "probe <- [];\nhits <- [];\n"
        "function Contact(other) { ::hits.append(user * 10 + other.user); }\n"
        "function InitContact(id) { user = id; callbackGroup = 1; callbackMask = 1; "
        "SetCollisionCallbackFunction(::Contact); ::probe.append(this); }\n"));
    function_4aa3a0_this(PTR(root + 1), PTR(closure), "InitContact");
    {
        int32_t pair_actors[3];
        for (int i = 0; i < 3; ++i) {
            pair_actors[i] = function_463b40_this(manager,
                closure[0], closure[1], closure[2], 0, 0, -1,
                PTR(&g16), 0x05000002, i + 1, 0);
            CHECK(pair_actors[i] != 0);
            for (int j = 0; j < 4; ++j)
                *(float *)(intptr_t)(pair_actors[i] + 440 + j * 4) =
                    (float)(10 * i + (j >= 2 ? 10 : 0));
        }
        CHECK(retdec_actor_manager_refresh(manager) == 3);
        CHECK(*(int32_t *)(intptr_t)(manager + 124) != 0);
        CHECK(*(int32_t *)(intptr_t)(manager + 128) -
              *(int32_t *)(intptr_t)(manager + 124) >= 3 * 4);
        function_462e80(manager);
        CHECK(execute_source(vm, root + 2,
            "if (hits.len() != 4 || hits[0] != 12 || hits[1] != 21 || "
            "hits[2] != 23 || hits[3] != 32) throw \"collision pair order\";"));
        CHECK(function_48aa20(vm) == top);
        CHECK(execute_source(vm, root + 2,
            "hits.clear();\nprobe[0].callbackMask = 0;\n"));
        function_462e80(manager);
        CHECK(execute_source(vm, root + 2,
            "if (hits.len() != 3 || hits[0] != 21 || hits[1] != 23 || hits[2] != 32) "
            "throw \"directional masks\";\nhits.clear();\n"));
        *(unsigned char *)(intptr_t)(pair_actors[1] + 40) = 0;
        function_462e80(manager);
        CHECK(execute_source(vm, root + 2,
            "if (hits.len() != 0) throw \"inactive/disjoint pair\";"));
        *(unsigned char *)(intptr_t)(pair_actors[1] + 40) = 1;
        CHECK(execute_source(vm, root + 2,
            "probe[0].callbackMask = 1;\n"
            "probe[0].SetCollisionCallbackFunction(function(other) { "
            "::hits.append(user * 10 + other.user); callbackGroup = 0; });\n"));
        function_462e80(manager);
        CHECK(execute_source(vm, root + 2,
            "if (hits.len() != 3 || hits[0] != 12 || hits[1] != 23 || hits[2] != 32) "
            "throw \"callback mask mutation\";\n"
            "foreach (actor in probe) { actor.callbackGroup = 0; actor.callbackMask = 0; }\n"
            "child <- null;\nchildSteps <- 0;\n"
            "function InitChild(id) { vx = 5; ::child = this; "
            "SetUpdateFunction(function() { ::childSteps++; }); }\n"
            "probe[0].SetUpdateFunction(function() { "
            "::CreateActor(::InitChild, 100.0, 0.0, -1.0, 4); "
            "::probe[1].Release(); SetUpdateFunction(null); });\n"));
        target = PTR(function_469b40);
        CHECK(function_415550_this(PTR(root), PTR("CreateActor"),
            PTR(&target), 4, PTR(function_471df0), 0) >= 0);
        *(int32_t *)(intptr_t)(manager + 64) = -1;
        CHECK(retdec_actor_manager_update(manager, PTR(g_retdec_camera_state)) == 3);
        CHECK(execute_source(vm, root + 2,
            "if (child.x != 105.0 || childSteps != 0) throw \"post-step refresh\";"));
        CHECK(*(int32_t *)(intptr_t)(pair_actors[1] + 28) == 0);
        CHECK(function_48aa20(vm) == top);
    }
    function_4a9d70_this(PTR(closure));
    puts("PASS: stage creation, collision lifecycle and pair dispatch");
    return 0;
}
