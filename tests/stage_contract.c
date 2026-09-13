/* Exercise the actual reconstructed functions without WinMain, graphics or DAT startup. */
#include "../src/decompiled/6kinoko_rebuilt.c"

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)
#define PTR(value) ((int32_t)(intptr_t)(value))

static int draw_count;
static int vm_failures;
static int expected_vm_error;
static LONG CALLBACK contract_exception(PEXCEPTION_POINTERS info) {
    if (info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
        info->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
        fprintf(stderr, "contract exception=%08lx rva=%08lx address=%p\n",
            info->ExceptionRecord->ExceptionCode,
            info->ContextRecord->Eip - (DWORD)(uintptr_t)GetModuleHandleA(NULL),
            info->ExceptionRecord->ExceptionAddress);
        void *stack[20];
        USHORT count=CaptureStackBackTrace(0,20,stack,NULL);
        for(USHORT i=0;i<count;++i)
            fprintf(stderr,"  stack rva=%08lx\n",(DWORD)(uintptr_t)stack[i]-(DWORD)(uintptr_t)GetModuleHandleA(NULL));
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
static int32_t __fastcall count_draw(void *self, void *unused, int32_t x, int32_t y) {
    (void)self; (void)unused; (void)x; (void)y;
    ++draw_count;
    return 0;
}

static void position_actor(int32_t actor, float x, float y) {
    *(float *)(intptr_t)(actor + 240) = x;
    *(float *)(intptr_t)(actor + 244) = y;
    *(float *)(intptr_t)(actor + 440) = x - 8;
    *(float *)(intptr_t)(actor + 444) = y - 16;
    *(float *)(intptr_t)(actor + 448) = x + 8;
    *(float *)(intptr_t)(actor + 452) = y;
}

void retdec_trace(const char *message) {
    if (!expected_vm_error && message && strstr(message, "stagevm:failure-error")) ++vm_failures;
    if (!expected_vm_error && message && (strstr(message, "stagevm:compile-error") ||
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
    if (!result && !expected_vm_error && *(int32_t *)(intptr_t)(vm + 64) == 0x08000010)
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

struct pat_fixture {
    unsigned char bytes[4096];
    uint32_t size;
};

static int execute_asset(int32_t vm, const int32_t *environment, const char *path) {
    int32_t reader = 0, script[26] = {0};
    unsigned char *bytes;
    int result;
    if (!function_407370(PTR(&reader), path)) return 0;
    script[24] = *(int32_t *)(intptr_t)(reader + 12);
    bytes = (unsigned char *)malloc((size_t)script[24]);
    result = bytes && retdec_reader_read_exact(reader, bytes, (uint32_t)script[24]);
    script[23] = PTR(bytes);
    if (result) result = retdec_execute_embedded_act_script(vm, PTR(script), environment);
    retdec_destroy_reader((int32_t *)(intptr_t)reader);
    free(bytes);
    return result;
}

static void pat_value(struct pat_fixture *pat, int32_t value, uint32_t width) {
    if (pat->size + width > sizeof(pat->bytes)) abort();
    memcpy(pat->bytes + pat->size, &value, width);
    pat->size += width;
}

static void pat_alias(struct pat_fixture *pat, int32_t to, int32_t from) {
    pat_value(pat, -1, 4);
    pat_value(pat, to, 4);
    pat_value(pat, from, 4);
}

static void pat_node(struct pat_fixture *pat, int32_t take, uint32_t frames) {
    pat_value(pat, take, 4);
    pat_value(pat, 0, 2);
    pat_value(pat, 0, 2);
    pat_value(pat, 1, 1);
    pat_value(pat, frames, 4);
}

static void pat_frame(struct pat_fixture *pat, int16_t duration,
                      const int32_t *bounds) {
    pat_value(pat, 0, 4);
    for (int i = 0; i < 6; ++i) pat_value(pat, 0, 2);
    pat_value(pat, duration, 2);
    pat_value(pat, 1, 1);
    for (int i = 0; i < 20; ++i) pat_value(pat, 0, 2);
    pat_value(pat, 0, 1);
    pat_value(pat, 0, 4);
    pat_value(pat, 0, 4);
    pat_value(pat, bounds != NULL, 1);
    if (bounds != NULL)
        for (int i = 0; i < 4; ++i) pat_value(pat, bounds[i], 4);
    pat_value(pat, 0, 1);
    pat_value(pat, 0, 1);
    for (int i = 0; i < 6; ++i) pat_value(pat, 0, 4);
    for (int i = 0; i < 3; ++i) pat_value(pat, 0, 2);
}

static int32_t pat_lookup(int32_t manager, int32_t take) {
    int32_t entry = 0;
    function_4706c0_this(manager + 36, &entry, &take);
    return entry == *(int32_t *)(intptr_t)(manager + 40) ? 0 :
        *(int32_t *)(intptr_t)(entry + 16);
}

static int test_pat_records(int32_t manager) {
    struct pat_fixture pat = {{0}, 0};
    const int32_t base = 0x60000100;
    const int32_t bounds[4] = {-6, -20, 10, -1};
    const int32_t later_bounds[4] = {-4, -8, 7, -1};
    int32_t reader[7] = {0}, head, second, third, blank, actor;
    int32_t initial_count = *(int32_t *)(intptr_t)(manager + 44);
    char directory[MAX_PATH], path[MAX_PATH];
    HANDLE file;
    DWORD written;

    pat_value(&pat, 13, 4);
    pat_alias(&pat, base, base + 1);
    pat_alias(&pat, base + 1, base + 2);
    pat_alias(&pat, base + 3, base + 999);
    pat_node(&pat, base + 2, 3);
    pat_frame(&pat, 2, NULL);
    pat_frame(&pat, 3, bounds);
    pat_frame(&pat, 4, later_bounds);
    pat_node(&pat, -2, 1);
    pat_frame(&pat, 5, later_bounds);
    pat_alias(&pat, base + 4, base + 2);
    pat_node(&pat, -2, 1);
    pat_frame(&pat, 6, NULL);
    pat_node(&pat, base + 5, 1);
    pat_frame(&pat, 7, NULL);
    pat_alias(&pat, base + 6, base + 2);
    pat_alias(&pat, base + 6, base + 5);
    pat_alias(&pat, base + 5, base + 999);
    pat_alias(&pat, base + 7, base + 8);
    pat_alias(&pat, base + 8, base + 7);
    CHECK(GetTempPathA(sizeof(directory), directory));
    CHECK(GetTempFileNameA(directory, "pat", 0, path));
    file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    CHECK(file != INVALID_HANDLE_VALUE);
    CHECK(WriteFile(file, pat.bytes, pat.size, &written, NULL) && written == pat.size);
    reader[1] = PTR(file);
    reader[3] = pat.size;
    g765 = 1;
    CHECK(retdec_pat_read_animations(PTR(reader), manager, 0));
    CHECK(reader[5] == reader[3]);
    CHECK(CloseHandle(file));
    g765 = 0;

    head = pat_lookup(manager, base + 2);
    CHECK(head);
    CHECK(pat_lookup(manager, base) == head);
    CHECK(pat_lookup(manager, base + 1) == head);
    CHECK(pat_lookup(manager, base + 4) == head);
    CHECK(pat_lookup(manager, base + 6) == head);
    CHECK(!pat_lookup(manager, base + 3));
    CHECK(!pat_lookup(manager, base + 7));
    CHECK(!pat_lookup(manager, base + 8));
    CHECK(*(int32_t *)(intptr_t)(manager + 44) == initial_count + 6);
    CHECK(*(uint8_t *)(intptr_t)(head + 25) == 1);
    CHECK(memcmp((void *)(intptr_t)(head + 28), bounds, sizeof(bounds)) == 0);
    CHECK(*(int32_t *)(intptr_t)(head + 44) == 9);
    CHECK(*(int32_t *)(intptr_t)(head + 4) == 0);
    second = *(int32_t *)(intptr_t)head;
    CHECK(second && second != head);
    third = *(int32_t *)(intptr_t)second;
    CHECK(third && third != head && third != second);
    CHECK(*(int32_t *)(intptr_t)third == head);
    CHECK(*(int32_t *)(intptr_t)(second + 4) == head);
    CHECK(*(int32_t *)(intptr_t)(third + 4) == second);
    CHECK(memcmp((void *)(intptr_t)(second + 28), later_bounds, sizeof(bounds)) == 0);
    CHECK(*(int32_t *)(intptr_t)(second + 44) == 5);
    CHECK(*(int32_t *)(intptr_t)(third + 44) == 6);
    CHECK(*(uint8_t *)(intptr_t)(third + 25) == 0);
    blank = pat_lookup(manager, base + 5);
    CHECK(blank && blank != head && *(int32_t *)(intptr_t)blank == blank);
    CHECK(*(int32_t *)(intptr_t)(blank + 4) == 0);

    actor = function_463b40_this(manager, PTR(&g16), g483, g484,
        100, 200, -1, PTR(&g16), g483, g484, 0);
    CHECK(actor);
    function_462280_this(actor, base);
    CHECK(*(int32_t *)(intptr_t)(actor + 200) == head);
    CHECK(*(float *)(intptr_t)(actor + 440) == 93.5f);
    CHECK(*(float *)(intptr_t)(actor + 444) == 180.0f);
    CHECK(*(float *)(intptr_t)(actor + 448) == 110.5f);
    CHECK(*(float *)(intptr_t)(actor + 452) == 200.0f);
    for (int i = 0; i < 18; ++i) retdec_actor_tick(actor);
    CHECK(*(int32_t *)(intptr_t)(actor + 200) == head);
    CHECK(*(int32_t *)(intptr_t)(actor + 204) == *(int32_t *)(intptr_t)(head + 8));
    CHECK(*(int32_t *)(intptr_t)(actor + 212) == 0);
    function_462280_this(actor, base + 5);
    CHECK(*(float *)(intptr_t)(actor + 440) == 100.0f);
    CHECK(*(float *)(intptr_t)(actor + 444) == 200.0f);
    CHECK(*(float *)(intptr_t)(actor + 448) == 100.0f);
    CHECK(*(float *)(intptr_t)(actor + 452) == 200.0f);
    CHECK(*(int32_t *)(intptr_t)(actor + 388) == 0);
    function_469700();
    return 0;
}

static int test_player_pat(int32_t manager, const char *path, uint32_t offset) {
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    int32_t reader[7] = {0}, actor;
    int32_t seen[4096], unique = 0, takes = 0, aliases = 0;
    unsigned char version;
    unsigned short textures;
    CHECK(file != INVALID_HANDLE_VALUE);
    reader[1] = PTR(file);
    reader[3] = GetFileSize(file, NULL);
    ((unsigned char *)reader)[24] = (unsigned char)((offset >> 1) | 0x23);
    g765 = 1;
    CHECK(retdec_pat_read_u8(PTR(reader), &version) && version == 5);
    CHECK(retdec_pat_read_u16(PTR(reader), &textures));
    CHECK(retdec_pat_skip_bytes(PTR(reader), textures * 128u));
    CHECK(retdec_pat_read_animations(PTR(reader), manager, 0));
    CHECK(reader[5] == reader[3]);
    CHECK(CloseHandle(file));
    g765 = 0;
    actor = function_463b40_this(manager, PTR(&g16), g483, g484,
        100, 200, -1, PTR(&g16), g483, g484, 0);
    CHECK(actor);
    for (int32_t take = 0; take < 10000; ++take) {
        int32_t node = pat_lookup(manager, take);
        int32_t index;
        if (!node) continue;
        ++takes;
        for (index = 0; index < unique && seen[index] != node; ++index) {}
        if (index == unique) {
            CHECK(unique < 4096);
            seen[unique++] = node;
        } else {
            ++aliases;
        }
        for (int direction = -1; direction <= 1; direction += 2) {
            *(float *)(intptr_t)(actor + 272) = (float)direction;
            function_462280_this(actor, take);
            CHECK(*(int32_t *)(intptr_t)(actor + 200) == node);
            CHECK(*(int32_t *)(intptr_t)(actor + 204) == *(int32_t *)(intptr_t)(node + 8));
            CHECK(*(int32_t *)(intptr_t)(actor + 204) != 0);
            CHECK(*(float *)(intptr_t)(actor + 440) <= *(float *)(intptr_t)(actor + 448));
            CHECK(*(float *)(intptr_t)(actor + 444) <= *(float *)(intptr_t)(actor + 452));
        }
    }
    printf("PAT textures=%u takes=%d unique=%d aliases=%d bytes=%d\n",
        textures, takes, unique, aliases, reader[5]);
    CHECK(takes > 0);
    function_462280_this(actor, 0);
    CHECK(*(float *)(intptr_t)(actor + 440) == 89.5f);
    CHECK(*(float *)(intptr_t)(actor + 444) == 170.0f);
    CHECK(*(float *)(intptr_t)(actor + 448) == 110.5f);
    CHECK(*(float *)(intptr_t)(actor + 452) == 201.0f);
    CHECK(*(int16_t *)(intptr_t)(actor + 388) == 21);
    CHECK(*(int16_t *)(intptr_t)(actor + 390) == 31);
    {
        unsigned char chip[48] = {0};
        float floor_layout[8] = {0};
        KinokoCollisionRecord floor = {chip, floor_layout, 0};
        *(int16_t *)(chip + 12) = 256;
        *(int16_t *)(chip + 14) = 32;
        floor_layout[4] = 240;
        CHECK(kinoko_actor_collision_move((void *)(intptr_t)actor, &floor, 1, 4, 50) == 0);
        CHECK(*(float *)(intptr_t)(actor + 240) == 104);
        CHECK(*(float *)(intptr_t)(actor + 244) == 239);
        CHECK(*(int32_t *)(intptr_t)(actor + 288) == 0);
        CHECK(*(int32_t *)(intptr_t)(actor + 296) == 1);
        CHECK(*(float *)(intptr_t)(actor + 304) == 21);
        function_462280_this(actor, 0);
        CHECK(*(float *)(intptr_t)(actor + 452) == 240);
    }
    function_469700();
    puts("PASS: original player PAT consumed through the runtime parser");
    return 0;
}

static int test_player_walking(int32_t manager, int32_t vm, int32_t *root,
                               const char *ground_path, const char *player_path,
                               const char *constant_path, const char *reference_dir,
                               const char *stage_path) {
    int32_t scripts[3], init[3], actor;
    int32_t layout[100] = {0}, layer[80] = {0}, resource[20] = {0};
    int32_t records[1][8] = {{0x443, 0, 240}};
    struct retdec_mcd_chip chip = {0};
    struct retdec_mcd_data data = {1, &chip, 0, NULL};
    CHECK(execute_file(vm, root + 2, constant_path));
    CHECK(execute_source(vm, root + 2, "t_player <- {};"));
    function_4aa3a0_this(PTR(root + 1), PTR(scripts), "t_player");
    CHECK(execute_file(vm, scripts + 1, player_path));
    CHECK(execute_file(vm, scripts + 1, ground_path));
    /* Isolate ground input from ladder/swim and the rest of the game scene. */
    CHECK(execute_source(vm, root + 2,
        "t_player.SetLaddar <- function() { return false; };\n"
        "input <- { x = 0, y = 0, b0 = 0, b2 = 0, b3 = 0 };\n"
        "camera <- { top = -1000, bottom = 1000 };\n"
        "time = 1000; stageWaterLevel = 10000; stageWaterType = 0;\n"
        "stageLayerVector = -1; stageIce = false; stageTimeStop = false;\n"
        "function InitWalkingProbe(id) {\n"
        "  user = { type = TYPE_2HEAD, take = 0, hold = null, water = false,\n"
        "           rolling = false, pitch = 1.0, dash_count = 0, hover = 0,\n"
        "           deadCount = 0, clearCount = 0, moveCount = 0, goalCount = 0,\n"
        "           changingCount = 0, invincibleCount = 0, count8head = 0,\n"
        "           countUFO = 0, inertia = 0.0, hitblock = false, slide = false,\n"
        "           hand = null, swim = false, ladder = false, hitCount = 0,\n"
        "           vx = 0.0, vector = false };\n"
        "  user.SetTake <- ::t_player.SetTake.bindenv(this);\n"
        "  user.SetDead <- function(value) { throw \"unexpected player death\"; };\n"
        "  user.SetTake(TAKE_STAND); collisionMask = 1;\n"
        "  funcUpdate = ::t_player.Stand.bindenv(this);\n"
        "  SetUpdateFunction(::t_player.Update);\n"
        "  ::walkingProbe <- this;\n"
        "}\n"));
    CHECK(function_468950_this(PTR(g_514300_storage), manager));
    layout[0] = PTR(&g327);
    layout[60] = 256;
    layout[61] = 32;
    layout[66] = PTR(records);
    layout[67] = PTR(records + 1);
    layout[78] = PTR(layer);
    layout[79] = PTR(resource);
    *((uint8_t *)layer + 140) = 1;
    resource[16] = PTR(&data);
    chip.chip_id = 0x443;
    *(int16_t *)(chip.bytes + 12) = 256;
    *(int16_t *)(chip.bytes + 14) = 32;
    *(int16_t *)(chip.bytes + 34) = 0;
    CHECK(function_4693a0(PTR(layout)));
    function_4aa3a0_this(PTR(root + 1), PTR(init), "InitWalkingProbe");
    actor = function_463b40_this(manager, init[0], init[1], init[2],
        100, 239, -1, PTR(&g16), g483, g484, 0);
    CHECK(actor);
    CHECK(execute_source(vm, root + 2,
        "if (typeof walkingProbe.funcUpdate != \"function\") throw \"missing walking callback\";"));
    retdec_actor_manager_refresh(manager);
    function_468620_this(PTR(g_514300_storage));
    retdec_actor_update_motion(actor);
    CHECK(*(int32_t *)(intptr_t)(actor + 296) == 1);
    for (int phase = 0; phase < 3; ++phase) {
        float start_x = *(float *)(intptr_t)(actor + 240);
        CHECK(execute_source(vm, root + 2, phase == 0 ? "input.x = 1;" :
            phase == 1 ? "input.x = -1;" : "input.x = 0;"));
        for (int frame = 0; frame < 30; ++frame) {
            float old_x = *(float *)(intptr_t)(actor + 240);
            int failures_before = vm_failures;
            retdec_actor_tick(actor);
            CHECK(vm_failures == failures_before);
            function_468620_this(PTR(g_514300_storage));
            retdec_actor_update_motion(actor);
            if (*(float *)(intptr_t)(actor + 304) < 12 ||
                *(float *)(intptr_t)(actor + 452) != 240)
                fprintf(stderr, "walk phase=%d frame=%d x=%g -> %g bottom=%g width=%g "
                    "vx=%g direction=%g take=%d\n", phase, frame, old_x,
                    *(float *)(intptr_t)(actor + 240), *(float *)(intptr_t)(actor + 452),
                    *(float *)(intptr_t)(actor + 304), *(float *)(intptr_t)(actor + 256),
                    *(float *)(intptr_t)(actor + 272), *(int32_t *)(intptr_t)(actor + 208));
            CHECK(*(float *)(intptr_t)(actor + 304) >= 12);
            CHECK(*(float *)(intptr_t)(actor + 452) == 240);
            CHECK(*(int32_t *)(intptr_t)(actor + 288) == 0);
            CHECK(*(int32_t *)(intptr_t)(actor + 296) == 1);
            CHECK(execute_source(vm, root + 2,
                "if (walkingProbe.freeWidth < 12 || walkingProbe.hitTop || "
                "walkingProbe.bottom != 240) throw \"invalid script-visible bounds\";"));
        }
        printf("walk phase=%d x=%g -> %g vx=%g take=%d\n", phase, start_x,
            *(float *)(intptr_t)(actor + 240), *(float *)(intptr_t)(actor + 256),
            *(int32_t *)(intptr_t)(actor + 208));
        if (phase == 0) CHECK(*(float *)(intptr_t)(actor + 240) > start_x + 30);
        if (phase == 1) CHECK(*(float *)(intptr_t)(actor + 240) < start_x);
        if (phase == 2) CHECK(*(float *)(intptr_t)(actor + 256) == 0);
    }
    function_469700();
    function_468950_this(PTR(g_514300_storage), manager);
    if (reference_dir != NULL) {
        int32_t act[60];
        float entry_x = 0, entry_y = 0;
        int found_entry = 0;
        char path[MAX_PATH];
        for (char archive = 'a'; archive <= 'c'; ++archive) {
            sprintf_s(path, sizeof(path), "%s/6kinoko_%c.dat", reference_dir, archive);
            CHECK(function_410500(path));
        }
        CHECK(g678 == 0);
        function_427530(PTR(act));
        CHECK(function_428000(PTR(act), stage_path));
        for (int32_t entry = act[52]; entry != act[53]; entry += 4) {
            int32_t actual_layer = *(int32_t *)(intptr_t)entry;
            int32_t sentinel = *(int32_t *)(intptr_t)(actual_layer + 180);
            printf("stage layer=%s mask=%d offset=(%g,%g)\n",
                retdec_std_string_data(actual_layer + 112),
                *(uint8_t *)(intptr_t)(actual_layer + 140),
                *(float *)(intptr_t)(actual_layer + 144), *(float *)(intptr_t)(actual_layer + 148));
            for (int32_t item = *(int32_t *)(intptr_t)sentinel; item != sentinel;
                 item = *(int32_t *)(intptr_t)item) {
                int32_t key = *(int32_t *)(intptr_t)(item + 8);
                int32_t actual_layout = *(int32_t *)(intptr_t)(key + 4);
                if (!retdec_map_chip_data(actual_layout)) continue;
                const char *layer_name = retdec_std_string_data(actual_layer + 112);
                if (strncmp(layer_name, "te", 2) == 0 || strncmp(layer_name, "wa", 2) == 0)
                    CHECK(function_4693a0(actual_layout));
                int32_t begin = *(int32_t *)(intptr_t)(actual_layout + 264);
                int32_t end = *(int32_t *)(intptr_t)(actual_layout + 268);
                if (strncmp(layer_name, "ev", 2) == 0) {
                    for (int32_t record = begin; record != end; record += 32) {
                        if (*(int32_t *)(intptr_t)record == 1) {
                            struct retdec_mcd_chip *marker = retdec_mcd_find_chip(
                                retdec_map_chip_data(actual_layout), 1);
                            CHECK(marker);
                            entry_x = *(int32_t *)(intptr_t)(record + 4) +
                                *(int16_t *)(marker->bytes + 12) * 0.5f;
                            entry_y = (float)(*(int32_t *)(intptr_t)(record + 8) +
                                *(int16_t *)(marker->bytes + 14) - 1);
                            found_entry = 1;
                        }
                    }
                }
                printf("  records=%d max=(%d,%d)\n", (end - begin) / 32,
                    *(int32_t *)(intptr_t)(actual_layout + 240),
                    *(int32_t *)(intptr_t)(actual_layout + 244));
                for (int32_t record = begin; record < end && record < begin + 5 * 32; record += 32) {
                    struct retdec_mcd_chip *actual_chip = retdec_mcd_find_chip(
                        retdec_map_chip_data(actual_layout), *(uint32_t *)(intptr_t)record);
                    printf("  id=%x xy=(%d,%d) size=(%d,%d) flags=%x shape=%d\n",
                        *(int32_t *)(intptr_t)record, *(int32_t *)(intptr_t)(record + 4),
                        *(int32_t *)(intptr_t)(record + 8),
                        actual_chip ? *(int16_t *)(actual_chip->bytes + 12) : 0,
                        actual_chip ? *(int16_t *)(actual_chip->bytes + 14) : 0,
                        actual_chip ? *(int32_t *)(actual_chip->bytes + 16) : 0,
                        actual_chip ? *(int16_t *)(actual_chip->bytes + 34) : 0);
                }
            }
        }
        CHECK(found_entry);
        actor = function_463b40_this(manager, init[0], init[1], init[2],
            entry_x, entry_y, -1, PTR(&g16), g483, g484, 0);
        CHECK(actor);
        retdec_actor_manager_refresh(manager);
        function_468620_this(PTR(g_514300_storage));
        retdec_actor_update_motion(actor);
        CHECK(*(int32_t *)(intptr_t)(actor + 296) == 1);
        for (int direction = 1; direction >= -1; direction -= 2) {
            CHECK(execute_source(vm, root + 2, direction == 1 ? "input.x = 1;" : "input.x = -1;"));
            for (int frame = 0; frame < 60; ++frame) {
                int failures_before = vm_failures;
                CHECK(execute_source(vm, root + 2,
                    "if (typeof walkingProbe.GetChipFlag() != \"integer\") "
                    "throw \"invalid chip flag result\";"));
                retdec_actor_tick(actor);
                function_468620_this(PTR(g_514300_storage));
                retdec_actor_update_motion(actor);
                if (failures_before != vm_failures || *(float *)(intptr_t)(actor + 304) < 12 ||
                    *(float *)(intptr_t)(actor + 452) > 896)
                    fprintf(stderr, "actual stage dir=%d frame=%d xy=(%g,%g) free=(%g,%g) "
                        "flags=%x hits=(%d,%d,%d,%d) vx=%g\n", direction, frame,
                        *(float *)(intptr_t)(actor + 240), *(float *)(intptr_t)(actor + 244),
                        *(float *)(intptr_t)(actor + 304), *(float *)(intptr_t)(actor + 308),
                        *(int32_t *)(intptr_t)(actor + 472), *(int32_t *)(intptr_t)(actor + 284),
                        *(int32_t *)(intptr_t)(actor + 288), *(int32_t *)(intptr_t)(actor + 292),
                        *(int32_t *)(intptr_t)(actor + 296), *(float *)(intptr_t)(actor + 256));
                CHECK(failures_before == vm_failures);
                CHECK(*(float *)(intptr_t)(actor + 304) >= 12);
                CHECK(*(int32_t *)(intptr_t)(actor + 288) == 0);
            }
        }
        function_469700();
        function_468950_this(PTR(g_514300_storage), manager);
        retdec_destroy_cact_object(PTR(act));
        printf("PASS: original %s terrain walking in both directions (TYPE_2HEAD)\n", stage_path);
    }
    function_4a9d70_this(PTR(init));
    function_4a9d70_this(PTR(scripts));
    puts("PASS: original ground scripts walking, turning and stopping on a flat floor");
    return 0;
}

/* Recover ESP even for the broken entry, so an ABI regression reports a failure. */
static __declspec(naked) int32_t probe_set_step_stack(int32_t actor, int32_t object) {
    __asm {
        push ebp
        mov ebp, esp
        mov ecx, [ebp + 8]
        mov edx, [ebp + 12]
        push [edx + 8]
        push [edx + 4]
        push [edx]
        call function_4606d0
        mov eax, esp
        sub eax, ebp
        mov esp, ebp
        pop ebp
        ret
    }
}

static int test_actor_step(int32_t manager, int32_t vm, int32_t *root) {
    int32_t actors[3], object[3], controls[2], weak_counts[2], refs[2];
    int32_t top = function_48aa20(vm);
    for (int i = 0; i < 3; ++i) {
        actors[i] = function_463b40_this(manager, PTR(&g16), g483, g484,
            0, 0, -1, PTR(&g16), g483, g484, 0);
        CHECK(actors[i]);
    }
    function_4a9840_this(PTR(root + 1), "stepRider", actors[0] + 44);
    function_4a9840_this(PTR(root + 1), "stepFirst", actors[1] + 44);
    function_4a9840_this(PTR(root + 1), "stepSecond", actors[2] + 44);
    for (int i = 0; i < 2; ++i) {
        controls[i] = *(int32_t *)(intptr_t)(actors[i + 1] + 28);
        weak_counts[i] = *(int32_t *)(intptr_t)(controls[i] + 8);
        refs[i] = *(int32_t *)(intptr_t)(*(int32_t *)(intptr_t)(actors[i + 1] + 52) + 4);
    }
    function_4a9500_this(object, actors[1] + 44);
    int32_t stack_delta = probe_set_step_stack(actors[0], PTR(object));
    if (stack_delta != 0) fprintf(stderr, "SetStep ESP delta: %d (expected 0)\n", stack_delta);
    CHECK(stack_delta == 0);
    CHECK(*(int32_t *)(intptr_t)(actors[0] + 32) == *(int32_t *)(intptr_t)(actors[1] + 24));
    CHECK(execute_source(vm, root + 2,
        "if (stepRider.step != stepFirst) throw \"native step binding missing\";\n"
        "stepRider.SetStep(null);"));
    for (int round = 0; round < 64; ++round) {
        CHECK(execute_source(vm, root + 2,
            "stepRider.SetStep(stepFirst);\nstepRider.SetStep(stepFirst);\n"
            "if (stepRider.step != stepFirst) throw \"first step binding\";\n"
            "stepRider.SetStep(stepSecond);\n"
            "if (stepRider.step != stepSecond) throw \"replacement step binding\";"));
        CHECK(*(int32_t *)(intptr_t)(actors[0] + 36) == controls[1]);
        CHECK(*(int32_t *)(intptr_t)(controls[0] + 8) == weak_counts[0]);
        CHECK(*(int32_t *)(intptr_t)(controls[1] + 8) == weak_counts[1] + 1);
        CHECK(execute_source(vm, root + 2,
            "stepRider.SetStep(null);\n"
            "if (stepRider.step != null) throw \"step detach\";"));
        CHECK(*(int32_t *)(intptr_t)(actors[0] + 32) == 0);
        CHECK(*(int32_t *)(intptr_t)(actors[0] + 36) == 0);
        for (int i = 0; i < 2; ++i) {
            CHECK(*(int32_t *)(intptr_t)(controls[i] + 8) == weak_counts[i]);
            CHECK(*(int32_t *)(intptr_t)(*(int32_t *)(intptr_t)(actors[i + 1] + 52) + 4) == refs[i]);
        }
        CHECK(function_48aa20(vm) == top);
    }
    function_469700();
    CHECK(execute_source(vm, root + 2, "stepRider = null;\nstepFirst = null;\nstepSecond = null;"));
    puts("PASS: SetStep thiscall stack and 64 native bind/rebind/replace/detach cycles");
    return 0;
}

static int test_actor_reset(int32_t manager, int32_t vm, int32_t *root) {
    int32_t init[3], seed[3], actor, parent, parent_object[3], parent_control;
    int32_t stack_top = function_48aa20(vm);
    CHECK(execute_source(vm, root + 2,
        "resetCalls <- 0; resetTicks <- 0; resetSeed <- { priority = 7 };\n"
        "function ResetInit(value) {\n"
        "  if (value != ::resetSeed) throw \"reset argument changed\";\n"
        "  ::resetCalls++; user = { generation = ::resetCalls };\n"
        "  priority = value.priority; vx = 3.0;\n"
        "  SetUpdateFunction(function() { ::resetTicks++; });\n"
        "  SetCollisionCallbackFunction(function(other) {});\n"
        "  ::resetProbe <- this;\n"
        "}"));
    function_4aa3a0_this(PTR(root + 1), PTR(init), "ResetInit");
    function_4aa3a0_this(PTR(root + 1), PTR(seed), "resetSeed");
    actor = function_463b40_this(manager, init[0], init[1], init[2],
        100, 200, -1, seed[0], seed[1], seed[2], 0);
    CHECK(actor);
    parent = function_463b40_this(manager, PTR(&g16), g483, g484,
        0, 0, -1, PTR(&g16), g483, g484, 0);
    CHECK(parent);
    parent_control = *(int32_t *)(intptr_t)(parent + 28);
    for (int round = 0; round < 32; ++round) {
        int32_t old_weak[2], locked[2];
        int32_t parent_weak_count = *(int32_t *)(intptr_t)(parent_control + 8);
        int32_t argument_refs = *(int32_t *)(intptr_t)(seed[2] + 4);
        int32_t original_handle = *(int32_t *)(intptr_t)(actor + 12);
        old_weak[0] = *(int32_t *)(intptr_t)(actor + 24);
        old_weak[1] = *(int32_t *)(intptr_t)(actor + 28);
        InterlockedIncrement((volatile LONG *)(intptr_t)(old_weak[1] + 8));
        function_4a9500_this(parent_object, parent + 44);
        function_4606d0_this(actor, PTR(parent_object));
        CHECK(execute_source(vm, root + 2,
            "oldReset <- resetProbe; oldUser <- oldReset.user; oldStep <- oldReset.step;\n"
            "resetProbe.x = 700; resetProbe.y = 800;\n"
            "resetProbe.direction = 1; resetProbe.priority = 123;\n"
            "resetProbe.SetChipFlag(32); resetProbe.Reset();\n"
            "if (resetProbe == oldReset || oldReset.user != oldUser || oldReset.step != oldStep) "
            "throw \"old reset instance fields changed\";\n"
            "if (resetProbe.x != 100 || resetProbe.y != 200 || resetProbe.direction != -1 || "
            "resetProbe.priority != 7 || resetProbe.user.generation != resetCalls) "
            "throw \"reset initializer not replayed\";"));
        function_45e410_this(PTR(old_weak), locked);
        CHECK(locked[0] == 0 && locked[1] == 0);
        retdec_actor_release_weak(old_weak[1]);
        CHECK(*(int32_t *)(intptr_t)(parent_control + 8) == parent_weak_count);
        CHECK(*(int32_t *)(intptr_t)(seed[2] + 4) == argument_refs);
        CHECK(*(int32_t *)(intptr_t)(actor + 12) == original_handle);
        CHECK(*(int64_t *)(intptr_t)(actor + 392) == 32);
        CHECK(*(int32_t *)(intptr_t)(actor + 36) == 0);
        CHECK(*(int32_t *)(intptr_t)(actor + 112) == 0x08000100);
        CHECK(*(int32_t *)(intptr_t)(actor + 140) == 0x08000100);
        retdec_actor_tick(actor);
        CHECK(retdec_actor_manager_refresh(manager) == 2);
        CHECK(function_48aa20(vm) == stack_top);
    }
    CHECK(execute_source(vm, root + 2,
        "if (resetCalls != 33 || resetTicks != 32) throw \"reset callback counts\";\n"
        "resetProbe.SetUpdateFunction(function() {\n"
        " local previous = user; Reset();\n"
        " if(user!=previous) throw \"Reset cleared executing instance\";\n"
        " user.afterReset <- 42;\n"
        "});"));
    retdec_actor_tick(actor);
    CHECK(vm_failures==0);
    CHECK(*(int32_t *)(intptr_t)(actor+112)==0x08000100);
    retdec_actor_tick(actor);
    CHECK(execute_source(vm,root+2,
        "if(resetCalls!=34 || resetTicks!=33) throw \"post-Reset callback continuation\";"));
    function_469700();
    function_4a9d70_this(PTR(init));
    function_4a9d70_this(PTR(seed));
    puts("PASS: Reset replays initialization and retires old references across 32 resets");
    return 0;
}

static int test_stone_placement(int32_t manager, int32_t vm, int32_t *root) {
    int32_t reader = 0, script[26] = {0}, scripts[3], init[3], rider_init[3];
    unsigned char version;
    unsigned short textures;
    unsigned char *bytes;
    CHECK(g765 != 0 && g678 == 0);
    CHECK(function_407370(PTR(&reader), "data/actor/item/item.pat"));
    CHECK(retdec_pat_read_u8(reader, &version));
    CHECK(retdec_pat_read_u16(reader, &textures));
    CHECK(retdec_pat_skip_bytes(reader, textures * 128u));
    CHECK(retdec_pat_read_animations(reader, manager, 0));
    CHECK(*(int32_t *)(intptr_t)(reader + 20) - *(int32_t *)(intptr_t)(reader + 16) ==
        *(int32_t *)(intptr_t)(reader + 12));
    retdec_destroy_reader((int32_t *)(intptr_t)reader);
    reader = 0;
    CHECK(pat_lookup(manager, 1130));
    CHECK(execute_source(vm, root + 2,
        "t_item <- {};\nstoneSounds <- [];\nstoneEffects <- [];\n"
        "player <- null;\n"
        "camera <- { left = -1000, right = 1000, top = -1000, bottom = 1000 };\n"
        "function InitStoneRider(v) {\n"
        "  user = { stone = null }; SetTake(0);\n"
        "  collisionGroup = GP_PLAYER; callbackGroup = GP_PLAYER;\n"
        "  collisionMask = GP_LIFT; ::player = this;\n"
        "}\n"
        "function PlaySE(id) { ::stoneSounds.append(id); }\n"
        "function CreateEffect(x,y,z,id) { ::stoneEffects.append(id); return {}; }"));
    function_4aa3a0_this(PTR(root + 1), PTR(rider_init), "InitStoneRider");
    function_4aa3a0_this(PTR(root + 1), PTR(scripts), "t_item");
    CHECK(function_407370(PTR(&reader), "data/script/bullet.cv4"));
    script[24] = *(int32_t *)(intptr_t)(reader + 12);
    bytes = (unsigned char *)malloc((size_t)script[24]);
    CHECK(bytes);
    CHECK(retdec_reader_read_exact(reader, bytes, (uint32_t)script[24]));
    script[23] = PTR(bytes);
    CHECK(retdec_execute_embedded_act_script(vm, PTR(script), scripts + 1));
    retdec_destroy_reader((int32_t *)(intptr_t)reader);
    free(bytes);
    function_4aa3a0_this(PTR(scripts), PTR(init), "InitStone");
    CHECK(init[1] == 0x08000100);
    for (int round = 0; round < 8; ++round) {
        int direction = (round & 1) ? -1 : 1;
        int32_t top = function_48aa20(vm);
        int failures = vm_failures;
        int32_t rider = function_463b40_this(manager, rider_init[0], rider_init[1], rider_init[2],
            100, 200, -1, PTR(&g16), g483, g484, 0);
        CHECK(rider);
        CHECK(execute_source(vm, root + 2, direction == 1 ?
            "player.direction = 1.0;" : "player.direction = -1.0;"));
        int32_t stone = function_463b40_this(manager, init[0], init[1], init[2],
            100, 200, -1, PTR(&g16), g483, g484, 0);
        CHECK(stone && vm_failures == failures);
        CHECK(*(float *)(intptr_t)(stone + 240) == 100.0f + 40.0f * direction);
        CHECK(*(float *)(intptr_t)(stone + 244) == 200);
        CHECK(*(float *)(intptr_t)(stone + 256) == 0);
        CHECK(*(float *)(intptr_t)(stone + 260) == 0);
        CHECK(*(int32_t *)(intptr_t)(stone + 208) == 1130);
        CHECK(*(int64_t *)(intptr_t)(stone + 392) == 32);
        CHECK(*(int32_t *)(intptr_t)(stone + 236) == 0x800000);
        function_4a9840_this(PTR(root + 1), "stoneProbe", stone + 44);
        CHECK(execute_source(vm, root + 2,
            "if (player.user.stone != stoneProbe || stoneProbe.collisionGroup != GP_LIFT || "
            "stoneProbe.collisionMask != GP_TERRAIN || stoneProbe.user.time != 0) "
            "throw \"stone initialization incomplete\";\n"
            "player.x = stoneProbe.x;\n"
            "player.y = stoneProbe.top + 2 - (player.bottom - player.y);\n"
            "player.vy = 1.0;"));
        retdec_actor_refresh_bounds(rider);
        CHECK(function_462ce0(stone, rider) >= 0);
        CHECK(vm_failures == failures);
        CHECK(*(int32_t *)(intptr_t)(rider + 32) == *(int32_t *)(intptr_t)(stone + 24));
        CHECK(*(int32_t *)(intptr_t)(rider + 36) == *(int32_t *)(intptr_t)(stone + 28));
        CHECK(execute_source(vm, root + 2,
            "if (!stoneProbe.user.ride || player.step != stoneProbe) "
            "throw \"original stone callback did not bind rider\";\n"
            "player.vy = 0.0; stoneProbe.vx = player.direction * 2.0;"));
        CHECK(retdec_actor_manager_refresh(manager) == 2);
        for (int frame = 0; frame < 8; ++frame) {
            float old_x = *(float *)(intptr_t)(rider + 240);
            retdec_actor_tick(stone);
            function_468620_this(PTR(g_514300_storage));
            retdec_actor_update_motion(stone);
            retdec_actor_update_motion(rider);
            CHECK(vm_failures == failures);
            CHECK(*(float *)(intptr_t)(rider + 240) == old_x + direction * 2.0f);
            CHECK(*(int32_t *)(intptr_t)(rider + 36) == *(int32_t *)(intptr_t)(stone + 28));
        }
        if (round & 1) {
            CHECK(execute_source(vm, root + 2, "stoneProbe.Release();\nstoneProbe = null;"));
            CHECK(retdec_actor_manager_refresh(manager) == 1);
            int32_t locked[2];
            function_45e410_this(rider + 32, locked);
            CHECK(locked[0] == 0 && locked[1] == 0);
            retdec_actor_update_motion(rider);
            CHECK(*(float *)(intptr_t)(rider + 264) == 0);
            CHECK(execute_source(vm, root + 2, "player.SetStep(null);"));
        } else {
            CHECK(execute_source(vm, root + 2, "player.x = stoneProbe.right + 64;"));
            retdec_actor_update_motion(rider);
            CHECK(*(int32_t *)(intptr_t)(rider + 36) == 0);
            CHECK(execute_source(vm, root + 2,
                "if (player.step != null) throw \"walk-off did not detach\";"));
            retdec_actor_tick(stone);
            CHECK(vm_failures == failures);
            CHECK(execute_source(vm, root + 2,
                "if (stoneProbe.user.time != 0 || stoneSounds[stoneSounds.len()-1] != 35) "
                "throw \"original stone falling transition\";\n"
                "stoneProbe = null;"));
        }
        CHECK(function_48aa20(vm) == top);
        CHECK(execute_source(vm, root + 2, "stoneOwner <- player.user;"));
        function_469700();
        CHECK(execute_source(vm, root + 2,
            "if (stoneOwner.stone != null || player.user != null) "
            "throw \"stone or rider retained state after clear\";"));
    }
    CHECK(execute_source(vm, root + 2,
        "if (stoneSounds.len() != 12 || stoneEffects.len() != 8) throw \"stone media calls\";\n"
        "foreach (id in stoneSounds) if (id != 33 && id != 35) throw \"stone sound id\";\n"
        "foreach (id in stoneEffects) if (id != 1960) throw \"stone effect id\";"));
    function_4a9d70_this(PTR(rider_init));
    function_4a9d70_this(PTR(init));
    function_4a9d70_this(PTR(scripts));
    puts("PASS: original stone callbacks/PAT, eight placements/rides, walk-off and bound release");
    return 0;
}

static int test_floating_items(int32_t manager, int32_t vm, int32_t *root) {
    const char *initializers[] = {"InitPoint", "InitSave", "Init1up"};
    int32_t scripts[3], init[3];
    int32_t stack_top = function_48aa20(vm);
    CHECK(execute_source(vm, root + 2,
        "PR_FRONT <- 65535;\nt_item <- {};\nt_effect <- { InitNumber = function(v) {} };\n"
        "player <- { x = 100.0, top = 160.0, bottom = 200.0 };\n"
        "floatRewards <- [0,0,0];\nfloatNumbers <- [];\n"
        "function AddPoint() { ::floatRewards[0]++; }\n"
        "function WriteCurrentSaveData() { ::floatRewards[1]++; }\n"
        "function AddLife() { ::floatRewards[2]++; }\n"
        "function CreateActor(init,x,y,z,value) { ::floatNumbers.append(value); return {}; }\n"
        "function PlaySE(id) {}\nsrand(12345);"));
    function_4aa3a0_this(PTR(root + 1), PTR(scripts), "t_item");
    CHECK(execute_asset(vm, scripts + 1, "data/script/item.cv4"));
    for (int kind = 0; kind < 3; ++kind) {
        function_4aa3a0_this(PTR(scripts), PTR(init), initializers[kind]);
        CHECK(init[1] == 0x08000100);
        for (int round = 0; round < 4; ++round) {
            int init_failures = vm_failures;
            int32_t actor = function_463b40_this(manager, init[0], init[1], init[2],
                100, 180, -1, PTR(&g16), g483, g484, 0);
            int homing_frames = 0, released = 0;
            float max_distance = 0;
            CHECK(actor && vm_failures == init_failures &&
                *(int32_t *)(intptr_t)(actor + 208) == 1101 + kind);
            for (int frame = 0; frame < 180; ++frame) {
                float dx = 100.0f - *(float *)(intptr_t)(actor + 240);
                float dy = 180.0f - *(float *)(intptr_t)(actor + 244);
                float distance = sqrtf(dx * dx + dy * dy);
                if (distance > max_distance) max_distance = distance;
                int failures = vm_failures;
                retdec_actor_tick(actor);
                CHECK(vm_failures == failures);
                if (*(uint8_t *)(intptr_t)(actor + 22)) {
                    CHECK(distance < 16.0f && homing_frames > 0 && max_distance > 40.0f);
                    released = 1;
                    break;
                }
                float vx = *(float *)(intptr_t)(actor + 256);
                float vy = *(float *)(intptr_t)(actor + 260);
                if (fabsf(sqrtf(vx * vx + vy * vy) - 12.0f) < 0.0001f) {
                    if (!(vx * dx + vy * dy > 0.0f)) {
                        int32_t user[3], count[3];
                        function_4aa3a0_this(actor + 44, PTR(user), "user");
                        function_4aa3a0_this(PTR(user), PTR(count), "count");
                        fprintf(stderr, "float kind=%d round=%d frame=%d delta=(%g,%g) v=(%g,%g) count=%d\n",
                            kind, round, frame, dx, dy, vx, vy, count[2]);
                        function_4a9d70_this(PTR(count));
                        function_4a9d70_this(PTR(user));
                    }
                    CHECK(vx * dx + vy * dy > 0.0f);
                    CHECK(fabsf(sqrtf(vx * vx + vy * vy) - 12.0f) < 0.0001f);
                    ++homing_frames;
                }
                retdec_actor_update_motion(actor);
            }
            CHECK(released);
            CHECK(retdec_actor_manager_refresh(manager) == 0);
            CHECK(function_48aa20(vm) == stack_top);
        }
        function_4a9d70_this(PTR(init));
    }
    CHECK(execute_source(vm, root + 2,
        "if (floatRewards[0]!=4 || floatRewards[1]!=4 || floatRewards[2]!=4 || "
        "floatNumbers.len()!=12) throw \"floating reward count\";\n"
        "foreach (i,n in floatNumbers) if (n != (i<4 ? 100 : (i<8 ? 0 : 10000))) "
        "throw \"floating reward value\";"));
    function_4a9d70_this(PTR(scripts));
    puts("PASS: original point/save/1up scripts return to player before reward/release (12 flights)");
    return 0;
}

static int test_star_landing(int32_t manager, int32_t vm, int32_t *root) {
    int32_t scripts[3], init[3];
    int32_t layout[100]={0}, layer[80]={0}, resource[20]={0};
    int32_t records[1][8]={{1,0,400}};
    struct retdec_mcd_chip chip={0};
    struct retdec_mcd_data data={1,&chip,0,NULL};
    int32_t reader=0;
    unsigned char version;
    unsigned short texture_count;
    int32_t resource_base=(*(int32_t *)(intptr_t)(manager+72)-*(int32_t *)(intptr_t)(manager+68))/4;
    CHECK(function_407370(PTR(&reader),"data/actor/item/item.pat"));
    CHECK(retdec_pat_read_u8(reader,&version) && retdec_pat_read_u16(reader,&texture_count));
    CHECK(retdec_pat_skip_bytes(reader,texture_count*128u));
    g_retdec_act_texture_slots[1].width=1024;
    g_retdec_act_texture_slots[1].height=1024;
    for(int i=0;i<texture_count;++i) CHECK(retdec_pat_append_resource(manager,1));
    CHECK(retdec_pat_read_animations(reader,manager,resource_base));
    retdec_destroy_reader((int32_t *)(intptr_t)reader);
    float native_camera[24]={0};
    native_camera[20]=2000;
    native_camera[21]=1200;
    int failures=vm_failures;
    CHECK(execute_source(vm,root+2,
        "map <- {height=2000,width=2000};\n"
        "camera <- {left=0.0,top=0.0,right=2000.0,bottom=1200.0};\n"
        "stageWaterLevel=10000;\n"));
    function_4aa3a0_this(PTR(root+1),PTR(scripts),"t_item");
    CHECK(function_468950_this(PTR(g_514300_storage),manager));
    layout[0]=PTR(&g327); layout[60]=2000; layout[61]=32;
    layout[66]=PTR(records); layout[67]=PTR(records+1);
    layout[78]=PTR(layer); layout[79]=PTR(resource);
    *((uint8_t *)layer+140)=1;
    resource[16]=PTR(&data);
    chip.chip_id=1;
    *(int16_t *)(chip.bytes+12)=2000;
    *(int16_t *)(chip.bytes+14)=32;
    CHECK(function_4693a0(PTR(layout)));
    int32_t actual_map[60];
    const char *names[]={"InitStarC","InitStarD"};
    for(int kind=0;kind<6;++kind) {
        if(kind==2) {
            function_469700();
            function_468950_this(PTR(g_514300_storage),manager);
            function_427530(PTR(actual_map));
            CHECK(function_428000(PTR(actual_map),"data/map/w1-c01a.act"));
            for(int32_t slot=actual_map[52];slot!=actual_map[53];slot+=4) {
                int32_t actual_layer=*(int32_t *)(intptr_t)slot;
                const char *name=retdec_std_string_data(actual_layer+112);
                if(strncmp(name,"te",2) && strncmp(name,"wa",2)) continue;
                int32_t head=*(int32_t *)(intptr_t)(actual_layer+180);
                for(int32_t n=*(int32_t *)(intptr_t)head;n!=head;n=*(int32_t *)(intptr_t)n) {
                    int32_t key=*(int32_t *)(intptr_t)(n+8);
                    CHECK(function_4693a0(*(int32_t *)(intptr_t)(key+4)));
                }
            }
            char setup[160];
            sprintf_s(setup,sizeof(setup),"map.height=%d; map.width=%d;",actual_map[3],actual_map[2]);
            CHECK(execute_source(vm,root+2,setup));
        }
        function_4aa3a0_this(PTR(scripts),PTR(init),names[kind%2]);
        CHECK(init[1]==0x08000100);
        int32_t block=0, bumper=0, actor=0;
        if(kind<4) {
            actor=function_463b40_this(manager,init[0],init[1],init[2],
                512,kind<2 ? 160.0f : 768.0f,1,PTR(&g16),g483,g484,0);
        } else {
            int32_t enemy[3], block_init[3], bumper_init[3];
            CHECK(execute_source(vm,root+2,
                "function InitStarBumper(v) { user={type=TYPE_2HEAD,ball=null};\n"
                " SetTake(100); callbackGroup=GP_PLAYER; ::player=this; }\n"
                "starRewards <- 0;\nfunction AddStar() { ::starRewards++; }\n"));
            function_4aa3a0_this(PTR(root+1),PTR(enemy),"t_enemy");
            function_4aa3a0_this(PTR(enemy),PTR(block_init),"Init0436");
            function_4aa3a0_this(PTR(root+1),PTR(bumper_init),"InitStarBumper");
            bumper=function_463b40_this(manager,bumper_init[0],bumper_init[1],bumper_init[2],
                kind==4 ? 500.0f : 524.0f,820,-1,PTR(&g16),g483,g484,0);
            block=function_463b40_this(manager,block_init[0],block_init[1],block_init[2],
                512,768,-1,PTR(&g16),0x05000002,0x436,0);
            CHECK(block && bumper && vm_failures==failures);
            function_4a9840_this(PTR(root+1),"starBlock",block+44);
            CHECK(execute_source(vm,root+2,"starBlock.user.SetDamage(player);"));
            retdec_actor_manager_refresh(manager);
            for(int n=0;n<*(int32_t *)(intptr_t)(manager+116);++n) {
                int32_t candidate=(*(int32_t **)(intptr_t)(manager+100))[n];
                if(*(int32_t *)(intptr_t)(candidate+208)==1060) actor=candidate;
            }
            CHECK(actor);
            CHECK(execute_source(vm,root+2,"player.x=0;"));
            retdec_actor_refresh_bounds(bumper);
            function_4a9d70_this(PTR(enemy)); function_4a9d70_this(PTR(block_init));
            function_4a9d70_this(PTR(bumper_init));
        }
        int contacts=0, bounces=0;
        CHECK(actor && vm_failures==failures);
        CHECK(*(int32_t *)(intptr_t)(actor+208)==1060);
        CHECK(retdec_actor_manager_refresh(manager)>0);
        for(int frame=0;frame<300;++frame) {
            int grounded=*(int32_t *)(intptr_t)(actor+296);
            retdec_actor_manager_update(manager,PTR(native_camera));
            CHECK(vm_failures==failures);
            float vy=*(float *)(intptr_t)(actor+260);
            if(grounded && vy<0) ++bounces;
            CHECK(retdec_actor_render(actor,PTR(native_camera))==1);
            float x=*(float *)(intptr_t)(actor+240),y=*(float *)(intptr_t)(actor+244);
            if(!_finite(x) || !_finite(y) || *(uint8_t *)(intptr_t)(actor+22))
                fprintf(stderr,"star invalid kind=%d frame=%d xy=(%g,%g) vy=%g ground=%d contacts=%d\n",
                    kind,frame,x,y,vy,grounded,contacts);
            CHECK(_finite(x) && _finite(y));
            CHECK(!*(uint8_t *)(intptr_t)(actor+22));
            if(*(int32_t *)(intptr_t)(actor+296)) {
                ++contacts;
                int32_t sprite=*(int32_t *)(intptr_t)(actor+204);
                float sprite_top=*(float *)(intptr_t)(sprite+180);
                float sprite_bottom=*(float *)(intptr_t)(sprite+216);
                if(contacts==1) fprintf(stderr,"star land kind=%d xy=(%g,%g) bounds=(%g,%g) spriteY=(%g,%g)\n",
                    kind,x,y,*(float *)(intptr_t)(actor+444),*(float *)(intptr_t)(actor+452),sprite_top,sprite_bottom);
                CHECK(_finite(sprite_top) && _finite(sprite_bottom) && sprite_bottom>sprite_top);
                CHECK(sprite_top<*(float *)(intptr_t)(actor+452));
            }
        }
        CHECK(contacts>0 && bounces>0);
        if(bumper) {
            CHECK(execute_source(vm,root+2,"if(starRewards!=0) throw \"star rewarded before pickup\";"));
            *(float *)(intptr_t)(bumper+240)=*(float *)(intptr_t)(actor+240);
            *(float *)(intptr_t)(bumper+244)=*(float *)(intptr_t)(actor+244);
            retdec_actor_refresh_bounds(bumper);
            function_462ce0(actor,bumper);
            CHECK(vm_failures==failures && *(uint8_t *)(intptr_t)(actor+22));
            CHECK(execute_source(vm,root+2,"if(starRewards!=1) throw \"star pickup reward\";"));
        }
        kinoko_actor_release(actor, NULL);
        if(block) kinoko_actor_release(block, NULL);
        if(bumper) kinoko_actor_release(bumper, NULL);
        retdec_actor_manager_refresh(manager);
        function_4a9d70_this(PTR(init));
    }
    function_469700();
    function_468950_this(PTR(g_514300_storage),manager);
    retdec_destroy_cact_object(PTR(actual_map));
    function_4a9d70_this(PTR(scripts));
    puts("PASS: original moving stars land, bounce and remain collectible");
    return 0;
}

static int test_hidden_layer(int32_t vm, int32_t *root) {
    int32_t act[60], resource[48] = {0}, parent[2] = {g483,g484};
    int32_t hidden = 0, active = 0, layout = 0, script[3];
    int32_t compile_target = PTR(function_471b30), stack_top = function_48aa20(vm);
    int failures = vm_failures;
    g874 = 1;
    CHECK(function_415550_this(PTR(root), PTR("CompileFile"), PTR(&compile_target),
        4, PTR(retdec_compile_file_native), 0) >= 0);
    function_427530(PTR(act));
    CHECK(function_428000(PTR(act), "data/map/w1-c01a.act"));
    for (int32_t slot = act[52]; slot != act[53]; slot += 4) {
        int32_t layer = *(int32_t *)(intptr_t)slot;
        if (strcmp(retdec_std_string_data(layer + 112), "hidden") == 0) hidden = layer;
    }
    CHECK(hidden);
    CHECK(retdec_publish_cact_layer_class(vm, PTR(root)));
    CHECK(retdec_sqrat_new_table(vm, parent));
    CHECK(retdec_sqrat_set_pair(vm, root + 2, retdec_std_string_data(PTR(act) + 16), parent));
    CHECK(execute_source(vm, parent, "resource <- {};"));
    resource[39] = root[2];
    resource[40] = root[3];
    act[52] = PTR(&hidden);
    act[53] = PTR(&hidden + 1);
    CHECK(retdec_publish_act_layers(vm, PTR(act), PTR(resource), &active));
    CHECK(vm_failures == failures && active == 1);
    CHECK(*(int32_t *)(intptr_t)(hidden + 220) == 0x08000100);
    CHECK(*(int32_t *)(intptr_t)(hidden + 240) == 0x08000100);
    script[0] = PTR(&g16);
    script[1] = *(int32_t *)(intptr_t)(hidden + 316);
    script[2] = *(int32_t *)(intptr_t)(hidden + 320);
    CHECK(execute_source(vm, script + 1,
        "if (u != this || typeof Init != \"function\" || typeof Update != \"function\") "
        "throw \"hidden script environment\";"));
    int32_t layout_object[3];
    function_4aa3a0_this(PTR(script), PTR(layout_object), "layout");
    layout = function_4a9b40_this(PTR(layout_object), 0);
    CHECK(layout && retdec_map_chip_data(layout));
    function_4a9d70_this(PTR(layout_object));
    CHECK(retdec_execute_act_callback(hidden + 204, 4, NULL) >= 0);
    CHECK(*(int32_t *)(intptr_t)(layout + 328) == 1);
    CHECK(execute_source(vm, root + 2,
        "player <- { left=2232.0, right=2264.0, top=800.0, bottom=832.0 };"));
    for (int frame = 0; frame < 24; ++frame) CHECK(function_41efb0(hidden) >= 0);
    CHECK(vm_failures == failures);
    CHECK(*(float *)(intptr_t)(layout + 320) <= 0.001f);
    CHECK(execute_source(vm, root + 2, "player.left=100.0; player.right=120.0;"));
    for (int frame = 0; frame < 24; ++frame) CHECK(function_41efb0(hidden) >= 0);
    CHECK(*(float *)(intptr_t)(layout + 320) >= 0.999f);
    CHECK(vm_failures == failures && function_48aa20(vm) == stack_top);
    retdec_sqrat_release_pair(vm, parent);
    puts("PASS: original ACT hidden-layer include, callbacks, native fade-out and fade-in");
    return 0;
}

static int test_enemy_reentry(int32_t manager, int32_t vm, int32_t *root) {
    int32_t reader = 0, scripts[3], init[3], actors[3];
    float saved_camera[4];
    int32_t create_target = PTR(function_469b40);
    int32_t layout[100] = {0}, layer[80] = {0}, resource[20] = {0};
    int32_t records[1][8] = {{1, -8000, 200}};
    struct retdec_mcd_chip chip = {0};
    struct retdec_mcd_data data = {1,&chip,0,NULL};
    unsigned char version;
    unsigned short textures;
    int failures = vm_failures;
    CHECK(function_415550_this(PTR(root), PTR("CreateActor"), PTR(&create_target),
        4, PTR(function_471df0), 0) >= 0);
    {
        int32_t globals[4]={0,vm,g483,g484}, callback[2]={g483,g484};
        CHECK(retdec_sqrat_new_table(vm,globals+2));
        CHECK(function_48e520_this(globals[3],root[3]));
        CHECK(execute_asset(vm,globals+2,"data/script/global.cv4"));
        CHECK(retdec_sqrat_get(PTR(globals),"GetCallbackFuncTable",callback));
        CHECK(retdec_sqrat_set_pair(vm,root+2,"GetCallbackFuncTable",callback));
        retdec_sqrat_release_pair(vm,callback);
        retdec_sqrat_release_pair(vm,globals+2);
    }
    CHECK(execute_source(vm, root + 2,
        "t_enemy <- {};\ncamera <- {left=-8000.0,right=8000.0,top=-2000.0,bottom=2000.0};\n"
        "player <- {x=0.0,y=100.0,direction=-1.0,user={hold=null,water=false}};\n"
        "function PlaySE(id) {}\n PR_FRONT <- 65535;\n stageWaterLevel <- 10000;\nupdateMask <- -1;\n"));
    CHECK(execute_asset(vm, root + 2, "data/script/enemy.cv4"));
    CHECK(vm_failures == failures);
    CHECK(function_407370(PTR(&reader), "data/actor/enemy/enemy.pat"));
    CHECK(retdec_pat_read_u8(reader, &version));
    CHECK(retdec_pat_read_u16(reader, &textures));
    CHECK(retdec_pat_skip_bytes(reader, textures * 128u));
    CHECK(retdec_pat_read_animations(reader, manager, 0));
    retdec_destroy_reader((int32_t *)(intptr_t)reader);
    CHECK(function_468950_this(PTR(g_514300_storage), manager));
    layout[0] = PTR(&g327); layout[60] = 16000; layout[61] = 32;
    layout[66] = PTR(records); layout[67] = PTR(records + 1);
    layout[78] = PTR(layer); layout[79] = PTR(resource);
    *((uint8_t *)layer + 140) = 1;
    resource[16] = PTR(&data);
    chip.chip_id = 1;
    *(int16_t *)(chip.bytes + 12) = 16000;
    *(int16_t *)(chip.bytes + 14) = 32;
    CHECK(function_4693a0(PTR(layout)));
    function_4aa3a0_this(PTR(root + 1), PTR(scripts), "t_enemy");
    function_4aa3a0_this(PTR(scripts), PTR(init), "Init0107");
    int32_t fairy = function_463b40_this(manager, init[0], init[1], init[2],
        100,160,-1,PTR(&g16),0x05000002,0x107,0);
    CHECK(fairy && vm_failures==failures);
    function_4a9840_this(PTR(root+1),"fairy",fairy+44);
    *(int32_t *)(intptr_t)(manager+64)=-1;
    *(float *)(g_retdec_camera_state+72)=-8000;
    *(float *)(g_retdec_camera_state+76)=-2000;
    *(float *)(g_retdec_camera_state+80)=8000;
    *(float *)(g_retdec_camera_state+84)=2000;
    CHECK(execute_source(vm,root+2,
        "if (!(\"OnReset\" in fairy.user)) throw \"OnReset not found\";\n"
        "if (typeof fairy.user.OnReset!=\"function\") throw \"OnReset type\";\n"
        "if(fairy.user.blowOff) throw \"unexpected blowOff\";\n"
        "fairy.vy=-5.0;\nt_enemy.EnemyUpdate_Dead.call(fairy);\n"
        "if(fairy.vy<=-5.0) throw \"dead gravity missing\";\n"
        "fairy.vy=0.0; fairy.user.frameCount=0;"));
    for(int round=0;round<4;++round) {
        for(int frame=0;frame<240;++frame) {
            retdec_actor_manager_update(manager,PTR(g_retdec_camera_state));
            CHECK(vm_failures==failures);
        }
        CHECK(execute_source(vm,root+2,
            "if(fairy.user.data.ball.len()!=4) throw \"four balls missing\";\n"
            "for(local i=0;i<4;i++) {\n"
            "if(fairy.user.data.ball[i]==null) throw \"expired ball\";\n"
            "if(fairy.user.data.ball[i].user.data.p!=fairy) throw \"ball parent\";\n"
            "}\n"));
        CHECK(execute_source(vm,root+2,
            "for(local i=0;i<4;i++) { if(fairy.user.data.ball[i].user.frameCount==0 && "
            "fairy.user.data.ball[i].user.data.enable) throw \"ball not updating\"; }"));
        for(int i=0;i<4;++i) {
            char code[80]; int32_t obj[3];
            sprintf_s(code,sizeof(code),"probeBall <- fairy.user.data.ball[%d];",i);
            CHECK(execute_source(vm,root+2,code));
            function_4aa3a0_this(PTR(root+1),PTR(obj),"probeBall");
            int32_t ball=function_4a9b40_this(PTR(obj),0);
            printf("ball %d/%d active=%d visible=%d release=%d xy=%g,%g callback=%x\n",round,i,
                *(uint8_t *)(intptr_t)(ball+40),*(uint8_t *)(intptr_t)(ball+21),
                *(uint8_t *)(intptr_t)(ball+22),*(float *)(intptr_t)(ball+240),
                *(float *)(intptr_t)(ball+244),*(int32_t *)(intptr_t)(ball+112));
            function_4a9d70_this(PTR(obj));
        }
        printf("PASS: ball generation %d\n",round); fflush(stdout);
        // Original map flags make a reset actor wait for native visibility.
        *(uint32_t *)(intptr_t)(fairy+392)=0x20000;
        CHECK(execute_source(vm,root+2,
            "camera.left=-1140; camera.right=-500;"));
        *(float *)(g_retdec_camera_state+72)=-1140;
        *(float *)(g_retdec_camera_state+80)=-500;
        retdec_actor_manager_update(manager,PTR(g_retdec_camera_state));
        expected_vm_error=0;
        CHECK(*(int32_t *)(intptr_t)(fairy+112)==0x08000100);
        function_4a9840_this(PTR(root+1),"fairy",fairy+44);
        CHECK(execute_source(vm,root+2,
            "if(fairy.x!=fairy.ox) throw \"reset origin\";\n"
            "camera.left=-8000; camera.right=8000;"));
        *(float *)(g_retdec_camera_state+72)=-8000;
        *(float *)(g_retdec_camera_state+80)=8000;
    }
    int32_t death_init[3];
    function_4aa3a0_this(PTR(scripts),PTR(death_init),"Init0106");
    int32_t victim=function_463b40_this(manager,death_init[0],death_init[1],death_init[2],
        300,160,-1,PTR(&g16),0x05000002,0x106,0);
    CHECK(victim);
    function_4a9840_this(PTR(root+1),"victim",victim+44);
    CHECK(execute_source(vm,root+2,
        "t_item <- { InitPoint=function(v){}, Init1up=function(v){} };\n"
        "attacker <- { user={hitCount=0},callbackGroup=0 };\n"
        "t_enemy.EnemyCollision_Damage.call(victim,attacker);\n"
        "if(victim.vy!=-5 || victim.user.blowOff) throw \"ordinary death setup\";"));
    for(int frame=0;frame<90;++frame) {
        retdec_actor_tick(victim);
        retdec_actor_update_motion(victim);
        CHECK(vm_failures==failures);
        if(frame==30) CHECK(*(float *)(intptr_t)(victim+260)>0);
    }
    CHECK(*(float *)(intptr_t)(victim+244)>160);
    CHECK(execute_source(vm,root+2,
        "fairy.user.eventHandler.OnHitStep(attacker);\n"
        "if(fairy.vy!=-5) throw \"stomp death setup\";"));
    for(int frame=0;frame<90;++frame) {
        retdec_actor_tick(fairy);
        retdec_actor_update_motion(fairy);
        CHECK(vm_failures==failures);
    }
    CHECK(*(float *)(intptr_t)(fairy+244)>160);
    CHECK(*(float *)(intptr_t)(fairy+260)>0);
    puts("PASS: original collision death rises briefly then falls under gravity");
    function_4a9d70_this(PTR(death_init));
    function_4a9d70_this(PTR(init));
    function_4a9d70_this(PTR(scripts));
    function_469700();
    return 0;
}

static int test_enemy_scripts(int32_t manager, int32_t vm, int32_t *root) {
    int32_t reader = 0, scripts[3], init[3], actors[3];
    float saved_camera[4];
    int32_t create_target = PTR(function_469b40);
    int32_t layout[100] = {0}, layer[80] = {0}, resource[20] = {0};
    int32_t records[1][8] = {{1, -8000, 200}};
    struct retdec_mcd_chip chip = {0};
    struct retdec_mcd_data data = {1,&chip,0,NULL};
    unsigned char version;
    unsigned short textures;
    int failures = vm_failures;
    CHECK(function_415550_this(PTR(root), PTR("CreateActor"), PTR(&create_target),
        4, PTR(function_471df0), 0) >= 0);
    {
        int32_t globals[4]={0,vm,g483,g484}, callback[2]={g483,g484};
        CHECK(retdec_sqrat_new_table(vm,globals+2));
        CHECK(function_48e520_this(globals[3],root[3]));
        CHECK(execute_asset(vm,globals+2,"data/script/global.cv4"));
        CHECK(retdec_sqrat_get(PTR(globals),"GetCallbackFuncTable",callback));
        CHECK(retdec_sqrat_set_pair(vm,root+2,"GetCallbackFuncTable",callback));
        retdec_sqrat_release_pair(vm,callback);
        retdec_sqrat_release_pair(vm,globals+2);
    }
    CHECK(execute_source(vm, root + 2,
        "t_enemy <- {};\ncamera <- {left=-8000.0,right=8000.0,top=-2000.0,bottom=2000.0};\n"
        "player <- {x=0.0,y=100.0,user={hold=null,water=false}};\n"
        "stageWaterLevel=10000;\nupdateMask <- -1;\n"));
    CHECK(execute_asset(vm, root + 2, "data/script/enemy.cv4"));
    CHECK(vm_failures == failures);
    CHECK(function_407370(PTR(&reader), "data/actor/enemy/enemy.pat"));
    CHECK(retdec_pat_read_u8(reader, &version));
    CHECK(retdec_pat_read_u16(reader, &textures));
    CHECK(retdec_pat_skip_bytes(reader, textures * 128u));
    CHECK(retdec_pat_read_animations(reader, manager, 0));
    retdec_destroy_reader((int32_t *)(intptr_t)reader);
    CHECK(function_468950_this(PTR(g_514300_storage), manager));
    layout[0] = PTR(&g327); layout[60] = 16000; layout[61] = 32;
    layout[66] = PTR(records); layout[67] = PTR(records + 1);
    layout[78] = PTR(layer); layout[79] = PTR(resource);
    *((uint8_t *)layer + 140) = 1;
    resource[16] = PTR(&data);
    chip.chip_id = 1;
    *(int16_t *)(chip.bytes + 12) = 16000;
    *(int16_t *)(chip.bytes + 14) = 32;
    CHECK(function_4693a0(PTR(layout)));
    function_4aa3a0_this(PTR(root + 1), PTR(scripts), "t_enemy");
    function_4aa3a0_this(PTR(scripts), PTR(init), "Init0106");
    for (int i = 0; i < 2; ++i) {
        actors[i] = function_463b40_this(manager, init[0], init[1], init[2],
            100.0f + 200.0f * i, 160, -1, PTR(&g16), 0x05000002, 0x106, 0);
        CHECK(actors[i] && vm_failures == failures);
        function_4a9840_this(PTR(root + 1), i ? "enemyB" : "enemyA", actors[i] + 44);
    }
    CHECK(execute_source(vm, root + 2,
        "if (t_enemy.nextID!=2) throw \"enemy constructor not called\";\n"
        "if (enemyA.user==enemyB.user) throw \"shared enemy user\";\n"
        "if (enemyA.user.eventHandler==enemyB.user.eventHandler) throw \"shared enemy events\";\n"
        "if (enemyA.user.data==enemyB.user.data) throw \"shared enemy data\";\n"
        "if (enemyA.user.childRef==enemyB.user.childRef) throw \"shared enemy children\";\n"
        "if (enemyA.user.actor!=enemyA || enemyB.user.actor!=enemyB) throw \"enemy owner binding\";\n"
        "if (typeof enemyA.funcUpdate!=\"function\" || enemyA.user.takeID!=4010) "
        "throw \"enemy initialization\";"));
    CHECK(retdec_actor_manager_refresh(manager) == 3);
    for (int frame = 0; frame < 40; ++frame) {
        for (int i = 0; i < 2; ++i) retdec_actor_tick(actors[i]);
        CHECK(vm_failures == failures);
        function_468620_this(PTR(g_514300_storage));
        for (int i = 0; i < 2; ++i) {
            retdec_actor_update_motion(actors[i]);
            CHECK(_finite(*(float *)(intptr_t)(actors[i] + 244)));
        }
    }
    CHECK(execute_source(vm, root + 2,
        "if (!(enemyA.x<100 && enemyB.x<300 && enemyA.hitBottom && enemyB.hitBottom)) "
        "throw \"enemy walking and landing\";\n"
        "enemyA.hitLeft=1; enemyA.xPrev=enemyA.x;"));
    retdec_actor_tick(actors[0]);
    CHECK(vm_failures == failures);
    CHECK(execute_source(vm, root + 2,
        "if (enemyA.direction!=1.0 || enemyA.vx<=0.0 || enemyB.direction!=-1.0) "
        "throw \"enemy wall reversal\";"));
    function_4a9d70_this(PTR(init));
    function_4aa3a0_this(PTR(scripts), PTR(init), "Init0101");
    actors[2] = function_463b40_this(manager, init[0], init[1], init[2],
        600, 160, -1, PTR(&g16), 0x05000002, 0x101, 0);
    CHECK(actors[2] && vm_failures == failures);
    function_4a9840_this(PTR(root+1), "enemyC", actors[2]+44);
    memcpy(saved_camera,g_retdec_camera_state+72,sizeof(saved_camera));
    *(float *)(g_retdec_camera_state+72)=-8000;
    *(float *)(g_retdec_camera_state+76)=-2000;
    *(float *)(g_retdec_camera_state+80)=8000;
    *(float *)(g_retdec_camera_state+84)=2000;
    *(int32_t *)(intptr_t)(manager+64)=-1;
    for (int frame=0;frame<3600;++frame) {
        retdec_actor_manager_update(manager,PTR(g_retdec_camera_state));
        CHECK(vm_failures==failures);
        for(int i=0;i<3;++i) {
            for(int offset=240;offset<=244;offset+=4)
                CHECK(_finite(*(float *)(intptr_t)(actors[i]+offset)));
            CHECK(*(int32_t *)(intptr_t)(actors[i]+112)==0x08000100);
            CHECK(*(float *)(intptr_t)(actors[i]+244)<210.0f);
        }
    }
    CHECK(execute_source(vm,root+2,
        "if(enemyA.user.frameCount<3600 || enemyB.user.frameCount<3600 || "
        "enemyC.user.frameCount<3600) throw \"walking enemy reset early\";"));
    for(int round=0;round<16;++round) {
        CHECK(execute_source(vm,root+2,
            "camera.left=-300; camera.right=1000;\n"
            "enemyA.x=4000; enemyB.x=4200; enemyC.x=4400;"));
        for(int i=0;i<3;++i) retdec_actor_tick(actors[i]);
        CHECK(vm_failures==failures);
        CHECK(execute_source(vm,root+2,
            "if(enemyA.x!=-16777215 || enemyB.x!=-16777215 || enemyC.x!=-16777215) "
            "throw \"offscreen waiting state\";\n"
            "camera.left=-10000; camera.right=-9000;"));
        for(int i=0;i<3;++i) retdec_actor_tick(actors[i]);
        CHECK(vm_failures==failures);
        const char *names[]={"enemyA","enemyB","enemyC"};
        for(int i=0;i<3;++i) function_4a9840_this(PTR(root+1),names[i],actors[i]+44);
        CHECK(execute_source(vm,root+2,
            "if(enemyA.x!=enemyA.ox || enemyB.x!=enemyB.ox || enemyC.x!=enemyC.ox) "
            "throw \"offscreen reset origin\";\n"
            "camera.left=-8000; camera.right=8000;"));
        for(int frame=0;frame<120;++frame)
            retdec_actor_manager_update(manager,PTR(g_retdec_camera_state));
        CHECK(vm_failures==failures);
        for(int i=0;i<3;++i) {
            CHECK(_finite(*(float *)(intptr_t)(actors[i]+244)));
            CHECK(*(int32_t *)(intptr_t)(actors[i]+112)==0x08000100);
            CHECK(*(int32_t *)(intptr_t)(actors[i]+140)==0x08000100);
        }
    }
    records[0][1] = -200;
    *(int16_t *)(chip.bytes + 12) = 400;
    CHECK(execute_source(vm,root+2,
        "camera.left=-8000; camera.right=8000;\n"
        "enemyA.direction=1.0; enemyB.direction=1.0; enemyC.direction=1.0;"));
    int landed[3]={0}, fell[3]={0};
    for(int i=0;i<3;++i) {
        *(float *)(intptr_t)(actors[i]+240)=-120.0f+100.0f*i;
        *(float *)(intptr_t)(actors[i]+244)=160.0f;
        *(float *)(intptr_t)(actors[i]+260)=0;
        retdec_actor_refresh_bounds(actors[i]);
    }
    for(int frame=0;frame<600;++frame) {
        retdec_actor_manager_update(manager,PTR(g_retdec_camera_state));
        CHECK(vm_failures==failures);
        for(int i=0;i<3;++i) {
            float x=*(float *)(intptr_t)(actors[i]+240);
            float y=*(float *)(intptr_t)(actors[i]+244);
            if(!_finite(x) || !_finite(y))
                fprintf(stderr,"ledge invalid actor=%d frame=%d xy=(%g,%g)\n",i,frame,x,y);
            CHECK(_finite(x) && _finite(y));
            if(*(int32_t *)(intptr_t)(actors[i]+296)) landed[i]=1;
            if(landed[i] && x>200 && y>220 && y<600) fell[i]=1;
        }
    }
    CHECK(landed[0] && landed[1] && landed[2]);
    CHECK(fell[0] && fell[1] && fell[2]);
    memcpy(g_retdec_camera_state+72,saved_camera,sizeof(saved_camera));
    function_469700();
    CHECK(function_468950_this(PTR(g_514300_storage), manager));
    function_4a9d70_this(PTR(init));
    function_4a9d70_this(PTR(scripts));
    puts("PASS: fairy/white kedama walking, ledge falling, collisions and 48 offscreen resets");
    return 0;
}

static int test_delegate_lifetime(int32_t vm, int32_t *root) {
    int32_t first[2] = {g483,g484}, second[2] = {g483,g484};
    int32_t shared = *(int32_t *)(intptr_t)(vm + 140);
    CHECK(execute_source(vm, root + 2,
        "delegateBase <- {value=7};\n"
        "delegateA <- delegate delegateBase : {};\n"
        "delegateB <- delegate delegateBase : {};\n"
        "delegateA.extra <- 9;\n"
        "if (delegateA==delegateB || delegateA.value!=7 || (\"extra\" in delegateB)) "
        "throw \"delegate isolation\";\n"
        "delegateBase.value=11;\n"
        "if (delegateA.value!=11 || delegateB.value!=11) throw \"delegate inheritance\";"));
    CHECK(retdec_sqrat_new_table(vm, first));
    CHECK(retdec_sqrat_new_table(vm, second));
    int32_t first_refs = *(int32_t *)(intptr_t)(first[1]+4);
    CHECK(function_48e520_this(second[1], first[1]));
    CHECK(!function_48e520_this(first[1], second[1]));
    CHECK(!function_48e520_this(first[1], first[1]));
    CHECK(*(int32_t *)(intptr_t)(first[1]+4) == first_refs+1);
    CHECK(function_48e520_this(second[1], 0));
    for (int i=0;i<32;++i) {
        int32_t userdata = function_48bec0(shared, 16);
        int32_t weak[5] = {0,1,0,0x0A000080,userdata};
        CHECK(userdata && function_48e520_this(userdata, first[1]));
        *(int32_t *)(intptr_t)(userdata+8)=PTR(weak);
        CHECK(*(int32_t *)(intptr_t)(first[1]+4) == first_refs+1);
        retdec_call_thiscall0_result((void *)(intptr_t)userdata, function_48be70);
        CHECK(*(int32_t *)(intptr_t)(userdata+24)==0 && weak[4]==userdata);
        CHECK(function_48e520_this(userdata, first[1]));
        retdec_call_thiscall1_result((void *)(intptr_t)userdata, function_48bf50, 0);
        CHECK(weak[3]==g483 && weak[4]==0);
        CHECK(*(int32_t *)(intptr_t)(first[1]+4)==first_refs);
        free((void *)(intptr_t)userdata);
    }
    retdec_sqrat_release_pair(vm, first);
    retdec_sqrat_release_pair(vm, second);
    puts("PASS: delegate isolation/cycle rejection and userdata finalization/destruction");
    return 0;
}

static int test_stone_block(int32_t manager, int32_t vm, int32_t *root) {
    int32_t enemy[3], item[3], block_init[3], stone_init[3], rider_init[3];
    int32_t rider, stone, block;
    int failures=vm_failures;
    function_4aa3a0_this(PTR(root+1), PTR(enemy), "t_enemy");
    function_4aa3a0_this(PTR(root+1), PTR(item), "t_item");
    CHECK(execute_asset(vm, enemy+1, "data/script/block.cv4"));
    CHECK(execute_asset(vm, item+1, "data/script/bullet.cv4"));
    function_4aa3a0_this(PTR(enemy), PTR(block_init), "Init0435");
    function_4aa3a0_this(PTR(item), PTR(stone_init), "InitStone");
    function_4aa3a0_this(PTR(root+1), PTR(rider_init), "InitStoneRider");
    rider=function_463b40_this(manager,rider_init[0],rider_init[1],rider_init[2],
        100,150,-1,PTR(&g16),g483,g484,0);
    stone=function_463b40_this(manager,stone_init[0],stone_init[1],stone_init[2],
        100,150,-1,PTR(&g16),g483,g484,0);
    CHECK(rider && stone && vm_failures==failures);
    function_4a9840_this(PTR(root+1), "fallingStone", stone+44);
    CHECK(execute_source(vm,root+2,
        "player.x=fallingStone.x;\n"
        "player.y=fallingStone.top+2-(player.bottom-player.y); player.vy=1.0;"));
    retdec_actor_refresh_bounds(rider);
    function_462ce0(stone,rider);
    CHECK(execute_source(vm,root+2,"player.x=fallingStone.right+64;"));
    retdec_actor_update_motion(rider);
    retdec_actor_tick(stone);
    block=function_463b40_this(manager,block_init[0],block_init[1],block_init[2],
        *(float *)(intptr_t)(stone+240),240,-1,PTR(&g16),0x05000002,0x435,0);
    CHECK(block && vm_failures==failures);
    function_4a9840_this(PTR(root+1),"stoneBlock",block+44);
    int contacted=0;
    retdec_actor_manager_refresh(manager);
    for(int frame=0;frame<80;++frame) {
        retdec_actor_tick(stone);
        function_468620_this(PTR(g_514300_storage));
        retdec_actor_update_motion(stone);
        function_462ce0(stone,block);
        CHECK(vm_failures==failures);
        CHECK(_finite(*(float *)(intptr_t)(stone+244)));
        if(*(uint8_t *)(intptr_t)(stone+22)) {contacted=1;break;}
    }
    CHECK(contacted);
    CHECK(execute_source(vm,root+2,
        "if (stoneBlock.callbackMask!=0 || stoneBlock.user.SetDamage!=null || stoneBlock.user.direction!=1) "
        "throw \"stone did not activate original block callback\";"));
    function_469700();
    CHECK(function_468950_this(PTR(g_514300_storage),manager));
    function_4a9d70_this(PTR(enemy)); function_4a9d70_this(PTR(item));
    function_4a9d70_this(PTR(block_init)); function_4a9d70_this(PTR(stone_init));
    function_4a9d70_this(PTR(rider_init));
    puts("PASS: original stone ride/walk-off/fall, block callback, region query and item spawn");
    return 0;
}

static int margin_calls;
static int32_t margin_args[4];
static int32_t capture_bgm_margin(int32_t path, int32_t a, int32_t b, int32_t c, int32_t d) {
    if (strcmp((const char *)(intptr_t)path, "data/bgm/st1.ogg") != 0) return 0;
    ++margin_calls;
    margin_args[0]=a; margin_args[1]=b; margin_args[2]=c; margin_args[3]=d;
    return 0;
}

static int test_player_form_exit(int32_t manager, int32_t vm, int32_t *root) {
    int32_t init[3], actor, margin_target=PTR(capture_bgm_margin);
    int failures=vm_failures, stack_top=function_48aa20(vm);
    CHECK(function_415550_this(PTR(root), PTR("PlayBgmMargin"), PTR(&margin_target),
        4, PTR(function_4720e0), 0)>=0);
    CHECK(execute_source(vm, root+2,
        "transformFaces <- 0;\n"
        "PlayerImage <- {SetFaceType=function(t){::transformFaces++;}};\n"
        "PlayerStatus <- {ItemCross=false};\n"
        "stageBgm=\"data/bgm/st1.ogg\";\n"
        "input.x=0; input.b3=0; input.b0=0; input.b2=0;\n"
        "camera <- {left=-1000.0,right=1000.0,top=-1000.0,bottom=1000.0};"));
    function_4aa3a0_this(PTR(root+1),PTR(init),"InitWalkingProbe");
    actor=function_463b40_this(manager,init[0],init[1],init[2],
        100,200,-1,PTR(&g16),g483,g484,0);
    CHECK(actor);
    function_4a9840_this(PTR(root+1),"transformProbe",actor+44);
    CHECK(execute_source(vm,root+2,
        "transformProbe.user.beforeType <- TYPE_2HEAD;\n"
        "transformProbe.user.beforeTake <- TAKE_STAND;\n"
        "transformProbe.user.count8headTime <- 600;\n"
        "transformProbe.user.SetType <- t_player.SetType.bindenv(transformProbe);\n"
        "transformProbe.funcUpdate=function(){};\ntransformProbe.collisionMask=0;"));
    for(int round=0;round<32;++round) {
        CHECK(execute_source(vm,root+2,
            "stageBgmCurrent=\"data/bgm/8head.ogg\";\n"
            "transformProbe.user.beforeType=TYPE_2HEAD; transformProbe.user.type=TYPE_8HEAD;\n"
            "transformProbe.user.count8head=91; transformProbe.user.invincibleCount=0;\n"
            "transformProbe.user.SetTake(TAKE_STAND);"));
        retdec_actor_tick(actor);
        retdec_actor_update_motion(actor);
        CHECK(vm_failures==failures);
        CHECK(execute_source(vm,root+2,
            "if (transformProbe.user.type!=TYPE_2HEAD || transformProbe.user.count8head!=90 || "
            "transformProbe.take!=TYPE_2HEAD*100+TAKE_STAND || PlayerStatus.ItemCross) "
            "throw \"eight-head restoration\";"));
        CHECK(_finite(*(float *)(intptr_t)(actor+244)));
        CHECK(function_48aa20(vm)==stack_top);
    }
    CHECK(margin_calls==32 && margin_args[0]==1000 && margin_args[1]==2000 &&
        margin_args[2]==100 && margin_args[3]==1);
    function_469700();
    function_4a9d70_this(PTR(init));
    puts("PASS: original eight-head expiry, SetType/SetTake and native BGM adapter (32 transitions)");
    return 0;
}

static void *retired_vm_stack;
static int32_t sort_native_compare(int32_t vm) {
    int32_t left, right;
    if (function_48a7d0(vm,2,&left)<0 || function_48a7d0(vm,3,&right)<0) return -1;
    function_48a4f0(vm, right-left);
    return 1;
}

static int test_array_sort(int32_t vm, int32_t *root) {
    int top = function_48aa20(vm);
    CHECK(execute_source(vm, root+2,
        "sortValues <- [5,1,3,1,-2,8];\n"
        "sortValues.sort(function(a,b) { return a-b; });\n"
        "local expected=[-2,1,1,3,5,8];\n"
        "foreach(i,v in expected) if(sortValues[i]!=v) throw \"comparator order\";"));
    CHECK(function_48aa20(vm)==top);
    CHECK(retdec_sqrat_set_native_closure(vm,root+2,"SortNativeCompare",PTR(sort_native_compare),NULL,0));
    CHECK(execute_source(vm,root+2,
        "[].sort(); [3].sort();\n"
        "sortValues.sort(SortNativeCompare);\n"
        "local descending=[8,5,3,1,1,-2];\n"
        "foreach(i,v in descending) if(sortValues[i]!=v) throw \"native comparator\";\n"
        "sortValues.sort();\n"
        "local ascending=[-2,1,1,3,5,8];\n"
        "foreach(i,v in ascending) if(sortValues[i]!=v) throw \"default comparator\";\n"
        "sortStrings <- [\"z\",\"a\",\"b\"]; sortStrings.sort();\n"
        "if(sortStrings[0]!=\"a\" || sortStrings[2]!=\"z\") throw \"string order\";\n"
        "class SortValue { rank=0; constructor(n) { rank=n; } }\n"
        "sortObjects <- [SortValue(5),SortValue(1),SortValue(3),SortValue(1),SortValue(-2),SortValue(8)];\n"
        "sortOriginal <- clone sortObjects;\n"));
    int32_t array[3], objects[6], refs[6];
    function_4aa3a0_this(PTR(root+1),PTR(array),"sortObjects");
    int32_t *values=*(int32_t **)(intptr_t)(array[2]+24);
    for(int i=0;i<6;++i) {
        objects[i]=values[2*i+1];
        refs[i]=*(int32_t *)(intptr_t)(objects[i]+4);
    }
    CHECK(execute_source(vm,root+2,
        "sortRelocated <- false;\n"
        "sortObjects.sort(function(a,b) {\n"
        " if(!sortRelocated) { sortRelocated=true; RelocateStack(); }\n"
        " local nested=[3,0,1]; nested.sort();\n"
        " return a.rank-b.rank;\n"
        "});\n"
        "for(local round=0;round<64;round++) {\n"
        " sortObjects.sort(function(a,b) { return b.rank-a.rank; });\n"
        " sortObjects.sort(function(a,b) { return a.rank-b.rank; });\n"
        "}\n"
        "local expected=[-2,1,1,3,5,8];\n"
        "foreach(i,v in expected) if(sortObjects[i].rank!=v) throw \"instance order\";\n"
        "foreach(original in sortOriginal) {\n"
        " local found=0; foreach(value in sortObjects) if(value==original) found++;\n"
        " if(found!=1) throw \"instance ownership\";\n"
        "}"));
    CHECK(retired_vm_stack);
    free(retired_vm_stack); retired_vm_stack=NULL;
    for(int i=0;i<6;++i) CHECK(*(int32_t *)(intptr_t)(objects[i]+4)==refs[i]);
    function_4a9d70_this(PTR(array));
    expected_vm_error=1;
    CHECK(execute_source(vm,root+2,
        "local caught=0;\n"
        "try { [2,1].sort(function(a,b){throw \"sort failed\";}); }\n"
        "catch(e) { if(e!=\"sort failed\") throw e; caught++; }\n"
        "try { [2,1].sort(function(a,b){throw 17;}); }\n"
        "catch(e) { if(e!=\"compare func failed\") throw e; caught++; }\n"
        "try { [2,1].sort(function(a,b){return 1;}); }\n"
        "catch(e) { if(e!=\"Invalid qsort, probably compare function defect\") throw e; caught++; }\n"
        "if(caught!=3) throw \"sort exception propagation\";"));
    expected_vm_error=0;
    CHECK(function_48aa20(vm)==top);
    puts("PASS: array sort values/instances/native callbacks, ownership, nesting, VM relocation and errors");
    return 0;
}

static char aux_output[16384];
static void __cdecl capture_aux_output(int32_t vm, const char *format, ...) {
    va_list arguments;
    size_t used = strlen(aux_output);
    (void)vm;
    va_start(arguments, format);
    vsnprintf(aux_output + used, sizeof(aux_output) - used, format, arguments);
    va_end(arguments);
}

static int test_standard_error_handler(int32_t vm, int32_t *root) {
    int32_t shared = *(int32_t *)(intptr_t)(vm + 140);
    int32_t old_handler[2] = {g483, g484};
    int32_t old_print = function_48b8d0(vm);
    int32_t old_compiler = *(int32_t *)(intptr_t)(shared + 160);
    int32_t top = function_48aa20(vm);
    retdec_squirrel_assign(old_handler, (int32_t *)(intptr_t)(vm + 72));
    function_48b8b0(vm, PTR(capture_aux_output));
    function_4c5c80(vm);
    CHECK(*(int32_t *)(intptr_t)(shared + 160) == PTR(function_4c5c40));
    CHECK(*(int32_t *)(intptr_t)(vm + 72) == 0x08000200);
    CHECK(*(int32_t *)(intptr_t)(*(int32_t *)(intptr_t)(vm+76)+60) == PTR(function_4c5bc0));
    CHECK(execute_source(vm, root+2,
        "function AuxOuter() {\n"
        " local outerValue=31;\n"
        " local callback=function(arg):(outerValue) {\n"
        "  local fraction=1.25;\n"
        "  local enabled=true;\n"
        "  local captured=outerValue;\n"
        "  throw \"aux failure\";\n"
        " };\n"
        " callback(9);\n"
        "}"));
    expected_vm_error = 1;
    CHECK(!execute_source(vm, root+2, "AuxOuter();"));
    expected_vm_error = 0;
    if (!strstr(aux_output, "AN ERROR HAS OCCURED [aux failure]"))
        fprintf(stderr, "aux output: %s\n", aux_output);
    CHECK(strstr(aux_output, "AN ERROR HAS OCCURED [aux failure]"));
    CHECK(strstr(aux_output, "*FUNCTION [unknown()] stage contract line [7]"));
    CHECK(strstr(aux_output, "*FUNCTION [AuxOuter()]"));
    CHECK(strstr(aux_output, "[outerValue] 31"));
    CHECK(strstr(aux_output, "[fraction] 1.25"));
    CHECK(strstr(aux_output, "[enabled] true"));
    CHECK(strstr(aux_output, "[arg] 9"));
    CHECK(function_48aa20(vm) == top);
    aux_output[0] = 0;
    expected_vm_error = 1;
    CHECK(!execute_source(vm, root+2, "throw 17;"));
    expected_vm_error = 0;
    CHECK(strstr(aux_output, "AN ERROR HAS OCCURED [unknown]"));
    CHECK(function_48aa20(vm) == top);
    aux_output[0] = 0;
    function_4c5c40(vm, PTR("bad token"), PTR("example.nut"), 4, 7);
    CHECK(strstr(aux_output, "example.nut line = (4) column = (7) : error bad token"));
    function_48b8b0(vm, old_print);
    function_48afa0(vm, old_compiler);
    retdec_squirrel_assign((int32_t *)(intptr_t)(vm+72), old_handler);
    retdec_release_squirrel_value(old_handler);
    puts("PASS: relocated standard error handlers, anonymous call stack, captured locals and stack balance");
    return 0;
}

static int test_vm_error_unwind(int32_t vm, int32_t *root) {
    int32_t top = function_48aa20(vm), base = *(int32_t *)(intptr_t)(vm + 52);
    int32_t frames = *(int32_t *)(intptr_t)(vm + 100);
    int32_t vargs = *(int32_t *)(intptr_t)(vm + 40);
    int32_t traps = *(int32_t *)(intptr_t)(vm + 124);
    int32_t native_depth = *(int32_t *)(intptr_t)(vm + 144);
    int result;
    expected_vm_error = 1;
    result = execute_source(vm, root + 2,
        "function ErrorLeaf(value) { throw 7; }\n"
        "function ErrorOuter() { ErrorLeaf(3); }\nErrorOuter();");
    expected_vm_error = 0;
    CHECK(!result);
    CHECK(*(int32_t *)(intptr_t)(vm + 52) == base);
    CHECK(*(int32_t *)(intptr_t)(vm + 100) == frames);
    CHECK(*(int32_t *)(intptr_t)(vm + 40) == vargs);
    CHECK(*(int32_t *)(intptr_t)(vm + 124) == traps);
    CHECK(*(int32_t *)(intptr_t)(vm + 144) == native_depth);
    CHECK(function_48aa20(vm) == top);
    CHECK(execute_source(vm,root+2,"afterError <- 17;\nif(afterError!=17) throw 1;"));
    expected_vm_error = 1;
    result = execute_source(vm, root + 2,
        "caughtCount <- 0;\n"
        "function CatchOuter() {\n"
        "  try { ErrorOuter(); } catch (e) { if(e!=7) throw 99; ::caughtCount++; }\n"
        "}\n"
        "for(local i=0;i<64;i++) CatchOuter();\n"
        "if(caughtCount!=64) throw 98;");
    expected_vm_error = 0;
    CHECK(result);
    CHECK(*(int32_t *)(intptr_t)(vm + 52) == base);
    CHECK(*(int32_t *)(intptr_t)(vm + 100) == frames);
    CHECK(*(int32_t *)(intptr_t)(vm + 124) == traps);
    CHECK(function_48aa20(vm) == top);
    expected_vm_error = 1;
    result = execute_source(vm,root+2,
        "nativeCaught <- 0;\n"
        "for(local i=0;i<32;i++) {\n"
        "  try { ErrorOuter.call(this); } catch(e) { if(e!=7) throw 97; nativeCaught++; }\n"
        "}\nif(nativeCaught!=32) throw 96;");
    expected_vm_error = 0;
    CHECK(result);
    CHECK(*(int32_t *)(intptr_t)(vm + 52) == base);
    CHECK(*(int32_t *)(intptr_t)(vm + 100) == frames);
    CHECK(*(int32_t *)(intptr_t)(vm + 124) == traps);
    CHECK(*(int32_t *)(intptr_t)(vm + 144) == native_depth);
    CHECK(function_48aa20(vm) == top);
    expected_vm_error = 1;
    result = execute_source(vm,root+2,
        "function VarError(...) { throw 7; }\nVarError(3,4,5);");
    expected_vm_error = 0;
    CHECK(!result);
    CHECK(*(int32_t *)(intptr_t)(vm + 40) == vargs);
    CHECK(*(int32_t *)(intptr_t)(vm + 52) == base);
    CHECK(*(int32_t *)(intptr_t)(vm + 100) == frames);
    CHECK(function_48aa20(vm) == top);
    {
        int32_t object[2]={g483,g484};
        CHECK(execute_source(vm,root+2,
            "sharedArgument <- {};\n"
            "function DefaultArgs(a=sharedArgument,b=sharedArgument) { "
            "if(a!=sharedArgument || b!=sharedArgument) throw 31; }\n"
            "function ObjectVarargs(...) { if(vargc!=2 || vargv[0]!=sharedArgument || "
            "vargv[1]!=sharedArgument) throw 32; }"));
        CHECK(retdec_sqrat_get(PTR(root),"sharedArgument",object));
        int32_t refs=*(int32_t *)(intptr_t)(object[1]+4);
        CHECK(execute_source(vm,root+2,"DefaultArgs();"));
        CHECK(execute_source(vm,root+2,"ObjectVarargs(sharedArgument,sharedArgument);"));
        CHECK(execute_source(vm,root+2,
            "for(local i=0;i<64;i++) { DefaultArgs(); ObjectVarargs(sharedArgument,sharedArgument); "
            "try { VarError(sharedArgument,sharedArgument); } catch(e) { if(e!=7) throw 33; } }"));
        CHECK(*(int32_t *)(intptr_t)(object[1]+4)==refs);
        CHECK(*(int32_t *)(intptr_t)(vm+40)==vargs);
        CHECK(execute_source(vm,root+2,
            "function OuterFactory(value) {\n"
            " local first=function():(value) { return function():(value) { return value; }; };\n"
            " return first();\n}\n"
            "captureSymbol <- sharedArgument;\n"
            "function SymbolCaptured():(captureSymbol) { return captureSymbol; }\n"
            "if(OuterFactory(sharedArgument)()!=sharedArgument || SymbolCaptured()!=sharedArgument) "
            "throw \"closure capture source\";"));
        retdec_sqrat_release_pair(vm,object);
    }
    CHECK(execute_source(vm,root+2,
        "callableObject <- delegate { _call=function(env,a,b) { return a*10+b; } } : {};\n"
        "function MetaNested(v) { local second=5; return ::callableObject(v,second); }\n"
        "if(MetaNested(7)!=75) throw \"metacall argument base\";"));
    {
        int32_t old_handler[2]={g483,g484};
        retdec_squirrel_assign(old_handler,(int32_t *)(intptr_t)(vm+72));
        CHECK(execute_source(vm,root+2,
            "errorObject <- {code=13};\nhandlerCalls <- 0;\nhandlerError <- null;\n"
            "seterrorhandler(function(e) { ::handlerCalls++; ::handlerError=e; });"));
        expected_vm_error=1;
        result=execute_source(vm,root+2,"throw errorObject;");
        expected_vm_error=0;
        CHECK(!result);
        CHECK(execute_source(vm,root+2,
            "if(handlerCalls!=1 || handlerError!=errorObject) throw \"error object lost\";\n"
            "try { ErrorOuter.pcall(this); } catch(e) { if(e!=7) throw 1; }\n"
            "if(handlerCalls!=1) throw \"pcall raised error hook\";"));
        retdec_squirrel_assign((int32_t *)(intptr_t)(vm+72),old_handler);
        retdec_release_squirrel_value(old_handler);
        CHECK(*(int32_t *)(intptr_t)(vm+52)==base && *(int32_t *)(intptr_t)(vm+100)==frames);
        CHECK(function_48aa20(vm)==top);
    }
    {
        int32_t manager = PTR(g_retdec_actor_manager_state), init[3];
        CHECK(execute_source(vm,root+2,
            "failedSteps <- 0;\nhealthySteps <- 0;\n"
            "function FailureStep() { ::failedSteps++; ErrorOuter(); }\n"
            "function HealthyStep() { ::healthySteps++; }\n"
            "function InitErrorActor(bad) { updateGroup=1; vx=1.0; "
            "SetUpdateFunction(bad ? ::FailureStep : ::HealthyStep); }"));
        function_4aa3a0_this(PTR(root+1), PTR(init), "InitErrorActor");
        int32_t broken = function_463b40_this(manager, init[0],init[1],init[2],
            100,200,-1,PTR(&g16),0x01000008,1,0);
        int32_t healthy = function_463b40_this(manager, init[0],init[1],init[2],
            300,200,-1,PTR(&g16),0x01000008,0,0);
        CHECK(broken && healthy);
        *(unsigned char *)(intptr_t)(broken+40)=1;
        *(unsigned char *)(intptr_t)(healthy+40)=1;
        *(int32_t *)(intptr_t)(manager+64)=1;
        expected_vm_error=1;
        for(int frame=0;frame<64;++frame)
            retdec_actor_manager_update(manager,PTR(g_retdec_camera_state));
        expected_vm_error=0;
        CHECK(execute_source(vm,root+2,
            "if(failedSteps!=1 || healthySteps!=64) throw \"update error counts\";"));
        CHECK(*(int32_t *)(intptr_t)(broken+112)!=0x08000100);
        CHECK(*(int32_t *)(intptr_t)(broken+116)==0);
        CHECK(*(int32_t *)(intptr_t)(healthy+112)==0x08000100);
        CHECK(execute_source(vm,root+2,
            "if(failedSteps!=1 || healthySteps!=64) throw \"update error retirement\";"));
        CHECK(*(int32_t *)(intptr_t)(vm+52)==base && *(int32_t *)(intptr_t)(vm+100)==frames);
        CHECK(function_48aa20(vm)==top);
        function_4a9840_this(PTR(root+1),"replacementProbe",healthy+44);
        CHECK(execute_source(vm,root+2,
            "replacementCalls <- 0;\n"
            "function ReplacementStep() { ::replacementCalls++; }\n"
            "replacementProbe.SetUpdateFunction(function() {\n"
            "SetUpdateFunction(::ReplacementStep); throw 123; });"));
        expected_vm_error=1;
        retdec_actor_tick(healthy);
        expected_vm_error=0;
        for(int i=0;i<8;++i) retdec_actor_tick(healthy);
        CHECK(execute_source(vm,root+2,
            "if(replacementCalls!=0) throw \"original failure retirement changed\";"));
        function_469700();
        function_4a9d70_this(PTR(init));
    }
    {
        int32_t transition[2]={g483,g484};
        if(retdec_sqrat_get(PTR(root),"UpdateStageChange",transition)) {
            CHECK(execute_source(vm,root+2,
                "reentryLoads <- 0;\nreentrySaves <- 0;\n"
                "function DisableInput() {}\n"
                "function SavePlayerState() { ::reentrySaves++; }\n"
                "function LoadStage(name) { ::reentryLoads++; }\n"
                "stageNameNext=\"w0-s01a.act\"; stageChangeCount=0;\n"
                "SetGlobalUpdateFunction(UpdateStageChange);"));
            for(int frame=0;frame<120;++frame)
                CHECK(retdec_actor_step_callback(PTR(g612))>=0);
            CHECK(execute_source(vm,root+2,
                "if(reentryLoads!=1 || reentrySaves!=1 || stageChangeCount!=-1) "
                "throw \"repeated stage reentry\";\nSetGlobalUpdateFunction(null);"));
            CHECK(execute_source(vm,root+2,
                "failedGlobalCalls <- 0;\n"
                "function FailedGlobal() { ::failedGlobalCalls++; throw 13; }\n"
                "SetGlobalUpdateFunction(FailedGlobal);"));
            int32_t old_mask=g459;
            g459=0;
            expected_vm_error=1;
            for(int frame=0;frame<64;++frame) function_469900();
            expected_vm_error=0;
            g459=old_mask;
            CHECK(execute_source(vm,root+2,
                "if(failedGlobalCalls!=1) throw \"global error repeated\";"));
            CHECK(g612[6]==0);
            CHECK(*(int32_t *)(intptr_t)(vm+52)==base && *(int32_t *)(intptr_t)(vm+100)==frames);
            CHECK(function_48aa20(vm)==top);
        }
        retdec_sqrat_release_pair(vm,transition);
    }
    puts("PASS: uncaught nested script errors restore VM frame, stack and varargs");
    return 0;
}

static int32_t relocate_vm_stack(int32_t vm) {
    int32_t size = *(int32_t *)(intptr_t)(vm + 28);
    int32_t capacity = *(int32_t *)(intptr_t)(vm + 32);
    int32_t *copy = (int32_t *)malloc((size_t)capacity * 8);
    if (!copy || retired_vm_stack) return -1;
    retired_vm_stack = *(void **)(intptr_t)(vm + 24);
    memcpy(copy, retired_vm_stack, (size_t)size * 8);
    *(int32_t *)(intptr_t)(vm + 24) = PTR(copy);
    function_48a4f0(vm, 12345);
    return 1;
}

static int test_native_stack_relocation(int32_t vm, int32_t *root) {
    CHECK(retdec_sqrat_set_native_closure(vm, root + 2, "RelocateStack",
        PTR(relocate_vm_stack), NULL, 0));
    CHECK(execute_source(vm,root+2,
        "relocationResult <- RelocateStack();\n"
        "if(relocationResult!=12345) throw \"native return used retired stack\";"));
    CHECK(retired_vm_stack);
    free(retired_vm_stack);
    retired_vm_stack = NULL;
    puts("PASS: native return survives VM stack relocation");
    return 0;
}

static int test_branch_motion(int32_t manager) {
    const char *paths[] = {"data/map/w1-c01a.act", "data/map/w1-c01b.act", "data/map/w1-c01a.act"};
    for (int round = 0; round < 3; ++round) {
        int32_t act[60];
        function_427530(PTR(act));
        CHECK(function_428000(PTR(act), paths[round]));
        CHECK(function_468950_this(PTR(g_514300_storage), manager));
        for (int32_t slot = act[52]; slot != act[53]; slot += 4) {
            int32_t layer = *(int32_t *)(intptr_t)slot;
            const char *name = retdec_std_string_data(layer + 112);
            if (strncmp(name,"te",2) != 0 && strncmp(name,"wa",2) != 0) continue;
            int32_t head = *(int32_t *)(intptr_t)(layer + 180);
            for (int32_t node = *(int32_t *)(intptr_t)head; node != head;
                 node = *(int32_t *)(intptr_t)node) {
                int32_t key = *(int32_t *)(intptr_t)(node + 8);
                int32_t layout = *(int32_t *)(intptr_t)(key + 4);
                if (retdec_map_chip_data(layout)) CHECK(function_4693a0(layout));
            }
        }
        for (int direction = -1; direction <= 1; direction += 2) {
            int32_t actor = function_463b40_this(manager, PTR(&g16), g483, g484,
                round == 1 ? 330.0f : 3120.0f, round == 1 ? 543.0f : 895.0f,
                (float)direction, PTR(&g16), g483, g484, 0);
            CHECK(actor);
            function_462280_this(actor, round == 1 ? 615 : 335);
            *(int32_t *)(intptr_t)(actor + 316) = 1;
            *(float *)(intptr_t)(actor + 256) = direction * 2.5f;
            *(float *)(intptr_t)(actor + 260) = -9.0f;
            retdec_actor_manager_refresh(manager);
            for (int frame = 0; frame < 240; ++frame) {
                function_468620_this(PTR(g_514300_storage));
                retdec_actor_update_motion(actor);
                float *y = (float *)(intptr_t)(actor + 244);
                float *vy = (float *)(intptr_t)(actor + 260);
                if (!_finite(*y) || !_finite(*(float *)(intptr_t)(actor + 308)))
                    fprintf(stderr,"branch motion invalid round=%d dir=%d frame=%d xy=(%g,%g) vy=%g pitch=%g\n",
                        round,direction,frame,*(float *)(intptr_t)(actor+240),*y,*vy,
                        *(float *)(intptr_t)(actor+276));
                CHECK(_finite(*y) && _finite(*(float *)(intptr_t)(actor + 308)));
                if (*(int32_t *)(intptr_t)(actor + 288) && *vy < 0) *vy = 0;
                if (*(int32_t *)(intptr_t)(actor + 296) && *vy >= 0) *vy = -9;
                else if (*vy < 9) *vy += 0.38f;
            }
            kinoko_actor_release(actor, NULL);
            retdec_actor_manager_refresh(manager);
        }
        function_469700();
        CHECK(function_468950_this(PTR(g_514300_storage), manager));
        retdec_destroy_cact_object(PTR(act));
    }
    puts("PASS: native branch-return motion stays finite across 1440 contact frames");
    return 0;
}

static int test_map_transition(int32_t vm, int32_t *root) {
    const char *paths[]={"data/map/w1-c01a.act","data/map/w1-c01b.act","data/map/w1-c01a.act"};
    int32_t map_state=PTR(g_retdec_map_manager_state);
    int32_t render_head[3]={0};
    render_head[0]=render_head[1]=PTR(render_head);
    g613=PTR(render_head);
    function_4a94e0_this(PTR(g722));
    function_4a95c0_this(PTR(g722),PTR(root+1));
    CHECK(function_46f4c0_this(map_state));
    function_46fac0();
    function_4669d0();
    {
        int32_t class_environment[3];
        CHECK(execute_source(vm,root+2,"classFixture <- {Actor={},Camera=Camera};"));
        function_4aa3a0_this(PTR(root+1),PTR(class_environment),"classFixture");
        CHECK(execute_asset(vm,class_environment+1,"data/script/class_def.cv4"));
        function_4a9d70_this(PTR(class_environment));
    }
    function_466270();
    CHECK(execute_asset(vm,root+2,"data/script/camera.cv4"));
    int32_t targets[]={PTR(function_469840),PTR(function_469700),PTR(function_469870),
        PTR(function_46a1d0),PTR(retdec_create_render_layer_fixed),PTR(function_469880),PTR(function_469d10)};
    int32_t adapters[]={PTR(function_471d90),PTR(function_471bc0),PTR(function_471bc0),
        PTR(function_471bc0),PTR(function_471f10),PTR(function_471f10),PTR(function_471e50)};
    const char *names[]={"LoadMap","ClearActor","ClearCollision","ClearRenderLayer", "CreateRenderLayer",
        "CreateCollision","CreateActorFromMap"};
    for(int i=0;i<7;++i)
        CHECK(function_415550_this(PTR(root),PTR(names[i]),PTR(targets+i),4,
            adapters[i],0)>=0);
    CHECK(execute_source(vm,root+2,"PlayerStatus <- {time=120};"));
    CHECK(execute_source(vm,root+2,"stageName=\"previous.act\";"));
    CHECK(execute_source(vm,root+2,
        "stageActorFlagTable <- {};\nstageVal <- {};\nloop <- [];\nloopPosition <- [];\n"
        "t_lift <- {};\nt_boss <- {};\nt_boss_ex <- {};\n"
        "function PlayBgm(a,b,c,d) {}\n"
        "function EventCallback(id,x1,y1,x2,y2) {}\n"
        "stageNoIntro=true;"));
    for(int i=0;i<3;++i) {
        fprintf(stderr,"map transition %s\n",paths[i]);
        CHECK(execute_source(vm,root+2,i==1 ? "LoadStage(\"w1-c01b.act\");" : "LoadStage(\"w1-c01a.act\");"));
        CHECK(*(int32_t *)(intptr_t)(map_state+12));
        fprintf(stderr,"map actors=%d layers=%d\n",retdec_actor_manager_refresh(PTR(g_retdec_actor_manager_state)),g614);
        CHECK(execute_source(vm,root+2,
            "player <- {x=800.0,y=850.0,vx=2.5,vy=0.0,direction=1.0,hitBottom=1,\n"
            " left=792.0,right=808.0,top=818.0,bottom=850.0,take=100,\n"
            " user={hold=null,water=false,deadCount=0,take=0}};\nInitCamera(player);\n"));
        for(int frame=0;frame<180;++frame) {
            CHECK(execute_source(vm,root+2,"player.x+=2.5; player.left+=2.5; player.right+=2.5;"));
            CHECK(kinoko_camera_update(PTR(g_retdec_camera_state), NULL)>=0);
            retdec_actor_manager_update(PTR(g_retdec_actor_manager_state),PTR(g_retdec_camera_state));
            CHECK(function_46f0b0(map_state)>=0);
            int32_t manager=PTR(g_retdec_actor_manager_state);
            int32_t *actors=*(int32_t **)(intptr_t)(manager+100);
            for(int n=0;n<*(int32_t *)(intptr_t)(manager+116);++n) {
                int32_t actor=actors[n];
                int32_t animation_frame=*(int32_t *)(intptr_t)(actor+204);
                if(animation_frame) {
                    int32_t handle=*(int32_t *)(intptr_t)(animation_frame+4);
                    *(int32_t *)(intptr_t)(animation_frame+4)=1;
                    retdec_actor_render(actor,PTR(g_retdec_camera_state));
                    *(int32_t *)(intptr_t)(animation_frame+4)=handle;
                }
                if(!_finite(*(float *)(intptr_t)(actor+240)) || !_finite(*(float *)(intptr_t)(actor+244)) ||
                    !_finite(*(float *)(intptr_t)(actor+308))) {
                    fprintf(stderr,"scene invalid round=%d frame=%d id=%x take=%d xy=(%g,%g) pitch=%g\n",
                        i,frame,*(int32_t *)(intptr_t)(actor+224),*(int32_t *)(intptr_t)(actor+208),
                        *(float *)(intptr_t)(actor+240),*(float *)(intptr_t)(actor+244),*(float *)(intptr_t)(actor+276));
                    CHECK(0);
                }
            }
        }
    }
    function_469700();
    function_469870();
    char retired_name[256];
    strcpy_s(retired_name,sizeof(retired_name),retdec_std_string_data(
        *(int32_t *)(intptr_t)(map_state+12)+16));
    int32_t runtime=*(int32_t *)(intptr_t)(map_state+20);
    CHECK(strcmp(retdec_std_string_data(runtime+164),retired_name)==0);
    int32_t sprites=PTR(calloc(2,184));
    CHECK(sprites);
    *(int32_t *)(intptr_t)(runtime+60)=sprites;
    *(int32_t *)(intptr_t)(runtime+64)=sprites+2*184;
    *(int32_t *)(intptr_t)(runtime+68)=sprites+2*184;
    CHECK(kinoko_act_end_stage(runtime, NULL)==0);
    CHECK(*(int32_t *)(intptr_t)(runtime+64)==sprites);
    CHECK(*(int32_t *)(intptr_t)(runtime+68)==sprites+2*184);
    function_469860();
    int32_t retired_environment[2]={g483,g484};
    CHECK(!retdec_sqrat_get(PTR(root),retired_name,retired_environment));
    function_46a1d0();
    g613=0;
    puts("PASS: first-stage passage load/release/reload, camera, actors, ACT update and draw transforms");
    return 0;
}

static int test_map_camera_fpu(void) {
    int32_t manager[8] = {0}, layer[32] = {0}, camera[24] = {0};
    __declspec(align(16)) unsigned char before[512], after[512];
    manager[3] = PTR(layer);
    *(float *)(camera + 10) = 100.0f;
    *(float *)(camera + 11) = 200.0f;
    *(float *)(camera + 12) = 98.75f;
    *(float *)(camera + 13) = 203.5f;
    _fxsave(before);
    for (int frame = 0; frame < 600; ++frame) {
        function_46edc0_this(PTR(manager), camera);
        _fxsave(after);
        if (before[4] != after[4])
            fprintf(stderr, "map camera leaked x87 stack at frame %d: tag %02X -> %02X\n",
                frame, before[4], after[4]);
        CHECK(before[4] == after[4]);
        CHECK((*(uint16_t *)(before+2) & 0x3800) == (*(uint16_t *)(after+2) & 0x3800));
        CHECK(*(float *)(layer + 22) == -2.0f);
        CHECK(*(float *)(layer + 23) == 3.0f);
    }
    puts("PASS: map camera floor results and balanced x87 stack over 600 rendered frames");
    return 0;
}

static int callback_external_refs(int32_t vm, const int32_t *object) {
    int32_t table = *(int32_t *)(intptr_t)(vm + 140) + 24;
    int32_t slots = *(int32_t *)(intptr_t)table;
    int32_t nodes = *(int32_t *)(intptr_t)(table + 8);
    for (int32_t i = 0; i < slots; ++i) {
        int32_t *node = (int32_t *)(intptr_t)(nodes + 16 * i);
        if (node[0] == object[1] && node[1] == object[2])
            return node[2];
    }
    return 0;
}

static int test_script_callback_binding(int32_t vm, int32_t *root) {
    int32_t actor[160] = {0}, camera[128] = {0};
    int32_t first[3], second[3], argument[3];
    int32_t empty[7] = {0}, empty_type, empty_value;
    void *methods[3] = {kinoko_actor_set_update_callback,
        kinoko_actor_set_collision_callback, kinoko_camera_set_update_callback};
    int32_t *receivers[3] = {actor, actor, camera};
    int32_t *callbacks[3] = {actor + 23, actor + 30, camera + 3};
    CHECK(execute_source(vm, root + 2,
        "callbackCount <- 0;\n"
        "callbackFirst <- function() { ::callbackCount += 1; };\n"
        "callbackSecond <- function() { ::callbackCount += 10; };"));
    function_4aa3a0_this(PTR(root + 1), PTR(first), "callbackFirst");
    function_4aa3a0_this(PTR(root + 1), PTR(second), "callbackSecond");
    function_4a9500_this(actor + 11, PTR(root + 1));
    function_4a9500_this(camera, PTR(root + 1));
    for (int i = 0; i < 3; ++i) {
        function_4a94e0_this(PTR(callbacks[i] + 1));
        function_4a94e0_this(PTR(callbacks[i] + 4));
    }
    /* Match the existing empty-function constructor, including its zero-type
       compatibility value; this migration does not normalize it to OT_NULL. */
    CHECK(retdec_function_45df10_impl(PTR(empty), 0) == PTR(empty));
    empty_type = empty[5];
    empty_value = empty[6];
    function_4a9d70_this(PTR(empty + 4));
    function_4a9d70_this(PTR(empty + 1));
    int32_t first_refs = callback_external_refs(vm, first);
    int32_t second_refs = callback_external_refs(vm, second);
    int32_t root_refs = callback_external_refs(vm, root + 1);
    int32_t stack_top = function_48aa20(vm);
    CHECK(first[1] == 0x08000100 && second[1] == 0x08000100);
    CHECK(first_refs > 0 && second_refs > 0);
    for (int iteration = 0; iteration < 64; ++iteration) {
        int32_t *source = (iteration / 2) % 2 ? second : first;
        for (int i = 0; i < 3; ++i) {
            function_4a9500_this(argument, PTR(source));
            retdec_call_thiscall3_result(receivers[i], methods[i],
                argument[0], argument[1], argument[2]);
            CHECK(callbacks[i][0] == vm);
            CHECK(callbacks[i][1] == PTR(&g16) && callbacks[i][4] == PTR(&g16));
            CHECK(callbacks[i][2] == root[2] && callbacks[i][3] == root[3]);
            CHECK(callbacks[i][5] == source[1] && callbacks[i][6] == source[2]);
        }
        CHECK(callback_external_refs(vm, first) == first_refs + (source == first ? 3 : 0));
        CHECK(callback_external_refs(vm, second) == second_refs + (source == second ? 3 : 0));
        CHECK(callback_external_refs(vm, root + 1) == root_refs + 3);
        CHECK(function_48aa20(vm) == stack_top);
    }
    retdec_call_thiscall0_result(camera, kinoko_camera_update);
    CHECK(function_48aa20(vm) == stack_top);
    CHECK(execute_source(vm, root + 2,
        "if (callbackCount != 10) throw \"camera callback result\";"));

    /* A non-closure clears collision callbacks but remains a non-dispatched
       Camera value, matching the original distinct method rules. */
    for (int i = 0; i < 3; ++i)
        retdec_call_thiscall3_result(receivers[i], methods[i], PTR(&g16), 0x05000002, 7);
    CHECK(actor[35] == empty_type && actor[36] == empty_value);
    CHECK(camera[8] == 0x05000002 && camera[9] == 7);
    CHECK(retdec_call_thiscall0_result(camera, kinoko_camera_update) == 0x05000002);
    for (int i = 0; i < 3; ++i) {
        retdec_call_thiscall3_result(receivers[i], methods[i], PTR(&g16), g483, g484);
        CHECK(callbacks[i][5] == (i == 1 ? empty_type : g483));
        CHECK(callbacks[i][6] == (i == 1 ? empty_value : g484));
        function_4a9d70_this(PTR(callbacks[i] + 4));
        function_4a9d70_this(PTR(callbacks[i] + 1));
    }
    CHECK(callback_external_refs(vm, first) == first_refs);
    CHECK(callback_external_refs(vm, second) == second_refs);
    CHECK(callback_external_refs(vm, root + 1) == root_refs);
    CHECK(function_48aa20(vm) == stack_top);
    CHECK(retdec_call_thiscall0_result(camera, kinoko_camera_update) == g483);
    function_4a9d70_this(PTR(camera));
    function_4a9d70_this(PTR(actor + 11));
    function_4a9d70_this(PTR(second));
    function_4a9d70_this(PTR(first));
    puts("PASS: C++ callback ABI, replacement/self-assignment, external refs, clear and Camera dispatch");
    return 0;
}

static int test_gc_chain_integrity(int32_t vm) {
    const int32_t shared = *(int32_t *)(intptr_t)(vm + 140);
    int32_t current = *(int32_t *)(intptr_t)(shared + 68), previous = 0;
    int32_t count = 0, found_vm = 0;
    while (current) {
        CHECK(++count < 100000);
        CHECK(retdec_gc_object_type(current) != 0);
        CHECK(*(int32_t *)(intptr_t)(current + 16) == previous);
        CHECK((*(uint32_t *)(intptr_t)(current + 4) & 0x80000000u) == 0);
        if (current == vm) found_vm = 1;
        previous = current;
        current = *(int32_t *)(intptr_t)(current + 12);
    }
    CHECK(found_vm);
    return 0;
}

static int test_gc_repeated_collection(int32_t vm, int32_t *root) {
    const int32_t shared = *(int32_t *)(intptr_t)(vm + 140);
    const int32_t top = function_48aa20(vm);
    CHECK(execute_source(vm, root + 2,
        "gcKeep <- { values = [17,23], action = function() { return 42; } };"));
    CHECK(test_gc_chain_integrity(vm) == 0);
    for (int round = 0; round < 32; ++round) {
        CHECK(execute_source(vm, root + 2,
            "gcTrashA <- {};\ngcTrashB <- {};\n"
            "gcTrashA.peer <- gcTrashB;\ngcTrashB.peer <- gcTrashA;\n"
            "gcTrashA.items <- [gcTrashA,gcTrashB];\n"
            "gcTrashB.callback <- function() { return 9; };\n"
            "delete ::gcTrashA;\ndelete ::gcTrashB;"));
        CHECK(function_49a520_this(shared, vm) >= 0);
        CHECK(test_gc_chain_integrity(vm) == 0);
        CHECK(function_48aa20(vm) == top);
        CHECK(execute_source(vm, root + 2,
            "if (gcKeep.values[0] != 17 || gcKeep.values[1] != 23 || "
            "gcKeep.action() != 42) throw \"GC lost a live object\";"));
        CHECK(test_gc_chain_integrity(vm) == 0);
    }
    CHECK(execute_source(vm, root + 2, "delete ::gcKeep;"));
    CHECK(function_49a520_this(shared, vm) >= 0);
    CHECK(test_gc_chain_integrity(vm) == 0);
    puts("PASS: repeated cyclic GC preserves root VM, live objects and doubly linked chain integrity");
    return 0;
}

static int test_gc_mark_link(void) {
    int32_t shared[40] = {0}, object[9] = {0}, head_storage[16] = {0};
    int32_t value[2] = {0x08000040, PTR(object)};
    object[1] = 1;
    object[5] = PTR(shared);
    shared[17] = PTR(object);
    head_storage[4] = 0x12345678;
    retdec_gc_mark_value(value, head_storage);
    CHECK(head_storage[0] == PTR(object));
    CHECK(head_storage[4] == 0x12345678);
    CHECK(object[3] == 0 && object[4] == 0);
    CHECK((uint32_t)object[1] == 0x80000001u);
    CHECK(shared[17] == 0);
    puts("PASS: GC Mark moves the node into the caller-owned chain without writing past its head");
    return 0;
}

static int test_shutdown_tree_cleanup(void) {
    for (int layout = 0; layout < 2; ++layout) {
        unsigned char sentinel[24] = {0};
        int32_t *nodes[7];
        int sentinel_offset = layout ? 21 : 17;
        int32_t sentinel_address = PTR(sentinel);
        sentinel[sentinel_offset] = 1;
        for (int i = 0; i < 7; ++i) {
            nodes[i] = (int32_t *)calloc(1, layout ? 24 : 20);
            CHECK(nodes[i]);
            nodes[i][0] = nodes[i][1] = nodes[i][2] = sentinel_address;
        }
        for (int i = 0; i < 3; ++i) {
            nodes[i][0] = PTR(nodes[2*i+1]);
            nodes[i][2] = PTR(nodes[2*i+2]);
            nodes[2*i+1][1] = nodes[2*i+2][1] = PTR(nodes[i]);
        }
        CHECK((layout ? function_429c70(PTR(nodes[0])) : function_4634d0(PTR(nodes[0])))
            == sentinel_address);
        CHECK(sentinel[sentinel_offset] == 1);
        CHECK((layout ? function_429c70(sentinel_address) : function_4634d0(sentinel_address))
            == sentinel_address);
    }

    int32_t manager[40] = {0}, animation_head[6] = {0}, priority_head[5] = {0};
    int32_t list_head[14] = {0}, textures[2] = {7, 13}, iteration[3] = {0};
    int32_t *animation_node = (int32_t *)calloc(1, 24);
    int32_t *priority_node = (int32_t *)calloc(1, 20);
    int32_t *list_node = (int32_t *)calloc(1, 56);
    unsigned char *frames = (unsigned char *)calloc(2, 248);
    CHECK(animation_node && priority_node && list_node && frames);
    *(int32_t *)(frames + 244) = PTR(malloc(12));
    *(int32_t *)(frames + 248 + 244) = PTR(malloc(20));
    CHECK(*(int32_t *)(frames + 244) && *(int32_t *)(frames + 492));
    ((unsigned char *)animation_head)[21] = 1;
    ((unsigned char *)priority_head)[17] = 1;
    animation_head[0] = animation_head[1] = animation_head[2] = PTR(animation_node);
    priority_head[0] = priority_head[1] = priority_head[2] = PTR(priority_node);
    animation_node[0] = animation_node[1] = animation_node[2] = PTR(animation_head);
    priority_node[0] = priority_node[1] = priority_node[2] = PTR(priority_head);
    /* No live Actor in this fixture; the priority node is still reclaimed. */
    priority_node[3] = 0;
    list_head[0] = list_head[1] = PTR(list_node);
    list_node[0] = list_node[1] = PTR(list_head);
    list_node[4] = PTR(frames);
    list_node[5] = list_node[6] = PTR(frames + 496);
    animation_node[4] = PTR(list_node + 2);
    manager[10] = PTR(animation_head); manager[11] = 1;
    manager[13] = PTR(list_head); manager[14] = 1;
    manager[17] = PTR(textures); manager[18] = manager[19] = PTR(textures + 2);
    manager[22] = PTR(priority_head); manager[23] = 1;
    manager[25] = PTR(iteration); manager[26] = manager[27] = PTR(iteration + 3);
    manager[29] = 8; ((unsigned char *)manager)[120] = 1;
    for (int repeat = 0; repeat < 2; ++repeat) {
        CHECK(function_464e20(PTR(manager)) == PTR(iteration));
        for (int i = 0; i < 3; ++i) {
            CHECK(animation_head[i] == PTR(animation_head));
            CHECK(priority_head[i] == PTR(priority_head));
        }
        CHECK(list_head[0] == PTR(list_head) && list_head[1] == PTR(list_head));
        CHECK(manager[11] == 0 && manager[14] == 0 && manager[23] == 0);
        CHECK(manager[18] == PTR(textures) && manager[19] == PTR(textures + 2));
        CHECK(manager[26] == PTR(iteration) && manager[27] == PTR(iteration + 3));
        CHECK(manager[29] == 0 && ((unsigned char *)manager)[120] == 0);
    }
    puts("PASS: shutdown tree recursion, sentinel preservation, frame payloads and repeatable manager clear");
    return 0;
}

static int test_animation_timing(void) {
    int32_t actor[160] = {0}, animation[12] = {0};
    unsigned char frames[3][248] = {{0}};
    animation[2] = PTR(frames);
    animation[3] = PTR(frames + 3);
    *(uint8_t *)((char *)animation + 24) = 1;
    *(int16_t *)(frames[0] + 240) = 2;
    *(int16_t *)(frames[1] + 240) = 0;
    *(int16_t *)(frames[2] + 240) = -3;
    actor[38] = actor[51] = PTR(frames);
    actor[50] = PTR(animation);
    actor[52] = 41;
    retdec_actor_tick(PTR(actor));
    CHECK(actor[53] == 0 && actor[54] == 1 && actor[51] == PTR(frames));
    retdec_actor_tick(PTR(actor));
    CHECK(actor[53] == 1 && actor[54] == 0 && actor[51] == PTR(frames + 1));
    retdec_actor_tick(PTR(actor));
    CHECK(actor[53] == 2 && actor[54] == 0 && actor[51] == PTR(frames + 2));
    retdec_actor_tick(PTR(actor));
    CHECK(actor[53] == 0 && actor[54] == 0 && actor[51] == PTR(frames));
    CHECK(actor[38] == actor[51]);

    *(uint8_t *)((char *)animation + 24) = 0;
    actor[53] = 2;
    actor[38] = actor[51] = PTR(frames + 2);
    for (int i = 0; i < 3; ++i) retdec_actor_tick(PTR(actor));
    CHECK(actor[53] == 2 && actor[54] == 0 && actor[51] == PTR(frames + 2));
    actor[54] = 17;
    kinoko_actor_advance_animation(PTR(actor), 40);
    CHECK(actor[54] == 17 && actor[53] == 2);
    actor[51] = 0;
    retdec_actor_tick(PTR(actor));
    CHECK(actor[54] == 17);
    actor[51] = PTR(frames);
    actor[50] = 0;
    retdec_actor_tick(PTR(actor));
    CHECK(actor[54] == 18 && actor[53] == 2);
    actor[50] = PTR(animation);
    animation[3] = animation[2];
    retdec_actor_tick(PTR(actor));
    CHECK(actor[54] == 19 && actor[53] == 2);

    animation[3] = PTR(frames + 3);
    actor[54] = INT32_MAX;
    retdec_actor_tick(PTR(actor));
    CHECK(actor[54] == INT32_MIN && actor[53] == 2);
    actor[54] = 1;
    actor[53] = 99;
    retdec_actor_tick(PTR(actor));
    CHECK(actor[53] == 100 && actor[54] == 0);
    CHECK((uint32_t)actor[51] == (uint32_t)PTR(frames) + 100u * 248u);
    CHECK(actor[38] == actor[51]);
    puts("PASS: C++ animation signed durations, loop/hold, changed take, empty state and x86 wrap");
    return 0;
}

static int test_actor_state_fields(void) {
    unsigned char actor[640] = {0}, manager[160] = {0};
    unsigned char source[48], expected[640], expected_manager[160] = {0};
    for (int i = 0; i < 48; ++i) source[i] = (unsigned char)(i * 7 + 3);
    memset(actor, 0x5a, sizeof(actor));
    memcpy(expected, actor, sizeof(actor));
    memcpy(expected + 376, source, sizeof(source));
    CHECK(kinoko_actor_set_init_data(PTR(actor), PTR(source)) == PTR(actor));
    CHECK(memcmp(actor, expected, sizeof(actor)) == 0);
    CHECK(kinoko_actor_set_init_data(PTR(actor), 0) == 0);
    CHECK(kinoko_actor_set_init_data(0, PTR(source)) == 0);
    CHECK(memcmp(actor, expected, sizeof(actor)) == 0);
    memset(actor, 0, sizeof(actor));
    *(int32_t *)(actor + 148) = PTR(manager);
    memcpy(expected, actor, sizeof(actor));
    expected[22] = 1;
    expected_manager[120] = 1;
    for (int i = 0; i < 2; ++i) {
        CHECK(retdec_call_thiscall0_result(actor, kinoko_actor_release) == 1);
        CHECK(memcmp(actor, expected, sizeof(actor)) == 0);
        CHECK(memcmp(manager, expected_manager, sizeof(manager)) == 0);
    }
    puts("PASS: C++ Actor exact init-data span and deferred-release byte writes/ABI");
    return 0;
}

static int test_actor_animation_sync(int32_t vm, int32_t *root,
                                      int32_t target, int32_t source) {
    const int offsets[6] = {152, 200, 204, 208, 212, 216};
    int32_t saved[6], source_frame = *(int32_t *)(intptr_t)(source + 212);
    int32_t source_time = *(int32_t *)(intptr_t)(source + 216);
    int32_t animation[4] = {0}, incoming[3];
    unsigned char frames[248 * 4] = {0};
    const int32_t indices[6] = {0, 1, 2, 3, 99, -1};
    const int32_t expected_indices[6] = {0, 1, 2, 2, 2, -1};
    for (int i = 0; i < 6; ++i)
        saved[i] = *(int32_t *)(intptr_t)(target + offsets[i]);
    animation[2] = PTR(frames + 248);
    animation[3] = PTR(frames + sizeof(frames));
    *(int32_t *)(intptr_t)(target + 200) = PTR(animation);
    int32_t refs = callback_external_refs(vm, (int32_t *)(intptr_t)(source + 44));
    int32_t top = function_48aa20(vm);
    for (int i = 0; i < 6; ++i) {
        *(int32_t *)(intptr_t)(source + 212) = indices[i];
        *(int32_t *)(intptr_t)(source + 216) = 100 + i;
        function_4a9500_this(incoming, source + 44);
        retdec_call_thiscall3_result((void *)(intptr_t)target, kinoko_actor_sync_animation,
            incoming[0], incoming[1], incoming[2]);
        CHECK(*(int32_t *)(intptr_t)(target + 212) == expected_indices[i]);
        CHECK(*(int32_t *)(intptr_t)(target + 216) == 100 + i);
        CHECK(*(int32_t *)(intptr_t)(target + 204) == animation[2] + 248 * expected_indices[i]);
        CHECK(*(int32_t *)(intptr_t)(target + 152) == *(int32_t *)(intptr_t)(target + 204));
        CHECK(*(int32_t *)(intptr_t)(target + 208) == saved[3]);
        CHECK(callback_external_refs(vm, (int32_t *)(intptr_t)(source + 44)) == refs);
        CHECK(function_48aa20(vm) == top);
    }
    animation[3] = animation[2];
    *(int32_t *)(intptr_t)(source + 212) = 20;
    function_4a9500_this(incoming, source + 44);
    retdec_call_thiscall3_result((void *)(intptr_t)target, kinoko_actor_sync_animation,
        incoming[0], incoming[1], incoming[2]);
    CHECK(*(int32_t *)(intptr_t)(target + 212) == 0);
    CHECK(*(int32_t *)(intptr_t)(target + 204) == animation[2]);

    *(int32_t *)(intptr_t)(target + 200) = 0;
    *(int32_t *)(intptr_t)(source + 216) = 999;
    function_4a9500_this(incoming, source + 44);
    retdec_call_thiscall3_result((void *)(intptr_t)target, kinoko_actor_sync_animation,
        incoming[0], incoming[1], incoming[2]);
    CHECK(*(int32_t *)(intptr_t)(target + 216) == 105);
    CHECK(callback_external_refs(vm, (int32_t *)(intptr_t)(source + 44)) == refs);

    *(int32_t *)(intptr_t)(target + 200) = PTR(animation);
    retdec_call_thiscall3_result((void *)(intptr_t)target, kinoko_actor_sync_animation,
        PTR(&g16), 0x05000002, 17);
    CHECK(*(int32_t *)(intptr_t)(target + 216) == 105);
    animation[3] = PTR(frames + sizeof(frames));
    *(int32_t *)(intptr_t)(target + 212) = 1;
    function_4a9500_this(incoming, target + 44);
    retdec_call_thiscall3_result((void *)(intptr_t)target, kinoko_actor_sync_animation,
        incoming[0], incoming[1], incoming[2]);
    CHECK(*(int32_t *)(intptr_t)(target + 212) == 1);
    CHECK(*(int32_t *)(intptr_t)(target + 204) == PTR(frames + 496));

    function_4a9840_this(PTR(root + 1), "syncTarget", target + 44);
    function_4a9840_this(PTR(root + 1), "syncSource", source + 44);
    *(int32_t *)(intptr_t)(source + 212) = 2;
    CHECK(execute_source(vm, root + 2,
        "syncTarget.SyncAnimation(syncSource); delete ::syncTarget; delete ::syncSource;"));
    CHECK(*(int32_t *)(intptr_t)(target + 212) == 2);
    CHECK(*(int32_t *)(intptr_t)(target + 216) == 999);
    CHECK(*(int32_t *)(intptr_t)(target + 204) == PTR(frames + 744));
    CHECK(callback_external_refs(vm, (int32_t *)(intptr_t)(source + 44)) == refs);
    CHECK(function_48aa20(vm) == top);
    for (int i = 0; i < 6; ++i)
        *(int32_t *)(intptr_t)(target + offsets[i]) = saved[i];
    *(int32_t *)(intptr_t)(source + 212) = source_frame;
    *(int32_t *)(intptr_t)(source + 216) = source_time;
    puts("PASS: C++ SyncAnimation ABI/script binding, frame bounds, self-sync, refs and stack balance");
    return 0;
}

static int test_act_resource_methods(void) {
    unsigned char resource[192] = {0};
    int32_t act[8] = {0}, holder = PTR(act);
    int32_t *words = (int32_t *)resource;
    int32_t address = PTR(resource);
    words[0] = PTR(&holder);
    act[1] = 10;
    CHECK(retdec_call_thiscall1_result(resource, kinoko_act_set_current_time, 37) == 0);
    CHECK(retdec_call_thiscall0_result(resource, kinoko_act_get_current_time) == 37);
    CHECK(retdec_call_thiscall0_result(resource, kinoko_act_get_current_frame) == 3);
    CHECK(retdec_call_thiscall0_result(resource, kinoko_act_increment_frame) == 0);
    CHECK(words[1] == 47);
    kinoko_act_set_current_time(address, NULL, -31);
    CHECK(kinoko_act_get_current_frame(address, NULL) == -3);
    words[1] = INT32_MAX - 4;
    kinoko_act_increment_frame(address, NULL);
    CHECK(words[1] == INT32_MIN + 5);
    act[1] = 0;
    CHECK(kinoko_act_get_current_frame(address, NULL) == 0);
    words[0] = 0;
    CHECK(kinoko_act_increment_frame(address, NULL) == 0);
    CHECK(words[1] == INT32_MIN + 5);

    uint32_t before = timeGetTime();
    int32_t deadline = retdec_call_thiscall1_result(resource, kinoko_act_sleep_to, 50);
    uint32_t after = timeGetTime();
    CHECK(words[25] == deadline);
    CHECK((uint32_t)deadline - before - 50u <= after - before);
    CHECK(retdec_call_thiscall1_result(resource, kinoko_act_sleep, 0) == 0);

    InitializeCriticalSection((struct retdec_RTL_CRITICAL_SECTION *)(resource + 20));
    resource[8] = 1;
    memset(resource + 108, 0x7f, 44);
    words[11] = 1234;
    words[12] = 5678;
    words[13] = 9000;
    words[38] = 4321;
    CHECK(retdec_call_thiscall0_result(resource, kinoko_act_end_stage) == 0);
    CHECK(resource[8] == 0 && words[12] == 1234 && words[13] == 9000);
    for (int i = 108; i < 152; ++i) CHECK(resource[i] == 0);
    CHECK(words[38] == 4321);
    CHECK(kinoko_act_end_stage(address, NULL) == (int32_t)E_FAIL);
    DeleteCriticalSection((struct retdec_RTL_CRITICAL_SECTION *)(resource + 20));
    puts("PASS: C++ ACT clock/ABI, time wrap, deferred sleep and stage cleanup");
    return 0;
}

static int test_sprite_geometry(void) {
    KinokoSprite sprite = {0};
    const float expected_x[4] = {91.5f, 155.5f, 91.5f, 155.5f};
    const float expected_y[4] = {194.5f, 194.5f, 218.5f, 218.5f};
    const float rotated_x[4] = {105.5f, 105.5f, 81.5f, 81.5f};
    const float rotated_y[4] = {191.5f, 255.5f, 191.5f, 255.5f};
    void *methods[3] = {g407.e7, g407.e8, g407.e9};
    union { float f; int32_t bits; } x = {100.0f}, y = {200.0f};
    int32_t saved_device = g678;

    sprite.width = 32;
    sprite.height = 48;
    sprite.pivot_x = 4;
    sprite.pivot_y = 10;
    sprite.scale_x = 2;
    sprite.scale_y = 0.5f;
    for (int i = 0; i < 4; ++i) {
        sprite.vertices[i].z = 0.25f;
        sprite.vertices[i].rhw = 1;
        sprite.vertices[i].color = 0x12345678;
        sprite.vertices[i].u = 0.125f;
        sprite.vertices[i].v = 0.75f;
    }
    g678 = 0;
    for (int method = 0; method < 3; ++method) {
        sprite.angle = 0;
        CHECK(retdec_call_thiscall2_result(&sprite, methods[method], x.bits, y.bits) == 0);
        for (int i = 0; i < 4; ++i) {
            CHECK(sprite.vertices[i].x == expected_x[i]);
            CHECK(sprite.vertices[i].y == expected_y[i]);
        }
        sprite.angle = 90;
        CHECK(retdec_call_thiscall2_result(&sprite, methods[method], x.bits, y.bits) == 0);
        for (int i = 0; i < 4; ++i) {
            CHECK(fabsf(sprite.vertices[i].x - rotated_x[i]) < 0.0001f);
            CHECK(fabsf(sprite.vertices[i].y - rotated_y[i]) < 0.0001f);
            CHECK(sprite.vertices[i].z == 0.25f && sprite.vertices[i].rhw == 1);
            CHECK(sprite.vertices[i].color == 0x12345678);
            CHECK(sprite.vertices[i].u == 0.125f && sprite.vertices[i].v == 0.75f);
        }
    }
    sprite.angle = 0;
    sprite.scale_x = -2;
    kinoko_sprite_transform(&sprite, x.f, y.f);
    CHECK(sprite.vertices[0].x == 107.5f && sprite.vertices[1].x == 43.5f);
    g678 = saved_device;
    puts("PASS: C++ sprite vtable, pivot/scale/rotation and retained vertex attributes");
    return 0;
}

static int error_releases;

static void __fastcall release_error_probe(void *self, void *unused) {
    (void)unused;
    if (((int32_t *)self)[1] == 0)
        ++error_releases;
}

static int test_error_value_ownership(int32_t vm) {
    int32_t methods[2] = {0, PTR(release_error_probe)};
    int32_t object[3] = {PTR(methods), 1, 0};
    int32_t value[2] = {0x08000080, PTR(object)};
    int32_t *error = (int32_t *)(intptr_t)(vm + 64);
    int32_t string_object;
    int32_t refs;

    function_48ac70(vm);
    error_releases = 0;
    function_499b00(vm, PTR(value));
    CHECK(object[1] == 2);
    function_499b00(vm, PTR(error));
    CHECK(object[1] == 2 && error_releases == 0);
    --object[1];
    CHECK(function_48ac00(vm, "ownership replacement") == -1);
    CHECK(error_releases == 1 && error[0] == 0x08000010);

    string_object = error[1];
    retdec_squirrel_addref(error[0], string_object);
    refs = *(int32_t *)(intptr_t)(string_object + 4);
    function_48ac70(vm);
    CHECK(error[0] == 0x01000001 && error[1] == 0);
    CHECK(*(int32_t *)(intptr_t)(string_object + 4) == refs - 1);
    retdec_squirrel_release(0x08000010, string_object);

    for (int i = 0; i < 128; ++i) {
        char expected[64];
        sprintf_s(expected, sizeof(expected), "ownership formatted error %d", i);
        function_499a20(vm, "ownership formatted error %d", i);
        CHECK(error[0] == 0x08000010);
        CHECK(*(int32_t *)(intptr_t)(error[1] + 4) == 1);
        CHECK(strcmp((char *)(intptr_t)(error[1] + 28), expected) == 0);
        function_499b00(vm, PTR(error));
        CHECK(*(int32_t *)(intptr_t)(error[1] + 4) == 1);
        CHECK(function_48ac00(vm, expected) == -1);
        CHECK(*(int32_t *)(intptr_t)(error[1] + 4) == 1);
    }
    function_48ac70(vm);

    object[1] = 1;
    value[0] = 0x08000080;
    value[1] = PTR(object);
    function_48e0e0_this(PTR(value), -37);
    CHECK(value[0] == 0x05000002 && value[1] == -37 && error_releases == 2);
    object[1] = 1;
    value[0] = 0x08000080;
    value[1] = PTR(object);
    function_48e120_this(PTR(value), 0.75f);
    CHECK(value[0] == 0x05000004 && *(float *)&value[1] == 0.75f && error_releases == 3);
    puts("PASS: source SQObjectPtr error replacement, self-assignment, reset and numeric ownership");
    return 0;
}

static int stage_owner_releases;
static int32_t __fastcall release_stage_owner(void *self, void *unused, int32_t flags) {
    int32_t *owner = *(int32_t **)((char *)self + 4);
    (void)unused;
    if (flags != 1 || owner[1] != 0 || ((int32_t *)(intptr_t)owner[2])[3] != 0)
        abort();
    ++stage_owner_releases;
    free(self);
    return 0;
}

static int test_global_stage_cleanup(void) {
    int32_t saved_head = g603, saved_count = g604;
    int32_t *head = calloc(3, 4), *tail = head;
    void *vtable[5] = {NULL, NULL, NULL, NULL, release_stage_owner};
    CHECK(head);
    head[0] = head[1] = PTR(head);
    g603 = PTR(head);
    g604 = 0;
    stage_owner_releases = 0;
    for (int i = 0; i < 2; ++i) {
        int32_t *node = calloc(3, 4), *owner = calloc(3, 4);
        int32_t *source = calloc(2, 4), *runtime = calloc(48, 4);
        CHECK(node && owner && source && runtime);
        source[0] = PTR(vtable);
        source[1] = PTR(owner);
        owner[0] = PTR(source);
        owner[1] = PTR(malloc(32));
        owner[2] = PTR(runtime);
        CHECK(function_44fde0(PTR(runtime), PTR(owner)) == PTR(runtime));
        runtime[3] = PTR(source); /* borrowed ACT, as current BeginStage */
        runtime[4] = PTR(malloc(24));
        runtime[11] = PTR(malloc(36));
        runtime[12] = runtime[11];
        node[0] = PTR(head);
        node[1] = PTR(tail);
        node[2] = PTR(owner);
        tail[0] = PTR(node);
        head[1] = PTR(node);
        tail = node;
        ++g604;
    }
    CHECK(function_465f70() == PTR(head));
    CHECK(stage_owner_releases == 2 && g604 == 0);
    CHECK(head[0] == PTR(head) && head[1] == PTR(head));
    CHECK(function_465f70() == PTR(head) && stage_owner_releases == 2);
    g603 = saved_head;
    g604 = saved_count;
    free(head);
    puts("PASS: global stage owners, runtime receivers, shared ACT ownership and repeated clear");
    return 0;
}

static ULONG WINAPI count_sound_release(void *self) {
    ++((int32_t *)self)[1];
    return 0;
}
static HRESULT WINAPI count_sound_stop(void *self) {
    ++((int32_t *)self)[2];
    return S_OK;
}
static int test_global_sound_cleanup(void) {
    int32_t old_head = g638, old_size = g639;
    int32_t *head = calloc(1, 24), *node = calloc(1, 24);
    void *vtable[19] = {0};
    int32_t buffers[3][3] = {0};
    CHECK(head && node);
    vtable[2] = count_sound_release;
    vtable[18] = count_sound_stop;
    for (int i = 0; i < 3; ++i) buffers[i][0] = PTR(vtable);
    g638 = PTR(head); g639 = 1;
    head[0] = head[1] = head[2] = PTR(node);
    ((unsigned char *)head)[21] = 1;
    node[0] = node[1] = node[2] = PTR(head);
    g_retdec_se_entry_count = 2;
    g_retdec_se_entries[0].buffer = buffers[0];
    g_retdec_se_entries[1].buffer = buffers[1];
    g_retdec_se_pool.stream_slots[0].buffer = buffers[2];
    g_retdec_se_pool.initialized = 1;
    CHECK(function_470890() == 1);
    CHECK(g639 == 0 && head[0] == PTR(head) && head[1] == PTR(head) && head[2] == PTR(head));
    CHECK(!g_retdec_se_entry_count && !g_retdec_se_pool.initialized);
    CHECK(function_470890() == 1);
    for (int i = 0; i < 3; ++i) CHECK(buffers[i][1] == 1);
    CHECK(buffers[0][2] == 1 && buffers[1][2] == 1);
    g638 = old_head; g639 = old_size;
    free(head);
    puts("PASS: SE buffers, streaming pool, sound lookup sentinel and repeatable shutdown");
    return 0;
}

static int test_global_script_cleanup(int32_t vm, int32_t *root) {
    int32_t *globals[] = {g602, g629, g611, g636, unk_5149EC};
    int32_t saved[5][3];
    CHECK(g645 == 0);
    for (int i = 0; i < 5; ++i) {
        memcpy(saved[i], globals[i], 12);
        function_4a9540_this(PTR(globals[i]), root[2], root[3]);
    }
    CHECK(function_4a8cc0() != 0);
    CHECK(function_470f30() == 0 && g644 == NULL && g645 == 0);
    for (int i = 0; i < 5; ++i) CHECK(globals[i][1] == g483 && globals[i][2] == 0);
    CHECK(function_470f30() == 0 && g645 == 0);
    for (int i = 0; i < 5; ++i) memcpy(globals[i], saved[i], 12);
    g644 = (char *)(intptr_t)vm;
    CHECK(execute_source(vm, root + 2, "if(typeof this!=\"table\") throw \"root lifetime\";"));
    puts("PASS: distinct global script receivers, root wrapper release and repeated shutdown");
    return 0;
}

static int test_generator_effects(int32_t vm, int32_t *root, const char *original_script) {
    const int32_t top = function_48aa20(vm);
    if (original_script) {
        CHECK(execute_file(vm, root + 2, original_script));
    } else {
        /* Same resume/cleanup sequence as EffectLayer.nut::Update, including
           the array receiver slot reused by len()-1 after a generator yield. */
        CHECK(execute_source(vm, root + 2,
            "effectList <- [];\n"
            "function Update() {\n"
            " for(local i=0; i<effectList.len(); i++)\n"
            "  if(effectList[i]) if(!(resume effectList[i])) effectList[i]=null;\n"
            " for(local i=effectList.len()-1; i>=0; i--)\n"
            "  if(effectList[i]==null) effectList.remove(i);\n"
            "}"));
    }
    CHECK(execute_source(vm, root + 2,
        "effectSteps <- 0;\n"
        "function ProbeEffect() { ::effectSteps++; yield true; ::effectSteps++; yield true; ::effectSteps++; return false; }\n"
        "effectList.append(ProbeEffect());\n"));
    for (int frame = 0; frame < 5; ++frame) {
        CHECK(execute_source(vm, root + 2, "Update();"));
        int32_t list[3], steps[3];
        function_4aa3a0_this(PTR(root+1), PTR(list), "effectList");
        function_4aa3a0_this(PTR(root+1), PTR(steps), "effectSteps");
        int32_t *array = (int32_t *)(intptr_t)list[2];
        CHECK(array[0] == PTR(&g69));
        CHECK(array[7] == (frame < 2 ? 1 : 0));
        CHECK(steps[2] == (frame < 2 ? frame + 1 : 3));
        function_4a9d70_this(PTR(steps));
        function_4a9d70_this(PTR(list));
        CHECK(function_48aa20(vm) == top);
    }
    CHECK(execute_source(vm, root + 2,
        "removedItem <- { value=17 };\n"
        "removeArray <- [null,removedItem,29];\n"
        "if(removeArray.remove(1.9)!=removedItem || removeArray.len()!=2) throw \"remove return\";\n"
        "if(removeArray.remove(0)!=null || removeArray.remove(0)!=29 || removeArray.len()!=0) throw \"remove order\";\n"));
    expected_vm_error = 1;
    CHECK(execute_source(vm, root + 2,
        "removeErrors <- 0;\n"
        "try { removeArray.remove(0); } catch(e) { if(e!=\"idx out of range\") throw e; removeErrors++; }\n"));
    expected_vm_error = 0;
    CHECK(execute_source(vm, root + 2,
        "if(removeErrors!=1 || removedItem.value!=17) throw \"remove ownership\";\n"));
    if (original_script) {
        /* Run unmodified original GenSmokeEffect and Update. The draw boundary
           records calls so this regression needs no D3D window or gameplay. */
        CHECK(execute_source(vm, root + 2,
            "smokeDraws <- 0; BLEND_ALPHA <- 1;\n"
            "resource <- {};\n"
            "resource[\"smoke-small_0000\"] <- 0;\n"
            "resource[\"smoke-small_0001\"] <- 1;\n"
            "resource[\"smoke-small_0002\"] <- 2;\n"
            "resource[\"smoke-small_0003\"] <- 3;\n"
            "resource[\"smoke-small_0004\"] <- 4;\n"
            "pl <- { BitBlt=function(x,y,w,h,r,sx,sy,blend,alpha) {\n"
            " if(x!=64 || y!=80 || w!=32 || h!=32 || r!=::smokeDraws/5 || alpha!=1.0) throw \"smoke drawing\";\n"
            " ::smokeDraws++;\n"
            "}};\nCreateSmoke(80,96);"));
        for (int frame = 0; frame < 26; ++frame) {
            CHECK(execute_source(vm, root + 2, "Update();"));
            CHECK(function_48aa20(vm) == top);
        }
        CHECK(execute_source(vm, root + 2,
            "if(smokeDraws!=25 || effectList.len()!=0) throw \"smoke lifecycle\";"));
    }
    CHECK(vm_failures == 0);
    puts("PASS: effect yields/results, caller stack, completion, array removal and ownership");
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--gc-link-probe") == 0)
        return test_gc_mark_link();
    if (argc == 2 && strcmp(argv[1], "--sound-module") == 0) {
        HMODULE module=LoadLibraryA("dsound.dll");
        char path[MAX_PATH];
        GetModuleFileNameA(module,path,MAX_PATH);
        printf("module=%s base=%p preferred=%08lx\n",path,module,
            ((IMAGE_NT_HEADERS *)((char *)module+((IMAGE_DOS_HEADER *)module)->e_lfanew))->OptionalHeader.ImageBase);
        void *ds=NULL, *buffer=NULL;
        retdec_direct_sound_create8_fn create=(retdec_direct_sound_create8_fn)GetProcAddress(module,"DirectSoundCreate8");
        CHECK(SUCCEEDED(create(NULL,&ds,NULL)));
        void **vt=*(void ***)ds;
        CHECK(SUCCEEDED(((retdec_dsound_set_cooperative_level_fn)vt[6])(ds,GetDesktopWindow(),1)));
        retdec_wave_format format={1,1,22050,44100,2,16,0};
        retdec_dsound_buffer_desc description={0};
        description.dwSize=sizeof(description); description.dwFlags=0x18088;
        description.dwBufferBytes=4096; description.lpwfxFormat=&format;
        CHECK(SUCCEEDED(((retdec_dsound_create_buffer_fn)vt[3])(ds,&description,&buffer,NULL)));
        printf("buffer=%p vtable=%p play=%p\n",buffer,*(void ***)buffer,(*(void ***)buffer)[12]);
        retdec_release_dsound_buffer(buffer);
        ((retdec_dsound_release_fn)vt[2])(ds);
        return 0;
    }
    AddVectoredExceptionHandler(1, contract_exception);
    CHECK(test_map_camera_fpu() == 0);
    CHECK(test_sprite_geometry() == 0);
    CHECK(test_actor_state_fields() == 0);
    CHECK(test_animation_timing() == 0);
    CHECK(test_shutdown_tree_cleanup() == 0);
    CHECK(test_global_stage_cleanup() == 0);
    CHECK(test_global_sound_cleanup() == 0);
    CHECK(test_gc_mark_link() == 0);
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
    CHECK(test_error_value_ownership(vm) == 0);
    CHECK(test_act_resource_methods() == 0);
    CHECK(retdec_sqrat_root_construct(PTR(root), vm));
    CHECK(test_global_script_cleanup(vm, root) == 0);
    if (argc == 3 && strcmp(argv[1], "--road-probe") == 0)
        return test_generator_effects(vm, root, argv[2]);
    if (argc == 3 && strcmp(argv[1], "--enemy-reentry") == 0) {
        char path[MAX_PATH];
        for(char archive='a';archive<='c';++archive) {
            sprintf_s(path,sizeof(path),"%s/6kinoko_%c.dat",argv[2],archive);
            CHECK(function_410500(path));
        }
        CHECK(retdec_construct_actor_manager(manager));
        function_460e00();
        CHECK(execute_source(vm,root+2,"Actor.funcUpdate <- null;"));
        int32_t compile_target=PTR(function_471b30);
        CHECK(function_415550_this(PTR(root),PTR("CompileFile"),PTR(&compile_target),
            4,PTR(retdec_compile_file_native),0)>=0);
        g874=1;
        CHECK(execute_asset(vm,root+2,"data/script/constant.cv4"));
        return test_enemy_reentry(manager,vm,root);
    }
    CHECK(test_generator_effects(vm, root, NULL) == 0);
    CHECK(test_gc_repeated_collection(vm, root) == 0);
    CHECK(test_script_callback_binding(vm, root) == 0);
    {
        int32_t anonymous[3];
        CHECK(execute_source(vm,root+2,"anonymousError <- function() { throw 17; };"));
        function_4aa3a0_this(PTR(root+1),PTR(anonymous),"anonymousError");
        int32_t proto=*(int32_t *)(intptr_t)(anonymous[2]+36);
        CHECK(*(int32_t *)(intptr_t)(proto+20)==g483);
        *(int32_t *)(intptr_t)(proto+24)=0;
        expected_vm_error=1;
        CHECK(!execute_source(vm,root+2,"anonymousError();"));
        expected_vm_error=0;
        function_4a9d70_this(PTR(anonymous));
        puts("PASS: anonymous script failure diagnostics and unwind");
    }
    CHECK(test_native_stack_relocation(vm, root) == 0);
    function_48ab90(vm, root[2], root[3]);
    CHECK(function_4c6c20(vm) == 0);
    function_48aa50(vm);
    CHECK(execute_source(vm, root + 2,
        "if (sqrt(10000.0) != 100.0 || sqrt(25) != 5.0) throw \"sqrt distance\";\n"
        "if (floor(-1.25) != -2.0 || ceil(-1.25) != -1.0) throw \"rounding\";\n"
        "if (fabs(asin(0.5)-PI/6)>0.00001 || fabs(acos(0.5)-PI/3)>0.00001) throw \"inverse trig\";\n"
        "if (fabs(tan(PI/4)-1.0)>0.00001 || fabs(atan(1.0)-PI/4)>0.00001) throw \"trig\";\n"
        "if (fabs(atan2(1.0,-1.0)-PI*0.75)>0.00001 || pow(2.0,3.0)!=8.0) throw \"binary math\";\n"
        "if (fabs(log(exp(2.0))-2.0)>0.00001 || fabs(log10(100.0)-2.0)>0.00001) throw \"log math\";\n"
        "if (typeof sqrt(25) != \"float\" || typeof floor(2.5) != \"float\") throw \"math type\";"));
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
    CHECK(test_delegate_lifetime(vm, root)==0);
    /* Declare the isolated fixture's script-managed callback slot before creating instances. */
    CHECK(execute_source(vm, root + 2, "Actor.funcUpdate <- null;"));
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
    CHECK(test_actor_animation_sync(vm, root, actors[0], actors[1]) == 0);
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
        int32_t map_state = PTR(g_retdec_map_manager_state);
        int32_t query_actor = function_463b40_this(manager, PTR(&g16), g483, g484,
            110, 210, -1, PTR(&g16), g483, g484, 0);
        CHECK(query_actor);
        function_4a9840_this(PTR(root + 1), "eventProbe", query_actor + 44);
        layout[60] = 32;
        layout[61] = 48;
        CHECK(*(int32_t *)(intptr_t)(map_state + 40) -
            *(int32_t *)(intptr_t)(map_state + 36) == 4);
        CHECK(execute_source(vm, root + 2, "eventProbe.GetChipID(0);"));
        CHECK(*(int32_t *)(intptr_t)(map_state + 56) == 0x443);
        CHECK(*(float *)(intptr_t)(map_state + 60) == 100);
        CHECK(*(float *)(intptr_t)(map_state + 72) == 248);
        CHECK(execute_source(vm, root + 2,
            "CreateEvent(\"en\",null,null);\neventProbe.x=300; eventProbe.y=400;\n"
            "eventProbe.GetChipID(1);"));
        CHECK(*(int32_t *)(intptr_t)(map_state + 56) == 0xc8a);
        CHECK(*(float *)(intptr_t)(map_state + 68) == 331);
        CHECK(execute_source(vm, root + 2,
            "CreateEvent(\"absent\",null,null);\neventProbe.GetChipID(2);"));
        CHECK(*(int32_t *)(intptr_t)(map_state + 56) == -1);
        CHECK(execute_source(vm, root + 2, "eventProbe.x=331; eventProbe.GetChipID(0);"));
        CHECK(*(int32_t *)(intptr_t)(map_state + 56) == -1);
        CHECK(execute_source(vm, root + 2, "eventProbe.Release();"));
        CHECK(retdec_actor_manager_refresh(manager) == 5);
    }
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
        CHECK(execute_source(vm, root + 2,
            "hits.clear();\nprobe[1].InterrputCollisionCallback();\n"
            "if (hits.len()!=4 || hits[0]!=21 || hits[1]!=12 || hits[2]!=23 || hits[3]!=32) "
            "throw \"immediate collision callback order\";"));
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
    {
        int32_t actor, animation[7] = {0};
        function_469700();
        CHECK(function_468950_this(PTR(g_514300_storage), manager));
        layout[66] = PTR(records);
        layout[67] = PTR(records + 1);
        layout[60] = 256;
        layout[61] = 32;
        *(uint8_t *)((char *)layer + 140) = 1;
        records[0][0] = 0x443;
        records[0][1] = 0;
        records[0][2] = 100;
        *(int16_t *)(chips[0].bytes + 12) = 256;
        *(int16_t *)(chips[0].bytes + 14) = 32;
        *(int16_t *)(chips[0].bytes + 34) = 0;
        *(uint32_t *)(chips[0].bytes + 16) = 0;
        CHECK(function_4693a0(PTR(layout)));
        actor = function_463b40_this(manager, PTR(&g16), g483, g484,
            50, 40, -1, PTR(&g16), g483, g484, 0);
        CHECK(actor);
        *(int32_t *)(intptr_t)(actor + 316) = 1;
        *(int32_t *)(intptr_t)(actor + 200) = PTR(animation);
        *((uint8_t *)animation + 25) = 1;
        *(float *)(intptr_t)(actor + 424) = -8;
        *(float *)(intptr_t)(actor + 428) = -16;
        *(float *)(intptr_t)(actor + 432) = 8;
        *(float *)(intptr_t)(actor + 436) = 0;
        position_actor(actor, 50, 40);
        *(float *)(intptr_t)(actor + 256) = 2;
        *(float *)(intptr_t)(actor + 260) = 5;
        retdec_actor_manager_refresh(manager);
        function_468620_this(PTR(g_514300_storage));
        retdec_actor_update_motion(actor);
        CHECK(*(float *)(intptr_t)(actor + 240) == 52);
        CHECK(*(float *)(intptr_t)(actor + 244) == 45);
        for (int i = 0; i < 12; ++i) {
            function_468620_this(PTR(g_514300_storage));
            retdec_actor_update_motion(actor);
        }
        CHECK(*(float *)(intptr_t)(actor + 244) == 100);
        CHECK(*(int32_t *)(intptr_t)(actor + 296) == 1);
        CHECK(*(int32_t *)(intptr_t)(actor + 36) != 0);
        *(float *)(intptr_t)(actor + 260) = -6;
        retdec_actor_update_motion(actor);
        CHECK(*(float *)(intptr_t)(actor + 244) == 94);
        CHECK(*(int32_t *)(intptr_t)(actor + 296) == 0);
        CHECK(*(int32_t *)(intptr_t)(actor + 36) == 0);
        records[0][1] = 110;
        records[0][2] = 0;
        *(int16_t *)(chips[0].bytes + 12) = 32;
        *(int16_t *)(chips[0].bytes + 14) = 100;
        layout[61] = 100;
        position_actor(actor, 94, 80);
        *(float *)(intptr_t)(actor + 256) = 12;
        *(float *)(intptr_t)(actor + 260) = 0;
        retdec_actor_update_motion(actor);
        CHECK(*(float *)(intptr_t)(actor + 240) == 102);
        CHECK(*(int32_t *)(intptr_t)(actor + 292) == 1);
        function_45dbd0_this(actor, 40, 0);
        CHECK(*(float *)(intptr_t)(actor + 240) == 102);
        CHECK(*(float *)(intptr_t)(actor + 248) == 102);
        CHECK(*(int32_t *)(intptr_t)(actor + 292) == 1);
        records[0][1] = 0;
        records[0][2] = 20;
        *(int16_t *)(chips[0].bytes + 12) = 256;
        *(int16_t *)(chips[0].bytes + 14) = 32;
        position_actor(actor, 50, 80);
        *(float *)(intptr_t)(actor + 256) = 0;
        *(float *)(intptr_t)(actor + 260) = -18;
        retdec_actor_update_motion(actor);
        CHECK(*(float *)(intptr_t)(actor + 244) == 68);
        CHECK(*(int32_t *)(intptr_t)(actor + 288) != 0);
        *(uint32_t *)(chips[0].bytes + 16) = 0x20;
        position_actor(actor, 50, 80);
        retdec_actor_update_motion(actor);
        CHECK(*(float *)(intptr_t)(actor + 244) == 62);
        CHECK(*(int32_t *)(intptr_t)(actor + 288) == 0);
        records[0][2] = 16;
        *(uint32_t *)(chips[0].bytes + 16) = 0;
        *(int16_t *)(chips[0].bytes + 12) = 32;
        *(int16_t *)(chips[0].bytes + 34) = 1;
        position_actor(actor, 16, 40);
        *(float *)(intptr_t)(actor + 260) = 0;
        retdec_actor_update_motion(actor);
        CHECK(*(float *)(intptr_t)(actor + 244) == 32);
        CHECK(*(float *)(intptr_t)(actor + 276) == -1);
        CHECK(*(int32_t *)(intptr_t)(actor + 296) == 1);
        *(int16_t *)(chips[0].bytes + 34) = 2;
        position_actor(actor, 16, 40);
        retdec_actor_update_motion(actor);
        CHECK(*(float *)(intptr_t)(actor + 244) == 32);
        CHECK(*(float *)(intptr_t)(actor + 276) == 1);
        *(uint8_t *)((char *)animation + 25) = 0;
        position_actor(actor, 16, 40);
        *(float *)(intptr_t)(actor + 260) = 5;
        retdec_actor_update_motion(actor);
        CHECK(*(float *)(intptr_t)(actor + 244) == 45);
        CHECK(*(int32_t *)(intptr_t)(actor + 296) == 0);
        {
            int32_t count = 0;
            KinokoCollisionRecord *found;
            layout[67] = PTR(records + 3);
            layout[60] = 32;
            layout[61] = 32;
            for (int i = 0; i < 3; ++i) {
                records[i][0] = 0x443;
                records[i][1] = i * 64;
                records[i][2] = 100;
            }
            position_actor(actor, 96, 100);
            *(int32_t *)(intptr_t)(actor + 480) = 1;
            CHECK(retdec_collision_query_map(PTR(layout), actor, 0, &count));
            found = (KinokoCollisionRecord *)(intptr_t)g_514300_storage[9];
            CHECK(count == 2 && found[0].index == 1 && found[1].index == 2);
            count = 0;
            position_actor(actor, 16, 100);
            CHECK(retdec_collision_query_map(PTR(layout), actor, 0, &count));
            found = (KinokoCollisionRecord *)(intptr_t)g_514300_storage[9];
            CHECK(count == 1 && found[0].index == 0);
            count = 0;
            position_actor(actor, 144, 100);
            CHECK(retdec_collision_query_map(PTR(layout), actor, 0, &count));
            found = (KinokoCollisionRecord *)(intptr_t)g_514300_storage[9];
            CHECK(count == 1 && found[0].index == 2);
        }
        function_469700();
        function_468950_this(PTR(g_514300_storage), manager);
    }
    if (argc > 2) {
        CHECK(execute_file(vm, root + 2, argv[2]));
        target = PTR(function_469a20);
        CHECK(function_415550_this(PTR(root), PTR("SetGlobalUpdateFunction"),
            PTR(&target), 4, PTR(function_471c70), 0) >= 0);
        CHECK(execute_source(vm, root + 2,
            "fadeCalls <- [];\n"
            "Fader1 <- { FadeOut = function(a,b,c,d) { ::fadeCalls.append(0); }, "
            "FadeIn = function(a,b,c,d) { ::fadeCalls.append(1); } };\n"
            "StageStart <- { pl = { visible = true } };\n"
            "updateMask <- 0x40000000;\nupdateMaskPause <- -1;\n"
            "function UpdateGlobal() {}\n"
            "stageChangeCount = 120;\nSetGlobalUpdateFunction(UpdateStageStart);\n"));
        for (int i = 0; i < 121; ++i)
            CHECK(retdec_actor_step_callback(PTR(g612)) >= 0);
        CHECK(execute_source(vm, root + 2,
            "if (stageChangeCount != -1 || StageStart.pl.visible || updateMask != -1 || "
            "fadeCalls.len() != 2) throw \"stage start countdown stalled\";"));
        for (int i = 0; i < 16; ++i)
            CHECK(retdec_actor_step_callback(PTR(g612)) >= 0);
        CHECK(execute_source(vm, root + 2,
            "if (stageChangeCount != -1 || fadeCalls.len() != 2) "
            "throw \"retired stage callback still running\";"));
        CHECK(function_48aa20(vm) == top);
        {
            int32_t player_pair[2] = {g483, g484};
            CHECK(retdec_publish_acting_player_class(vm, PTR(root)));
            act_resource[33] = PTR(act + 24);
            act[24] = 1;
            CHECK(retdec_publish_acting_player(vm, root + 2, "nativeStagePlayer",
                PTR(act_resource), player_pair));
            CHECK(execute_source(vm, root + 2,
                "StageStart.pl = nativeStagePlayer;\nfadeCalls.clear();\n"
                "stageChangeCount = 120;\nSetGlobalUpdateFunction(UpdateStageStart);\n"));
            for (int i = 0; i < 121; ++i)
                CHECK(retdec_actor_step_callback(PTR(g612)) >= 0);
            CHECK(*(uint8_t *)(act + 24) == 0);
            CHECK(function_48aa20(vm) == top);
            retdec_sqrat_release_pair(vm, player_pair);
        }
    }
    {
        int32_t draw_vtable[9] = {0}, fake_layout[2] = {0};
        draw_vtable[8] = PTR(count_draw);
        fake_layout[0] = PTR(draw_vtable);
        layout_key[1] = PTR(fake_layout);
        act_resource[3] = PTR(act);
        act[24] = 0;
        InitializeCriticalSection((struct retdec_RTL_CRITICAL_SECTION *)(act_resource + 5));
        CHECK(function_4525d0(PTR(act_resource), 0, 0) == 0);
        CHECK(draw_count == 0);
        act[24] = 1;
        CHECK(function_4525d0(PTR(act_resource), 0, 0) == 0);
        CHECK(draw_count == 1);
        DeleteCriticalSection((struct retdec_RTL_CRITICAL_SECTION *)(act_resource + 5));
    }
    {
        int32_t animation[14] = {0};
        int32_t frames[62] = {0};
        int32_t actor = function_463b40_this(manager, PTR(&g16), g483, g484,
            100, 200, -1, PTR(&g16), g483, g484, 0);
        CHECK(actor);
        animation[2] = PTR(frames);
        animation[3] = PTR(frames + 62);
        animation[7] = -6;
        animation[8] = -20;
        animation[9] = 10;
        animation[10] = -1;
        animation[11] = 7;
        *((uint8_t *)animation + 25) = 1;
        CHECK(retdec_pat_tree_put(manager, 0x60000001, PTR(animation)));
        for (int i = 0; i < 8; ++i)
            __frontend_reg_store_fpr(i, 700.0L + i);
        CHECK(retdec_call_thiscall1_result((void *)(intptr_t)actor,
            kinoko_actor_set_take_method, 0x60000001) == PTR(frames));
        CHECK(*(float *)(intptr_t)(actor + 424) == -6.5f);
        CHECK(*(float *)(intptr_t)(actor + 428) == -20.0f);
        CHECK(*(float *)(intptr_t)(actor + 432) == 10.5f);
        CHECK(*(float *)(intptr_t)(actor + 436) == 0.0f);
        CHECK(*(float *)(intptr_t)(actor + 440) == 93.5f);
        CHECK(*(float *)(intptr_t)(actor + 444) == 180.0f);
        CHECK(*(float *)(intptr_t)(actor + 448) == 110.5f);
        CHECK(*(float *)(intptr_t)(actor + 452) == 200.0f);
        CHECK(*(int16_t *)(intptr_t)(actor + 388) == 17);
        CHECK(*(int16_t *)(intptr_t)(actor + 390) == 20);
        CHECK(*(int32_t *)(intptr_t)(actor + 204) == PTR(frames));
        CHECK(*(int32_t *)(intptr_t)(actor + 220) == 7);
        *(float *)(intptr_t)(actor + 168) = 2.0f;
        *(float *)(intptr_t)(actor + 172) = 1.5f;
        *(float *)(intptr_t)(actor + 176) = 0.5f;
        *(float *)(intptr_t)(actor + 272) = 1.0f;
        function_462280_this(actor, 0x60000001);
        CHECK(*(float *)(intptr_t)(actor + 440) == 68.5f);
        CHECK(*(float *)(intptr_t)(actor + 448) == 119.5f);
        CHECK(*(int16_t *)(intptr_t)(actor + 388) == 51);
        function_462280_this(actor, 0x60000002);
        CHECK(*(int32_t *)(intptr_t)(actor + 200) == PTR(animation));
        CHECK(*(float *)(intptr_t)(actor + 440) == 68.5f);
        CHECK(*(int32_t *)(intptr_t)(actor + 208) == 0x60000002);
        CHECK(*(int32_t *)(intptr_t)(actor + 212) == 0);
        CHECK(*(int32_t *)(intptr_t)(actor + 216) == 0);
        *((uint8_t *)animation + 25) = 0;
        function_4a9840_this(PTR(root + 1), "animationTakeProbe", actor + 44);
        CHECK(execute_source(vm, root + 2,
            "animationTakeProbe.SetTake(0x60000001); delete ::animationTakeProbe;"));
        for (int offset = 424; offset < 440; offset += 4)
            CHECK(*(float *)(intptr_t)(actor + offset) == 0);
        CHECK(*(float *)(intptr_t)(actor + 352) == 0);
        CHECK(*(float *)(intptr_t)(actor + 356) == 0);
        CHECK(*(float *)(intptr_t)(actor + 440) == 100);
        CHECK(*(float *)(intptr_t)(actor + 444) == 200);
        CHECK(*(float *)(intptr_t)(actor + 448) == 100);
        CHECK(*(float *)(intptr_t)(actor + 452) == 200);
        CHECK(*(int32_t *)(intptr_t)(actor + 388) == 0);
        function_469700();
    }
    CHECK(test_pat_records(manager) == 0);
    CHECK(test_actor_step(manager, vm, root) == 0);
    CHECK(test_actor_reset(manager, vm, root) == 0);
    {
        int32_t actor = function_463b40_this(manager, PTR(&g16), g483, g484,
            100, 200, -1, PTR(&g16), g483, g484, 0);
        float dx = -40.0f, dy = 0.0f;
        int32_t dx_bits, dy_bits;
        int fault = 0;
        CHECK(actor);
        memcpy(&dx_bits, &dx, sizeof(dx_bits));
        memcpy(&dy_bits, &dy, sizeof(dy_bits));
        __try {
            retdec_call_thiscall2_result((void *)(intptr_t)actor, function_45dbd0, dx_bits, dy_bits);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fault = 1;
        }
        CHECK(fault == 0);
        CHECK(*(float *)(intptr_t)(actor + 240) == 60);
        CHECK(*(float *)(intptr_t)(actor + 244) == 200);
        __try {
            retdec_call_thiscall0_result((void *)(intptr_t)actor, function_45eb00);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fault = 1;
        }
        CHECK(fault == 0);
        CHECK(*(float *)(intptr_t)(actor + 240) == 100);
        CHECK(*(float *)(intptr_t)(actor + 244) == 200);
        {
            int32_t animation[7] = {0};
            unsigned char unchanged[544];
            *(int32_t *)(intptr_t)(actor + 200) = PTR(animation);
            *((uint8_t *)animation + 25) = 1;
            *(int32_t *)(intptr_t)(actor + 316) = 1;
            *(float *)(intptr_t)(actor + 424) = -8;
            *(float *)(intptr_t)(actor + 428) = -16;
            *(float *)(intptr_t)(actor + 432) = 8;
            *(float *)(intptr_t)(actor + 436) = 0;
            *(float *)(intptr_t)(actor + 256) = 12;
            *(float *)(intptr_t)(actor + 260) = -4;
            *(float *)(intptr_t)(actor + 264) = 7;
            *(float *)(intptr_t)(actor + 268) = 9;
            retdec_actor_refresh_bounds(actor);
            function_45dbd0_this(actor, 21, -18);
            CHECK(*(float *)(intptr_t)(actor + 240) == 121);
            CHECK(*(float *)(intptr_t)(actor + 244) == 182);
            CHECK(*(float *)(intptr_t)(actor + 248) == 116);
            CHECK(*(float *)(intptr_t)(actor + 252) == 184);
            CHECK(*(float *)(intptr_t)(actor + 256) == 12);
            CHECK(*(float *)(intptr_t)(actor + 260) == -4);
            CHECK(*(float *)(intptr_t)(actor + 264) == 7);
            CHECK(*(float *)(intptr_t)(actor + 268) == 9);
            memcpy(unchanged, (const void *)(intptr_t)actor, sizeof(unchanged));
            function_45dbd0_this(actor, 0, 0);
            CHECK(memcmp(unchanged, (const void *)(intptr_t)actor, sizeof(unchanged)) == 0);
            *((uint8_t *)animation + 25) = 0;
            function_45dbd0_this(actor, -40, 0);
            CHECK(*(float *)(intptr_t)(actor + 240) == 81);
            CHECK(*(float *)(intptr_t)(actor + 248) == 121);
        }
        function_469700();
    }
    {
        int32_t constructed[136];
        function_45e300_this(PTR(constructed));
        CHECK(constructed[27] == PTR(&g16));
        CHECK(constructed[28] == g483 && constructed[29] == g484);
    }
    {
        int32_t actor = function_463b40_this(manager, PTR(&g16), g483, g484,
            100, 200, -1, PTR(&g16), g483, g484, 0);
        int fault = 0;
        CHECK(actor);
        __try {
            retdec_call_thiscall1((void *)(intptr_t)actor, kinoko_actor_set_chip_flags, 0x20);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fault = 1;
        }
        CHECK(fault == 0);
        CHECK(*(int64_t *)(intptr_t)(actor + 392) == 0x20);
        retdec_call_thiscall1((void *)(intptr_t)actor, kinoko_actor_set_chip_flags, -1);
        CHECK(*(int64_t *)(intptr_t)(actor + 392) == -1);
        retdec_call_thiscall1((void *)(intptr_t)actor, kinoko_actor_set_chip_bound_type, 2);
        CHECK(*(int16_t *)(intptr_t)(actor + 410) == 2);
        retdec_call_thiscall1((void *)(intptr_t)actor, kinoko_actor_set_chip_flags, 0);
        {
            int32_t query_layout[100] = {0}, query_layer[80] = {0}, query_resource[24] = {0};
            int32_t query_records[3][8] = {{1,80,180}, {2,120,180}, {3,136,180}};
            struct retdec_mcd_chip query_chips[3] = {{0}};
            struct retdec_mcd_data query_data = {3, query_chips, 0, NULL};
            query_layout[0] = PTR(&g327);
            query_layout[60] = 16;
            query_layout[61] = 32;
            query_layout[66] = PTR(query_records);
            query_layout[67] = PTR(query_records + 3);
            query_layout[78] = PTR(query_layer);
            query_layout[79] = PTR(query_resource);
            *((uint8_t *)query_layer + 140) = 1;
            query_resource[16] = PTR(&query_data);
            for (int i = 0; i < 3; ++i) {
                query_chips[i].chip_id = i + 1;
                *(int16_t *)(query_chips[i].bytes + 12) = 16;
                *(int16_t *)(query_chips[i].bytes + 14) = 32;
                *(uint32_t *)(query_chips[i].bytes + 16) = 1u << i;
            }
            CHECK(function_468950_this(PTR(g_514300_storage), manager));
            CHECK(function_4693a0(PTR(query_layout)));
            function_4a9840_this(PTR(root + 1), "queryProbe", actor + 44);
            CHECK(execute_source(vm, root + 2,
                "for (local i=0;i<64;i++) {\n"
                "  if (!queryProbe.IsExistChip(80.0,180.0,96.0,212.0)) throw \"missing region\";\n"
                "  if (queryProbe.IsExistChip(300.0,180.0,320.0,212.0)) throw \"empty region\";\n"
                "}"));
            CHECK(function_48aa20(vm) == top);
            position_actor(actor, 100, 200);
            *(uint32_t *)(intptr_t)(actor + 472) = 0x8000;
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, kinoko_actor_get_chip_flags) == 1);
            CHECK(*(uint32_t *)(intptr_t)(actor + 472) == 0x8000);
            position_actor(actor, 128, 200);
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, kinoko_actor_get_chip_flags) == 6);
            position_actor(actor, 100, 200);
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, kinoko_actor_get_chip_flags) == 1);
            *((uint8_t *)query_layer + 140) = 0;
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, kinoko_actor_get_chip_flags) == 0);
            CHECK(execute_source(vm, root + 2,
                "if (queryProbe.IsExistChip(80.0,180.0,96.0,212.0)) throw \"disabled region\";"));
            *((uint8_t *)query_layer + 140) = 1;
            position_actor(actor, 200, 200);
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, kinoko_actor_get_chip_flags) == 0);
            *(float *)((char *)query_layer + 144) = 32.5f;
            position_actor(actor, 132, 200);
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, kinoko_actor_get_chip_flags) == 1);
            CHECK(function_468950_this(PTR(g_514300_storage), manager));
            {
                int32_t other = function_463b40_this(manager, PTR(&g16), g483, g484,
                    200, 300, -1, PTR(&g16), g483, g484, 0);
                int32_t candidates[2] = {actor, other};
                int32_t saved_begin = g_514300_storage[17], saved_count = g_514300_storage[21];
                CHECK(other);
                position_actor(actor, 100, 200);
                position_actor(other, 200, 300);
                g_514300_storage[17] = PTR(candidates);
                g_514300_storage[21] = 2;
                CHECK(execute_source(vm, root + 2,
                    "if (queryProbe.IsExistChip(92.0,184.0,108.0,200.0)) throw \"self region\";\n"
                    "if (!queryProbe.IsExistChip(208.0,300.0,220.0,320.0)) throw \"touching actor region\";\n"
                    "if (queryProbe.IsExistChip(208.5,300.0,220.0,320.0)) throw \"disjoint actor region\";"));
                g_514300_storage[17] = saved_begin;
                g_514300_storage[21] = saved_count;
                position_actor(actor, 132, 200);
            }
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, kinoko_actor_get_chip_flags) == 0);
            CHECK(function_4693a0(PTR(query_layout)));
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, kinoko_actor_get_chip_flags) == 1);
            CHECK(function_468950_this(PTR(g_514300_storage), manager));
        }
        function_469700();
    }
    {
        int32_t camera[128] = {0}, callback[3];
        CHECK(execute_source(vm, root + 2,
            "cameraProbeCount <- 0;\n"
            "function CameraProbeUpdate() { ::cameraProbeCount++; }"));
        function_4a9500_this(camera, PTR(root + 1));
        function_4aa3a0_this(PTR(root + 1), PTR(callback), "CameraProbeUpdate");
        retdec_call_thiscall3_result(camera, kinoko_camera_set_update_callback,
            callback[0], callback[1], callback[2]);
        int32_t camera_top = function_48aa20(vm);
        retdec_call_thiscall0_result(camera, kinoko_camera_update);
        CHECK(function_48aa20(vm) == camera_top);
        CHECK(execute_source(vm, root + 2,
            "if (cameraProbeCount != 1) throw \"camera update callback skipped\";"));
        function_4a9d70_this(PTR(camera + 7));
        CHECK(kinoko_camera_update(PTR(camera), NULL) == g483);
        CHECK(execute_source(vm, root + 2,
            "if (cameraProbeCount != 1) throw \"empty camera callback executed\";"));
        function_4a9d70_this(PTR(camera + 4));
        function_4a9d70_this(PTR(camera));
    }
    if (argc > 4)
        CHECK(test_player_pat(manager, argv[3], strtoul(argv[4], NULL, 0)) == 0);
    if (argc > 7)
        CHECK(test_player_walking(manager, vm, root, argv[5], argv[6], argv[7],
            argc > 8 ? argv[8] : NULL,
            argc > 9 ? argv[9] : "data/map/w1-c01a.act") == 0);
    if (argc > 8)
        CHECK(test_stone_placement(manager, vm, root) == 0);
    if (argc > 8)
        CHECK(test_floating_items(manager, vm, root) == 0);
    if (argc > 8)
        CHECK(test_hidden_layer(vm, root) == 0);
    if (argc > 8)
        CHECK(test_player_form_exit(manager, vm, root) == 0);
    if (argc > 8)
        CHECK(test_enemy_scripts(manager, vm, root) == 0);
    if (argc > 8)
        CHECK(test_stone_block(manager, vm, root) == 0);
    if (argc > 8)
        CHECK(test_star_landing(manager, vm, root) == 0);
    if (argc > 8)
        CHECK(test_branch_motion(manager) == 0);
    if (argc > 8)
        CHECK(test_map_transition(vm, root) == 0);
    CHECK(test_vm_error_unwind(vm, root) == 0);
    CHECK(test_array_sort(vm, root) == 0);
    CHECK(test_standard_error_handler(vm, root) == 0);
    CHECK(vm_failures == 0);
    CHECK(function_464e20(manager) == *(int32_t *)(intptr_t)(manager + 100));
    CHECK(function_464e20(manager) == *(int32_t *)(intptr_t)(manager + 100));
    puts("PASS: stage lifecycle, terrain motion, start visibility and animation loading/bounds");
    return 0;
}
