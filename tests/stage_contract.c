/* Exercise the actual reconstructed functions without WinMain, graphics or DAT startup. */
#include "../src/decompiled/6kinoko_rebuilt.c"

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)
#define PTR(value) ((int32_t)(intptr_t)(value))

static int draw_count;
static int vm_failures;
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
    if (message && strstr(message, "stagevm:failure-error")) ++vm_failures;
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

struct pat_fixture {
    unsigned char bytes[4096];
    uint32_t size;
};

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
            "oldReset <- resetProbe; resetProbe.x = 700; resetProbe.y = 800;\n"
            "resetProbe.direction = 1; resetProbe.priority = 123;\n"
            "resetProbe.SetChipFlag(32); resetProbe.Reset();\n"
            "if (resetProbe == oldReset || oldReset.user != null || oldReset.step != null) "
            "throw \"old reset instance retained state\";\n"
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
        "if (resetCalls != 33 || resetTicks != 32) throw \"reset callback counts\";"));
    function_469700();
    function_4a9d70_this(PTR(init));
    function_4a9d70_this(PTR(seed));
    puts("PASS: Reset replays initialization and retires old references across 32 resets");
    return 0;
}

static int test_stone_placement(int32_t manager, int32_t vm, int32_t *root) {
    int32_t reader = 0, script[26] = {0}, scripts[3], init[3];
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
        "player <- { direction = 1.0, user = { stone = null } };\n"
        "function PlaySE(id) { ::stoneSounds.append(id); }\n"
        "function CreateEffect(x,y,z,id) { ::stoneEffects.append(id); return {}; }"));
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
            "stoneProbe = null;"));
        CHECK(function_48aa20(vm) == top);
        function_469700();
        CHECK(execute_source(vm, root + 2,
            "if (player.user.stone != null) throw \"stone weak reference retained\";"));
    }
    CHECK(execute_source(vm, root + 2,
        "if (stoneSounds.len() != 8 || stoneEffects.len() != 8) throw \"stone media calls\";\n"
        "foreach (id in stoneSounds) if (id != 33) throw \"stone sound id\";\n"
        "foreach (id in stoneEffects) if (id != 1960) throw \"stone effect id\";"));
    function_4a9d70_this(PTR(init));
    function_4a9d70_this(PTR(scripts));
    puts("PASS: original InitStone and item PAT, both directions, eight placements/releases");
    return 0;
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
        function_462280_this(actor, 0x60000001);
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
        function_469700();
    }
    CHECK(test_pat_records(manager) == 0);
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
            retdec_call_thiscall1((void *)(intptr_t)actor, function_45f760, 0x20);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fault = 1;
        }
        CHECK(fault == 0);
        CHECK(*(int64_t *)(intptr_t)(actor + 392) == 0x20);
        retdec_call_thiscall1((void *)(intptr_t)actor, function_45f760, -1);
        CHECK(*(int64_t *)(intptr_t)(actor + 392) == -1);
        retdec_call_thiscall1((void *)(intptr_t)actor, function_45f780, 2);
        CHECK(*(int16_t *)(intptr_t)(actor + 410) == 2);
        retdec_call_thiscall1((void *)(intptr_t)actor, function_45f760, 0);
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
            position_actor(actor, 100, 200);
            *(uint32_t *)(intptr_t)(actor + 472) = 0x8000;
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, function_45f810) == 1);
            CHECK(*(uint32_t *)(intptr_t)(actor + 472) == 0x8000);
            position_actor(actor, 128, 200);
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, function_45f810) == 6);
            position_actor(actor, 100, 200);
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, function_45f810) == 1);
            *((uint8_t *)query_layer + 140) = 0;
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, function_45f810) == 0);
            *((uint8_t *)query_layer + 140) = 1;
            position_actor(actor, 200, 200);
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, function_45f810) == 0);
            *(float *)((char *)query_layer + 144) = 32.5f;
            position_actor(actor, 132, 200);
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, function_45f810) == 1);
            CHECK(function_468950_this(PTR(g_514300_storage), manager));
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, function_45f810) == 0);
            CHECK(function_4693a0(PTR(query_layout)));
            CHECK(retdec_call_thiscall0_result((void *)(intptr_t)actor, function_45f810) == 1);
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
        function_4663c0_this(PTR(camera), callback[0], callback[1], callback[2]);
        int32_t camera_top = function_48aa20(vm);
        retdec_call_thiscall0_result(camera, function_466470);
        CHECK(function_48aa20(vm) == camera_top);
        CHECK(execute_source(vm, root + 2,
            "if (cameraProbeCount != 1) throw \"camera update callback skipped\";"));
        function_4a9d70_this(PTR(camera + 7));
        CHECK(function_466470_this(PTR(camera)) == g483);
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
    CHECK(vm_failures == 0);
    puts("PASS: stage lifecycle, terrain motion, start visibility and animation loading/bounds");
    return 0;
}
