#include "kinoko/graphics_device.h"
#include "kinoko/native_buffer.h"
#include "kinoko/stage_cleanup.h"
#include "kinoko/act_array.h"
#include "kinoko/act_list.h"
#include "kinoko/string_font.h"
#include "kinoko/input_devices.h"
#include "kinoko/input_keys.h"
/* Exercise the actual reconstructed functions without WinMain, graphics or DAT startup. */
#include "../src/decompiled/6kinoko_rebuilt.c"
#include "stage_audio_contract.h"
#include "kinoko/squirrel_vm_bootstrap.h"
#include "kinoko/boost_hash.h"

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)
#define PTR(value) ((int32_t)(intptr_t)(value))

/* Fixture-only reader/legacy-address conveniences; production uses typed APIs. */
static int32_t fixture_pat_read_u8(int32_t reader, unsigned char *value)
{
    return retdec_reader_read_exact(reader, value, 1);
}

static int32_t fixture_pat_read_u16(int32_t reader, unsigned short *value)
{
    return retdec_reader_read_exact(reader, value, 2);
}

static int32_t fixture_pat_skip_bytes(int32_t reader, uint32_t size)
{
    unsigned char buffer[256];

    while (size != 0) {
        uint32_t chunk = size < sizeof(buffer) ? size : sizeof(buffer);
        if (!retdec_reader_read_exact(reader, buffer, chunk))
            return 0;
        size -= chunk;
    }
    return 1;
}

static int32_t fixture_pat_read_animations(int32_t reader, int32_t manager, uint32_t resource_base) {
    return kinoko_pat_read_animations((KinokoArchiveReader *)(intptr_t)reader,
        (KinokoActorManager *)(intptr_t)manager,resource_base);
}

static int32_t fixture_pat_load_file(int32_t manager,const char *file_name,const char *directory) {
    return kinoko_pat_load((KinokoActorManager *)(intptr_t)manager,file_name,directory);
}

static int32_t fixture_pat_tree_put(int32_t manager,int32_t key,int32_t value) {
    return kinoko_animation_bind((KinokoActorManager *)(intptr_t)manager,key,(KinokoAnimation *)(intptr_t)value);
}

static int32_t fixture_pat_append_resource(int32_t manager,int32_t handle) {
    kinoko_animation_add_texture((KinokoActorManager *)(intptr_t)manager,handle);return 1;
}

static int32_t fixture_collision_query_rect(int32_t state, int32_t layout,
    int32_t *cached, int32_t left, int32_t top, int32_t right, int32_t bottom,
    int32_t *count)
{
    return kinoko_map_collision_query((KinokoCollisionState *)(intptr_t)state,
        (KinokoActLayout *)(intptr_t)layout, cached, left, top, right, bottom, count);
}

static int32_t fixture_collision_query_map(int32_t layout, int32_t actor,
    int32_t layer_index, int32_t *count)
{
    return kinoko_collision_query_actor_map((KinokoCollisionState *)g_514300_storage,
        (KinokoActLayout *)(intptr_t)layout, (KinokoActor *)(intptr_t)actor, layer_index, count);
}

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

int kinoko_diagnostics_accepts(const char *label) {
    return label && strstr(label, "stagevm:failure") != NULL;
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
    if (!kinoko_reader_open((KinokoArchiveReader **)&reader, path)) return 0;
    script[24] = *(int32_t *)(intptr_t)(reader + 12);
    bytes = (unsigned char *)malloc((size_t)script[24]);
    result = bytes && retdec_reader_read_exact(reader, bytes, (uint32_t)script[24]);
    script[23] = PTR(bytes);
    if (result) result = retdec_execute_embedded_act_script(vm, PTR(script), environment);
    kinoko_reader_close((KinokoArchiveReader *)(intptr_t)reader);
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
        *(int32_t *)(intptr_t)entry;
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
    reader[0] = PTR(&kinoko_package_reader_methods);
    reader[1] = PTR(file);
    reader[3] = pat.size;
    kinoko_archive_count = 1;
    CHECK(fixture_pat_read_animations(PTR(reader), manager, 0));
    CHECK(reader[5] == reader[3]);
    CHECK(CloseHandle(file));
    kinoko_archive_count = 0;

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

    actor = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, 100, 200, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
    CHECK(actor);
    kinoko_actor_set_take((KinokoActor *)(intptr_t)(actor), base);
    CHECK(*(int32_t *)(intptr_t)(actor + 200) == head);
    CHECK(*(float *)(intptr_t)(actor + 440) == 93.5f);
    CHECK(*(float *)(intptr_t)(actor + 444) == 180.0f);
    CHECK(*(float *)(intptr_t)(actor + 448) == 110.5f);
    CHECK(*(float *)(intptr_t)(actor + 452) == 200.0f);
    for (int i = 0; i < 18; ++i) kinoko_actor_tick((KinokoActor *)(intptr_t)(actor));
    CHECK(*(int32_t *)(intptr_t)(actor + 200) == head);
    CHECK(*(int32_t *)(intptr_t)(actor + 204) == *(int32_t *)(intptr_t)(head + 8));
    CHECK(*(int32_t *)(intptr_t)(actor + 212) == 0);
    kinoko_actor_set_take((KinokoActor *)(intptr_t)(actor), base + 5);
    CHECK(*(float *)(intptr_t)(actor + 440) == 100.0f);
    CHECK(*(float *)(intptr_t)(actor + 444) == 200.0f);
    CHECK(*(float *)(intptr_t)(actor + 448) == 100.0f);
    CHECK(*(float *)(intptr_t)(actor + 452) == 200.0f);
    CHECK(*(int32_t *)(intptr_t)(actor + 388) == 0);
    kinoko_script_clear_actors();
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
    reader[0] = PTR(&kinoko_package_reader_methods);
    reader[1] = PTR(file);
    reader[3] = GetFileSize(file, NULL);
    ((unsigned char *)reader)[24] = (unsigned char)((offset >> 1) | 0x23);
    kinoko_archive_count = 1;
    CHECK(fixture_pat_read_u8(PTR(reader), &version) && version == 5);
    CHECK(fixture_pat_read_u16(PTR(reader), &textures));
    CHECK(fixture_pat_skip_bytes(PTR(reader), textures * 128u));
    CHECK(fixture_pat_read_animations(PTR(reader), manager, 0));
    CHECK(reader[5] == reader[3]);
    CHECK(CloseHandle(file));
    kinoko_archive_count = 0;
    actor = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, 100, 200, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
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
            kinoko_actor_set_take((KinokoActor *)(intptr_t)(actor), take);
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
    kinoko_actor_set_take((KinokoActor *)(intptr_t)(actor), 0);
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
        kinoko_actor_set_take((KinokoActor *)(intptr_t)(actor), 0);
        CHECK(*(float *)(intptr_t)(actor + 452) == 240);
    }
    kinoko_script_clear_actors();
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
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(scripts)), "t_player");
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
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(init)), "InitWalkingProbe");
    actor = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, 100, 239, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
    CHECK(actor);
    CHECK(execute_source(vm, root + 2,
        "if (typeof walkingProbe.funcUpdate != \"function\") throw \"missing walking callback\";"));
    kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager));
    function_468620_this(PTR(g_514300_storage));
    kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
    CHECK(*(int32_t *)(intptr_t)(actor + 296) == 1);
    for (int phase = 0; phase < 3; ++phase) {
        float start_x = *(float *)(intptr_t)(actor + 240);
        CHECK(execute_source(vm, root + 2, phase == 0 ? "input.x = 1;" :
            phase == 1 ? "input.x = -1;" : "input.x = 0;"));
        for (int frame = 0; frame < 30; ++frame) {
            float old_x = *(float *)(intptr_t)(actor + 240);
            int failures_before = vm_failures;
            kinoko_actor_tick((KinokoActor *)(intptr_t)(actor));
            CHECK(vm_failures == failures_before);
            function_468620_this(PTR(g_514300_storage));
            kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
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
    kinoko_script_clear_actors();
    function_468950_this(PTR(g_514300_storage), manager);
    if (reference_dir != NULL) {
        int32_t act[60];
        float entry_x = 0, entry_y = 0;
        int found_entry = 0;
        char path[MAX_PATH];
        for (char archive = 'a'; archive <= 'c'; ++archive) {
            sprintf_s(path, sizeof(path), "%s/6kinoko_%c.dat", reference_dir, archive);
            CHECK(kinoko_archive_mount(path));
        }
        CHECK(kinoko_graphics.device == 0);
        kinoko_act_document_initialize((KinokoActDocument *)act);
        CHECK(kinoko_act_document_load((KinokoActDocument *)act, stage_path));
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
        actor = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, entry_x, entry_y, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
        CHECK(actor);
        kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager));
        function_468620_this(PTR(g_514300_storage));
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
        CHECK(*(int32_t *)(intptr_t)(actor + 296) == 1);
        for (int direction = 1; direction >= -1; direction -= 2) {
            CHECK(execute_source(vm, root + 2, direction == 1 ? "input.x = 1;" : "input.x = -1;"));
            for (int frame = 0; frame < 60; ++frame) {
                int failures_before = vm_failures;
                CHECK(execute_source(vm, root + 2,
                    "if (typeof walkingProbe.GetChipFlag() != \"integer\") "
                    "throw \"invalid chip flag result\";"));
                kinoko_actor_tick((KinokoActor *)(intptr_t)(actor));
                function_468620_this(PTR(g_514300_storage));
                kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
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
        kinoko_script_clear_actors();
        function_468950_this(PTR(g_514300_storage), manager);
        retdec_destroy_cact_object(PTR(act));
        printf("PASS: original %s terrain walking in both directions (TYPE_2HEAD)\n", stage_path);
    }
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(init))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(scripts))));
    puts("PASS: original ground scripts walking, turning and stopping on a flat floor");
    return 0;
}

/* MSVC C does not expose __thiscall function-pointer syntax. Exercise the
   typed ECX/EDX adapter here; actor_lifecycle_contract.cpp independently calls
   the real thiscall ABI under /RTC1. These guards catch caller-storage writes. */
static __declspec(noinline) int32_t probe_set_step_entry(int32_t actor, int32_t object) {
    volatile uint32_t guards[4] = {0x12345678u, 0x87654321u, 0xa55aa55au, 0x5aa55aa5u};
    KinokoOwnedObjectWords argument;
    memcpy(&argument, (void *)(intptr_t)object, sizeof(argument));
    kinoko_actor_set_step_method(actor, NULL, argument);
    return guards[0] == 0x12345678u && guards[1] == 0x87654321u &&
        guards[2] == 0xa55aa55au && guards[3] == 0x5aa55aa5u;
}

static int test_actor_step(int32_t manager, int32_t vm, int32_t *root) {
    int32_t actors[3], object[3], controls[2], weak_counts[2], refs[2];
    int32_t top = function_48aa20(vm);
    for (int i = 0; i < 3; ++i) {
        actors[i] = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, 0, 0, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
        CHECK(actors[i]);
    }
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root + 1)), "stepRider", (const void *)(intptr_t)(actors[0] + 44));
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root + 1)), "stepFirst", (const void *)(intptr_t)(actors[1] + 44));
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root + 1)), "stepSecond", (const void *)(intptr_t)(actors[2] + 44));
    for (int i = 0; i < 2; ++i) {
        controls[i] = *(int32_t *)(intptr_t)(actors[i + 1] + 28);
        weak_counts[i] = *(int32_t *)(intptr_t)(controls[i] + 8);
        refs[i] = *(int32_t *)(intptr_t)(*(int32_t *)(intptr_t)(actors[i + 1] + 52) + 4);
    }
    kinoko_sqplus_object_copy_construct((void *)(intptr_t)(object), (const void *)(intptr_t)(actors[1] + 44));
    CHECK(probe_set_step_entry(actors[0], PTR(object)));
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
    kinoko_script_clear_actors();
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
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(init)), "ResetInit");
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(seed)), "resetSeed");
    actor = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, 100, 200, -1, &(const KinokoOwnedObjectWords){seed[0], seed[1], seed[2]}, (const void *)(intptr_t)(0)));
    CHECK(actor);
    parent = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, 0, 0, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
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
        kinoko_sqplus_object_copy_construct((void *)(intptr_t)(parent_object), (const void *)(intptr_t)(parent + 44));
        kinoko_actor_set_step_owned((KinokoActor *)(intptr_t)(actor), (KinokoOwnedObjectWords *)(intptr_t)(PTR(parent_object)));
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
        kinoko_native_weak_pair_lock(PTR(old_weak), locked);
        CHECK(locked[0] == 0 && locked[1] == 0);
        kinoko_native_release_weak(old_weak[1]);
        CHECK(*(int32_t *)(intptr_t)(parent_control + 8) == parent_weak_count);
        CHECK(*(int32_t *)(intptr_t)(seed[2] + 4) == argument_refs);
        CHECK(*(int32_t *)(intptr_t)(actor + 12) == original_handle);
        CHECK(*(int64_t *)(intptr_t)(actor + 392) == 32);
        CHECK(*(int32_t *)(intptr_t)(actor + 36) == 0);
        CHECK(*(int32_t *)(intptr_t)(actor + 112) == 0x08000100);
        CHECK(*(int32_t *)(intptr_t)(actor + 140) == 0x08000100);
        kinoko_actor_tick((KinokoActor *)(intptr_t)(actor));
        CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager)) == 2);
        CHECK(function_48aa20(vm) == stack_top);
    }
    CHECK(execute_source(vm, root + 2,
        "if (resetCalls != 33 || resetTicks != 32) throw \"reset callback counts\";\n"
        "resetProbe.SetUpdateFunction(function() {\n"
        " local previous = user; Reset();\n"
        " if(user!=previous) throw \"Reset cleared executing instance\";\n"
        " user.afterReset <- 42;\n"
        "});"));
    kinoko_actor_tick((KinokoActor *)(intptr_t)(actor));
    CHECK(vm_failures==0);
    CHECK(*(int32_t *)(intptr_t)(actor+112)==0x08000100);
    kinoko_actor_tick((KinokoActor *)(intptr_t)(actor));
    CHECK(execute_source(vm,root+2,
        "if(resetCalls!=34 || resetTicks!=33) throw \"post-Reset callback continuation\";"));
    kinoko_script_clear_actors();
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(init))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(seed))));
    puts("PASS: Reset replays initialization and retires old references across 32 resets");
    return 0;
}

static int test_stone_placement(int32_t manager, int32_t vm, int32_t *root) {
    int32_t reader = 0, script[26] = {0}, scripts[3], init[3], rider_init[3];
    unsigned char version;
    unsigned short textures;
    unsigned char *bytes;
    CHECK(kinoko_archive_count != 0 && kinoko_graphics.device == 0);
    CHECK(kinoko_reader_open((KinokoArchiveReader **)&reader, "data/actor/item/item.pat"));
    CHECK(fixture_pat_read_u8(reader, &version));
    CHECK(fixture_pat_read_u16(reader, &textures));
    CHECK(fixture_pat_skip_bytes(reader, textures * 128u));
    CHECK(fixture_pat_read_animations(reader, manager, 0));
    CHECK(*(int32_t *)(intptr_t)(reader + 20) - *(int32_t *)(intptr_t)(reader + 16) ==
        *(int32_t *)(intptr_t)(reader + 12));
    kinoko_reader_close((KinokoArchiveReader *)(intptr_t)reader);
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
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(rider_init)), "InitStoneRider");
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(scripts)), "t_item");
    CHECK(kinoko_reader_open((KinokoArchiveReader **)&reader, "data/script/bullet.cv4"));
    script[24] = *(int32_t *)(intptr_t)(reader + 12);
    bytes = (unsigned char *)malloc((size_t)script[24]);
    CHECK(bytes);
    CHECK(retdec_reader_read_exact(reader, bytes, (uint32_t)script[24]));
    script[23] = PTR(bytes);
    CHECK(retdec_execute_embedded_act_script(vm, PTR(script), scripts + 1));
    kinoko_reader_close((KinokoArchiveReader *)(intptr_t)reader);
    free(bytes);
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(scripts)), (void *)(intptr_t)(PTR(init)), "InitStone");
    CHECK(init[1] == 0x08000100);
    for (int round = 0; round < 8; ++round) {
        int direction = (round & 1) ? -1 : 1;
        int32_t top = function_48aa20(vm);
        int failures = vm_failures;
        int32_t rider = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){rider_init[0], rider_init[1], rider_init[2]}, 100, 200, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
        CHECK(rider);
        CHECK(execute_source(vm, root + 2, direction == 1 ?
            "player.direction = 1.0;" : "player.direction = -1.0;"));
        int32_t stone = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, 100, 200, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
        CHECK(stone && vm_failures == failures);
        CHECK(*(float *)(intptr_t)(stone + 240) == 100.0f + 40.0f * direction);
        CHECK(*(float *)(intptr_t)(stone + 244) == 200);
        CHECK(*(float *)(intptr_t)(stone + 256) == 0);
        CHECK(*(float *)(intptr_t)(stone + 260) == 0);
        CHECK(*(int32_t *)(intptr_t)(stone + 208) == 1130);
        CHECK(*(int64_t *)(intptr_t)(stone + 392) == 32);
        CHECK(*(int32_t *)(intptr_t)(stone + 236) == 0x800000);
        kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root + 1)), "stoneProbe", (const void *)(intptr_t)(stone + 44));
        CHECK(execute_source(vm, root + 2,
            "if (player.user.stone != stoneProbe || stoneProbe.collisionGroup != GP_LIFT || "
            "stoneProbe.collisionMask != GP_TERRAIN || stoneProbe.user.time != 0) "
            "throw \"stone initialization incomplete\";\n"
            "player.x = stoneProbe.x;\n"
            "player.y = stoneProbe.top + 2 - (player.bottom - player.y);\n"
            "player.vy = 1.0;"));
        kinoko_actor_refresh_collision_bounds((KinokoActor *)(intptr_t)(rider));
        CHECK(kinoko_collision_dispatch_pair((KinokoActor *)(intptr_t)(stone), (KinokoActor *)(intptr_t)(rider)) >= 0);
        CHECK(vm_failures == failures);
        CHECK(*(int32_t *)(intptr_t)(rider + 32) == *(int32_t *)(intptr_t)(stone + 24));
        CHECK(*(int32_t *)(intptr_t)(rider + 36) == *(int32_t *)(intptr_t)(stone + 28));
        CHECK(execute_source(vm, root + 2,
            "if (!stoneProbe.user.ride || player.step != stoneProbe) "
            "throw \"original stone callback did not bind rider\";\n"
            "player.vy = 0.0; stoneProbe.vx = player.direction * 2.0;"));
        CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager)) == 2);
        for (int frame = 0; frame < 8; ++frame) {
            float old_x = *(float *)(intptr_t)(rider + 240);
            kinoko_actor_tick((KinokoActor *)(intptr_t)(stone));
            function_468620_this(PTR(g_514300_storage));
            kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(stone));
            kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(rider));
            CHECK(vm_failures == failures);
            CHECK(*(float *)(intptr_t)(rider + 240) == old_x + direction * 2.0f);
            CHECK(*(int32_t *)(intptr_t)(rider + 36) == *(int32_t *)(intptr_t)(stone + 28));
        }
        if (round & 1) {
            CHECK(execute_source(vm, root + 2, "stoneProbe.Release();\nstoneProbe = null;"));
            CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager)) == 1);
            int32_t locked[2];
            kinoko_native_weak_pair_lock(rider + 32, locked);
            CHECK(locked[0] == 0 && locked[1] == 0);
            kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(rider));
            CHECK(*(float *)(intptr_t)(rider + 264) == 0);
            CHECK(execute_source(vm, root + 2, "player.SetStep(null);"));
        } else {
            CHECK(execute_source(vm, root + 2, "player.x = stoneProbe.right + 64;"));
            kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(rider));
            CHECK(*(int32_t *)(intptr_t)(rider + 36) == 0);
            CHECK(execute_source(vm, root + 2,
                "if (player.step != null) throw \"walk-off did not detach\";"));
            kinoko_actor_tick((KinokoActor *)(intptr_t)(stone));
            CHECK(vm_failures == failures);
            CHECK(execute_source(vm, root + 2,
                "if (stoneProbe.user.time != 0 || stoneSounds[stoneSounds.len()-1] != 35) "
                "throw \"original stone falling transition\";\n"
                "stoneProbe = null;"));
        }
        CHECK(function_48aa20(vm) == top);
        CHECK(execute_source(vm, root + 2, "stoneOwner <- player.user;"));
        kinoko_script_clear_actors();
        CHECK(execute_source(vm, root + 2,
            "if (stoneOwner.stone != null || player.user != null) "
            "throw \"stone or rider retained state after clear\";"));
    }
    CHECK(execute_source(vm, root + 2,
        "if (stoneSounds.len() != 12 || stoneEffects.len() != 8) throw \"stone media calls\";\n"
        "foreach (id in stoneSounds) if (id != 33 && id != 35) throw \"stone sound id\";\n"
        "foreach (id in stoneEffects) if (id != 1960) throw \"stone effect id\";"));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(rider_init))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(init))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(scripts))));
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
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(scripts)), "t_item");
    CHECK(execute_asset(vm, scripts + 1, "data/script/item.cv4"));
    for (int kind = 0; kind < 3; ++kind) {
        kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(scripts)), (void *)(intptr_t)(PTR(init)), initializers[kind]);
        CHECK(init[1] == 0x08000100);
        for (int round = 0; round < 4; ++round) {
            int init_failures = vm_failures;
            int32_t actor = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, 100, 180, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
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
                kinoko_actor_tick((KinokoActor *)(intptr_t)(actor));
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
                        kinoko_sqplus_object_get_value((void *)(intptr_t)(actor + 44), (void *)(intptr_t)(PTR(user)), "user");
                        kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(user)), (void *)(intptr_t)(PTR(count)), "count");
                        fprintf(stderr, "float kind=%d round=%d frame=%d delta=(%g,%g) v=(%g,%g) count=%d\n",
                            kind, round, frame, dx, dy, vx, vy, count[2]);
                        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(count))));
                        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(user))));
                    }
                    CHECK(vx * dx + vy * dy > 0.0f);
                    CHECK(fabsf(sqrtf(vx * vx + vy * vy) - 12.0f) < 0.0001f);
                    ++homing_frames;
                }
                kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
            }
            CHECK(released);
            CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager)) == 0);
            CHECK(function_48aa20(vm) == stack_top);
        }
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(init))));
    }
    CHECK(execute_source(vm, root + 2,
        "if (floatRewards[0]!=4 || floatRewards[1]!=4 || floatRewards[2]!=4 || "
        "floatNumbers.len()!=12) throw \"floating reward count\";\n"
        "foreach (i,n in floatNumbers) if (n != (i<4 ? 100 : (i<8 ? 0 : 10000))) "
        "throw \"floating reward value\";"));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(scripts))));
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
    int32_t resource_base=kinoko_integer_vector_size(manager+68);
    CHECK(kinoko_reader_open((KinokoArchiveReader **)&reader,"data/actor/item/item.pat"));
    CHECK(fixture_pat_read_u8(reader,&version) && fixture_pat_read_u16(reader,&texture_count));
    CHECK(fixture_pat_skip_bytes(reader,texture_count*128u));
    g_retdec_act_texture_slots[1].width=1024;
    g_retdec_act_texture_slots[1].height=1024;
    for(int i=0;i<texture_count;++i) CHECK(fixture_pat_append_resource(manager,1));
    CHECK(fixture_pat_read_animations(reader,manager,resource_base));
    kinoko_reader_close((KinokoArchiveReader *)(intptr_t)reader);
    float native_camera[24]={0};
    native_camera[20]=2000;
    native_camera[21]=1200;
    int failures=vm_failures;
    CHECK(execute_source(vm,root+2,
        "map <- {height=2000,width=2000};\n"
        "camera <- {left=0.0,top=0.0,right=2000.0,bottom=1200.0};\n"
        "stageWaterLevel=10000;\n"));
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(scripts)), "t_item");
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
            kinoko_script_clear_actors();
            function_468950_this(PTR(g_514300_storage),manager);
            kinoko_act_document_initialize((KinokoActDocument *)actual_map);
            CHECK(kinoko_act_document_load((KinokoActDocument *)actual_map,"data/map/w1-c01a.act"));
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
        kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(scripts)), (void *)(intptr_t)(PTR(init)), names[kind%2]);
        CHECK(init[1]==0x08000100);
        int32_t block=0, bumper=0, actor=0;
        if(kind<4) {
            actor=(int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, 512, kind<2 ? 160.0f : 768.0f, 1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
        } else {
            int32_t enemy[3], block_init[3], bumper_init[3];
            CHECK(execute_source(vm,root+2,
                "function InitStarBumper(v) { user={type=TYPE_2HEAD,ball=null};\n"
                " SetTake(100); callbackGroup=GP_PLAYER; ::player=this; }\n"
                "starRewards <- 0;\nfunction AddStar() { ::starRewards++; }\n"));
            kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(enemy)), "t_enemy");
            kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(enemy)), (void *)(intptr_t)(PTR(block_init)), "Init0436");
            kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(bumper_init)), "InitStarBumper");
            bumper=(int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){bumper_init[0], bumper_init[1], bumper_init[2]}, kind==4 ? 500.0f : 524.0f, 820, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
            block=(int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){block_init[0], block_init[1], block_init[2]}, 512, 768, -1, &(const KinokoOwnedObjectWords){PTR(&g16), 0x05000002, 0x436}, (const void *)(intptr_t)(0)));
            CHECK(block && bumper && vm_failures==failures);
            kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), "starBlock", (const void *)(intptr_t)(block+44));
            CHECK(execute_source(vm,root+2,"starBlock.user.SetDamage(player);"));
            kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager));
            for(int n=0;n<*(int32_t *)(intptr_t)(manager+116);++n) {
                int32_t candidate=(*(int32_t **)(intptr_t)(manager+100))[n];
                if(*(int32_t *)(intptr_t)(candidate+208)==1060) actor=candidate;
            }
            CHECK(actor);
            CHECK(execute_source(vm,root+2,"player.x=0;"));
            kinoko_actor_refresh_collision_bounds((KinokoActor *)(intptr_t)(bumper));
            (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(enemy)))); (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(block_init))));
            (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(bumper_init))));
        }
        int contacts=0, bounces=0;
        CHECK(actor && vm_failures==failures);
        CHECK(*(int32_t *)(intptr_t)(actor+208)==1060);
        CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager))>0);
        for(int frame=0;frame<300;++frame) {
            int grounded=*(int32_t *)(intptr_t)(actor+296);
            kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(native_camera)));
            CHECK(vm_failures==failures);
            float vy=*(float *)(intptr_t)(actor+260);
            if(grounded && vy<0) ++bounces;
            CHECK(kinoko_actor_render((KinokoActor *)(intptr_t)(actor), (KinokoCamera *)(intptr_t)(PTR(native_camera)))==1);
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
            kinoko_actor_refresh_collision_bounds((KinokoActor *)(intptr_t)(bumper));
            kinoko_collision_dispatch_pair((KinokoActor *)(intptr_t)(actor), (KinokoActor *)(intptr_t)(bumper));
            CHECK(vm_failures==failures && *(uint8_t *)(intptr_t)(actor+22));
            CHECK(execute_source(vm,root+2,"if(starRewards!=1) throw \"star pickup reward\";"));
        }
        kinoko_actor_release((KinokoActor *)(intptr_t)actor, NULL);
        if(block) kinoko_actor_release((KinokoActor *)(intptr_t)block, NULL);
        if(bumper) kinoko_actor_release((KinokoActor *)(intptr_t)bumper, NULL);
        kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager));
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(init))));
    }
    kinoko_script_clear_actors();
    function_468950_this(PTR(g_514300_storage),manager);
    retdec_destroy_cact_object(PTR(actual_map));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(scripts))));
    puts("PASS: original moving stars land, bounce and remain collectible");
    return 0;
}

static int test_hidden_layer(int32_t vm, int32_t *root) {
    int32_t act[60], resource[48] = {0}, parent[2] = {g483,g484};
    int32_t hidden = 0, active = 0, layout = 0, script[3];
    int32_t compile_target = PTR(kinoko_script_compile_file_argument), stack_top = function_48aa20(vm);
    int failures = vm_failures;
    g874 = 1;
    CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR("CompileFile")), (const void *)(intptr_t)(PTR(&compile_target)), 4, (void *)(intptr_t)(PTR(retdec_compile_file_native)), 0)) >= 0);
    kinoko_act_document_initialize((KinokoActDocument *)act);
    CHECK(kinoko_act_document_load((KinokoActDocument *)act, "data/map/w1-c01a.act"));
    for (int32_t slot = act[52]; slot != act[53]; slot += 4) {
        int32_t layer = *(int32_t *)(intptr_t)slot;
        if (strcmp(retdec_std_string_data(layer + 112), "hidden") == 0) hidden = layer;
    }
    CHECK(hidden);
    CHECK(retdec_publish_cact_layer_class(vm, PTR(root)));
    CHECK(kinoko_sqrat_new_table((struct SQVM *)(intptr_t)(vm), parent));
    CHECK(kinoko_sqrat_set_pair((struct SQVM *)(intptr_t)(vm), root + 2, retdec_std_string_data(PTR(act) + 16), parent));
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
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(script)), (void *)(intptr_t)(PTR(layout_object)), "layout");
    layout = (int32_t)(intptr_t)(kinoko_sqplus_object_instance((void *)(intptr_t)(PTR(layout_object)), (void *)(intptr_t)(0)));
    CHECK(layout && retdec_map_chip_data(layout));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(layout_object))));
    CHECK(retdec_execute_act_callback(hidden + 204, 4, NULL) >= 0);
    CHECK(*(int32_t *)(intptr_t)(layout + 328) == 1);
    CHECK(execute_source(vm, root + 2,
        "player <- { left=2232.0, right=2264.0, top=800.0, bottom=832.0 };"));
    for (int frame = 0; frame < 24; ++frame) CHECK(kinoko_act_layer_update((KinokoActLayer *)(intptr_t)hidden) >= 0);
    CHECK(vm_failures == failures);
    CHECK(*(float *)(intptr_t)(layout + 320) <= 0.001f);
    CHECK(execute_source(vm, root + 2, "player.left=100.0; player.right=120.0;"));
    for (int frame = 0; frame < 24; ++frame) CHECK(kinoko_act_layer_update((KinokoActLayer *)(intptr_t)hidden) >= 0);
    CHECK(*(float *)(intptr_t)(layout + 320) >= 0.999f);
    CHECK(vm_failures == failures && function_48aa20(vm) == stack_top);
    kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), parent);
    puts("PASS: original ACT hidden-layer include, callbacks, native fade-out and fade-in");
    return 0;
}

static int test_enemy_reentry(int32_t manager, int32_t vm, int32_t *root) {
    int32_t reader = 0, scripts[3], init[3], actors[3];
    float saved_camera[4];
    int32_t create_target = PTR(kinoko_script_create_actor);
    int32_t layout[100] = {0}, layer[80] = {0}, resource[20] = {0};
    int32_t records[1][8] = {{1, -8000, 200}};
    struct retdec_mcd_chip chip = {0};
    struct retdec_mcd_data data = {1,&chip,0,NULL};
    unsigned char version;
    unsigned short textures;
    int failures = vm_failures;
    CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR("CreateActor")), (const void *)(intptr_t)(PTR(&create_target)), 4, (void *)(intptr_t)(PTR(kinoko_script_create_actor_entry)), 0)) >= 0);
    {
        int32_t globals[4]={0,vm,g483,g484}, callback[2]={g483,g484};
        CHECK(kinoko_sqrat_new_table((struct SQVM *)(intptr_t)(vm), globals+2));
        CHECK(function_48e520_this(globals[3],root[3]));
        CHECK(execute_asset(vm,globals+2,"data/script/global.cv4"));
        CHECK(kinoko_sqrat_get((void *)(intptr_t)(PTR(globals)), "GetCallbackFuncTable", (void *)(intptr_t)(callback)));
        CHECK(kinoko_sqrat_set_pair((struct SQVM *)(intptr_t)(vm), root+2, "GetCallbackFuncTable", callback));
        kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), callback);
        kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), globals+2);
    }
    CHECK(execute_source(vm, root + 2,
        "t_enemy <- {};\ncamera <- {left=-8000.0,right=8000.0,top=-2000.0,bottom=2000.0};\n"
        "player <- {x=0.0,y=100.0,direction=-1.0,user={hold=null,water=false}};\n"
        "function PlaySE(id) {}\n PR_FRONT <- 65535;\n stageWaterLevel <- 10000;\nupdateMask <- -1;\n"));
    CHECK(execute_asset(vm, root + 2, "data/script/enemy.cv4"));
    CHECK(vm_failures == failures);
    CHECK(kinoko_reader_open((KinokoArchiveReader **)&reader, "data/actor/enemy/enemy.pat"));
    CHECK(fixture_pat_read_u8(reader, &version));
    CHECK(fixture_pat_read_u16(reader, &textures));
    CHECK(fixture_pat_skip_bytes(reader, textures * 128u));
    CHECK(fixture_pat_read_animations(reader, manager, 0));
    kinoko_reader_close((KinokoArchiveReader *)(intptr_t)reader);
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
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(scripts)), "t_enemy");
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(scripts)), (void *)(intptr_t)(PTR(init)), "Init0107");
    int32_t fairy = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, 100, 160, -1, &(const KinokoOwnedObjectWords){PTR(&g16), 0x05000002, 0x107}, (const void *)(intptr_t)(0)));
    CHECK(fairy && vm_failures==failures);
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), "fairy", (const void *)(intptr_t)(fairy+44));
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
            kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
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
            kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(obj)), "probeBall");
            int32_t ball=(int32_t)(intptr_t)(kinoko_sqplus_object_instance((void *)(intptr_t)(PTR(obj)), (void *)(intptr_t)(0)));
            printf("ball %d/%d active=%d visible=%d release=%d xy=%g,%g callback=%x\n",round,i,
                *(uint8_t *)(intptr_t)(ball+40),*(uint8_t *)(intptr_t)(ball+21),
                *(uint8_t *)(intptr_t)(ball+22),*(float *)(intptr_t)(ball+240),
                *(float *)(intptr_t)(ball+244),*(int32_t *)(intptr_t)(ball+112));
            (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(obj))));
        }
        printf("PASS: ball generation %d\n",round); fflush(stdout);
        // Original map flags make a reset actor wait for native visibility.
        *(uint32_t *)(intptr_t)(fairy+392)=0x20000;
        CHECK(execute_source(vm,root+2,
            "camera.left=-1140; camera.right=-500;"));
        *(float *)(g_retdec_camera_state+72)=-1140;
        *(float *)(g_retdec_camera_state+80)=-500;
        kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
        expected_vm_error=0;
        CHECK(*(int32_t *)(intptr_t)(fairy+112)==0x08000100);
        kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), "fairy", (const void *)(intptr_t)(fairy+44));
        CHECK(execute_source(vm,root+2,
            "if(fairy.x!=fairy.ox) throw \"reset origin\";\n"
            "camera.left=-8000; camera.right=8000;"));
        *(float *)(g_retdec_camera_state+72)=-8000;
        *(float *)(g_retdec_camera_state+80)=8000;
    }
    int32_t death_init[3];
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(scripts)), (void *)(intptr_t)(PTR(death_init)), "Init0106");
    int32_t victim=(int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){death_init[0], death_init[1], death_init[2]}, 300, 160, -1, &(const KinokoOwnedObjectWords){PTR(&g16), 0x05000002, 0x106}, (const void *)(intptr_t)(0)));
    CHECK(victim);
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), "victim", (const void *)(intptr_t)(victim+44));
    CHECK(execute_source(vm,root+2,
        "t_item <- { InitPoint=function(v){}, Init1up=function(v){} };\n"
        "attacker <- { user={hitCount=0},callbackGroup=0 };\n"
        "t_enemy.EnemyCollision_Damage.call(victim,attacker);\n"
        "if(victim.vy!=-5 || victim.user.blowOff) throw \"ordinary death setup\";"));
    for(int frame=0;frame<90;++frame) {
        kinoko_actor_tick((KinokoActor *)(intptr_t)(victim));
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(victim));
        CHECK(vm_failures==failures);
        if(frame==30) CHECK(*(float *)(intptr_t)(victim+260)>0);
    }
    CHECK(*(float *)(intptr_t)(victim+244)>160);
    CHECK(execute_source(vm,root+2,
        "fairy.user.eventHandler.OnHitStep(attacker);\n"
        "if(fairy.vy!=-5) throw \"stomp death setup\";"));
    for(int frame=0;frame<90;++frame) {
        kinoko_actor_tick((KinokoActor *)(intptr_t)(fairy));
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(fairy));
        CHECK(vm_failures==failures);
    }
    CHECK(*(float *)(intptr_t)(fairy+244)>160);
    CHECK(*(float *)(intptr_t)(fairy+260)>0);
    puts("PASS: original collision death rises briefly then falls under gravity");
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(death_init))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(init))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(scripts))));
    kinoko_script_clear_actors();
    return 0;
}

static int test_entity_stutter(int32_t manager, int32_t vm, int32_t *root, const char *name) {
    const unsigned int rounding = kinoko_enter_game_math();
    int32_t create_target=PTR(kinoko_script_create_actor), scripts[3], init[3], reader=0;
    function_48ab90(vm,root[2],root[3]);
    CHECK(function_4c6c20(vm)==0);
    function_48aa50(vm);
    unsigned char version; unsigned short textures;
    CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR("CreateActor")), (const void *)(intptr_t)(PTR(&create_target)), 4, (void *)(intptr_t)(PTR(kinoko_script_create_actor_entry)), 0))>=0);
    int32_t globals[4]={0,vm,g483,g484}, callback[2]={g483,g484};
    CHECK(kinoko_sqrat_new_table((struct SQVM *)(intptr_t)(vm), globals+2));
    CHECK(function_48e520_this(globals[3],root[3]));
    CHECK(execute_asset(vm,globals+2,"data/script/global.cv4"));
    CHECK(kinoko_sqrat_get((void *)(intptr_t)(PTR(globals)), "GetCallbackFuncTable", (void *)(intptr_t)(callback)));
    CHECK(kinoko_sqrat_set_pair((struct SQVM *)(intptr_t)(vm), root+2, "GetCallbackFuncTable", callback));
    CHECK(execute_source(vm,root+2,
        "t_enemy <- {};\ncamera <- {left=-8000.0,right=8000.0,top=-2000.0,bottom=2000.0};\n"
        "player <- {x=0.0,y=100.0,user={hold=null,water=false}};\n"
        "stageWaterLevel <- 10000;\nupdateMask <- -1;\ncurrentTime <- 0;\nfunction PlaySE(id) {}\neffectCount <- 0;\nfunction CreateEffect(x,y,z,id) { ++::effectCount; }\n"));
    CHECK(execute_asset(vm,root+2,"data/script/enemy.cv4"));
    CHECK(kinoko_reader_open((KinokoArchiveReader **)&reader,"data/actor/enemy/enemy.pat"));
    CHECK(fixture_pat_read_u8(reader,&version) && fixture_pat_read_u16(reader,&textures));
    CHECK(fixture_pat_skip_bytes(reader,textures*128u));
    CHECK(fixture_pat_read_animations(reader,manager,0));
    kinoko_reader_close((KinokoArchiveReader *)(intptr_t)reader);
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(scripts)), "t_enemy");
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(scripts)), (void *)(intptr_t)(PTR(init)), name);
    CHECK(init[1]==0x08000100);
    for(int i=0;i<3;++i) {
        int32_t actor=(int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, 200.0f+i*200, 160, -1, &(const KinokoOwnedObjectWords){PTR(&g16), 0x05000002, (int32_t)strtol(name+4,NULL,16)}, (const void *)(intptr_t)(0)));
        CHECK(actor && vm_failures==0);
        *(unsigned char *)(intptr_t)(actor+40)=1;
    }
    CHECK(function_468950_this(PTR(g_514300_storage),manager));
    *(int32_t *)(intptr_t)(manager+64)=-1;
    kinoko_game_masks.update=-1;
    float camera[24]={0}; camera[18]=-8000;camera[19]=-2000;camera[20]=8000;camera[21]=2000;
    const int32_t stack_top = function_48aa20(vm);
    int peak_count = 0;
    LARGE_INTEGER frequency,start,end; QueryPerformanceFrequency(&frequency);
    for(int frame=0;frame<240;++frame) {
        QueryPerformanceCounter(&start);
        int count=kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(camera)));
        QueryPerformanceCounter(&end);
        CHECK(vm_failures==0 && function_48aa20(vm)==stack_top);
        CHECK(count>=3 && count<=512);
        if(count>peak_count) peak_count=count;
        if(strcmp(name,"Init0989")==0) CHECK(count==36);
        if(strcmp(name,"Init0cfb")==0) CHECK(count==393);
        if(frame<5 || frame%30==0) { printf("ENTITY %s frame=%d actors=%d ms=%.3f\n",name,frame,count,1000.0*(end.QuadPart-start.QuadPart)/frequency.QuadPart); fflush(stdout); }
    }
    if(strcmp(name,"Init0d0c")==0) {
        CHECK(peak_count>3);
        CHECK(execute_source(vm,root+2,"if(effectCount==0) throw \"cannon did not fire\";"));
    }
    kinoko_leave_game_math(rounding);
    return 0;
}

static int test_enemy_scripts(int32_t manager, int32_t vm, int32_t *root) {
    int32_t reader = 0, scripts[3], init[3], actors[3];
    float saved_camera[4];
    int32_t create_target = PTR(kinoko_script_create_actor);
    int32_t layout[100] = {0}, layer[80] = {0}, resource[20] = {0};
    int32_t records[1][8] = {{1, -8000, 200}};
    struct retdec_mcd_chip chip = {0};
    struct retdec_mcd_data data = {1,&chip,0,NULL};
    unsigned char version;
    unsigned short textures;
    int failures = vm_failures;
    CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR("CreateActor")), (const void *)(intptr_t)(PTR(&create_target)), 4, (void *)(intptr_t)(PTR(kinoko_script_create_actor_entry)), 0)) >= 0);
    {
        int32_t globals[4]={0,vm,g483,g484}, callback[2]={g483,g484};
        CHECK(kinoko_sqrat_new_table((struct SQVM *)(intptr_t)(vm), globals+2));
        CHECK(function_48e520_this(globals[3],root[3]));
        CHECK(execute_asset(vm,globals+2,"data/script/global.cv4"));
        CHECK(kinoko_sqrat_get((void *)(intptr_t)(PTR(globals)), "GetCallbackFuncTable", (void *)(intptr_t)(callback)));
        CHECK(kinoko_sqrat_set_pair((struct SQVM *)(intptr_t)(vm), root+2, "GetCallbackFuncTable", callback));
        kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), callback);
        kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), globals+2);
    }
    CHECK(execute_source(vm, root + 2,
        "t_enemy <- {};\ncamera <- {left=-8000.0,right=8000.0,top=-2000.0,bottom=2000.0};\n"
        "player <- {x=0.0,y=100.0,user={hold=null,water=false}};\n"
        "stageWaterLevel=10000;\nupdateMask <- -1;\n"));
    CHECK(execute_asset(vm, root + 2, "data/script/enemy.cv4"));
    CHECK(vm_failures == failures);
    CHECK(kinoko_reader_open((KinokoArchiveReader **)&reader, "data/actor/enemy/enemy.pat"));
    CHECK(fixture_pat_read_u8(reader, &version));
    CHECK(fixture_pat_read_u16(reader, &textures));
    CHECK(fixture_pat_skip_bytes(reader, textures * 128u));
    CHECK(fixture_pat_read_animations(reader, manager, 0));
    kinoko_reader_close((KinokoArchiveReader *)(intptr_t)reader);
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
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(scripts)), "t_enemy");
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(scripts)), (void *)(intptr_t)(PTR(init)), "Init0106");
    for (int i = 0; i < 2; ++i) {
        actors[i] = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, 100.0f + 200.0f * i, 160, -1, &(const KinokoOwnedObjectWords){PTR(&g16), 0x05000002, 0x106}, (const void *)(intptr_t)(0)));
        CHECK(actors[i] && vm_failures == failures);
        kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root + 1)), i ? "enemyB" : "enemyA", (const void *)(intptr_t)(actors[i] + 44));
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
    CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager)) == 3);
    for (int frame = 0; frame < 40; ++frame) {
        for (int i = 0; i < 2; ++i) kinoko_actor_tick((KinokoActor *)(intptr_t)(actors[i]));
        CHECK(vm_failures == failures);
        function_468620_this(PTR(g_514300_storage));
        for (int i = 0; i < 2; ++i) {
            kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actors[i]));
            CHECK(_finite(*(float *)(intptr_t)(actors[i] + 244)));
        }
    }
    CHECK(execute_source(vm, root + 2,
        "if (!(enemyA.x<100 && enemyB.x<300 && enemyA.hitBottom && enemyB.hitBottom)) "
        "throw \"enemy walking and landing\";\n"
        "enemyA.hitLeft=1; enemyA.xPrev=enemyA.x;"));
    kinoko_actor_tick((KinokoActor *)(intptr_t)(actors[0]));
    CHECK(vm_failures == failures);
    CHECK(execute_source(vm, root + 2,
        "if (enemyA.direction!=1.0 || enemyA.vx<=0.0 || enemyB.direction!=-1.0) "
        "throw \"enemy wall reversal\";"));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(init))));
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(scripts)), (void *)(intptr_t)(PTR(init)), "Init0101");
    actors[2] = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, 600, 160, -1, &(const KinokoOwnedObjectWords){PTR(&g16), 0x05000002, 0x101}, (const void *)(intptr_t)(0)));
    CHECK(actors[2] && vm_failures == failures);
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), "enemyC", (const void *)(intptr_t)(actors[2]+44));
    memcpy(saved_camera,g_retdec_camera_state+72,sizeof(saved_camera));
    *(float *)(g_retdec_camera_state+72)=-8000;
    *(float *)(g_retdec_camera_state+76)=-2000;
    *(float *)(g_retdec_camera_state+80)=8000;
    *(float *)(g_retdec_camera_state+84)=2000;
    *(int32_t *)(intptr_t)(manager+64)=-1;
    for (int frame=0;frame<3600;++frame) {
        kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
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
        for(int i=0;i<3;++i) kinoko_actor_tick((KinokoActor *)(intptr_t)(actors[i]));
        CHECK(vm_failures==failures);
        CHECK(execute_source(vm,root+2,
            "if(enemyA.x!=-16777215 || enemyB.x!=-16777215 || enemyC.x!=-16777215) "
            "throw \"offscreen waiting state\";\n"
            "camera.left=-10000; camera.right=-9000;"));
        for(int i=0;i<3;++i) kinoko_actor_tick((KinokoActor *)(intptr_t)(actors[i]));
        CHECK(vm_failures==failures);
        const char *names[]={"enemyA","enemyB","enemyC"};
        for(int i=0;i<3;++i) kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), names[i], (const void *)(intptr_t)(actors[i]+44));
        CHECK(execute_source(vm,root+2,
            "if(enemyA.x!=enemyA.ox || enemyB.x!=enemyB.ox || enemyC.x!=enemyC.ox) "
            "throw \"offscreen reset origin\";\n"
            "camera.left=-8000; camera.right=8000;"));
        for(int frame=0;frame<120;++frame)
            kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
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
        kinoko_actor_refresh_collision_bounds((KinokoActor *)(intptr_t)(actors[i]));
    }
    for(int frame=0;frame<600;++frame) {
        kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
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
    kinoko_script_clear_actors();
    CHECK(function_468950_this(PTR(g_514300_storage), manager));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(init))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(scripts))));
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
    CHECK(kinoko_sqrat_new_table((struct SQVM *)(intptr_t)(vm), first));
    CHECK(kinoko_sqrat_new_table((struct SQVM *)(intptr_t)(vm), second));
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
    kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), first);
    kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), second);
    puts("PASS: delegate isolation/cycle rejection and userdata finalization/destruction");
    return 0;
}

static int test_stone_block(int32_t manager, int32_t vm, int32_t *root) {
    int32_t enemy[3], item[3], block_init[3], stone_init[3], rider_init[3];
    int32_t rider, stone, block;
    int failures=vm_failures;
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(enemy)), "t_enemy");
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(item)), "t_item");
    CHECK(execute_asset(vm, enemy+1, "data/script/block.cv4"));
    CHECK(execute_asset(vm, item+1, "data/script/bullet.cv4"));
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(enemy)), (void *)(intptr_t)(PTR(block_init)), "Init0435");
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(item)), (void *)(intptr_t)(PTR(stone_init)), "InitStone");
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(rider_init)), "InitStoneRider");
    rider=(int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){rider_init[0], rider_init[1], rider_init[2]}, 100, 150, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
    stone=(int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){stone_init[0], stone_init[1], stone_init[2]}, 100, 150, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
    CHECK(rider && stone && vm_failures==failures);
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), "fallingStone", (const void *)(intptr_t)(stone+44));
    CHECK(execute_source(vm,root+2,
        "player.x=fallingStone.x;\n"
        "player.y=fallingStone.top+2-(player.bottom-player.y); player.vy=1.0;"));
    kinoko_actor_refresh_collision_bounds((KinokoActor *)(intptr_t)(rider));
    kinoko_collision_dispatch_pair((KinokoActor *)(intptr_t)(stone), (KinokoActor *)(intptr_t)(rider));
    CHECK(execute_source(vm,root+2,"player.x=fallingStone.right+64;"));
    kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(rider));
    kinoko_actor_tick((KinokoActor *)(intptr_t)(stone));
    block=(int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){block_init[0], block_init[1], block_init[2]}, *(float *)(intptr_t)(stone+240), 240, -1, &(const KinokoOwnedObjectWords){PTR(&g16), 0x05000002, 0x435}, (const void *)(intptr_t)(0)));
    CHECK(block && vm_failures==failures);
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), "stoneBlock", (const void *)(intptr_t)(block+44));
    int contacted=0;
    kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager));
    for(int frame=0;frame<80;++frame) {
        kinoko_actor_tick((KinokoActor *)(intptr_t)(stone));
        function_468620_this(PTR(g_514300_storage));
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(stone));
        kinoko_collision_dispatch_pair((KinokoActor *)(intptr_t)(stone), (KinokoActor *)(intptr_t)(block));
        CHECK(vm_failures==failures);
        CHECK(_finite(*(float *)(intptr_t)(stone+244)));
        if(*(uint8_t *)(intptr_t)(stone+22)) {contacted=1;break;}
    }
    CHECK(contacted);
    CHECK(execute_source(vm,root+2,
        "if (stoneBlock.callbackMask!=0 || stoneBlock.user.SetDamage!=null || stoneBlock.user.direction!=1) "
        "throw \"stone did not activate original block callback\";"));
    kinoko_script_clear_actors();
    CHECK(function_468950_this(PTR(g_514300_storage),manager));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(enemy)))); (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(item))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(block_init)))); (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(stone_init))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(rider_init))));
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
    CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR("PlayBgmMargin")), (const void *)(intptr_t)(PTR(&margin_target)), 4, (void *)(intptr_t)(PTR(function_4720e0)), 0))>=0);
    CHECK(execute_source(vm, root+2,
        "transformFaces <- 0;\n"
        "PlayerImage <- {SetFaceType=function(t){::transformFaces++;}};\n"
        "PlayerStatus <- {ItemCross=false};\n"
        "stageBgm=\"data/bgm/st1.ogg\";\n"
        "input.x=0; input.b3=0; input.b0=0; input.b2=0;\n"
        "camera <- {left=-1000.0,right=1000.0,top=-1000.0,bottom=1000.0};"));
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(init)), "InitWalkingProbe");
    actor=(int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, 100, 200, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
    CHECK(actor);
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), "transformProbe", (const void *)(intptr_t)(actor+44));
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
        kinoko_actor_tick((KinokoActor *)(intptr_t)(actor));
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
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
    kinoko_script_clear_actors();
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(init))));
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
    CHECK(kinoko_sqrat_set_native_closure((struct SQVM *)(intptr_t)(vm), root+2, "SortNativeCompare", (void *)(intptr_t)(PTR(sort_native_compare)), NULL, 0));
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
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(array)), "sortObjects");
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
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(array))));
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
    /* Source handlers have source addresses, not reconstructed function IDs. */
    CHECK(*(int32_t *)(intptr_t)(shared + 160) != 0);
    CHECK(*(int32_t *)(intptr_t)(vm + 72) == 0x08000200);
    CHECK(*(int32_t *)(intptr_t)(*(int32_t *)(intptr_t)(vm+76)+60) != 0);
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
    {
        typedef void (__cdecl *compiler_handler)(int32_t, const char*, const char*, int32_t, int32_t);
        compiler_handler handler = (compiler_handler)(intptr_t)*(int32_t *)(intptr_t)(shared + 160);
        handler(vm, "bad token", "example.nut", 4, 7);
    }
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
        CHECK(kinoko_sqrat_get((void *)(intptr_t)(PTR(root)), "sharedArgument", (void *)(intptr_t)(object)));
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
        kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), object);
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
        kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(init)), "InitErrorActor");
        int32_t broken = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, 100, 200, -1, &(const KinokoOwnedObjectWords){PTR(&g16), 0x01000008, 1}, (const void *)(intptr_t)(0)));
        int32_t healthy = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, 300, 200, -1, &(const KinokoOwnedObjectWords){PTR(&g16), 0x01000008, 0}, (const void *)(intptr_t)(0)));
        CHECK(broken && healthy);
        *(unsigned char *)(intptr_t)(broken+40)=1;
        *(unsigned char *)(intptr_t)(healthy+40)=1;
        *(int32_t *)(intptr_t)(manager+64)=1;
        expected_vm_error=1;
        for(int frame=0;frame<64;++frame)
            kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
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
        kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), "replacementProbe", (const void *)(intptr_t)(healthy+44));
        CHECK(execute_source(vm,root+2,
            "replacementCalls <- 0;\n"
            "function ReplacementStep() { ::replacementCalls++; }\n"
            "replacementProbe.SetUpdateFunction(function() {\n"
            "SetUpdateFunction(::ReplacementStep); throw 123; });"));
        expected_vm_error=1;
        kinoko_actor_tick((KinokoActor *)(intptr_t)(healthy));
        expected_vm_error=0;
        for(int i=0;i<8;++i) kinoko_actor_tick((KinokoActor *)(intptr_t)(healthy));
        CHECK(execute_source(vm,root+2,
            "if(replacementCalls!=0) throw \"original failure retirement changed\";"));
        kinoko_script_clear_actors();
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(init))));
    }
    {
        int32_t transition[2]={g483,g484};
        if(kinoko_sqrat_get((void *)(intptr_t)(PTR(root)), "UpdateStageChange", (void *)(intptr_t)(transition))) {
            CHECK(execute_source(vm,root+2,
                "reentryLoads <- 0;\nreentrySaves <- 0;\n"
                "function DisableInput() {}\n"
                "function SavePlayerState() { ::reentrySaves++; }\n"
                "function LoadStage(name) { ::reentryLoads++; }\n"
                "stageNameNext=\"w0-s01a.act\"; stageChangeCount=0;\n"
                "SetGlobalUpdateFunction(UpdateStageChange);"));
            for(int frame=0;frame<120;++frame)
                CHECK(kinoko_script_callback_invoke((KinokoScriptCallback *)(intptr_t)(PTR(g612)))>=0);
            CHECK(execute_source(vm,root+2,
                "if(reentryLoads!=1 || reentrySaves!=1 || stageChangeCount!=-1) "
                "throw \"repeated stage reentry\";\nSetGlobalUpdateFunction(null);"));
            CHECK(execute_source(vm,root+2,
                "failedGlobalCalls <- 0;\n"
                "function FailedGlobal() { ::failedGlobalCalls++; throw 13; }\n"
                "SetGlobalUpdateFunction(FailedGlobal);"));
            int32_t old_mask=kinoko_game_masks.update;
            kinoko_game_masks.update=0;
            expected_vm_error=1;
            for(int frame=0;frame<64;++frame) kinoko_game_update();
            expected_vm_error=0;
            kinoko_game_masks.update=old_mask;
            CHECK(execute_source(vm,root+2,
                "if(failedGlobalCalls!=1) throw \"global error repeated\";"));
            CHECK(g612[6]==0);
            CHECK(*(int32_t *)(intptr_t)(vm+52)==base && *(int32_t *)(intptr_t)(vm+100)==frames);
            CHECK(function_48aa20(vm)==top);
        }
        kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), transition);
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
    CHECK(kinoko_sqrat_set_native_closure((struct SQVM *)(intptr_t)(vm), root + 2, "RelocateStack", (void *)(intptr_t)(PTR(relocate_vm_stack)), NULL, 0));
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
        kinoko_act_document_initialize((KinokoActDocument *)act);
        CHECK(kinoko_act_document_load((KinokoActDocument *)act, paths[round]));
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
            int32_t actor = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, round == 1 ? 330.0f : 3120.0f, round == 1 ? 543.0f : 895.0f, (float)direction, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
            CHECK(actor);
            kinoko_actor_set_take((KinokoActor *)(intptr_t)(actor), round == 1 ? 615 : 335);
            *(int32_t *)(intptr_t)(actor + 316) = 1;
            *(float *)(intptr_t)(actor + 256) = direction * 2.5f;
            *(float *)(intptr_t)(actor + 260) = -9.0f;
            kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager));
            for (int frame = 0; frame < 240; ++frame) {
                function_468620_this(PTR(g_514300_storage));
                kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
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
            kinoko_actor_release((KinokoActor *)(intptr_t)actor, NULL);
            kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager));
        }
        kinoko_script_clear_actors();
        CHECK(function_468950_this(PTR(g_514300_storage), manager));
        retdec_destroy_cact_object(PTR(act));
    }
    puts("PASS: native branch-return motion stays finite across 1440 contact frames");
    return 0;
}

static int test_map_transition(int32_t vm, int32_t *root) {
    const char *paths[]={"data/map/w1-c01a.act","data/map/w1-c01b.act","data/map/w1-c01a.act"};
    int32_t map_state=PTR(g_retdec_map_manager_state);
    kinoko_initialize_render_queue();
    kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR(g722)));
    kinoko_sqplus_object_assign((void *)(intptr_t)(PTR(g722)), (const void *)(intptr_t)(PTR(root+1)));
    CHECK(kinoko_map_manager_construct((KinokoMapManager *)(intptr_t)map_state));
    function_46fac0();
    function_4669d0();
    {
        int32_t class_environment[3];
        CHECK(execute_source(vm,root+2,"classFixture <- {Actor={},Camera=Camera};"));
        kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(class_environment)), "classFixture");
        CHECK(execute_asset(vm,class_environment+1,"data/script/class_def.cv4"));
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(class_environment))));
    }
    function_466270();
    CHECK(execute_asset(vm,root+2,"data/script/camera.cv4"));
    int32_t targets[]={PTR(kinoko_script_load_map),PTR(kinoko_script_clear_actors),PTR(kinoko_script_clear_collision),
        PTR(kinoko_script_clear_render_layers),PTR(kinoko_script_create_render_layer),PTR(kinoko_script_create_collision),PTR(kinoko_script_create_map_actors)};
    int32_t adapters[]={PTR(function_471d90),PTR(function_471bc0),PTR(function_471bc0),
        PTR(function_471bc0),PTR(function_471f10),PTR(function_471f10),PTR(function_471e50)};
    const char *names[]={"LoadMap","ClearActor","ClearCollision","ClearRenderLayer", "CreateRenderLayer",
        "CreateCollision","CreateActorFromMap"};
    for(int i=0;i<7;++i)
        CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR(names[i])), (const void *)(intptr_t)(PTR(targets+i)), 4, (void *)(intptr_t)(adapters[i]), 0))>=0);
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
        fprintf(stderr,"map actors=%d layers=%d\n",kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(PTR(g_retdec_actor_manager_state))),kinoko_render_queue_size());
        CHECK(execute_source(vm,root+2,
            "player <- {x=800.0,y=850.0,vx=2.5,vy=0.0,direction=1.0,hitBottom=1,\n"
            " left=792.0,right=808.0,top=818.0,bottom=850.0,take=100,\n"
            " user={hold=null,water=false,deadCount=0,take=0}};\nInitCamera(player);\n"));
        for(int frame=0;frame<180;++frame) {
            CHECK(execute_source(vm,root+2,"player.x+=2.5; player.left+=2.5; player.right+=2.5;"));
            CHECK(kinoko_camera_update((KinokoCamera *)g_retdec_camera_state, NULL)>=0);
            kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(PTR(g_retdec_actor_manager_state)), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
            CHECK(kinoko_map_manager_update((KinokoMapManager *)(intptr_t)map_state)>=0);
            int32_t manager=PTR(g_retdec_actor_manager_state);
            int32_t *actors=*(int32_t **)(intptr_t)(manager+100);
            for(int n=0;n<*(int32_t *)(intptr_t)(manager+116);++n) {
                int32_t actor=actors[n];
                int32_t animation_frame=*(int32_t *)(intptr_t)(actor+204);
                if(animation_frame) {
                    int32_t handle=*(int32_t *)(intptr_t)(animation_frame+4);
                    *(int32_t *)(intptr_t)(animation_frame+4)=1;
                    kinoko_actor_render((KinokoActor *)(intptr_t)(actor), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
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
    kinoko_script_clear_actors();
    kinoko_script_clear_collision();
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
    CHECK(kinoko_act_end_stage((KinokoActRuntime *)(intptr_t)runtime, NULL)==0);
    CHECK(*(int32_t *)(intptr_t)(runtime+64)==sprites);
    CHECK(*(int32_t *)(intptr_t)(runtime+68)==sprites+2*184);
    kinoko_script_release_map();
    int32_t retired_environment[2]={g483,g484};
    CHECK(!kinoko_sqrat_get((void *)(intptr_t)(PTR(root)), retired_name, (void *)(intptr_t)(retired_environment)));
    kinoko_script_clear_render_layers();
    CHECK(kinoko_render_queue_size()==0);
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
        kinoko_map_manager_prepare((KinokoMapManager *)manager, (KinokoCamera *)camera);
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
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(first)), "callbackFirst");
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(second)), "callbackSecond");
    kinoko_sqplus_object_copy_construct((void *)(intptr_t)(actor + 11), (const void *)(intptr_t)(PTR(root + 1)));
    kinoko_sqplus_object_copy_construct((void *)(intptr_t)(camera), (const void *)(intptr_t)(PTR(root + 1)));
    for (int i = 0; i < 3; ++i) {
        kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR(callbacks[i] + 1)));
        kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR(callbacks[i] + 4)));
    }
    /* Match the existing empty-function constructor, including its zero-type
       compatibility value; this migration does not normalize it to OT_NULL. */
    CHECK((int32_t)(intptr_t)(kinoko_script_callback_construct((KinokoScriptCallback *)(intptr_t)(PTR(empty)), (const char *)(intptr_t)(0))) == PTR(empty));
    empty_type = empty[5];
    empty_value = empty[6];
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(empty + 4))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(empty + 1))));
    int32_t first_refs = callback_external_refs(vm, first);
    int32_t second_refs = callback_external_refs(vm, second);
    int32_t root_refs = callback_external_refs(vm, root + 1);
    int32_t stack_top = function_48aa20(vm);
    CHECK(first[1] == 0x08000100 && second[1] == 0x08000100);
    CHECK(first_refs > 0 && second_refs > 0);
    for (int iteration = 0; iteration < 64; ++iteration) {
        int32_t *source = (iteration / 2) % 2 ? second : first;
        for (int i = 0; i < 3; ++i) {
            kinoko_sqplus_object_copy_construct((void *)(intptr_t)(argument), (const void *)(intptr_t)(PTR(source)));
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
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(callbacks[i] + 4))));
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(callbacks[i] + 1))));
    }
    CHECK(callback_external_refs(vm, first) == first_refs);
    CHECK(callback_external_refs(vm, second) == second_refs);
    CHECK(callback_external_refs(vm, root + 1) == root_refs);
    CHECK(function_48aa20(vm) == stack_top);
    CHECK(retdec_call_thiscall0_result(camera, kinoko_camera_update) == g483);
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(camera))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(actor + 11))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(second))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(first))));
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
    {
        int32_t map=kinoko_integer_map_create();
        int32_t slot=kinoko_integer_map_put(map,-7,12);
        CHECK(kinoko_integer_map_find(map,-7)==slot && *(int32_t*)(intptr_t)slot==12);
        CHECK(kinoko_integer_map_put(map,-7,19)==slot && kinoko_integer_map_size(map)==1);
        for(int i=0;i<256;++i) kinoko_integer_map_put(map,i,i*3);
        CHECK(kinoko_integer_map_find(map,-8)==map && *(int32_t*)(intptr_t)slot==19);
        kinoko_integer_map_clear(map);CHECK(kinoko_integer_map_size(map)==0);
        kinoko_integer_map_destroy(map);
    }

    {
        int32_t list=0;CHECK(retdec_act_make_list(&list));
        int32_t* head=(int32_t*)(intptr_t)list;
        for(int i=0;i<128;++i) CHECK(retdec_act_append_list(PTR(&list),i+1));
        int32_t token=head[0],previous=list;
        for(int i=0;i<128;++i) {
            int32_t* link=(int32_t*)(intptr_t)token;
            CHECK(link[1]==previous && link[2]==i+1);previous=token;token=link[0];
        }
        CHECK(token==list && head[1]==previous);kinoko_act_list_drop_storage(list);
    }
    int32_t manager[40] = {0};
    int32_t textures[2] = {7, 13}, iteration[3] = {0};
    kinoko_animation_list_construct(PTR(manager)+52);
    int32_t animation=kinoko_animation_create(2);
    kinoko_animation_adopt(PTR(manager)+52,animation);
    unsigned char *frames=*(unsigned char**)(intptr_t)(animation+8);
    CHECK(animation && frames);
    *(int32_t *)(frames + 244) = PTR(malloc(12));
    *(int32_t *)(frames + 248 + 244) = PTR(malloc(20));
    CHECK(*(int32_t *)(frames + 244) && *(int32_t *)(frames + 492));
    /* No live Actor in this fixture; the priority node is still reclaimed. */
    manager[10]=kinoko_integer_map_create();
    kinoko_integer_map_put(manager[10],42,animation);manager[11]=1;
    kinoko_integer_vector_construct(PTR(manager)+68);
    for(int i=0;i<2;++i) kinoko_integer_vector_append(PTR(manager)+68,textures[i]);
    kinoko_priority_construct((void *)(intptr_t)(PTR(manager)+84));
    int32_t absent=0,inserted[2];
    inserted[0]=PTR(kinoko_actor_priority_insert(manager+21,(KinokoActor *)(intptr_t)absent)); inserted[1]=1;
    manager[25] = PTR(iteration); manager[26] = manager[27] = PTR(iteration + 3);
    manager[29] = 8; ((unsigned char *)manager)[120] = 1;
    for (int repeat = 0; repeat < 2; ++repeat) {
        CHECK((int32_t)(intptr_t)(kinoko_actor_manager_clear_resources((KinokoActorManager *)(intptr_t)(PTR(manager)))) == PTR(iteration));
        CHECK(kinoko_integer_map_size(manager[10])==0);
        CHECK(manager[11] == 0 && manager[14] == 0 && manager[23] == 0);
        CHECK(kinoko_integer_vector_size(PTR(manager)+68)==0);
        CHECK(manager[26] == PTR(iteration) && manager[27] == PTR(iteration + 3));
        CHECK(manager[29] == 0 && ((unsigned char *)manager)[120] == 0);
    }
    kinoko_integer_vector_destroy(PTR(manager)+68);
    kinoko_animation_list_destroy(PTR(manager)+52);
    kinoko_integer_map_destroy(manager[10]);
    kinoko_priority_destroy((void *)(intptr_t)(PTR(manager)+84));
    {
        int32_t tree[3]={0},actors[4][58]={{0}},nodes[4],result[2];
        const int priorities[]={7,-2,7,7};
        kinoko_priority_construct((void *)(intptr_t)(PTR(tree)));
        for(int i=0;i<4;++i) {
            actors[i][57]=priorities[i];int32_t actor=PTR(actors[i]);
            nodes[i]=PTR(kinoko_actor_priority_insert_ordered(tree,(KinokoActor *)(intptr_t)actor,i==3)); result[0]=nodes[i]; result[1]=1;
        }
        const int order[]={1,3,0,2};int32_t node=(int32_t)(intptr_t)(kinoko_actor_priority_first((void *)(intptr_t)(PTR(tree))));
        for(int i=0;i<4;++i) { CHECK((int32_t)(intptr_t)(kinoko_actor_priority_value((void *)(intptr_t)(node)))==PTR(actors[order[i]]));node=(int32_t)(intptr_t)(kinoko_actor_priority_next((void *)(intptr_t)(PTR(tree)), (void *)(intptr_t)(node))); }
        CHECK(node==tree[1] && tree[2]==4);
        result[0]=PTR(kinoko_actor_priority_erase_next(tree,(void *)(intptr_t)nodes[3]));CHECK(tree[2]==3 && result[0]==nodes[0]);
        kinoko_priority_clear((void *)(intptr_t)(PTR(tree)));CHECK(tree[2]==0 && (int32_t)(intptr_t)(kinoko_actor_priority_first((void *)(intptr_t)(PTR(tree))))==tree[1]);
        kinoko_priority_destroy((void *)(intptr_t)(PTR(tree)));
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
    kinoko_actor_tick((KinokoActor *)(intptr_t)(PTR(actor)));
    CHECK(actor[53] == 0 && actor[54] == 1 && actor[51] == PTR(frames));
    kinoko_actor_tick((KinokoActor *)(intptr_t)(PTR(actor)));
    CHECK(actor[53] == 1 && actor[54] == 0 && actor[51] == PTR(frames + 1));
    kinoko_actor_tick((KinokoActor *)(intptr_t)(PTR(actor)));
    CHECK(actor[53] == 2 && actor[54] == 0 && actor[51] == PTR(frames + 2));
    kinoko_actor_tick((KinokoActor *)(intptr_t)(PTR(actor)));
    CHECK(actor[53] == 0 && actor[54] == 0 && actor[51] == PTR(frames));
    CHECK(actor[38] == actor[51]);

    *(uint8_t *)((char *)animation + 24) = 0;
    actor[53] = 2;
    actor[38] = actor[51] = PTR(frames + 2);
    for (int i = 0; i < 3; ++i) kinoko_actor_tick((KinokoActor *)(intptr_t)(PTR(actor)));
    CHECK(actor[53] == 2 && actor[54] == 0 && actor[51] == PTR(frames + 2));
    actor[54] = 17;
    kinoko_actor_advance_animation((KinokoActor *)(intptr_t)(PTR(actor)), 40);
    CHECK(actor[54] == 17 && actor[53] == 2);
    actor[51] = 0;
    kinoko_actor_tick((KinokoActor *)(intptr_t)(PTR(actor)));
    CHECK(actor[54] == 17);
    actor[51] = PTR(frames);
    actor[50] = 0;
    kinoko_actor_tick((KinokoActor *)(intptr_t)(PTR(actor)));
    CHECK(actor[54] == 18 && actor[53] == 2);
    actor[50] = PTR(animation);
    animation[3] = animation[2];
    kinoko_actor_tick((KinokoActor *)(intptr_t)(PTR(actor)));
    CHECK(actor[54] == 19 && actor[53] == 2);

    animation[3] = PTR(frames + 3);
    actor[54] = INT32_MAX;
    kinoko_actor_tick((KinokoActor *)(intptr_t)(PTR(actor)));
    CHECK(actor[54] == INT32_MIN && actor[53] == 2);
    actor[54] = 1;
    actor[53] = 99;
    kinoko_actor_tick((KinokoActor *)(intptr_t)(PTR(actor)));
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
    CHECK(kinoko_actor_set_init_data((KinokoActor *)actor, source) == (KinokoActor *)actor);
    CHECK(memcmp(actor, expected, sizeof(actor)) == 0);
    CHECK(kinoko_actor_set_init_data((KinokoActor *)actor, 0) == 0);
    CHECK(kinoko_actor_set_init_data(0, source) == 0);
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
        kinoko_sqplus_object_copy_construct((void *)(intptr_t)(incoming), (const void *)(intptr_t)(source + 44));
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
    kinoko_sqplus_object_copy_construct((void *)(intptr_t)(incoming), (const void *)(intptr_t)(source + 44));
    retdec_call_thiscall3_result((void *)(intptr_t)target, kinoko_actor_sync_animation,
        incoming[0], incoming[1], incoming[2]);
    CHECK(*(int32_t *)(intptr_t)(target + 212) == 0);
    CHECK(*(int32_t *)(intptr_t)(target + 204) == animation[2]);

    *(int32_t *)(intptr_t)(target + 200) = 0;
    *(int32_t *)(intptr_t)(source + 216) = 999;
    kinoko_sqplus_object_copy_construct((void *)(intptr_t)(incoming), (const void *)(intptr_t)(source + 44));
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
    kinoko_sqplus_object_copy_construct((void *)(intptr_t)(incoming), (const void *)(intptr_t)(target + 44));
    retdec_call_thiscall3_result((void *)(intptr_t)target, kinoko_actor_sync_animation,
        incoming[0], incoming[1], incoming[2]);
    CHECK(*(int32_t *)(intptr_t)(target + 212) == 1);
    CHECK(*(int32_t *)(intptr_t)(target + 204) == PTR(frames + 496));

    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root + 1)), "syncTarget", (const void *)(intptr_t)(target + 44));
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root + 1)), "syncSource", (const void *)(intptr_t)(source + 44));
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

static int test_act_layer_access(void) {
    /* Deliberately independent raw ABI fixtures: no production schema offsets.
       Stack-owned records must survive freeing each temporary heap wrapper. */
    int32_t runtime[48] = {0}, document[60] = {0}, layer[50] = {0};
    int32_t layout[2] = {123, 456}, key[2] = {7, PTR(layout)};
    int32_t second_key[2] = {8, PTR(layout + 1)};
    int32_t head[3] = {0}, first[3] = {0}, second[3] = {0};
    int32_t layers[2] = {PTR(layer), 0}, document_holder = PTR(document);
    int32_t layer_holder = PTR(layer), output = 99;
    runtime[4] = PTR(&document_holder);
    document[52] = PTR(layers);
    document[53] = PTR(layers + 2);
    layer[45] = PTR(head); layer[46] = 2;
    head[0] = PTR(first); head[1] = PTR(second);
    first[0] = PTR(second); first[1] = PTR(head); first[2] = PTR(key);
    second[0] = PTR(head); second[1] = PTR(first); second[2] = PTR(second_key);
    CHECK(kinoko_act_first_key(NULL, 0) == 0);
    CHECK(kinoko_act_first_key((KinokoActRuntime *)runtime, 0) == 0); /* inactive */
    *((unsigned char *)runtime + 8) = 1;
    for (int i = 0; i < 1000; ++i) {
        CHECK(kinoko_act_first_key((KinokoActRuntime *)runtime, 0) == (KinokoActKey *)key);
        CHECK(kinoko_act_layer_layout((KinokoActRuntime *)runtime, 0) == (KinokoActLayout *)layout);
    }
    CHECK(key[0] == 7 && layout[0] == 123 && first[2] == PTR(key));
    CHECK(kinoko_act_first_key((KinokoActRuntime *)runtime, -1) == 0);
    CHECK(kinoko_act_first_key((KinokoActRuntime *)runtime, 1) == 0); /* null layer */
    CHECK(kinoko_act_first_key((KinokoActRuntime *)runtime, 2) == 0); /* past end */
    layer[49] = 1;
    CHECK(kinoko_act_first_key((KinokoActRuntime *)runtime, 0) == 0); /* extra tracks */
    layer[49] = 0; layer[46] = 0;
    CHECK(kinoko_act_first_key((KinokoActRuntime *)runtime, 0) == 0); /* empty keys */
    layer[46] = 2; first[2] = 0;
    CHECK(kinoko_act_first_key((KinokoActRuntime *)runtime, 0) == 0); /* null first key */
    first[2] = PTR(key);
    CHECK(kinoko_act_key_holder((KinokoActLayerHolder *)&layer_holder, 1, (KinokoActKeyHolder **)&output) == (KinokoActKeyHolder **)&output);
    CHECK(output && *(int32_t *)(intptr_t)output == PTR(second_key));
    free((void *)(intptr_t)output);
    CHECK(kinoko_act_key_holder((KinokoActLayerHolder *)&layer_holder, 2, (KinokoActKeyHolder **)&output) == (KinokoActKeyHolder **)&output);
    CHECK(output == 0);
    CHECK(kinoko_act_key_holder((KinokoActLayerHolder *)1, -1, (KinokoActKeyHolder **)&output) == (KinokoActKeyHolder **)&output);
    CHECK(output == 0); /* reject index before touching invalid holder */
    CHECK(kinoko_act_layer_holder((KinokoActSourceHolder *)1, -1, (KinokoActLayerHolder **)&output) == (KinokoActLayerHolder **)&output);
    CHECK(output == 0);
    CHECK(kinoko_act_layer_holder((KinokoActSourceHolder *)&document_holder, 0, 0) == 0);
    CHECK(kinoko_act_key_holder((KinokoActLayerHolder *)&layer_holder, 0, 0) == 0);
    head[0] = 0;
    CHECK(kinoko_act_first_key((KinokoActRuntime *)runtime, 0) == 0); /* missing node */
    head[0] = PTR(first); first[0] = 0;
    CHECK(kinoko_act_key_holder((KinokoActLayerHolder *)&layer_holder, 1, (KinokoActKeyHolder **)&output) == (KinokoActKeyHolder **)&output);
    CHECK(output == 0); /* broken chain guard retained */
    document[53] = document[52] - 4;
    CHECK(kinoko_act_first_key((KinokoActRuntime *)runtime, 0) == 0); /* reversed vector */
    document_holder = 0;
    CHECK(kinoko_act_first_key((KinokoActRuntime *)runtime, 0) == 0);
    runtime[4] = 0;
    CHECK(kinoko_act_first_key((KinokoActRuntime *)runtime, 0) == 0);
    puts("PASS: ACT layer/key query bounds, borrowed records and temporary holder ownership");
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
    kinoko_act_set_current_time((KinokoActRuntime *)(intptr_t)address, NULL, -31);
    CHECK(kinoko_act_get_current_frame((KinokoActRuntime *)(intptr_t)address, NULL) == -3);
    words[1] = INT32_MAX - 4;
    kinoko_act_increment_frame((KinokoActRuntime *)(intptr_t)address, NULL);
    CHECK(words[1] == INT32_MIN + 5);
    act[1] = 0;
    CHECK(kinoko_act_get_current_frame((KinokoActRuntime *)(intptr_t)address, NULL) == 0);
    words[0] = 0;
    CHECK(kinoko_act_increment_frame((KinokoActRuntime *)(intptr_t)address, NULL) == 0);
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
    /* Draw storage now owns native containers, not three raw vector words.
       Populate both through their APIs so EndStage clears real elements. */
    int32_t texture[25] = {0};
    texture[0] = PTR(kinoko_act_host_symbols()->texture_resource_vtable);
    texture[17] = 7;
    CHECK(retdec_act_bitblt_this(address, 3, 4, 32, 16,
        PTR(texture), 0, 0, 0, 1.0f) == 0);
    function_452c20(address + 60, 2);
    KinokoDrawSpan commands_before = kinoko_act_command_span(address);
    KinokoDrawSpan sprites_before = kinoko_act_sprite_span(address);
    CHECK(commands_before.end > commands_before.begin);
    CHECK(sprites_before.end > sprites_before.begin);
    const int32_t commands_owner = words[11], sprites_owner = words[15];
    words[38] = 4321;
    CHECK(retdec_call_thiscall0_result(resource, kinoko_act_end_stage) == 0);
    KinokoDrawSpan commands_after = kinoko_act_command_span(address);
    KinokoDrawSpan sprites_after = kinoko_act_sprite_span(address);
    CHECK(resource[8] == 0);
    CHECK(commands_after.end == commands_after.begin);
    CHECK(sprites_after.end == sprites_after.begin);
    CHECK(words[11] == commands_owner && words[15] == sprites_owner);
    CHECK(commands_after.capacity - commands_after.begin == commands_before.capacity - commands_before.begin);
    CHECK(sprites_after.capacity - sprites_after.begin == sprites_before.capacity - sprites_before.begin);
    for (int i = 108; i < 152; ++i) CHECK(resource[i] == 0);
    CHECK(words[38] == 4321);
    CHECK(kinoko_act_end_stage((KinokoActRuntime *)(intptr_t)address, NULL) == (int32_t)E_FAIL);
    kinoko_act_draw_storage_destroy(address);
    CHECK(words[11] == 0 && words[15] == 0);
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
    IDirect3DDevice9 *saved_device = kinoko_graphics.device;

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
    kinoko_graphics.device = 0;
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
    kinoko_graphics.device = saved_device;
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

static int native_instance_releases;
static int native_instance_release_pointer;
static int32_t native_instance_release(int32_t pointer, int32_t size) {
    (void)size;
    ++native_instance_releases;
    native_instance_release_pointer = pointer;
    return 0;
}

static int test_native_instance_receivers(int32_t vm, int32_t *root) {
    const int top = function_48aa20(vm);
    const int32_t pointer = 0x12345678;
    int32_t instance[3], types[3];
    CHECK(execute_source(vm, root + 2,
        "nativeConstructorCalls <- 0;\n"
        "class NativeBaseA {}\n"
        "class NativeBaseB {}\n"
        "class NativeProbe { __ot=null; __ca=[NativeBaseA,NativeBaseB]; constructor() { ++nativeConstructorCalls; } }\n"
        "NativeProbe.__ca.append(NativeProbe);\n"
        "class NativeEmpty { __ot=null; __ca=[]; }\n"
        "class NativeSingle { __ot=null; __ca=[NativeBaseA]; }\n"));
    const char *names[] = {"NativeBaseA", "NativeBaseB", "NativeProbe"};
    for (int i=0; i<3; ++i) {
        function_48a670(vm);
        function_48a480(vm, PTR(names[i]), -1);
        CHECK(function_48ce70(vm, -2) == 0);
        CHECK(function_48c780(vm, -1, 100+i) == 0);
        function_48c910(vm, top);
    }
    CHECK(function_4ab170(vm, PTR("MissingNativeClass"), pointer, 0) == 0);
    CHECK(function_48aa20(vm) == top);
    function_48ac70(vm);
    CHECK(function_4ab170(vm, PTR("nativeConstructorCalls"), pointer, 0) == 0);
    CHECK(function_48aa20(vm) == top);
    function_48ac70(vm);
    native_instance_releases = 0;
    CHECK(function_4ab170(vm, PTR("NativeProbe"), pointer, PTR(native_instance_release)) == 1);
    CHECK(function_48aa20(vm) == top+1);
    int32_t *slot=(int32_t *)(intptr_t)function_491880_this(vm, -1);
    CHECK(slot[0] == 0x0a008000);
    // Only the stack owns the returned instance; all temporary external refs are gone.
    CHECK(*(int32_t *)(intptr_t)(slot[1]+4) == 1);
    CHECK(*(int32_t *)(intptr_t)(slot[1]+32) == pointer);
    kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR(instance)));
    kinoko_sqplus_object_capture((void *)(intptr_t)(PTR(instance)), -1);
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(instance)), (void *)(intptr_t)(PTR(types)), "__ot");
    CHECK(kinoko_squirrel_object_size(PTR(types), vm) == 3);
    int32_t keys[] = {kinoko_native_void_type(), 100, 101};
    for(int i=0; i<3; ++i) {
        function_48ab90(vm, types[1], types[2]);
        function_48a4f0(vm, keys[i]);
        CHECK(function_48ce00(vm, -2) == 0);
        slot=(int32_t *)(intptr_t)function_491880_this(vm, -1);
        CHECK(slot[1] == pointer);
        function_48aa30(vm, 2);
    }
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(types))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(instance))));
    function_48c910(vm, top);
    CHECK(native_instance_releases == 1 && native_instance_release_pointer == pointer);
    CHECK(execute_source(vm, root+2, "if(nativeConstructorCalls!=0) throw 130;\n"));
    const char *short_names[] = {"NativeEmpty", "NativeSingle"};
    for(int i=0; i<2; ++i) {
        CHECK(function_4ab170(vm, PTR(short_names[i]), pointer, 0) == 1);
        kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR(instance)));
        kinoko_sqplus_object_capture((void *)(intptr_t)(PTR(instance)), -1);
        kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(instance)), (void *)(intptr_t)(PTR(types)), "__ot");
        CHECK(kinoko_squirrel_object_size(PTR(types), vm) == 1);
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(types))));
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(instance))));
        function_48c910(vm, top);
    }
    puts("PASS: native instance type maps, skipped constructor, failure stack and external-reference lifetime");
    return 0;
}

static int32_t render_queue_visits[4], render_queue_visit_count;
static int32_t __fastcall visit_render_queue(int32_t *object, void *unused, int32_t camera) {
    (void)unused;
    if (render_queue_visit_count < 4)
        render_queue_visits[render_queue_visit_count++] = object[1] + camera;
    return 0;
}
static int test_global_callback_destructor(int32_t vm) {
    int32_t saved_callback[7];
    const int top = function_48aa20(vm);
    CHECK(function_4d3e50() == 0);
    CHECK(kinoko_render_queue_size() == 0);
    CHECK(kinoko_render_queue_first() == kinoko_render_queue_identity());
    {
        int32_t table[1]={PTR(visit_render_queue)};
        int32_t a[2]={PTR(table),7}, b[2]={PTR(table),3};
        int32_t a_ptr=PTR(a), b_ptr=PTR(b), null_ptr=0;
        CHECK(function_46a210(&a_ptr));
        CHECK(function_46a210(&b_ptr));
        CHECK(function_46a210(&a_ptr));
        CHECK(function_46a210(&null_ptr));
        CHECK(kinoko_render_queue_size()==4);
        render_queue_visit_count=0;
        kinoko_draw_render_queue(100);
        CHECK(render_queue_visit_count==3 && render_queue_visits[0]==107 &&
              render_queue_visits[1]==103 && render_queue_visits[2]==107);
        kinoko_script_clear_render_layers();
        CHECK(kinoko_render_queue_size()==0 && a[1]==7 && b[1]==3);
    }

    memcpy(saved_callback, g612, sizeof(saved_callback));
    g612[0] = vm;
    kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR(g612 + 1)));
    kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR(g612 + 4)));
    native_instance_releases = 0;
    for(int member=0; member<2; ++member) {
        CHECK(function_4ab170(vm, PTR("NativeEmpty"), 21+member,
                             PTR(native_instance_release)) == 1);
        kinoko_sqplus_object_capture((void *)(intptr_t)(PTR(g612+1+3*member)), -1);
        function_48c910(vm, top);
    }
    CHECK(native_instance_releases == 0);
    function_4d4860();
    CHECK(native_instance_releases == 2);
    CHECK(native_instance_release_pointer == 21); // Environment released last.
    CHECK(g612[0] == vm);
    CHECK(g612[1] == PTR(&g16) && g612[4] == PTR(&g16));
    CHECK(g612[2] == 0x01000001 && g612[3] == 0);
    CHECK(g612[5] == 0x01000001 && g612[6] == 0);
    function_4d4860();
    CHECK(native_instance_releases == 2 && function_48aa20(vm) == top);
    memcpy(g612, saved_callback, sizeof(saved_callback));
    puts("PASS: original global callback destruction order, external owners, null reset and render sentinel");
    return 0;
}

struct compile_feed { const char *text; int offset; int32_t vm; };
static int32_t compiler_test_feed(int32_t context) {
    struct compile_feed *feed=(struct compile_feed *)(intptr_t)context;
    if(retdec_stack_vm()!=feed->vm) return 0;
    return feed->text[feed->offset] ? feed->text[feed->offset++] : 0;
}
static int compile_error_calls, compile_error_valid;
static int32_t compile_error_vm;
static const char *compile_error_source = "callback source";
static void compiler_test_error(int32_t vm, const char *error, const char *source, int32_t line, int32_t column) {
    ++compile_error_calls;
    compile_error_valid = vm == compile_error_vm && retdec_stack_vm() == vm &&
        error && *error && strcmp(source,compile_error_source)==0 && line>0 && column>0;
}
static int test_compiler_receivers(int32_t vm, int32_t *root) {
    const int top=function_48aa20(vm);
    int32_t shared=*(int32_t *)(intptr_t)(vm+140);
    int32_t previous_handler=*(int32_t *)(intptr_t)(shared+160);
    compile_error_vm=vm;
    compile_error_calls=compile_error_valid=0;
    function_48afa0(vm,PTR(compiler_test_error));
    CHECK(function_48d0b0(vm,PTR("local =;"),8,(int32_t *)"callback source",0)==-1);
    CHECK(compile_error_calls==0 && function_48aa20(vm)==top);
    CHECK(function_48d0b0(vm,PTR("local =;"),8,(int32_t *)"callback source",1)==-1);
    CHECK(compile_error_calls==1 && compile_error_valid && function_48aa20(vm)==top);
    CHECK(*(int32_t *)(intptr_t)(vm+64)==0x08000010);
    function_48afa0(vm,previous_handler);
    // Length-limited source must ignore trailing bytes, unlike the old lost buffer state.
    CHECK(function_48d0b0(vm,PTR("return 42;INVALID"),10,(int32_t *)"bounded source",0)==0);
    function_48ab90(vm,root[2],root[3]);
    CHECK(function_48ace0(vm,1,1,0)==0);
    int32_t *value=(int32_t *)(intptr_t)function_491880_this(vm,-1);
    CHECK(value[0]==0x05000002 && value[1]==42);
    function_48c910(vm,top);
    struct compile_feed feed={"return 73;",0,vm};
    CHECK(function_48c1f0(vm,PTR(compiler_test_feed),(int32_t *)&feed,PTR("reader source"),0)==0);
    function_48ab90(vm,root[2],root[3]);
    CHECK(function_48ace0(vm,1,1,0)==0);
    value=(int32_t *)(intptr_t)function_491880_this(vm,-1);
    CHECK(value[0]==0x05000002 && value[1]==73);
    function_48c910(vm,top);
    expected_vm_error=1;
    int compile_result=execute_source(vm,root+2,
        "compilerFactory <- compilestring(\"const CompilerSaved=31;\\n enum CompilerEnum { first=7, second=9 }\\n return function(x) { return x+CompilerSaved+CompilerEnum.second; };\",\"factory source\");\n"
        "compilerClosure <- compilerFactory();\n"
        "if(compilerClosure(2)!=42) throw 120;\n"
        "local later=compilestring(\"return CompilerSaved+CompilerEnum.first;\");\n"
        "if(later()!=38) throw 121;\n"
        "local caught=false;\n"
        "try { compilestring(\"local =;\",\"bad source\"); } catch(e) { caught=typeof e==\"string\"; }\n"
        "if(!caught) throw 122;\n"
        "collectgarbage();\n"
        "if(compilerClosure(3)!=43) throw 123;\n"
        "if(compilestring(\"return CompilerEnum.second;\")()!=9) throw 124;\n");
    expected_vm_error=0;
    CHECK(compile_result);
    {
        const char *text = "sameVmCompile <- CompilerSaved + CompilerEnum.first;";
        int32_t script[26] = {0};
        script[23] = PTR(text); script[24] = (int32_t)strlen(text);
        CHECK(retdec_execute_act_source_script(vm, PTR(script), root+2));
        CHECK(execute_source(vm, root+2, "if(sameVmCompile!=38) throw 125;\n"));
        const char *bad = "local =;";
        script[23] = PTR(bad); script[24] = (int32_t)strlen(bad);
        expected_vm_error = 1;
        compile_error_source = "";
        compile_error_calls = compile_error_valid = 0;
        function_48afa0(vm, PTR(compiler_test_error));
        CHECK(!retdec_execute_act_source_script(vm, PTR(script), root+2));
        CHECK(compile_error_calls == 1 && compile_error_valid);
        function_48afa0(vm, previous_handler);
        compile_error_source = "callback source";
        expected_vm_error = 0;
        CHECK(sq_gettop(kinoko_vm(vm)) == top);
    }
    CHECK(function_48aa20(vm)==top);
    CHECK(retdec_explicit_vm==0);
    puts("PASS: source compiler current-VM constants/enums, callbacks, errors, closures and GC");
    return 0;
}

static int test_act_script_source_registration(int32_t vm, int32_t *root) {
    const int top = sq_gettop(kinoko_vm(vm));
    int32_t script[26] = {0}, environment[2] = {g483,g484}, wrapper[5] = {0};
    int32_t extension[7]; memcpy(extension, kinoko_act_script_extension, sizeof(extension));
    int32_t old_archive = kinoko_archive_count;
    CHECK(g556 == 15); /* Script extension has not been configured by WinMain. */
    retdec_string_assign_n(&g554, ".cv4", 4);
    CHECK(g555 == 4 && g556 >= 16 && strcmp(retdec_std_string_data(PTR(&g554)), ".cv4") == 0);
    CHECK(kinoko_sqrat_new_table((struct SQVM *)(intptr_t)(vm), environment));
    wrapper[0] = kinoko_sqrat_object_vtable(); wrapper[1] = vm;
    wrapper[2] = environment[0]; wrapper[3] = environment[1];
    script[21] = 15;
    CHECK(retdec_register_act_script(PTR(script), PTR(wrapper)) == 0); /* No source yet. */
    const char *initial = "counter <- 0;\n function Init() { counter += 1; }\n";
    script[23] = PTR(_strdup(initial)); script[24] = (int32_t)strlen(initial);
    ((unsigned char*)script)[100] = 1;
    CHECK(function_415fd0(PTR(script), PTR(wrapper)) == 0);
    kinoko_archive_count = 0;
    char actual[MAX_PATH], requested[MAX_PATH], command[512];
    sprintf_s(actual,sizeof(actual),"act-source-%lu.cv4",GetCurrentProcessId());
    sprintf_s(requested,sizeof(requested),"act-source-%lu.nut",GetCurrentProcessId());
    FILE *file = NULL;
    CHECK(fopen_s(&file,actual,"wb") == 0);
    const char *source = "counter += 10;\n function Init() { counter += 20; }\n";
    CHECK(fwrite(source,1,strlen(source),file) == strlen(source)); fclose(file);
    sprintf_s(command,sizeof(command),"if(!CompileFile(\"%s\", this)) throw 1;\n",requested);
    CHECK(execute_source(vm, environment, command));
    CHECK(retdec_execute_act_callback(PTR(script),4,"test:act-compile-file"));
    CHECK(execute_source(vm, environment,"if(counter!=30) throw 2;\n"));
    unsigned char *bytecode = NULL; int32_t bytecode_size = 0;
    const char *increment = "counter += 1;";
    CHECK(retdec_squirrel_compile_source(increment,(int32_t)strlen(increment),"ACT file",&bytecode,&bytecode_size));
    sprintf_s(actual,sizeof(actual),"act-bytecode-%lu.cv4",GetCurrentProcessId());
    sprintf_s(requested,sizeof(requested),"act-bytecode-%lu.nut",GetCurrentProcessId());
    CHECK(fopen_s(&file,actual,"wb") == 0);
    CHECK(fwrite(bytecode,1,bytecode_size,file) == (size_t)bytecode_size); fclose(file); free(bytecode);
    sprintf_s(command,sizeof(command),"if(!CompileFile(\"%s\", this)) throw 3;\n if(counter!=32) throw 4;\n",requested);
    CHECK(execute_source(vm, environment, command));
    CHECK(execute_source(vm, environment,"if(CompileFile(\"missing.nut\")) throw 5;\n"));
    retdec_destroy_cact_script(PTR(script));
    kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), environment);
    memcpy(kinoko_act_script_extension, extension, sizeof(extension)); kinoko_archive_count = old_archive;
    CHECK(sq_gettop(kinoko_vm(vm)) == top);
    puts("PASS: original ACT script registration, extension rewrite, callback refresh and bytecode double execution");
    return 0;
}

static int test_csv_receivers(int32_t vm, int32_t *root) {
    const int top=function_48aa20(vm);
    CHECK(kinoko_csv_populate(vm,
        "# ignored\r\nid,n,f,b,s\r\n,i,f,b,s\r\n"
        "csvA,-12,1.25,true,\"hello\"\r\n"
        "csvMissing,7\n"
        "csvA,19,2.5,True,replaced,ignored\n"
        "csvUncommitted,8,3,t,end",root+2)==0);
    CHECK(execute_source(vm,root+2,
        "if(csvA.n!=19 || csvA.f!=2.5 || csvA.b!=false || csvA.s!=\"replaced\") throw 110;\n"
        "if(csvMissing.n!=7 || csvMissing.f!=0.0 || csvMissing.b!=false || csvMissing.s!=\"\") throw 111;\n"
        "if(\"csvUncommitted\" in this) throw 112;\n"));
    CHECK(kinoko_csv_populate(vm,
        "id,a,b\n,s,s\n"
        "csvComma,\"left,right\"\n"
        "csvLine,\"one\r\ntwo\",ok\n"
        "csvCarry,part#ignored\nrest,done\n",root+2)==0);
    CHECK(execute_source(vm,root+2,
        "if(csvComma.a!=\"left\" || csvComma.b!=\"right\") throw 113;\n"
        "if(csvLine.a!=\"one\\r\\ntwo\" || csvLine.b!=\"ok\") throw 114;\n"
        "if(csvCarry.a!=\"partrest\" || csvCarry.b!=\"done\") throw 115;\n"));
    CHECK(kinoko_csv_populate(vm,"id,a,b\n,i\nnever,1,2\n",root+2)==2);
    int32_t null_pair[2]={g483,g484};
    CHECK(kinoko_csv_populate(vm,"",null_pair)==1);

    // Exercise the actual four-word callback ABI and resource reader in both modes.
    const char *fixture="id,value\n,i\ncsvFile,47\n";
    const char saved_encoding=g874;
    const int32_t saved_package=kinoko_archive_count;
    kinoko_archive_count=0;
    for(int encoded=0;encoded<2;++encoded) {
        char path[MAX_PATH], actual[MAX_PATH];
        CHECK(GetTempFileNameA(".","csv",0,path)!=0);
        strcpy_s(actual,sizeof actual,path);
        if(encoded) strcpy_s(actual+strlen(actual)-4,5,".cv1");
        FILE *file=NULL;
        CHECK(fopen_s(&file,actual,"wb")==0);
        unsigned char key=0x8b,step=0x71;
        for(size_t i=0;i<strlen(fixture);++i) {
            unsigned char ch=(unsigned char)fixture[i];
            if(encoded) { ch^=key; key=(unsigned char)(key+step); step=(unsigned char)(step-0x6b); }
            fputc(ch,file);
        }
        fclose(file);
        const int32_t refs=*(int32_t *)(intptr_t)(root[3]+4);
        g874=(char)encoded;
        function_48ab90(vm,root[2],root[3]);
        function_48a480(vm,PTR(path),-1);
        function_48ab90(vm,root[2],root[3]);
        CHECK(function_471160(PTR(function_403000),vm,2)==1);
        int32_t *result=(int32_t *)(intptr_t)function_491880_this(vm,-1);
        CHECK(result[0]==0x01000008 && result[1]==1);
        function_48c910(vm,top);
        CHECK(*(int32_t *)(intptr_t)(root[3]+4)==refs);
        CHECK(execute_source(vm,root+2,"if(csvFile.value!=47) throw 116;\n"));
    }
    g874=saved_encoding;
    kinoko_archive_count=saved_package;
    CHECK(function_48aa20(vm)==top);
    puts("PASS: original CSV quirks, typed rows, callback ownership and plain/encrypted readers");
    return 0;
}

static int test_thread_receivers(int32_t vm, int32_t *root) {
    const int top = function_48aa20(vm);
    const int base = *(int32_t *)(intptr_t)(vm+52);
    // A native name owns its string independently of a root-table key.
    function_48a480(vm, PTR("native-name-owned"), -1);
    int32_t *key=(int32_t *)(intptr_t)function_491880_this(vm,-1);
    int32_t name=key[1];
    int32_t refs=*(int32_t *)(intptr_t)(name+4);
    function_48d850(vm,PTR(kinoko_sq_noop_constructor),0);
    CHECK(function_48c580(vm,-1,PTR("native-name-owned"))==0);
    CHECK(*(int32_t *)(intptr_t)(name+4)==refs+1);
    CHECK(function_48c580(vm,-1,PTR("native-name-owned"))==0);
    CHECK(*(int32_t *)(intptr_t)(name+4)==refs+1);
    CHECK(function_48c580(vm,-1,PTR("native-name-replaced"))==0);
    CHECK(*(int32_t *)(intptr_t)(name+4)==refs);
    function_48aa30(vm,2);
    CHECK(execute_source(vm,root+2,
        "threadShared <- {tag=29};\n"
        "threadSteps <- 0;\n"
        "threadProbe <- newthread(function(...) {\n"
        " ::threadSteps++;\n"
        " local first=suspend(vargv[0]);\n"
        " if(first!=::threadShared || vargv[0]!=::threadShared) throw 91;\n"
        " ::threadSteps++;\n"
        " local second=suspend(17);\n"
        " if(second!=null) throw 92;\n"
        " ::threadSteps++; return 23;\n"
        "});\n"
        "if(threadProbe.getstatus()!=\"idle\") throw 93;\n"
        "if(threadProbe.call(threadShared)!=threadShared || threadSteps!=1 || "
        "threadProbe.getstatus()!=\"suspended\") throw 94;\n"
        "if(threadProbe.wakeup(threadShared)!=17 || threadSteps!=2) throw 95;\n"
        "if(threadProbe.wakeup()!=23 || threadSteps!=3 || threadProbe.getstatus()!=\"idle\") throw 96;\n"
        "threadTrap <- newthread(function() {\n"
        " try { local x=suspend(1); throw x; } catch(e) { return e; }\n"
        "});\n"
        "if(threadTrap.call()!=1 || threadTrap.wakeup(threadShared)!=threadShared) throw 97;\n"));
    expected_vm_error=1;
    int result=execute_source(vm,root+2,
        "threadFailure <- newthread(function() { local x=suspend(2); throw x; });\n"
        "if(threadFailure.call()!=2) throw 98;\n"
        "local caught=0;\n"
        "try { threadFailure.wakeup(threadShared); } catch(e) { if(e!=threadShared) throw 99; caught++; }\n"
        "try { threadFailure.wakeup(); } catch(e) { if(e!=\"cannot wakeup a idle thread\") throw e; caught++; }\n"
        "try { local f=function() { local x=suspend(); }; f.call(this); } catch(e) { if(e!=\"cannot suspend through native calls/metamethods\") throw e; caught++; }\n"
        "if(caught!=3) throw 100;\n");
    expected_vm_error=0;
    if (!result) {
        int32_t *error=(int32_t *)(intptr_t)(vm+64);
        fprintf(stderr,"thread failure type=%08x data=%08x top=%d base=%d frames=%d selected=%08x\n",
            error[0],error[1],function_48aa20(vm),*(int32_t *)(intptr_t)(vm+52),
            *(int32_t *)(intptr_t)(vm+100),retdec_explicit_vm);
        if(error[0]==0x08000010) fprintf(stderr,"thread error: %s\n",(char *)(intptr_t)(error[1]+28));
    }
    CHECK(result);
    CHECK(function_48aa20(vm)==top && *(int32_t *)(intptr_t)(vm+52)==base);
    puts("PASS: thread suspend/wakeup results, varargs/traps, errors and parent VM restoration");
    return 0;
}

static int test_receiver_operations(int32_t vm, int32_t *root) {
    int top = function_48aa20(vm);
    CHECK(execute_source(vm, root+2,
        "cloneShared <- {value=7};\n"
        "cloneArray <- [cloneShared,2];\n"
        "cloneCopy <- clone cloneArray;\n"
        "cloneCopy[1]=8;\n"
        "if(cloneCopy==cloneArray || cloneArray[1]!=2 || cloneCopy[0]!=cloneShared) throw 81;\n"
        "class ReceiverClone { value=0; shared=null;\n"
        " function _cloned(other) { value=other.value+1; shared=other.shared; }\n"
        "}\n"
        "cloneInstance <- ReceiverClone();\n"
        "cloneInstance.value=12; cloneInstance.shared=cloneShared;\n"
        "clonedInstance <- clone cloneInstance;\n"
        "if(clonedInstance==cloneInstance || clonedInstance.value!=13 || "
        "clonedInstance.shared!=cloneShared || cloneInstance.value!=12) throw 82;\n"
        "cloneTable <- delegate { _cloned=function(other) { value=other.value+2; } } : {value=3};\n"
        "clonedTable <- clone cloneTable;\n"
        "if(clonedTable.value!=5 || cloneTable.value!=3) throw 83;\n"
        "class ReceiverIterator { entry=cloneShared;\n"
        " function _nexti(previous) { return previous==null ? \"entry\" : null; }\n"
        "}\n"
        "iteratorObject <- ReceiverIterator();\n"
        "for(local round=0;round<32;round++) {\n"
        " local count=0; foreach(k,v in iteratorObject) {\n"
        "  if(k!=\"entry\" || v!=cloneShared) throw 84; count++;\n"
        " }\n"
        " if(count!=1) throw 85;\n"
        "}\n"
        "local textCount=0; foreach(k,v in \"abc\") { if(v!=97+k) throw 86; textCount++; }\n"
        "if(textCount!=3) throw 87;\n"
        "function ReceiverVarargs(...) { return vargv[0.9]; }\n"
        "if(ReceiverVarargs(cloneShared)!=cloneShared) throw 88;\n"));
    int32_t array[3];
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(array)), "cloneArray");
    int32_t methods[2]={0,PTR(release_error_probe)};
    int32_t old[3]={PTR(methods),1,0};
    int32_t destination[2]={0x08000080,PTR(old)};
    int releases=error_releases;
    CHECK(kinoko_sq_clone(vm,PTR(array+1),PTR(destination)));
    CHECK(error_releases==releases+1 && destination[0]==0x08000040);
    CHECK(destination[1]!=array[2]);
    /* Aliased source/output must keep the original alive until copying ends. */
    CHECK(kinoko_sq_clone(vm,PTR(destination),PTR(destination)));
    function_489f30_this(PTR(destination));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(array))));
    expected_vm_error=1;
    int result=execute_source(vm,root+2,
        "function ReceiverBadIndex(index,...) { return vargv[index]; }\n"
        "function ReceiverEmpty(...) { return vargv[0]; }\n"
        "local caught=0;\n"
        "try { ReceiverBadIndex(-1,7); } catch(e) { if(e!=\"vargv index out of range\") throw e; caught++; }\n"
        "try { ReceiverBadIndex(1,7); } catch(e) { if(e!=\"vargv index out of range\") throw e; caught++; }\n"
        "try { ReceiverBadIndex(\"bad\",7); } catch(e) { if(e!=\"indexing 'vargv' with string\") throw e; caught++; }\n"
        "try { ReceiverEmpty(); } catch(e) { if(e!=\"the function doesn't have var args\") throw e; caught++; }\n"
        "class ReceiverBadIterator { function _nexti(previous) { return \"missing\"; } }\n"
        "try { foreach(k,v in ReceiverBadIterator()) {} }\n"
        "catch(e) { if(e!=\"_nexti returned an invalid idx\") throw e; caught++; }\n"
        "if(caught!=5) throw 89;\n");
    expected_vm_error=0;
    CHECK(result && function_48aa20(vm)==top);
    puts("PASS: C++ clone alias/ownership/metamethods, iterator lifetimes and vararg bounds/errors");
    return 0;
}

static int test_recovered_object_entries(int32_t vm, int32_t *root) {
    int32_t table[3], array[3], text[3];
    const int32_t stack_before = function_48aa20(vm);
    CHECK(execute_source(vm, root + 2,
        "entry_table <- {a=1,b=2};\nentry_array <- [1,2,3];\nentry_text <- \"entries\";\n"));
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(table)), "entry_table");
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(array)), "entry_array");
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(text)), "entry_text");
    CHECK(kinoko_sqplus_object_size((void *)(intptr_t)(PTR(table))) == 2);
    CHECK(kinoko_sqplus_object_size((void *)(intptr_t)(PTR(array))) == 3);
    CHECK(kinoko_sqplus_object_size((void *)(intptr_t)(PTR(text))) == 7);
    CHECK(kinoko_sqplus_object_reverse((void *)(intptr_t)(PTR(array))) == 1);
    int32_t *reversed = *(int32_t **)(intptr_t)(array[2] + 24);
    CHECK(reversed[1] == 3 && reversed[3] == 2 && reversed[5] == 1);
    CHECK(kinoko_sqplus_object_size((void *)(intptr_t)(PTR(array))) == 3);
    CHECK(function_48aa20(vm) == stack_before);

    int32_t methods[2] = {0, PTR(release_error_probe)};
    int32_t object[3] = {PTR(methods), 1, 0};
    int32_t source[2] = {0x08000080, PTR(object)};
    int32_t destination[2] = {0x01000001, 0};
    const int releases_before = error_releases;
    CHECK(function_489f50_this(PTR(destination), PTR(source)) == PTR(destination));
    CHECK(object[1] == 2);
    CHECK(function_489f50_this(PTR(destination), PTR(destination)) == PTR(destination));
    CHECK(object[1] == 2);
    function_489f30_this(PTR(destination));
    CHECK(object[1] == 1 && error_releases == releases_before);
    function_489f30_this(PTR(source));
    CHECK(error_releases == releases_before + 1);

    /* Invoke the actual SquirrelObject vtable through the original ABI.
       flags=0 destroys a stack wrapper; flags=1 also frees a heap wrapper. */
    CHECK(retdec_call_thiscall1_result(table, (void *)(intptr_t)g16.e0, 0) == PTR(table));
    CHECK(table[0] == PTR(&g16) && table[1] == 0x01000001 && table[2] == 0);
    int32_t *heap = malloc(3 * sizeof(int32_t));
    CHECK(heap != NULL);
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(heap)), "entry_array");
    int32_t heap_address = PTR(heap);
    CHECK(retdec_call_thiscall1_result(heap, (void *)(intptr_t)g16.e0, 1) == heap_address);
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(array))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(text))));
    CHECK(function_48aa20(vm) == stack_before);
    puts("PASS: recovered size/reverse, pair assignment/release, and virtual wrapper destruction");
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
    void *vtable[5] = {NULL, NULL, NULL, NULL, release_stage_owner};
    kinoko_stage_list_construct();
    const int32_t identity=g603;
    CHECK(kinoko_stages_update()==identity);
    CHECK(kinoko_stages_prepare_draw()==identity);
    CHECK(kinoko_stages_draw()==identity);
    /* Empty payloads preserve the distinct return policies of each pass:
       preparation reports zero, update/draw retain the sentinel result. */
    kinoko_stage_list_append(0);
    CHECK(kinoko_stages_update()==identity);
    CHECK(kinoko_stages_prepare_draw()==0);
    CHECK(kinoko_stages_draw()==identity);
    CHECK(kinoko_clear_global_stages()==identity && g604==0);
    stage_owner_releases = 0;
    for (int i = 0; i < 2; ++i) {
        int32_t *owner = calloc(3, 4);
        int32_t *source = calloc(2, 4), *runtime = calloc(48, 4);
        CHECK(owner && source && runtime);
        source[0] = PTR(vtable);
        source[1] = PTR(owner);
        owner[0] = PTR(source);
        owner[1] = PTR(malloc(32));
        owner[2] = PTR(runtime);
        /* Constructor initializes known members only, not padding/unknowns. */
        const int untouched[] = {9, 10, 11, 56, 72, 80, 92, 105, 106, 107, 188};
        for (unsigned u = 0; u < sizeof(untouched)/sizeof(untouched[0]); ++u)
            ((unsigned char *)runtime)[untouched[u]] = 0xa5;
        CHECK(kinoko_act_runtime_initialize((KinokoActRuntime *)runtime, (KinokoActSourceHolder *)owner) == (KinokoActRuntime *)runtime);
        for (unsigned u = 0; u < sizeof(untouched)/sizeof(untouched[0]); ++u)
            CHECK(((unsigned char *)runtime)[untouched[u]] == 0xa5);
        CHECK(runtime[0] == PTR(owner) && runtime[3] == 0 && runtime[4] == 0);
        CHECK(runtime[38] == 0 && runtime[39] == OT_NULL && runtime[40] == 0);
        CHECK(runtime[45] == 0 && runtime[46] == 15);
        CHECK(runtime[21] && runtime[22]==0 && runtime[24]==0);
        CHECK(kinoko_act_find_first((KinokoActRuntime *)runtime,"__kinoko_missing_find_contract__/*.none")==0);
        CHECK(runtime[24]==0 && kinoko_act_find_name((KinokoActRuntime *)runtime,1)==NULL);
        CHECK(!kinoko_act_find_next((KinokoActRuntime *)runtime,1) && !kinoko_act_find_close((KinokoActRuntime *)runtime,1));
        int32_t first=kinoko_act_find_first((KinokoActRuntime *)runtime,"*");
        int32_t second=kinoko_act_find_first((KinokoActRuntime *)runtime,"*");
        CHECK(first==1 && second==2 && runtime[22]==2);
        CHECK(kinoko_act_find_name((KinokoActRuntime *)runtime,first)!=NULL);
        CHECK(kinoko_act_find_close((KinokoActRuntime *)runtime,first) && runtime[22]==1);
        CHECK(!kinoko_act_find_close((KinokoActRuntime *)runtime,first));
        CHECK(kinoko_act_find_name((KinokoActRuntime *)runtime,first)==NULL);
        CHECK(kinoko_act_find_first((KinokoActRuntime *)runtime,"*")==3 && runtime[22]==2);
        /* Two open searches remain for the runtime destructor to close. */
        runtime[3] = 0; /* fresh runtime has no active clone; source remains borrowed through word 0 */
        runtime[4] = PTR(malloc(24));
        kinoko_stage_list_append((KinokoStageOwner *)owner);
    }
    CHECK(function_465f70()==identity);
    CHECK(stage_owner_releases==2 && g604==0);
    CHECK(kinoko_stage_list_first()==kinoko_stage_list_end());
    CHECK(function_465f70()==identity && stage_owner_releases==2);
    kinoko_stage_list_destroy();
    g603=saved_head;g604=saved_count;
    puts("PASS: global stage owners, runtime receivers, shared ACT ownership and repeated clear");
    return 0;
}

static int test_global_sound_cleanup(void) {
    int32_t old_head = g638, old_size = g639;
    g638=kinoko_integer_map_create();g639=1;
    kinoko_integer_map_put(g638,1,123);
    CHECK(kinoko_test_sound_cleanup(function_470890)==0);
    CHECK(g639==0 && kinoko_integer_map_size(g638)==0);
    kinoko_integer_map_destroy(g638);
    g638=old_head;g639=old_size;
    puts("PASS: SE buffers, streaming pool, sound lookup sentinel and repeatable shutdown");
    return 0;
}

static int test_global_script_cleanup(int32_t vm, int32_t *root) {
    int32_t *globals[] = {g602, g629, g611, g636, unk_5149EC};
    int32_t saved[5][3];
    CHECK(g645 == 0);
    for (int i = 0; i < 5; ++i) {
        memcpy(saved[i], globals[i], 12);
        kinoko_sqplus_object_construct_value((void *)(intptr_t)(PTR(globals[i])), root[2], root[3]);
    }
    CHECK((int32_t)(intptr_t)(kinoko_sqplus_root_object()) != 0);
    CHECK(kinoko_game_release_script_state() == 0 && g644 == NULL && g645 == 0);
    for (int i = 0; i < 5; ++i) CHECK(globals[i][1] == g483 && globals[i][2] == 0);
    CHECK(kinoko_game_release_script_state() == 0 && g645 == 0);
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
        kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(list)), "effectList");
        kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(steps)), "effectSteps");
        /* Source SQArray owns its C++ vtable; validate the public object type. */
        CHECK(list[1] == 0x08000040 && list[2] != 0);
        int32_t *array = (int32_t *)(intptr_t)list[2];
        CHECK(array[7] == (frame < 2 ? 1 : 0));
        CHECK(steps[2] == (frame < 2 ? frame + 1 : 3));
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(steps))));
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(list))));
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

static ULONG WINAPI count_texture_release(IDirect3DBaseTexture9 *texture) {
    ++((int32_t *)texture)[1];
    return 0;
}

static int test_texture_lifetime(void) {
    IDirect3DBaseTexture9Vtbl vtable = {0};
    int32_t texture[2] = {0};
    IDirect3DDevice9 *old_device = kinoko_graphics.device;
    vtable.Release = count_texture_release;
    texture[0] = PTR(&vtable);
    kinoko_graphics.device = 0;
    for (int cycle = 0; cycle < 5000; ++cycle) {
        int32_t handle = kinoko_texture_register(
            (IDirect3DBaseTexture9 *)texture, 256, 256);
        CHECK(handle != 0);
        CHECK(kinoko_texture_slots[handle].texture == (IDirect3DBaseTexture9 *)texture);
        CHECK(kinoko_texture_release(handle) == 1);
        CHECK(texture[1] == cycle + 1);
        CHECK(kinoko_texture_slots[handle].texture == NULL);
    }
    {
        struct retdec_mcd_data *data = calloc(1, sizeof(*data));
        CHECK(data);
        data->texture_count = 2;
        data->textures = calloc(2, sizeof(*data->textures));
        CHECK(data->textures);
        for (int i = 0; i < 2; ++i)
            data->textures[i].handle = kinoko_texture_register(
                (IDirect3DBaseTexture9 *)texture, 64, 64);
        retdec_mcd_free(data);
        CHECK(texture[1] == 5002);
        int32_t *resource = calloc(1, 100);
        CHECK(resource);
        resource[0] = PTR(&g365);
        resource[17] = kinoko_texture_register((IDirect3DBaseTexture9 *)texture, 64, 64);
        retdec_destroy_cact_resource(PTR(resource));
        CHECK(texture[1] == 5003);
        int32_t layer[88] = {0}, layout[116] = {0}, render_layer[2] = {0}, camera[24] = {0};
        layout[0] = PTR(&g327); layout[78] = PTR(layer); layout[79] = 1;
        render_layer[1] = PTR(layout);
        CHECK(retdec_call_thiscall0_result(layout, kinoko_map_update_all_entry) == 0);
        CHECK(retdec_call_thiscall4_result(layout, kinoko_map_update_visible_entry, 1, 2, 3, 4) == 0);
        CHECK(retdec_call_thiscall2_result(layout, kinoko_map_draw_entry, 0, 0) == 0);
        CHECK(retdec_call_thiscall1_result(render_layer, kinoko_map_render_layer_entry, PTR(camera)) == 0);
    }
    kinoko_graphics.device = old_device;
    puts("PASS: 5000 texture load/unload cycles release COM objects and reuse handles");
    return 0;
}

static int test_stage_update_mask(int32_t manager, int32_t vm, int32_t *root) {
    int32_t closure[3], actors[2];
    /* This fixture skips game startup, but Update always visits Input first. */
    const int32_t input = PTR(kinoko_game_objects()->input);
    kinoko_input_devices_construct(input);
    kinoko_input_cluster_construct(input + 196);
    kinoko_input_keys_construct((KinokoKeyTracker *)(intptr_t)(input + 392));
    const int32_t saved_stages=g603,saved_stage_count=g604;
    kinoko_stage_list_construct();
    CHECK(execute_source(vm, root + 2,
        "maskActors <- [];\n"
        "function InitMaskActor(group) { updateGroup=group; user={steps=0}; vx=1.0; "
        "callbackGroup=0; callbackMask=0; collisionMask=0; "
        "SetUpdateFunction(function() { user.steps++; }); ::maskActors.append(this); }"));
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(closure)), "InitMaskActor");
    for (int i = 0; i < 2; ++i) {
        actors[i] = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){closure[0], closure[1], closure[2]}, 10.0f, 20.0f, -1, &(const KinokoOwnedObjectWords){PTR(&g16), 0x05000002, 4 << i}, (const void *)(intptr_t)(0)));
        CHECK(actors[i]);
        *(uint8_t *)(intptr_t)(actors[i] + 40) = 1;
    }
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(closure))));
    /* Exercise 469900 itself: writing only g622 used to leave manager+64=-1. */
    kinoko_game_masks.update = 0x40000004; /* Original GP_ACT | GP_PLAYER damage/death mask. */
    for (int frame = 0; frame < 30; ++frame) kinoko_game_update();
    CHECK(*(int32_t *)(intptr_t)(manager + 64) == kinoko_game_masks.update);
    CHECK(execute_source(vm, root + 2,
        "if(maskActors[0].user.steps!=30 || maskActors[1].user.steps!=0) "
        "throw \"damage update groups\";"));
    CHECK(*(float *)(intptr_t)(actors[0] + 240) == 40.0f);
    CHECK(*(float *)(intptr_t)(actors[1] + 240) == 10.0f);
    kinoko_game_masks.update = 12; /* Restore both Actor groups without dispatching map/camera. */
    kinoko_game_update();
    CHECK(execute_source(vm, root + 2,
        "if(maskActors[0].user.steps!=31 || maskActors[1].user.steps!=1) "
        "throw \"resume update groups\";"));
    CHECK(*(float *)(intptr_t)(actors[1] + 240) == 11.0f);
    kinoko_game_masks.update = 0;
    kinoko_game_update();
    CHECK(*(float *)(intptr_t)(actors[1] + 240) == 11.0f);
    CHECK(vm_failures == 0);
    kinoko_stage_list_destroy();g603=saved_stages;g604=saved_stage_count;
    kinoko_input_keys_destroy((KinokoKeyTracker *)(intptr_t)(input + 392));
    kinoko_input_cluster_delete(input + 196, NULL, 0);
    kinoko_input_devices_destroy(input);
    puts("PASS: stage damage/death mask freezes enemy callbacks and motion for 30 frames; player continues; groups resume");
    return 0;
}

static int test_crystal_countdown(int32_t vm, int32_t *root, const char *directory) {
    char path[MAX_PATH];
    int32_t schedule[3], ticks[3];
    int32_t expected[64], expected_count = 0;
    for (char archive = 'a'; archive <= 'c'; ++archive) {
        sprintf_s(path, sizeof(path), "%s/6kinoko_%c.dat", directory, archive);
        CHECK(kinoko_archive_mount(path));
    }
    CHECK(execute_asset(vm, root + 2, "data/script/stage.cv4"));
    CHECK(execute_source(vm, root + 2,
        "GP_PLAYER <- 4; updateMask <- 4; tickFrames <- []; clockFrame <- 0;\n"
        "function PlaySE(id) { if(id==120) tickFrames.append(clockFrame); }\n"
        "SetSwitchBlue();"));
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(schedule)), "stageSwitchCountArray");
    CHECK(schedule[1] == 0x08000040);
    int32_t count = *(int32_t *)(intptr_t)(schedule[2] + 28);
    int32_t *values = *(int32_t **)(intptr_t)(schedule[2] + 24);
    CHECK(count == 44);
    int32_t next = count - 1;
    for (int frame = 0; frame < 900; ++frame) {
        const int elapsed_ms = 15000 - (900 - frame) * 1000 / 60;
        if (next >= 0 && values[next * 2 + 1] < elapsed_ms) {
            CHECK(values[next * 2] == 0x05000002);
            expected[expected_count++] = frame;
            --next;
        }
    }
    CHECK(expected_count == 44);
    CHECK(execute_source(vm, root + 2,
        "for(clockFrame=0; clockFrame<900; clockFrame++) UpdateStage();"));
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(ticks)), "tickFrames");
    CHECK(ticks[1] == 0x08000040);
    const int actual_count = *(int32_t *)(intptr_t)(ticks[2] + 28);
    int32_t *actual = *(int32_t **)(intptr_t)(ticks[2] + 24);
    printf("COUNTDOWN expected=%d actual=%d remaining=%d\n", expected_count, actual_count,
        *(int32_t *)(intptr_t)(schedule[2] + 28));
    CHECK(actual_count == expected_count);
    for (int i = 0; i < expected_count; ++i) {
        printf("TICK %d expected_frame=%d actual_frame=%d\n", i, expected[i], actual[2*i+1]);
        CHECK(actual[2*i] == 0x05000002 && actual[2*i+1] == expected[i]);
    }
    CHECK(*(int32_t *)(intptr_t)(schedule[2] + 28) == 0);
    CHECK(execute_source(vm, root + 2,
        "if(stageSwitchCount!=0 || stageSwitchBlue!=2) throw \"switch expiration\";"));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(schedule))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(ticks))));
    CHECK(vm_failures == 0);
    puts("PASS: original 900-frame blue-crystal countdown consumes all 44 timestamps at the original cadence");
    return 0;
}

static int test_array_pop_values(int32_t vm, int32_t *root) {
    const int32_t top = function_48aa20(vm);
    CHECK(execute_source(vm, root + 2,
        "popObject <- {value=7};\n"
        "popValues <- [popObject,null,3.25,\"tail\"];\n"
        "if(popValues.top()!=\"tail\" || popValues.len()!=4) throw \"array top\";\n"
        "if(popValues.pop()!=\"tail\" || popValues.len()!=3) throw \"pop string\";\n"
        "if(popValues.pop()!=3.25 || popValues.pop()!=null) throw \"pop scalar\";\n"
        "poppedObject <- popValues.pop();\n"
        "if(poppedObject!=popObject || poppedObject.value!=7 || popValues.len()!=0) throw \"pop object ownership\";\n"
        "popErrors <- 0;\n"));
    expected_vm_error = 1;
    const int caught_errors = execute_source(vm, root + 2,
        "try { popValues.pop(); } catch(e) { if(e==\"empty array\") popErrors++; }\n"
        "try { popValues.top(); } catch(e) { if(e==\"top() on a empty array\") popErrors++; }");
    expected_vm_error = 0;
    CHECK(caught_errors);
    CHECK(execute_source(vm, root + 2,
        "if(popErrors!=2) throw \"empty array error return\";\n"
        "popValues.append(popValues);\n"
        "poppedSelf <- popValues.pop();\n"
        "if(poppedSelf!=popValues || popValues.len()!=0) throw \"pop self reference\";\n"
        "for(local i=0;i<512;i++) popValues.append(i);\n"
        "for(local i=511;i>=0;i--) if(popValues.pop()!=i) throw \"pop shrink order\";\n"
        "if(popValues.len()!=0) throw \"pop shrink length\";"));
    CHECK(function_48aa20(vm) == top);
    CHECK(execute_source(vm, root + 2, "apiPop <- [10,20,30];"));
    int32_t array[3];
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(array)), "apiPop");
    function_48ab90(vm, array[1], array[2]);
    CHECK(function_48dd10(vm, -1, 0) == 0);
    CHECK(function_48aa20(vm) == top + 1);
    CHECK(*(int32_t *)(intptr_t)(array[2] + 28) == 2);
    CHECK(function_48dd10(vm, -1, 1) == 0);
    CHECK(function_48aa20(vm) == top + 2);
    int32_t value = 0;
    CHECK(function_48a7d0(vm, -1, &value) == 0 && value == 20);
    function_48c910(vm, top);
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(array))));
    puts("PASS: Squirrel array top/pop values, ownership, empty errors, self-reference, shrink and API push/no-push");
    return 0;
}


static int test_moving_map(int32_t vm, int32_t *root, int32_t manager, const char *directory, int underwater, float start_x, float start_y) {
    const uint32_t previous_rounding=kinoko_enter_game_math();
    char path[MAX_PATH];
    for(char archive='a';archive<='c';++archive) {
        sprintf_s(path,sizeof(path),"%s/6kinoko_%c.dat",directory,archive);
        CHECK(kinoko_archive_mount(path));
    }
    CHECK(kinoko_test_bgm_preserves_game_math()==0);
    CHECK(kinoko_actor_manager_construct((KinokoActorManager *)(intptr_t)(manager)));
    kinoko_actor_register_script_class();
    CHECK(execute_source(vm,root+2,"Actor.funcUpdate <- null; player <- null;"));
    int32_t reader=0;
    uint8_t version; uint16_t textures;
    CHECK(kinoko_reader_open((KinokoArchiveReader **)&reader,"data/actor/marisa/marisa.pat"));
    CHECK(fixture_pat_read_u8(reader,&version));
    CHECK(fixture_pat_read_u16(reader,&textures));
    CHECK(fixture_pat_skip_bytes(reader,textures*128u));
    CHECK(fixture_pat_read_animations(reader,manager,0));
    kinoko_reader_close((KinokoArchiveReader *)(intptr_t)reader);
    CHECK(execute_asset(vm,root+2,"data/script/constant.cv4"));
        int32_t player_scripts[3];
        CHECK(execute_source(vm,root+2,"t_player <- {};"));
        kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(player_scripts)), "t_player");
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player.cv4"));
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player_ground.cv4"));
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player_jump.cv4"));
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player_ex.cv4"));
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player_suwa.cv4"));
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player_ufo.cv4"));
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player_start.cv4"));
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(player_scripts))));
        CHECK(execute_source(vm,root+2,

            "input <- {x=0,y=0,b0=0,b1=0,b2=0,b3=0,k0=0,k1=0,k2=0,k3=0};\n"
            "camera <- {top=-1000,bottom=2000};\n"
            "time <- 1000; stageWaterLevel <- 10000; stageWaterType <- 0;\n"
            "stageLayerVector <- -1; stageIce <- false; stageTimeStop <- false;\n"
            "function InitPlatformRider(id) {\n"
            "user={type=riderType,take=0,hold=null,water=riderWater,rolling=false,pitch=1.0,"
            "dash_count=0,hover=0,deadCount=0,clearCount=0,moveCount=0,goalCount=0,"
            "changingCount=0,invincibleCount=0,count8head=0,countUFO=0,inertia=0.0,"
            "hitblock=false,slide=false,hand=null,swim=false,ladder=false,hitCount=0,vx=0.0,vector=false};\n"
            "user.SetTake <- ::t_player.SetTake.bindenv(this);\n"
            "user.SetDead <- function(v){throw \"unexpected green rider death\";};\n"
            "user.SetTake(TAKE_STAND); collisionMask=GP_TERRAIN|GP_LIFT; "
            "collisionGroup=GP_PLAYER; callbackGroup=GP_PLAYER; priority=PR_PLAYER; updateGroup=GP_PLAYER; "
            "funcUpdate=::t_player.Stand.bindenv(this); SetUpdateFunction(::t_player.Update); ::player=this; }"));

    /* Isolate the already-submerged state; use the packaged player state machine. */
    CHECK(execute_source(vm,root+2, underwater
        ? "stageWaterLevel=6; riderWater <- true; riderType <- TYPE_2HEAD;"
        : "riderWater <- false; riderType <- TYPE_USA;"));

    int32_t act[60]={0}, moving_layer=0, moving_layout=0, vector_layout=0;
    kinoko_act_document_initialize((KinokoActDocument *)act);
    CHECK(kinoko_act_document_load((KinokoActDocument *)act,"data/map/w3-c02b.act"));
    for(int32_t slot=act[52];slot!=act[53];slot+=4) {
        int32_t layer=*(int32_t *)(intptr_t)slot;
        const char *name=retdec_std_string_data(layer+112);
        if(strcmp(name,"terrain-kabe")!=0 && strcmp(name,"vector")!=0) continue;
        int32_t head=*(int32_t *)(intptr_t)(layer+180);
        int32_t node=*(int32_t *)(intptr_t)head;
        CHECK(node!=head);
        int32_t key=*(int32_t *)(intptr_t)(node+8);
        if(strcmp(name,"vector")==0) { vector_layout=*(int32_t *)(intptr_t)(key+4); continue; }
        moving_layer=layer;
        moving_layout=*(int32_t *)(intptr_t)(key+4);
    }
    CHECK(moving_layer && moving_layout);
    int32_t map_state=PTR(g_retdec_map_manager_state), map_object[3];
    kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR(g722)));
    kinoko_sqplus_object_assign((void *)(intptr_t)(PTR(g722)), (const void *)(intptr_t)(PTR(root+1)));
    CHECK(kinoko_map_manager_construct((KinokoMapManager *)(intptr_t)map_state));
    function_46fac0();
    CHECK((int32_t)(intptr_t)(kinoko_sqplus_object_new_instance((void *)(intptr_t)(PTR(map_object)), (const void *)(intptr_t)(PTR(g636)))));
    kinoko_sqplus_object_assign((void *)(intptr_t)(map_state), (const void *)(intptr_t)(PTR(map_object)));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(map_object))));
    kinoko_sqplus_object_set_instance((void *)(intptr_t)(map_state), (void *)(intptr_t)(map_state));
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), "map", (const void *)(intptr_t)(map_state));
    CHECK(vector_layout);
    *(int32_t *)(intptr_t)(map_state+36)=PTR(&vector_layout);
    *(int32_t *)(intptr_t)(map_state+40)=PTR(&vector_layout+1);
    CHECK(execute_source(vm,root+2,"stageLayerVector=0;"));

    function_48ab90(vm,root[2],root[3]);
    CHECK(function_4c6c20(vm)==0);
    function_48aa50(vm);
    /* Publish the real CActLayer descriptors and invoke its captured callback. */
    int32_t resource[48]={0}, parent[2], active=0;
    CHECK(retdec_publish_cact_layer_class(vm,PTR(root)));
    CHECK(kinoko_sqrat_new_table((struct SQVM *)(intptr_t)(vm), parent));
    CHECK(kinoko_sqrat_set_pair((struct SQVM *)(intptr_t)(vm), root+2, retdec_std_string_data(PTR(act)+16), parent));
    CHECK(execute_source(vm,parent,"resource <- {};"));
    resource[39]=root[2]; resource[40]=root[3];
    act[52]=PTR(&moving_layer); act[53]=PTR(&moving_layer+1);
    CHECK(retdec_publish_act_layers(vm,PTR(act),PTR(resource),&active));
    CHECK(active==1);
    CHECK(retdec_execute_act_callback(moving_layer+204,24,NULL)>=0);
    CHECK(function_468950_this(PTR(g_514300_storage),manager));
    int32_t support=function_4693a0(moving_layout);
    CHECK(support);
    int32_t init[3];
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(init)), "InitPlatformRider");
    int32_t rider=(int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, start_x, start_y+*(float *)(intptr_t)(moving_layer+148), -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
    CHECK(rider);
    *(int32_t *)(intptr_t)(rider+316)=1;
    *(uint8_t *)(intptr_t)(rider+40)=1;
    kinoko_game_masks.update=-1; *(int32_t *)(intptr_t)(manager+64)=-1;
    kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager));
    function_468620_this(PTR(g_514300_storage));
    kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(rider));
    int failures=0, changes=0, previous_take=*(int32_t *)(intptr_t)(rider+208);
    for(int frame=0;frame<1500;++frame) {
        CHECK(retdec_execute_act_callback(moving_layer+204,24,NULL)>=0);
        kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(0));
        int hit=*(int32_t *)(intptr_t)(rider+296);
        int take=*(int32_t *)(intptr_t)(rider+208);
        if(take!=previous_take) ++changes;
        if(!hit || take!=previous_take || frame<6)
            printf("MAP frame=%d offset=%.9g y=%.9g bottom=%.9g carry=%.9g hit=%d take=%d step=%08x\n",
                frame,*(float *)(intptr_t)(moving_layer+148),*(float *)(intptr_t)(rider+244),
                *(float *)(intptr_t)(rider+452),*(float *)(intptr_t)(rider+268),hit,take,
                *(uint32_t *)(intptr_t)(rider+36));
        if(!hit) ++failures;
        previous_take=take;
        CHECK(vm_failures==0);
    }
    printf("MAP lost-contact=%d take-changes=%d\n",failures,changes);
    CHECK(failures==0 && changes==0);
    kinoko_leave_game_math(previous_rounding);
    return 0;
}

static int test_platform_riding(int32_t vm, int32_t *root, int32_t manager, const char *directory, int green) {
    kinoko_enter_game_math();
    char path[MAX_PATH];
    int32_t map_act[60]={0}, rail_layouts[8]={0}, terrain_layouts[16]={0};
    int rail_count=0, terrain_count=0;
    float green_x=0, green_y=0;
    int32_t green_source=0;
    for (char archive='a'; archive<='c'; ++archive) {
        sprintf_s(path,sizeof(path),"%s/6kinoko_%c.dat",directory,archive);
        CHECK(kinoko_archive_mount(path));
    }
    CHECK(kinoko_actor_manager_construct((KinokoActorManager *)(intptr_t)(manager)));
    kinoko_actor_register_script_class();
    CHECK(execute_source(vm,root+2,
        "if(Actor.step!=null || Actor.user!=null) throw \"Actor null class defaults\";"));
    CHECK(execute_source(vm,root+2,"Actor.funcUpdate <- null;"));
    int32_t target=PTR(kinoko_script_set_init);
    CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR("SetInitFunctionByID")), (const void *)(intptr_t)(PTR(&target)), 4, (void *)(intptr_t)(PTR(function_471d30)), 0))>=0);
    CHECK(execute_asset(vm,root+2,"data/script/constant.cv4"));
    for(int kind=0;kind<2;++kind) {
        int32_t reader=0;
        uint8_t version;
        uint16_t textures;
        CHECK(kinoko_reader_open((KinokoArchiveReader **)&reader,kind ? "data/actor/marisa/marisa.pat" : "data/actor/item/item.pat"));
        CHECK(fixture_pat_read_u8(reader,&version));
        CHECK(fixture_pat_read_u16(reader,&textures));
        CHECK(fixture_pat_skip_bytes(reader,textures*128u));
        CHECK(fixture_pat_read_animations(reader,manager,0));
        kinoko_reader_close((KinokoArchiveReader *)(intptr_t)reader);
    }
    CHECK(pat_lookup(manager,1823));
    if(green) {
        int32_t map_state=PTR(g_retdec_map_manager_state), map_object[3];
        kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR(g722)));
        kinoko_sqplus_object_assign((void *)(intptr_t)(PTR(g722)), (const void *)(intptr_t)(PTR(root+1)));
        CHECK(kinoko_map_manager_construct((KinokoMapManager *)(intptr_t)map_state));
        function_46fac0();
        CHECK((int32_t)(intptr_t)(kinoko_sqplus_object_new_instance((void *)(intptr_t)(PTR(map_object)), (const void *)(intptr_t)(PTR(g636)))));
        kinoko_sqplus_object_assign((void *)(intptr_t)(map_state), (const void *)(intptr_t)(PTR(map_object)));
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(map_object))));
        kinoko_sqplus_object_set_instance((void *)(intptr_t)(map_state), (void *)(intptr_t)(map_state));
        kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), "map", (const void *)(intptr_t)(map_state));
        kinoko_act_document_initialize((KinokoActDocument *)map_act);
        CHECK(kinoko_act_document_load((KinokoActDocument *)map_act,"data/map/w2-c05b.act"));
        *(int32_t *)(intptr_t)(map_state+76)=map_act[2];
        *(int32_t *)(intptr_t)(map_state+80)=map_act[3];
        for(int32_t slot=map_act[52];slot!=map_act[53];slot+=4) {
            int32_t layer=*(int32_t *)(intptr_t)slot;
            const char *name=retdec_std_string_data(layer+112);
            int32_t head=*(int32_t *)(intptr_t)(layer+180);
            for(int32_t node=*(int32_t *)(intptr_t)head;node!=head;node=*(int32_t *)(intptr_t)node) {
                int32_t key=*(int32_t *)(intptr_t)(node+8);
                int32_t layout=*(int32_t *)(intptr_t)(key+4);
                if(!retdec_map_chip_data(layout)) continue;
                for(int32_t record=*(int32_t *)(intptr_t)(layout+264);
                    record!=*(int32_t *)(intptr_t)(layout+268);record+=32) {
                    if(*(int32_t *)(intptr_t)record!=1207) continue;
                    struct retdec_mcd_chip *chip=retdec_mcd_find_chip(retdec_map_chip_data(layout),1207);
                    CHECK(chip);
                    green_x=(float)((double)*(int32_t *)(intptr_t)(record+4)+retdec_mcd_i16(chip->bytes+12)*0.5+1.0);
                    green_y=(float)((double)*(int32_t *)(intptr_t)(record+8)+retdec_mcd_i16(chip->bytes+14)*
                        ((retdec_mcd_u32(chip->bytes+16)&0x10000u)?0.5:1.0));
                    green_source=PTR(chip->bytes);
                }
                if(strncmp(name,"ra",2)==0) { CHECK(rail_count<8); rail_layouts[rail_count++]=layout; }
                if(strncmp(name,"te",2)==0 || strncmp(name,"wa",2)==0) {
                    CHECK(terrain_count<16); terrain_layouts[terrain_count++]=layout;
                }
            }
        }
        CHECK(rail_count>0 && green_source);
        printf("GREEN map spawn=(%g,%g) flags=%08x rails=%d terrain=%d\n",green_x,green_y,retdec_mcd_u32((unsigned char *)(intptr_t)green_source+16),rail_count,terrain_count);
        /* 46F6D0 reverses map.layer_name before stage.nut creates rail events. */
        for(int i=0;i<rail_count/2;++i) {
            int32_t swap=rail_layouts[i]; rail_layouts[i]=rail_layouts[rail_count-1-i];
            rail_layouts[rail_count-1-i]=swap;
        }
        *(int32_t *)(intptr_t)(map_state+36)=PTR(rail_layouts);
        *(int32_t *)(intptr_t)(map_state+40)=PTR(rail_layouts+rail_count);
        sprintf_s(path,sizeof(path),"stageRailCount <- %d; stageSwitchRail <- 0;",rail_count);
        CHECK(execute_source(vm,root+2,path));
    }
    CHECK(execute_source(vm,root+2,
        "t_lift <- {};\nplayer <- null;\n"
        "function InitPlatformRider(v) { SetTake(TYPE_2HEAD*100+TAKE_STAND); "
        "updateGroup=GP_PLAYER; priority=PR_PLAYER; collisionGroup=GP_PLAYER; "
        "callbackGroup=GP_PLAYER; collisionMask=GP_LIFT; SetStep(null); ::player=this; "
        "SetUpdateFunction(function() { vy=hitBottom ? 0.0 : 0.5; }); }"));
    int32_t scripts[3], init[3], rider_init[3];
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(scripts)), "t_lift");
    CHECK(execute_asset(vm,scripts+1,"data/script/lift.cv4"));
    if(green) {
        int32_t player_scripts[3];
        CHECK(execute_source(vm,root+2,"t_player <- {};"));
        kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(player_scripts)), "t_player");
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player.cv4"));
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player_ground.cv4"));
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player_jump.cv4"));
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player_ex.cv4"));
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player_suwa.cv4"));
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player_ufo.cv4"));
        CHECK(execute_asset(vm,player_scripts+1,"data/script/player_start.cv4"));
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(player_scripts))));
        CHECK(execute_source(vm,root+2,

            "input <- {x=0,y=0,b0=0,b1=0,b2=0,b3=0,k0=0,k1=0,k2=0,k3=0};\n"
            "camera <- {top=-1000,bottom=2000};\n"
            "time <- 1000; stageWaterLevel <- 10000; stageWaterType <- 0;\n"
            "stageLayerVector <- -1; stageIce <- false; stageTimeStop <- false;\n"
            "function InitPlatformRider(id) {\n"
            "user={type=TYPE_USA,take=0,hold=null,water=false,rolling=false,pitch=1.0,"
            "dash_count=0,hover=0,deadCount=0,clearCount=0,moveCount=0,goalCount=0,"
            "changingCount=0,invincibleCount=0,count8head=0,countUFO=0,inertia=0.0,"
            "hitblock=false,slide=false,hand=null,swim=false,ladder=false,hitCount=0,vx=0.0,vector=false};\n"
            "user.SetTake <- ::t_player.SetTake.bindenv(this);\n"
            "user.SetDead <- function(v){throw \"unexpected green rider death\";};\n"
            "user.SetTake(TAKE_STAND); collisionMask=GP_TERRAIN|GP_LIFT; "
            "collisionGroup=GP_PLAYER; callbackGroup=GP_PLAYER; priority=PR_PLAYER; updateGroup=GP_PLAYER; "
            "funcUpdate=::t_player.Stand.bindenv(this); SetUpdateFunction(::t_player.Update); ::player=this; }"));
    }
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(scripts)), (void *)(intptr_t)(PTR(init)), green ? "InitRail" : "Init04c7");
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(rider_init)), "InitPlatformRider");
    CHECK(init[1]==0x08000100 && rider_init[1]==0x08000100);
    int32_t platform=(int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){init[0], init[1], init[2]}, green ? green_x : 608, green ? green_y : 672, -1, &(const KinokoOwnedObjectWords){PTR(&g16), 0x05000002, green ? 1207 : 1223}, (const void *)(intptr_t)(green_source)));
    int32_t rider=(int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){rider_init[0], rider_init[1], rider_init[2]}, green ? green_x : 608, green ? green_y-8 : 664, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
    CHECK(platform && rider && vm_failures==0);
    kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root+1)), "platformProbe", (const void *)(intptr_t)(platform+44));
    CHECK(execute_source(vm,root+2,
        "player.y=platformProbe.top-(player.bottom-player.y); player.vy=0.5;"));
    kinoko_actor_refresh_collision_bounds((KinokoActor *)(intptr_t)(rider));
    *(uint8_t *)(intptr_t)(platform+40)=1;
    *(uint8_t *)(intptr_t)(rider+40)=1;
    CHECK(function_468950_this(PTR(g_514300_storage),manager));
    if(green) {
        for(int i=0;i<terrain_count;++i) CHECK(function_4693a0(terrain_layouts[i]));
        *(int32_t *)(intptr_t)(rider+316)=3;
    }
    kinoko_game_masks.update=-1;
    *(int32_t *)(intptr_t)(manager+64)=-1;
    CHECK(kinoko_collision_dispatch_pair((KinokoActor *)(intptr_t)(platform), (KinokoActor *)(intptr_t)(rider))>=0);
    CHECK(*(int32_t *)(intptr_t)(rider+36)==*(int32_t *)(intptr_t)(platform+28));
    if(green) {
        for(int frame=0;frame<90;++frame) {
            const float old_platform_x=*(float *)(intptr_t)(platform+240);
            const float old_player_x=*(float *)(intptr_t)(rider+240);
            const int active_count=kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
            CHECK(active_count>=2);
            printf("GREEN frame=%d platform=(%.6f,%.6f) player=(%.6f,%.6f) carry=(%.6f,%.6f) step=%08x hit=%d\n",
                frame,*(float *)(intptr_t)(platform+240),*(float *)(intptr_t)(platform+244),
                *(float *)(intptr_t)(rider+240),*(float *)(intptr_t)(rider+244),
                *(float *)(intptr_t)(rider+264),*(float *)(intptr_t)(rider+268),
                *(uint32_t *)(intptr_t)(rider+36),*(int32_t *)(intptr_t)(rider+296));
            CHECK(*(int32_t *)(intptr_t)(rider+36)==*(int32_t *)(intptr_t)(platform+28));
            CHECK(fabsf((*(float *)(intptr_t)(rider+240)-old_player_x)-
                (*(float *)(intptr_t)(platform+240)-old_platform_x))<0.001f);
            CHECK(vm_failures==0);
        }
        puts("PASS: original w2-c05b green rail platform carries rider through native terrain collision");
        return 0;
    }
    for(int frame=0;frame<45;++frame) {
        const float before=*(float *)(intptr_t)(platform+244);
        CHECK(kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)))==2);
        printf("PLATFORM frame=%d y=%.6f vy=%.6f playerBottom=%.6f parentdy=%.6f step=%08x hit=%d\n",
            frame,*(float *)(intptr_t)(platform+244),*(float *)(intptr_t)(platform+260),
            *(float *)(intptr_t)(rider+452),*(float *)(intptr_t)(rider+268),
            *(uint32_t *)(intptr_t)(rider+36),*(int32_t *)(intptr_t)(rider+296));
        CHECK(*(float *)(intptr_t)(platform+244)>=before);
        CHECK(*(int32_t *)(intptr_t)(rider+36)==*(int32_t *)(intptr_t)(platform+28));
        CHECK(fabsf(*(float *)(intptr_t)(rider+452)-*(float *)(intptr_t)(platform+444))<0.001f);
    }
    /* Original SetRide detaches upward-moving riders before platform Update. */
    const float jump_platform_y=*(float *)(intptr_t)(platform+244);
    const float jump_player_y=*(float *)(intptr_t)(rider+244);
    CHECK(execute_source(vm,root+2,
        "player.SetUpdateFunction(null); player.vy=-5.0;"));
    CHECK(kinoko_collision_dispatch_pair((KinokoActor *)(intptr_t)(platform), (KinokoActor *)(intptr_t)(rider))>=0);
    CHECK(*(int32_t *)(intptr_t)(rider+36)==0);
    kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
    CHECK(*(float *)(intptr_t)(platform+244)==jump_platform_y-2.0f);
    CHECK(*(float *)(intptr_t)(rider+244)==jump_player_y-5.0f);
    CHECK(execute_source(vm,root+2,
        "player.x=platformProbe.right+64; player.vy=0.0;"));
    for(int frame=0;frame<180;++frame)
        kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
    CHECK(*(float *)(intptr_t)(platform+244)==672.0f);
    CHECK(execute_source(vm,root+2,
        "player.x=platformProbe.x; player.y=platformProbe.top-(player.bottom-player.y); "
        "player.vy=0.5; player.SetUpdateFunction(function(){vy=hitBottom?0.0:0.5;});"));
    kinoko_actor_refresh_collision_bounds((KinokoActor *)(intptr_t)(rider));
    CHECK(kinoko_collision_dispatch_pair((KinokoActor *)(intptr_t)(platform), (KinokoActor *)(intptr_t)(rider))>=0);
    for(int frame=0;frame<12;++frame)
        kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
    CHECK(*(int32_t *)(intptr_t)(rider+36)==*(int32_t *)(intptr_t)(platform+28));
    CHECK(execute_source(vm,root+2,
        "player.x=platformProbe.right+64; player.SetUpdateFunction(null); player.vy=0.0;"));
    kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(rider));
    CHECK(*(int32_t *)(intptr_t)(rider+36)==0);
    for(int frame=0;frame<180;++frame)
        kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state)));
    CHECK(*(float *)(intptr_t)(platform+244)==672.0f);
    CHECK(*(float *)(intptr_t)(platform+260)==0.0f);
    CHECK(execute_source(vm,root+2,"if(player.step!=null) throw \"walk off\";"));
    CHECK(vm_failures==0);
    puts("PASS: original orange LiftBack carries rider down; jump and walk-off detach; return and reboard work");
    return 0;
}

/* Render the original PAT water frames without a device/window. This exercises
   the real loader and Actor render preparation, not a replacement water rule. */
static int test_quad_colors(void) {
    KinokoColoredQuad quad={0};
    const uint32_t colors[4]={0x80c86432u,0x11223344u,0x55667788u,0x99aabbccu};
    CHECK((uint32_t)retdec_call_thiscall1_result(&quad,g407.e2,PTR(colors))==colors[3]);
    for(int i=0;i<4;++i) CHECK(quad.vertices[i].color==colors[i]);
    CHECK((uint32_t)retdec_call_thiscall1_result(&quad,g407.e3,0x80808080u)==0x40643219u);
    for(int i=0;i<4;++i) CHECK(quad.vertices[i].color==0x40643219u);
    for(unsigned base=0;base<256;++base) {
        for(unsigned factor=0;factor<256;++factor) {
            const uint32_t packed=base*0x01010101u;
            CHECK((uint32_t)retdec_call_thiscall1_result(&quad,g407.e1,packed)==packed);
            CHECK((uint32_t)retdec_call_thiscall1_result(&quad,g407.e3,factor*0x01010101u)==
                  (base*factor/255u)*0x01010101u);
        }
    }
    puts("PASS: original color vtable ABI, distinct vertices and all 65536 channel products");
    return 0;
}

static int test_water_alpha(int32_t manager, const char *directory) {
    char path[MAX_PATH];
    const char *patterns[] = { "data/map/map.pat" };
    for (char archive='a'; archive<='c'; ++archive) {
        sprintf_s(path,sizeof(path),"%s/6kinoko_%c.dat",directory,archive);
        CHECK(kinoko_archive_mount(path));
    }
    CHECK(kinoko_actor_manager_construct((KinokoActorManager *)(intptr_t)(manager)));
    for (int p=0; p<1; ++p) {
        int32_t reader=0;
        uint8_t version;
        uint16_t textures;
        CHECK(kinoko_reader_open((KinokoArchiveReader **)&reader,patterns[p]));
        CHECK(fixture_pat_read_u8(reader,&version));
        CHECK(fixture_pat_read_u16(reader,&textures));
        CHECK(fixture_pat_skip_bytes(reader,textures*128u));
        CHECK(fixture_pat_read_animations(reader,manager,0));
        kinoko_reader_close((KinokoArchiveReader *)(intptr_t)reader);
    }
    for (int take=9700; take<=9730; take+=10) {
        int32_t animation=pat_lookup(manager,take);
        CHECK(animation);
        for (int32_t frame=*(int32_t *)(intptr_t)(animation+8);
             frame<*(int32_t *)(intptr_t)(animation+12); frame+=248) {
            int32_t actor[200]={0};
            int32_t extra=*(int32_t *)(intptr_t)(frame+244);
            uint32_t expected=extra ? *(uint32_t *)(intptr_t)(extra+4) : 0xffffffffu;
            actor[10]=1; ((uint8_t *)actor)[21]=1;
            actor[51]=frame; actor[38]=frame;
            ((float *)actor)[42]=1.0f;
            ((float *)actor)[43]=((float *)actor)[44]=1.0f;
            ((float *)actor)[68]=-1.0f;
            actor[45]=actor[46]=actor[47]=actor[48]=255;
            *(int32_t *)(intptr_t)(frame+4)=1; /* No device: only prepare vertices. */
            CHECK(kinoko_actor_render((KinokoActor *)(intptr_t)(PTR(actor)), (KinokoCamera *)(intptr_t)(0))==1);
            printf("take=%d PAT=%08x rendered=%08x\n",take,expected,
                   *(uint32_t *)(intptr_t)(frame+24));
            for(int vertex=0;vertex<4;++vertex)
                CHECK(*(uint32_t *)(intptr_t)(frame+24+28*vertex)==expected);
            /* A script fade multiplies PAT alpha; repeated renders must not
               compound the previous frame's already-modulated color. */
            actor[45]=128;
            for(int repeat=0;repeat<3;++repeat) {
                CHECK(kinoko_actor_render((KinokoActor *)(intptr_t)(PTR(actor)), (KinokoCamera *)(intptr_t)(0))==1);
                CHECK(*(uint32_t *)(intptr_t)(frame+24)==
                    (((expected>>24)*128u/255u)<<24 | (expected&0xffffffu)));
            }
            *(int32_t *)(intptr_t)(frame+244)=0;
            CHECK(kinoko_actor_render((KinokoActor *)(intptr_t)(PTR(actor)), (KinokoCamera *)(intptr_t)(0))==1);
            CHECK(*(uint32_t *)(intptr_t)(frame+24)==0x80ffffffu);
            *(int32_t *)(intptr_t)(frame+244)=extra;
        }
    }
    puts("PASS: original water PAT colors survive Actor render preparation");
    return 0;
}

static int test_portrait_regions(const char *directory) {
    char path[MAX_PATH];
    for (char archive='a'; archive<='c'; ++archive) {
        sprintf_s(path,sizeof(path),"%s/6kinoko_%c.dat",directory,archive);
        CHECK(kinoko_archive_mount(path));
    }
    int32_t act[60]={0};
    CHECK(kinoko_act_document_initialize((KinokoActDocument *)act));
    CHECK(kinoko_act_document_load((KinokoActDocument *)act,"data/system/playerimage.act"));
    /* Independent values from the original PlayerImage ACT, not resource order.
       Shared atlases must keep distinct crops after deserialization. */
    const char *names[]={"face_1","face_2","face_3","face_4","face_5",
                         "face_6","face_7","face_0","face_Default"};
    const int ids[]={11,14,15,16,17,18,19,20,21};
    const int atlas[]={1,1,1,2,2,3,2,3,3};
    const float x[]={0,137,273,0,137,137,273,0,273};
    CHECK((act[57]-act[56])/4==9);
    for (int i=0;i<9;++i) {
        int32_t resource=0;
        for (int32_t entry=act[56];entry<act[57];entry+=4) {
            const int32_t candidate=*(int32_t*)(intptr_t)entry;
            if (strcmp(retdec_std_string_data(candidate+8),names[i])==0) resource=candidate;
        }
        CHECK(resource);
        CHECK(*(int32_t*)(intptr_t)(resource+4)==ids[i]);
        sprintf_s(path,sizeof(path),"Data/System/face%d",atlas[i]);
        CHECK(strcmp(retdec_std_string_data(resource+40),path)==0);
        CHECK(*(float*)(intptr_t)(resource+80)==x[i]);
        CHECK(*(float*)(intptr_t)(resource+84)==0);
        CHECK(*(float*)(intptr_t)(resource+88)==136);
        CHECK(*(float*)(intptr_t)(resource+92)==480);
        CHECK(*(uint8_t*)(intptr_t)(resource+96)==0);
    }
    retdec_destroy_cact_object(PTR(act));
    puts("PASS: all nine original PlayerImage atlas regions preserve names, IDs and crop rectangles");
    return 0;
}

static int test_chip_shared_ownership(void) {
    int32_t source[60]={0}, texture[2]={0};
    IDirect3DBaseTexture9Vtbl vtable={0}; vtable.Release=count_texture_release;
    texture[0]=PTR(&vtable);
    CHECK(kinoko_act_document_initialize((KinokoActDocument *)source));
    int32_t *resource=(int32_t*)calloc(1,100);
    struct retdec_mcd_data *data=(struct retdec_mcd_data*)calloc(1,sizeof(*data));
    CHECK(resource && data);
    data->textures=(struct retdec_mcd_texture*)calloc(1,sizeof(*data->textures));
    CHECK(data->textures);
    data->texture_count=1;
    data->textures[0].handle=kinoko_texture_register((IDirect3DBaseTexture9*)texture,64,64);
    CHECK(data->textures[0].handle);
    resource[0]=PTR(&g313); resource[7]=resource[14]=resource[23]=15;
    retdec_string_assign_cstr(resource+2,"a long named chip resource");
    retdec_string_assign_cstr(resource+9,"data/very-long-chip-file.mcd");
    retdec_string_assign_cstr(resource+18,"a long shared resource prefix/");
    resource[16]=PTR(data);
    kinoko_act_array_append(PTR(source)+224,PTR(resource));
    int32_t virtual_copy=retdec_call_thiscall0_result(resource,(void*)g313.e9);
    CHECK(virtual_copy && virtual_copy!=PTR(resource));
    CHECK(*(int32_t*)(intptr_t)(virtual_copy+64)==PTR(data));
    CHECK(*(int32_t*)(intptr_t)(virtual_copy+68)==resource[17] && resource[17]);
    for(int i=0;i<3;++i) {
        const int offsets[]={8,36,72};
        CHECK(strcmp(retdec_std_string_data(virtual_copy+offsets[i]),retdec_std_string_data(PTR(resource)+offsets[i]))==0);
        CHECK(retdec_std_string_data(virtual_copy+offsets[i])!=retdec_std_string_data(PTR(resource)+offsets[i]));
    }
    int32_t first=PTR(kinoko_act_clone((KinokoActDocument *)source,NULL));
    int32_t second=PTR(kinoko_act_clone((KinokoActDocument *)source,NULL));
    CHECK(first && second && resource[17]);
    const int32_t first_resource=**(int32_t**)(intptr_t)(first+224);
    const int32_t second_resource=**(int32_t**)(intptr_t)(second+224);
    CHECK(*(int32_t*)(intptr_t)(first_resource+64)==PTR(data));
    CHECK(*(int32_t*)(intptr_t)(second_resource+68)==resource[17]);
    retdec_destroy_cact_object(PTR(source));
    CHECK(texture[1]==0);
    retdec_destroy_cact_with_flags(first,1);
    CHECK(texture[1]==0);
    retdec_destroy_cact_with_flags(second,1);
    CHECK(texture[1]==0);
    retdec_destroy_cact_resource(virtual_copy);
    CHECK(texture[1]==1);
    texture[1]=0;
    resource=(int32_t*)calloc(1,100);
    CHECK(resource);
    resource[0]=PTR(&g365); resource[7]=resource[15]=15;
    resource[1]=47;
    retdec_string_assign_cstr(resource+2,"a long texture resource name");
    retdec_string_assign_cstr(resource+10,"data/system/a-long-texture-name");
    resource[17]=kinoko_texture_register((IDirect3DBaseTexture9*)texture,64,64);
    resource[18]=64; resource[19]=64;
    ((float*)resource)[20]=3.0f; ((float*)resource)[21]=7.0f;
    ((float*)resource)[22]=12.0f; ((float*)resource)[23]=24.0f;
    int32_t texture_copy=retdec_call_thiscall0_result(resource,(void*)g365.e9);
    resource[0]=PTR(&g379);
    int32_t target_copy=retdec_call_thiscall0_result(resource,(void*)g379.e9);
    CHECK(texture_copy && target_copy);
    CHECK(*(int32_t*)(intptr_t)texture_copy==PTR(&g365) && *(int32_t*)(intptr_t)target_copy==PTR(&g379));
    CHECK(*(uint8_t*)(intptr_t)(texture_copy+36)==1 && *(uint8_t*)(intptr_t)(target_copy+36)==1);
    CHECK(memcmp((void*)(intptr_t)(texture_copy+72),resource+18,25)==0);
    CHECK(memcmp((void*)(intptr_t)(target_copy+72),resource+18,25)==0);
    retdec_destroy_cact_resource(PTR(resource));
    CHECK(retdec_call_thiscall0_result((void*)(intptr_t)texture_copy,(void*)g365.e11)==1);
    CHECK(*(int32_t*)(intptr_t)(texture_copy+68)==0 && texture[1]==0);
    CHECK(retdec_call_thiscall0_result((void*)(intptr_t)texture_copy,(void*)g365.e11)==1);
    retdec_destroy_cact_resource(texture_copy);
    CHECK(texture[1]==0);
    retdec_destroy_cact_resource(target_copy);
    CHECK(texture[1]==1);
    puts("PASS: native resource clone virtuals, deep names, real Boost ownership and final texture release");
    return 0;
}

static int test_act_reentry(const char *directory) {
    char path[MAX_PATH];
    const char *assets[]={"data/system/title/titlemenu.act","data/worldmap/worldmap.act"};
    for(char archive='a';archive<='c';++archive) {
        sprintf_s(path,sizeof(path),"%s/6kinoko_%c.dat",directory,archive);
        CHECK(kinoko_archive_mount(path));
    }
    for(int asset=0;asset<2;++asset) {
        int32_t source[60]={0}, runtime[48]={0}, holder=PTR(source);
        CHECK(kinoko_act_document_initialize((KinokoActDocument *)source));
        CHECK(kinoko_act_document_load((KinokoActDocument *)source,assets[asset]));
        runtime[0]=PTR(&holder);
        for(int visit=0;visit<3;++visit) {
            CHECK(retdec_bind_act_resource_object(PTR(runtime)));
            const int32_t active=runtime[3];
            CHECK(active!=PTR(source));
            CHECK(*(int32_t *)(intptr_t)(active+212)-*(int32_t *)(intptr_t)(active+208)==source[53]-source[52]);
            for(int index=0;index<(source[53]-source[52])/4;++index) {
                int32_t original=*(int32_t *)(intptr_t)(source[52]+4*index);
                int32_t layer=*(int32_t *)(intptr_t)(*(int32_t *)(intptr_t)(active+208)+4*index);
                CHECK(layer!=original);
                CHECK(*(float *)(intptr_t)(layer+148)==*(float *)(intptr_t)(original+148));
                *(float *)(intptr_t)(layer+148)+=64.0f*(visit+1);
                int32_t oh=*(int32_t *)(intptr_t)(original+180), lh=*(int32_t *)(intptr_t)(layer+180);
                int32_t on=*(int32_t *)(intptr_t)oh, ln=*(int32_t *)(intptr_t)lh;
                while(on!=oh && ln!=lh) {
                    int32_t ok=*(int32_t *)(intptr_t)(on+8), lk=*(int32_t *)(intptr_t)(ln+8);
                    int32_t ol=*(int32_t *)(intptr_t)(ok+4), ll=*(int32_t *)(intptr_t)(lk+4);
                    CHECK(ok!=lk);
                    if(ol) {
                        CHECK(ll && ll!=ol);
                        if(*(int32_t *)(intptr_t)ol==PTR(&g327)) {
                            int32_t ob=*(int32_t *)(intptr_t)(ol+264), oe=*(int32_t *)(intptr_t)(ol+268);
                            int32_t lb=*(int32_t *)(intptr_t)(ll+264);
                            CHECK(*(int32_t *)(intptr_t)(ll+268)-lb==oe-ob);
                            CHECK(!ob || (ob!=lb && memcmp((void *)(intptr_t)ob,(void *)(intptr_t)lb,oe-ob)==0));
                            if(oe>ob) { ((uint8_t *)(intptr_t)lb)[24]^=1; ((float *)(intptr_t)lb)[7]=0; }
                        }
                    }
                    on=*(int32_t *)(intptr_t)on; ln=*(int32_t *)(intptr_t)ln;
                }
                CHECK(on==oh && ln==lh);
            }
        }
        if (asset==1) {
            int checked=0;
            for (int32_t entry=source[56];entry<source[57];entry+=4) {
                int32_t* original=*(int32_t**)(intptr_t)entry;
                if(original[0]!=PTR(&g313)) continue;
                const int index=(entry-source[56])/4;
                int32_t* copy=*(int32_t**)(intptr_t)(*(int32_t*)(intptr_t)(runtime[3]+224)+index*4);
                const int32_t previous=original[16];
                CHECK(previous && copy[16]==previous && original[17]==copy[17]);
                const uint32_t count=((struct retdec_mcd_data*)(intptr_t)previous)->chip_count;
                CHECK(retdec_call_thiscall1_result(original,(void*)g313.e10,PTR("./"))==1);
                CHECK(original[16] && original[16]!=previous && copy[16]==previous);
                CHECK(((struct retdec_mcd_data*)(intptr_t)previous)->chip_count==count);
                CHECK(strcmp(retdec_std_string_data(PTR(original+18)),"./")==0);
                const int32_t refreshed=original[16];
                CHECK(retdec_call_thiscall1_result(original,(void*)g313.e10,PTR("missing-prefix"))==0);
                CHECK(original[16]==refreshed && copy[16]==previous);
                CHECK(strcmp(retdec_std_string_data(PTR(original+18)),"./")==0);
                ++checked;
            }
            CHECK(checked>0);
        }
        retdec_destroy_cact_with_flags(runtime[3],1);
        free((void *)(intptr_t)runtime[4]);
        retdec_destroy_cact_object(PTR(source));
    }
    puts("PASS: title cursor and world-map records restart from independent ACT copies");
    return 0;
}

static int test_script_registrations(int32_t vm, int32_t *root) {
    const int32_t top = sq_gettop(kinoko_vm(vm));
    const char *globals[] = {"ShowCallStack", "CompileFile", "PostQuitMessage", "ReadCSV",
        "LoadTable", "SaveTable", "SetGlobalUpdateFunction", "SetInitFunctionByID",
        "LoadAnimationData", "CreateActor", "CreateActorFromMap", "ClearActor", "MoveActor",
        "ClearCollision", "CreateCollision", "CreateEvent", "ClearRenderLayer", "CreateRenderLayer",
        "LoadAct", "LoadMap", "LoadSE", "ReleaseMap", "MessageBox", "dprint", "Sleep",
        "timeGetTime", "PlaySE", "PlayBgm", "PlayBgmMargin", "FadeBgm", "StopBgm", "PauseBgm"};
    for (int repeat = 0; repeat < 2; ++repeat) {
        kinoko_register_global_methods(PTR(root));
        CHECK(sq_gettop(kinoko_vm(vm)) == top);
        for (int i = 0; i < sizeof(globals)/sizeof(globals[0]); ++i) {
            sq_pushroottable(kinoko_vm(vm));
            sq_pushstring(kinoko_vm(vm), globals[i], -1);
            CHECK(SQ_SUCCEEDED(sq_get(kinoko_vm(vm), -2)));
            CHECK(sq_gettype(kinoko_vm(vm), -1) == OT_NATIVECLOSURE);
            sq_settop(kinoko_vm(vm), top);
        }
        CHECK(execute_source(vm, root + 2,
            "Sleep(0); if(typeof timeGetTime()!=\"integer\") throw \"native return ABI\";"));
    }
    int32_t input[384] = {0};
    input[8] = 73; /* Original GetAssign(-1,3): record +16, then (3+1)*4. */
    function_46d950();
    CHECK(sq_gettop(kinoko_vm(vm)) == top);
    sq_pushroottable(kinoko_vm(vm));
    sq_pushstring(kinoko_vm(vm), "registrationInput", -1);
    CHECK(function_4ab170(vm, PTR("Input"), PTR(input), 0));
    CHECK(SQ_SUCCEEDED(sq_newslot(kinoko_vm(vm), -3, SQFalse)));
    sq_settop(kinoko_vm(vm), top);
    CHECK(execute_source(vm, root + 2,
        "registrationInput.x=-17; registrationInput.y=29;\n"
        "registrationInput.b2=123; registrationInput.kr1=true;\n"
        "registrationInput.k5=456; registrationInput.s0=789; registrationInput.s9=987;\n"
        "if(registrationInput.k2!=123 || !registrationInput.br1 || "
        "registrationInput.GetAssign(-1,3)!=73) throw \"Input alias/receiver\";\n"
        "registrationInput.br1=false; if(registrationInput.kr1) throw \"Input bool alias\";"));
    CHECK(input[359] == -17 && input[360] == 29);
    CHECK(input[363] == 123 && input[366] == 456);
    CHECK(input[368] == 789 && input[377] == 987);
    CHECK(((uint8_t*)input)[1469] == 0 && ((uint8_t*)input)[1468] == 0);
    CHECK(input[358] == 0 && input[378] == 0);
    CHECK(execute_source(vm, root + 2, "delete registrationInput;"));
    CHECK(sq_gettop(kinoko_vm(vm)) == top);
    puts("PASS: global registration repeat/stack/ABI and Input field aliases/native receiver");
    return 0;
}

static int test_physical_input(void) {
    int32_t device[42] = {0}, tracker[261] = {0}, joystick[20] = {0};
    unsigned char scans[] = {0x80, 0xff};
    unsigned char saved_keys[256];
    KinokoControllerState *saved_states = kinoko_input_snapshot.controllers;
    int32_t saved_count = kinoko_input_snapshot.controller_count;
    int i;
    memcpy(saved_keys, g_retdec_keyboard_state, 256);
    memset(g_retdec_keyboard_state, 0, 256);
    device[1] = -1;
    device[2] = 0x80; device[3] = 0xff;
    device[4] = 0xcb; device[5] = 0xcd;
    for (i = 6; i < 18; ++i) device[i] = -1;
    device[6] = 0xff;
    device[21] = 37;
    g_retdec_keyboard_state[0xcb] = g_retdec_keyboard_state[0xcd] = 0x80;
    g_retdec_keyboard_state[0xff] = 0x80;
    CHECK(kinoko_input_device_update((KinokoInputDevice*)device, NULL) == PTR(device) + 128);
    CHECK(device[18] == -1 && device[19] == 1 && device[20] == 1);
    CHECK(device[21] == 37 && ((float*)device)[36] == -1.0f);
    device[20] = INT32_MAX;
    kinoko_input_device_update((KinokoInputDevice*)device, NULL);
    CHECK(device[20] == INT32_MIN);
    memset(g_retdec_keyboard_state, 0, 256);
    kinoko_input_device_update((KinokoInputDevice*)device, NULL);
    CHECK(device[18] == 0 && ((unsigned char*)device)[128] == 1);
    CHECK(((unsigned char*)device)[130] == 0);
    kinoko_input_device_update((KinokoInputDevice*)device, NULL);
    CHECK(((unsigned char*)device)[128] == 0);
    kinoko_input_keys_construct((KinokoKeyTracker *)(intptr_t)(PTR(tracker)));
    for (i=0;i<2;++i) kinoko_input_keys_add((KinokoKeyTracker *)(intptr_t)(PTR(tracker)),scans[i]);
    g_retdec_keyboard_state[0xff] = g_retdec_keyboard_state[0x9d] = 0x80;
    CHECK(kinoko_input_keys_update((KinokoKeyTracker *)(intptr_t)(PTR(tracker))) == 1);
    CHECK(tracker[255] == 1 && tracker[128] == 0);
    CHECK(kinoko_input_key_pressed((KinokoKeyTracker *)(intptr_t)(PTR(tracker)), 0x1ff, 0, 0, 1) == 1);
    CHECK(kinoko_input_key_pressed((KinokoKeyTracker *)(intptr_t)(PTR(tracker)), 0xff, 1, 0, 1) == 0);
    kinoko_input_keys_update((KinokoKeyTracker *)(intptr_t)(PTR(tracker)));
    CHECK(kinoko_input_key_pressed((KinokoKeyTracker *)(intptr_t)(PTR(tracker)), 0xff, 0, 0, 0) == 0);
    kinoko_input_snapshot.controllers = (KinokoControllerState*)joystick; kinoko_input_snapshot.controller_count = 1; device[1] = 0; device[6] = 0;
    CHECK(kinoko_input_controller_state(-1) == NULL);
    CHECK(kinoko_input_controller_state(1) == NULL);
    CHECK(kinoko_input_controller_state(0) == (KinokoControllerState*)joystick);
    joystick[0] = -501; joystick[1] = 501; joystick[2] = 250;
    ((unsigned char*)joystick)[48] = 1;
    kinoko_input_device_update((KinokoInputDevice*)device, NULL);
    CHECK(device[18] == -1 && device[19] == 1 && device[20] == 1);
    CHECK(((float*)device)[38] == 0.25f && device[21] == 37);
    joystick[0] = -500; joystick[1] = 500;
    ((unsigned char*)joystick)[48] = 0;
    kinoko_input_device_update((KinokoInputDevice*)device, NULL);
    CHECK(device[18] == 0 && device[19] == 0 && device[20] == 0);
    CHECK(((unsigned char*)device)[128] && ((unsigned char*)device)[129] && ((unsigned char*)device)[130]);
    device[1] = -2;
    kinoko_input_device_update((KinokoInputDevice*)device, NULL);
    for (i = 18; i < 42; ++i) CHECK(device[i] == 0);
    kinoko_input_keys_clear((KinokoKeyTracker *)(intptr_t)(PTR(tracker)));
    for(i=0;i<256;++i) { kinoko_input_keys_add((KinokoKeyTracker *)(intptr_t)(PTR(tracker)),(uint8_t)i); kinoko_input_keys_add((KinokoKeyTracker *)(intptr_t)(PTR(tracker)),(uint8_t)i); }
    CHECK(kinoko_input_keys_size((KinokoKeyTracker *)(intptr_t)(PTR(tracker)))==256);
    for(i=0;i<256;++i) CHECK(kinoko_input_keys_at((KinokoKeyTracker *)(intptr_t)(PTR(tracker)),i)==i);
    kinoko_input_keys_destroy((KinokoKeyTracker *)(intptr_t)(PTR(tracker)));
    kinoko_input_snapshot.controllers = saved_states; kinoko_input_snapshot.controller_count = saved_count;
    memcpy(g_retdec_keyboard_state, saved_keys, 256);
    return 0;
}

static int test_input_configuration(void) {
    int32_t manager[384]={0};
    kinoko_input_devices_construct(PTR(manager));
    kinoko_input_devices_resize(PTR(manager),2);
    int32_t (*devices)[42]=(int32_t(*)[42])(intptr_t)kinoko_input_devices_begin(PTR(manager));
    char path[MAX_PATH]; GetModuleFileNameA(NULL,path,MAX_PATH);
    char *filename=strrchr(path,'\\'); CHECK(filename!=NULL);
    sprintf_s(filename+1,MAX_PATH-(filename+1-path),"input-contract-%lu.dat",GetCurrentProcessId());
    manager[4]=0xfe; manager[8]=55;
    devices[0][1]=0; devices[0][9]=7;
    devices[1][1]=1; devices[1][9]=19;
    const int old_count=kinoko_input_snapshot.controller_count; kinoko_input_snapshot.controller_count=2;
    function_46b7c0(PTR(manager),PTR(path));
    memset(manager+4,0,68); memset(devices[0]+1,0,68); memset(devices[1]+1,0,68);
    function_46b880(PTR(manager),PTR(path));
    CHECK(manager[4]==0xfe && manager[8]==55);
    CHECK(devices[0][9]==7 && devices[1][9]==7); /* First saved device broadcasts to all. */
    CHECK(function_46be40(PTR(manager),-1,3)==55);
    function_46bbe0(PTR(manager),1,3,23);
    CHECK(devices[1][9]==23 && devices[0][9]==7);
    unsigned char previous[256]; memcpy(previous,g_retdec_keyboard_state,256);
    memset(g_retdec_keyboard_state,0,256);
    g_retdec_keyboard_state[58]=g_retdec_keyboard_state[112]=g_retdec_keyboard_state[148]=0x80;
    CHECK(function_46bc90(PTR(manager),-1,3)==0);
    g_retdec_keyboard_state[149]=0x80;
    CHECK(function_46bc90(PTR(manager),-1,3)==1 && manager[8]==149);
    memcpy(g_retdec_keyboard_state,previous,256); kinoko_input_snapshot.controller_count=old_count;
    kinoko_input_devices_destroy(PTR(manager));
    puts("PASS: Input config two-record format/broadcast, assignment offsets and original excluded keys");
    return 0;
}

static int test_input_copy(void) {
    int32_t source[378]={0}, target[378]={0}, empty[378]={0};
    kinoko_input_devices_construct(PTR(source));
    kinoko_input_devices_construct(PTR(target));
    kinoko_input_devices_construct(PTR(empty));
    kinoko_input_devices_resize(PTR(source),2);
    int32_t (*devices)[42]=(int32_t(*)[42])(intptr_t)kinoko_input_devices_begin(PTR(source));
    unsigned char keys[9]={3,7,11,19,23,29,31,37,41};
    kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR(source)));
    kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR(target)));
    kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR(empty)));
    source[3]=0x1111; target[3]=0x2222;
    source[49]=0x3333; target[49]=0x4444;
    memset((char*)source+16,0x17,164);
    memset((char*)source+200,0x23,164);
    kinoko_input_keys_construct((KinokoKeyTracker *)(intptr_t)(PTR(source)+392));
    kinoko_input_keys_construct((KinokoKeyTracker *)(intptr_t)(PTR(target)+392));
    kinoko_input_keys_construct((KinokoKeyTracker *)(intptr_t)(PTR(empty)+392));
    memset((char*)source+392,0x35,1024);
    memset((char*)source+1436,0x47,76);
    devices[0][0]=devices[1][0]=PTR(g35);
    devices[0][1]=101; devices[1][41]=303;
    source[48]=12345; target[48]=54321; /* vector allocator byte is not copied */
    kinoko_input_cluster_construct(PTR(source)+196);
    kinoko_input_cluster_construct(PTR(target)+196);
    kinoko_input_cluster_construct(PTR(empty)+196);
    for(int i=0;i<40;++i) kinoko_input_cluster_append(PTR(source)+196,PTR(devices[i%2]));
    source[91]=0x5555; target[91]=0x6666; /* preserve iterator proxy */
    for(int i=0;i<9;++i) kinoko_input_keys_add((KinokoKeyTracker *)(intptr_t)(PTR(source)+392),keys[i]);
    source[357]=45678; target[357]=87654;
    ((char*)source)[388]=9; ((char*)source)[1432]=1; ((char*)source)[1434]=3;
    CHECK(function_46ed80(PTR(target),PTR(source))==PTR(target));
    CHECK(target[3]==0x2222 && target[49]==0x4444);
    CHECK(target[48]==54321 && target[91]==0x6666 && target[357]==87654);
    CHECK(memcmp((char*)target+16,(char*)source+16,164)==0);
    CHECK(memcmp((char*)target+200,(char*)source+200,164)==0);
    CHECK(memcmp((char*)target+392,(char*)source+392,1024)==0);
    CHECK(memcmp((char*)target+1436,(char*)source+1436,76)==0);
    CHECK(target[45]!=source[45] && target[354]!=source[354] && target[92]!=source[92]);
    CHECK(memcmp((void*)(intptr_t)kinoko_input_devices_begin(PTR(target)),devices,336)==0);
    CHECK(kinoko_input_keys_size((KinokoKeyTracker *)(intptr_t)(PTR(target)+392))==9);
    for(int i=0;i<9;++i) CHECK(kinoko_input_keys_at((KinokoKeyTracker *)(intptr_t)(PTR(target)+392),i)==keys[i]);
    CHECK(((char*)target)[388]==9 && ((char*)target)[1432]==1 && ((char*)target)[1434]==3);
    CHECK(kinoko_input_cluster_size(PTR(target)+196)==40);
    for(int i=0;i<40;++i)
        CHECK(kinoko_input_cluster_at(PTR(target)+196,i)==PTR(devices[i%2]));
    {
        int32_t buffer=kinoko_input_devices_begin(PTR(target)), queue=target[92], byte_buffer=target[354];
        kinoko_input_devices_resize(PTR(source),1);
        kinoko_input_keys_clear((KinokoKeyTracker *)(intptr_t)(PTR(source)+392));
        for(int i=0;i<4;++i) kinoko_input_keys_add((KinokoKeyTracker *)(intptr_t)(PTR(source)+392),keys[i]);
        CHECK(kinoko_input_keys_size((KinokoKeyTracker *)(intptr_t)(PTR(target)+392))==9);
        kinoko_input_cluster_clear(PTR(source)+196);
        kinoko_input_cluster_append(PTR(source)+196,PTR(devices[0]));
        devices[0][1]=909;
        CHECK(function_46ed80(PTR(target),PTR(source))==PTR(target));
        CHECK(kinoko_input_devices_begin(PTR(target))==buffer && kinoko_input_devices_end(PTR(target))==buffer+168);
        CHECK(target[92]==queue && kinoko_input_cluster_size(PTR(target)+196)==1 && target[354]==byte_buffer && kinoko_input_keys_size((KinokoKeyTracker *)(intptr_t)(PTR(target)+392))==4);
        CHECK(((int32_t*)(intptr_t)buffer)[1]==909);
        CHECK(function_46ed80(PTR(target),PTR(target))==PTR(target) && kinoko_input_devices_begin(PTR(target))==buffer);
        CHECK(function_46ed80(PTR(target),PTR(empty))==PTR(target));
        CHECK(kinoko_input_devices_begin(PTR(target))==kinoko_input_devices_end(PTR(target)));
        CHECK(kinoko_input_cluster_size(PTR(target)+196)==0);
        CHECK(target[354]==byte_buffer && kinoko_input_keys_size((KinokoKeyTracker *)(intptr_t)(PTR(target)+392))==0);
    }
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(source))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(target))));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(empty))));
    kinoko_input_cluster_delete(PTR(source)+196,NULL,0);
    kinoko_input_cluster_delete(PTR(target)+196,NULL,0);
    kinoko_input_cluster_delete(PTR(empty)+196,NULL,0);
    kinoko_input_keys_destroy((KinokoKeyTracker *)(intptr_t)(PTR(source)+392));
    kinoko_input_keys_destroy((KinokoKeyTracker *)(intptr_t)(PTR(target)+392));
    kinoko_input_keys_destroy((KinokoKeyTracker *)(intptr_t)(PTR(empty)+392));
    kinoko_input_devices_destroy(PTR(source));
    kinoko_input_devices_destroy(PTR(target));
    kinoko_input_devices_destroy(PTR(empty));
    puts("PASS: Input copy owns vectors and native deque while keeping shallow device pointers");
    return 0;
}

static int test_string_layout_binding(int32_t vm,int32_t* root) {
    int32_t object[65]={0},copy[65]={0},klass[2]={g483,g484},instance[2]={g483,g484};
    CHECK(kinoko_construct_string_layout(PTR(object))==PTR(object));
    CHECK(kinoko_construct_string_layout(PTR(copy))==PTR(copy));
    int32_t top=function_48aa20(vm);
    CHECK(kinoko_publish_string_layout_class(vm,PTR(root),klass));
    CHECK(retdec_create_bound_instance(vm,root+2,"StringProbe",klass,PTR(object),instance));
    kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), instance);
    CHECK(execute_source(vm,root+2,
        "StringProbe.fontHeight=999; StringProbe.fontWeight=-1; StringProbe.colorR=-3; StringProbe.baseB=999;\n"
        "if(StringProbe.fontHeight!=127 || StringProbe.fontWeight!=1 || StringProbe.colorR!=0 || StringProbe.baseB!=255) throw \"font clamp\";\n"
        "StringProbe.charactorSpace=-2; StringProbe.lineSpace=-3; StringProbe.stFontFaceName=\"\";\n"
        "if(StringProbe.charactorSpace!=0 || StringProbe.lineSpace!=0 || StringProbe.stFontFaceName.len()!=13) throw \"font defaults\";\n"
        "StringProbe.stText=\"X\"; if(StringProbe.stText!=\"X\") throw \"text property\";\n"
        "if(!StringProbe.PushBack(\"AB\") || !StringProbe.PopFront(1) || !StringProbe.PopBack(1)) throw \"text pop\";\n"
        "if(StringProbe.GetCharacterBytes(\"a\")!=1 || StringProbe.GetCharacterBytes(null)!=0 || StringProbe.PopBack(-1)) throw \"text args\";\n"
        "if(!StringProbe.Rebuild()) throw \"rebuild\";\n"));
    CHECK(object[12]==1 && retdec_std_string_data(PTR(object)+32)[0]=='A'); /* original ASCII PopFront erases zero */
    CHECK(((unsigned char*)object)[228]==1);
    int32_t atlas[109]={0};atlas[108]=2;
    for(int i=0;i<2;++i) {
        int32_t* glyph=(int32_t*)(intptr_t)kinoko_string_append_glyph(PTR(object));
        glyph[0]=10+i;glyph[2]=0;glyph[63]=PTR(atlas);
    }
    CHECK(kinoko_string_replicate(PTR(copy),PTR(object))==1 && atlas[108]==4 && kinoko_string_queue_size(PTR(copy))==2);
    CHECK(copy[44]!=object[44]);
    CHECK(((int32_t*)(intptr_t)kinoko_string_queue_at(PTR(copy),0))[0]==10);
    CHECK(kinoko_string_replicate(PTR(copy),PTR(copy))==1 && atlas[108]==4);
    CHECK(kinoko_string_clear(PTR(copy))==1 && atlas[108]==4 && kinoko_string_queue_size(PTR(copy))==2);
    kinoko_clear_string_layout(PTR(copy));CHECK(atlas[108]==2);
    CHECK(execute_source(vm,root+2,"delete ::StringProbe;\n"));
    kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), klass);kinoko_clear_string_layout(PTR(object));
    CHECK(atlas[108]==0 && function_48aa20(vm)==top);
    puts("PASS: actual Sqrat CStringLayout methods, property clamps, pending-text quirk and borrowed atlas replication");
    return 0;
}

static int test_string_layout_lifetime(void) {
    int32_t layout[65];memset(layout,0xa5,sizeof(layout));
    CHECK(kinoko_construct_string_layout(PTR(layout))==PTR(layout));
    CHECK(layout[0]==PTR(&g350) && layout[6]==15 && layout[13]==15 && layout[20]>=16);
    CHECK(layout[5]==0 && layout[12]==0 && layout[19]==13);
    const unsigned char face[]={0x82,0x6c,0x82,0x72,0x20,0x83,0x53,0x83,0x56,0x83,0x62,0x83,0x4e,0};
    CHECK(memcmp(retdec_std_string_data(PTR(layout)+60),face,sizeof(face))==0);
    CHECK(layout[22]==16 && layout[23]==1 && layout[31]==2 && layout[36]==-1);
    CHECK(layout[27]==255 && layout[28]==255 && layout[29]==255);
    CHECK(((float*)layout)[34]==1 && ((float*)layout)[35]==1 && ((float*)layout)[38]==1);
    CHECK(layout[44]!=0 && kinoko_string_queue_size(PTR(layout))==0);
    retdec_string_assign_cstr(layout+1,"rendered");
    kinoko_string_push_back(PTR(layout),"pending");
    layout[50]=91;layout[51]=7;layout[52]=11;layout[53]=37;
    int32_t clone=kinoko_method_clone_string_layout(PTR(layout),NULL);CHECK(clone);
    int32_t* copied=(int32_t*)(intptr_t)clone;
    CHECK(copied[0]==PTR(g350) && copied[44]!=layout[44]);
    CHECK(copied[5]==8 && copied[12]==7 && copied[50]==91 && copied[53]==37);
    CHECK(kinoko_string_atlas_size(clone)==0 && kinoko_string_queue_size(clone)==0);
    CHECK(kinoko_method_set_string_layer(clone,NULL,0)<0);
    CHECK(kinoko_method_update_string_layout(clone,NULL)<0);
    CHECK(kinoko_method_draw_string_layout(clone,NULL,0,0)<0);
    CHECK(kinoko_string_add_character(clone,"\t")==1 && copied[51]==64);
    CHECK(kinoko_string_add_character(clone,"\n")==1 && copied[51]==0 && copied[52]==27);
    kinoko_method_delete_string_layout(clone,NULL,1);
    CHECK(kinoko_method_delete_string_layout(PTR(layout),NULL,0)==PTR(layout));
    CHECK(layout[44]==0 && layout[40]==0 && layout[41]==0 && layout[42]==0);
    CHECK(layout[19]==0 && layout[20]==15);
    int32_t* cookie=(int32_t*)malloc(4+520);CHECK(cookie);cookie[0]=2;
    CHECK(kinoko_construct_string_layout(PTR(cookie+1))==PTR(cookie+1));
    CHECK(kinoko_construct_string_layout(PTR(cookie+66))==PTR(cookie+66));
    CHECK(kinoko_method_delete_string_layout(PTR(cookie+1),NULL,2)==PTR(cookie));
    CHECK(cookie[45]==0 && cookie[110]==0);free(cookie);
    puts("PASS: CStringLayout original CP932 defaults, receiver, proxy and scalar/array destruction");
    return 0;
}

static int test_string_glyph_cache(void) {
    int32_t layout[65]={0};
    kinoko_construct_string_layout(PTR(layout));
    int32_t* atlas=(int32_t*)(intptr_t)kinoko_string_append_atlas(PTR(layout));
    atlas[(24+344)/4]=PTR(malloc(32));
    atlas[5]=4;atlas[108]=2;
    layout[6]=layout[13]=15;
    retdec_string_assign_cstr(layout+1,"AB");
    retdec_string_assign_cstr(layout+8,"CD");
    layout[22]=19;
    const int32_t storage=layout[44];
    for(int i=0;i<2;++i) {
        int32_t* glyph=(int32_t*)(intptr_t)kinoko_string_append_glyph(PTR(layout));
        glyph[2]=4;glyph[63]=PTR(atlas);
    }
    CHECK(function_441250(layout)==1 && kinoko_string_atlas_size(PTR(layout))==1);
    CHECK(function_4410c0(PTR(layout))==1);
    CHECK(kinoko_string_atlas_size(PTR(layout))==0);
    CHECK(layout[44]==storage && kinoko_string_queue_size(PTR(layout))==0);
    CHECK(layout[5]==0 && layout[12]==4 && memcmp(retdec_std_string_data(PTR(layout)+32),"ABCD",5)==0);
    CHECK(((unsigned char*)layout)[228]==1 && layout[51]==0 && layout[52]==0 && layout[53]==0 && layout[54]==19);
    kinoko_clear_string_layout(PTR(layout));
    puts("PASS: native glyph deque protects live atlas; rebuild releases references/storage and preserves text order");
    return 0;
}

static int test_map_manager_copy(void) {
    int32_t source[21]={0},target[21]={0},first,last;
    uint32_t capacity;
    kinoko_sqplus_object_initialize(source);
    kinoko_sqplus_object_initialize(target);
    kinoko_map_containers_construct(PTR(source));kinoko_map_containers_construct(PTR(target));
    source[3]=123; source[4]=456; source[5]=789;
    first=kinoko_map_append_render(PTR(source),222);
    last=kinoko_map_append_render(PTR(source),444);
    CHECK(first!=last && kinoko_map_render_at(PTR(source),0)==first);
    *(int32_t*)(intptr_t)first=111;*(int32_t*)(intptr_t)last=333;
    kinoko_map_append_event(PTR(source),71);kinoko_map_append_event(PTR(source),83);
    kinoko_map_append_event(PTR(source),97);
    source[8]=1;target[8]=2;source[12]=3;target[12]=4;
    for(int i=13;i<21;++i) source[i]=100+i;
    CHECK(function_4701b0(PTR(target),PTR(source))==PTR(target));
    CHECK(source[5]==0 && target[5]==789 && target[3]==123 && target[4]==456);
    CHECK(target[8]==2 && target[12]==4 && kinoko_map_render_count(PTR(target))==2);
    CHECK(kinoko_map_render_at(PTR(target),0)!=first);
    first=kinoko_map_render_at(PTR(target),0);last=kinoko_map_render_at(PTR(target),1);
    CHECK(*(int32_t*)(intptr_t)first==PTR(&g37) && *(int32_t*)(intptr_t)last==PTR(&g37));
    CHECK(*(int32_t*)(intptr_t)(first+4)==222 && *(int32_t*)(intptr_t)(last+4)==444);
    CHECK(kinoko_map_event_count(PTR(target))==3 && kinoko_map_event_at(PTR(target),1)==83);
    CHECK(memcmp(target+13,source+13,32)==0);
    CHECK(function_4701b0(PTR(target),PTR(target))==PTR(target) && target[5]==789);
    CHECK(kinoko_map_render_at(PTR(target),0)==first);
    target[5]=0; /* borrowed marker; no fabricated player destruction */
    capacity=kinoko_map_event_capacity(PTR(target));
    kinoko_map_containers_clear(PTR(source));kinoko_map_append_event(PTR(source),71);
    CHECK(function_4701b0(PTR(target),PTR(source))==PTR(target));
    CHECK(kinoko_map_render_count(PTR(target))==0);
    CHECK(kinoko_map_event_count(PTR(target))==1 && kinoko_map_event_at(PTR(target),0)==71);
    CHECK(kinoko_map_event_capacity(PTR(target))==capacity);
    kinoko_map_containers_clear(PTR(source));
    CHECK(function_4701b0(PTR(target),PTR(source))==PTR(target));
    CHECK(kinoko_map_event_count(PTR(target))==0 && kinoko_map_event_capacity(PTR(target))==capacity);
    kinoko_map_append_event(PTR(target),0);kinoko_map_append_event(PTR(target),71);
    kinoko_map_append_event(PTR(target),71);
    CHECK(kinoko_map_event_count(PTR(target))==3 && kinoko_map_event_at(PTR(target),0)==0);
    CHECK(kinoko_map_event_at(PTR(target),1)==71 && kinoko_map_event_at(PTR(target),2)==71);
    kinoko_map_containers_destroy(PTR(source));kinoko_map_containers_destroy(PTR(target));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(source))));(int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(target))));
    puts("PASS: native map list/vector copies, stable render addresses, duplicates/nulls and capacity retention");
    return 0;
}

static int test_input_aggregation(void) {
    int32_t cluster[50] = {0}, devices[3][42] = {{0}};
    kinoko_input_cluster_construct(PTR(cluster));
    for(int i=0;i<3;++i) kinoko_input_cluster_append(PTR(cluster),PTR(devices[i]));
    ((uint8_t*)devices[0])[4]=10; ((uint8_t*)devices[1])[4]=20; ((uint8_t*)devices[2])[4]=30;
    devices[0][18]=-5; devices[1][18]=5; devices[2][18]=3;
    devices[0][19]=2; devices[1][19]=-6;
    devices[0][20]=9; devices[1][20]=9;
    ((uint8_t*)devices[1])[130]=1; /* A held button must not inherit a release edge. */
    ((uint8_t*)devices[0])[131]=1; /* Zero count does accumulate release edges. */
    devices[2][31]=7; /* The twelfth button participates (not only ten buttons). */
    ((float*)devices[0])[36]=-0.75f; ((float*)devices[1])[36]=0.75f;
    ((float*)devices[2])[41]=-0.9f;
    cluster[48]=99;
    CHECK(function_4077c0(PTR(cluster))==3);
    CHECK(cluster[18]==-5 && cluster[19]==-6 && cluster[20]==9 && cluster[31]==7);
    CHECK(((uint8_t*)cluster)[130]==0 && ((uint8_t*)cluster)[131]==1);
    CHECK(((uint8_t*)cluster)[192]==30);
    CHECK(((float*)cluster)[36]==-0.75f && ((float*)cluster)[41]==-0.9f);
    kinoko_input_cluster_clear(PTR(cluster));
    CHECK(function_4077c0(PTR(cluster))==PTR(cluster)+72);
    for(int i=18;i<42;++i) CHECK(cluster[i]==0);
    CHECK(((uint8_t*)cluster)[192]==30); /* Last device survives an empty frame. */
    CHECK(kinoko_input_cluster_delete(PTR(cluster),NULL,0)==PTR(cluster));
    CHECK(cluster[0]==PTR(g35) && cluster[43]==0);
    puts("PASS: native InputCluster deque, signed axes, 12 buttons, release edges and device precedence");
    return 0;
}

static int test_table_serialization(int32_t vm, int32_t *root) {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL,path,MAX_PATH);
    char *filename=strrchr(path,'\\'); CHECK(filename!=NULL);
    sprintf_s(filename+1,MAX_PATH-(filename+1-path),"table-contract-%lu.dat",GetCurrentProcessId());
    CHECK(execute_source(vm,root+2,
        "serializationSource <- {n=123,f=1.25,b=true,s=\"abc\",empty=null,"
        "nested={value=-9},a=[4,null,false,\"tail\"]};\nserializationTarget <- {};"));
    int32_t source[3], target[3], owned[3];
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(source)), "serializationSource");
    CHECK(retdec_squirrel_object_copy(owned,source));
    CHECK(function_472e50(PTR(path),owned[0],owned[1],owned[2]));
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(target)), "serializationTarget");
    CHECK(retdec_squirrel_object_copy(owned,target));
    CHECK(function_472c90(PTR(path),owned[0],owned[1],owned[2]));
    CHECK(execute_source(vm,root+2,
        "if(serializationTarget.n!=123 || serializationTarget.f!=1.25 || !serializationTarget.b || "
        "serializationTarget.s!=\"abc\" || serializationTarget.nested.value!=-9 || "
        "serializationTarget.a.len()!=4 || serializationTarget.a[0]!=4 || "
        "serializationTarget.a[1]!=null || serializationTarget.a[2]!=false || "
        "serializationTarget.a[3]!=\"tail\" || (\"empty\" in serializationTarget)) throw \"save format\";"));
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(target)))); (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(source))));
    puts("PASS: original compressed table format nested arrays/tables/scalars and skipped null values");
    return 0;
}

static int test_camera_map_bindings(int32_t vm, int32_t *root) {
    const int top=sq_gettop(kinoko_vm(vm));
    int32_t camera[128]={0}, map[32]={0};
    function_4669d0(); function_46fac0();
    const char *names[]={"Camera","Map"}, *slots[]={"moduleCamera","moduleMap"};
    int32_t pointers[]={PTR(camera),PTR(map)};
    for(int i=0;i<2;++i) {
        sq_pushroottable(kinoko_vm(vm)); sq_pushstring(kinoko_vm(vm),slots[i],-1);
        CHECK(function_4ab170(vm,PTR(names[i]),pointers[i],0));
        CHECK(SQ_SUCCEEDED(sq_newslot(kinoko_vm(vm),-3,SQFalse)));
        sq_settop(kinoko_vm(vm),top);
    }
    CHECK(execute_source(vm,root+2,
        "moduleCamera.x=12.5; moduleCamera.offset_y=-3.0; moduleCamera.right=640.0;\n"
        "moduleMap.width=321; moduleMap.last_id=27; moduleMap.last_bottom=123.5;\n"
        "moduleCamera.SetUpdateFunction(function(){});"));
    CHECK(((float*)camera)[10]==12.5f && ((float*)camera)[15]==-3.0f && ((float*)camera)[20]==640.0f);
    CHECK(map[19]==321 && map[14]==27 && ((float*)map)[18]==123.5f);
    CHECK(camera[8]==0x08000100); /* Callback is retained through the real native receiver. */
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(camera+7)))); (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(camera+4))));
    CHECK(execute_source(vm,root+2,"delete moduleCamera; delete moduleMap;"));
    CHECK(sq_gettop(kinoko_vm(vm))==top);
    puts("PASS: Camera/Map native fields and Camera SetUpdateFunction receiver ownership");
    return 0;
}

static int test_map_registration(int32_t vm, int32_t *root) {
    const int top = sq_gettop(kinoko_vm(vm));
    int32_t layout[84] = {0}, chips[16] = {0};
    CHECK(function_433c90(0) == (int32_t)E_INVALIDARG);
    CHECK(function_433c90(vm) == 0);
    CHECK(function_433c90(vm) == 0);
    const char *classes[] = {"C2DMapLayout", "ChipLayout"};
    const char *slots[] = {"testMapLayout", "testChipLayout"};
    void *objects[] = {layout, chips};
    for (int i = 0; i < 2; ++i) {
        sq_pushroottable(kinoko_vm(vm));
        sq_pushstring(kinoko_vm(vm), slots[i], -1);
        sq_pushstring(kinoko_vm(vm), classes[i], -1);
        CHECK(SQ_SUCCEEDED(sq_get(kinoko_vm(vm), -3)));
        CHECK(SQ_SUCCEEDED(sq_createinstance(kinoko_vm(vm), -1)));
        CHECK(SQ_SUCCEEDED(sq_setinstanceup(kinoko_vm(vm), -1, objects[i])));
        sq_remove(kinoko_vm(vm), -2);
        CHECK(SQ_SUCCEEDED(sq_newslot(kinoko_vm(vm), -3, SQFalse)));
        sq_settop(kinoko_vm(vm), top);
    }
    CHECK(execute_source(vm, root+2,
        "if(testMapLayout.left!=0 || testMapLayout.right!=0 || testMapLayout.chipCount!=0) throw 1;\n"
        "testChipLayout.left=11; testChipLayout.top=19;\n"
        "testChipLayout.f_left=-3.75; testChipLayout.f_top=2.5;\n"
        "if(testChipLayout.left!=-3 || testChipLayout.top!=19) throw 2;\n"));
    CHECK(chips[1] == -3 && chips[2] == 19);
    CHECK(((float*)chips)[3] == -3.75f && ((float*)chips)[4] == 2.5f);
    chips[1] = 12; chips[9] = 34;
    layout[66] = PTR(chips); layout[67] = PTR(chips+16);
    CHECK(execute_source(vm, root+2,
        "if(testMapLayout.left!=12 || testMapLayout.right!=34 || testMapLayout.chipCount!=2) throw 3;\n"
        "delete testMapLayout; delete testChipLayout;\n"));
    CHECK(sq_gettop(kinoko_vm(vm)) == top);
    puts("PASS: original map registry, empty bounds and asymmetric fractional chip setters");
    return 0;
}

struct script_io_stream {
    int32_t* vtable;
    uint32_t position, size;
    int reading;
    unsigned char bytes[8192];
};
static int32_t __fastcall script_io_transfer(struct script_io_stream* self, void* unused, void* data, uint32_t size) {
    (void)unused;
    if (size > sizeof(self->bytes)-self->position || (self->reading && size > self->size-self->position)) return 0;
    if (size) {
        if (self->reading) memcpy(data,self->bytes+self->position,size);
        else memcpy(self->bytes+self->position,data,size);
    }
    self->position+=size;
    if (!self->reading && self->position>self->size) self->size=self->position;
    return 1;
}
static int32_t __fastcall script_io_seek(struct script_io_stream* self, void* unused, int32_t offset, int32_t origin) {
    (void)unused;
    self->position=(origin==0 ? 0 : origin==1 ? self->position : self->size)+offset;
    return self->position;
}

static int test_map_serialization(void) {
    int32_t methods[6]={0,0,0,PTR(script_io_transfer),0,PTR(script_io_seek)};
    struct script_io_stream stream={0}; stream.vtable=methods;
    int32_t holder=PTR(&stream), source[116]={0}, loaded[116]={0}, resource[25]={0};
    struct retdec_mcd_chip chips[2]={0};
    struct retdec_mcd_data data={2,chips,0,NULL};
    int32_t records[4][8]={{9,100,-30,0,0,41,0,0},{2,-5,-20,0,0,42,0,0},
                         {2,-5,-40,0,0,43,0,0},{99,200,-50,0,0,44,0,0}};
    chips[0].chip_id=9; chips[1].chip_id=2;
    *(int32_t*)(chips[0].bytes)=9; *(int32_t*)(chips[1].bytes)=2;
    *(int16_t*)(chips[0].bytes+12)=16; *(int16_t*)(chips[0].bytes+14)=8;
    *(int16_t*)(chips[1].bytes+12)=32; *(int16_t*)(chips[1].bytes+14)=12;
    source[0]=loaded[0]=PTR(&g327); source[79]=PTR(resource); resource[16]=PTR(&data);
    source[66]=PTR(records); source[67]=source[68]=PTR(records+4); source[113]=-1;
    source[59]=7; ((float*)source)[80]=0.75f; ((float*)source)[81]=1.25f;
    source[82]=3; loaded[82]=17;
    const unsigned char saved=g673;
    for (int compact=0; compact<2; ++compact) {
        g673=(unsigned char)compact; stream.position=stream.size=0; stream.reading=0;
        CHECK(retdec_call_thiscall1_result(source,(void*)g327.e0,PTR(&stream))==1);
        CHECK(stream.bytes[0]==!compact);
        if (!compact) CHECK(*(uint32_t*)(stream.bytes+1)==9); /* No blend property. */
        CHECK(records[0][1]==-5 && records[0][2]==-40 && records[0][5]==43);
        CHECK(records[1][2]==-20 && records[2][0]==9 && records[3][0]==99);
        CHECK(source[60]==32 && source[61]==12 && source[62]==-5 && source[63]==-50);
        CHECK(source[64]==200 && source[65]==200); /* Original bottom starts at last X. */
        CHECK(source[102]-source[101]==96 && source[110]-source[109]==40);
        CHECK(*(int32_t*)(intptr_t)source[101]==2 && *(int32_t*)(intptr_t)(source[101]+48)==9);
        CHECK(((int32_t*)(intptr_t)source[109])[2]==0 && ((int32_t*)(intptr_t)source[109])[9]==1);
        CHECK(((int32_t*)(intptr_t)source[109])[1]==-1);
        /* Writer emits only 12 bytes per map record. */
        CHECK(*(uint32_t*)(stream.bytes+stream.size-56)==4);
        CHECK(*(uint32_t*)(stream.bytes+stream.size-52)==12);
        stream.position=0; stream.reading=1;
        CHECK(retdec_call_thiscall2_result(loaded,(void*)g327.e1,PTR(&holder),1)==1);
        CHECK(stream.position==stream.size && loaded[82]==17);
        CHECK(memcmp(source+59,loaded+59,7*4)==0 && ((float*)loaded)[80]==0.75f);
        CHECK((loaded[67]-loaded[66])/32==(compact+1)*4); /* Read appends. */
        int32_t *tail=(int32_t*)(intptr_t)(loaded[66]+compact*128);
        for(int i=0;i<4;++i) {
            CHECK(memcmp(tail+i*8,records[i],12)==0 && tail[i*8+5]==i);
            CHECK(((uint8_t*)(tail+i*8))[24]==1 && ((float*)(tail+i*8))[7]==1.0f);
        }
    }
    /* A truncated record fails without replacing the owned record allocation. */
    int32_t before=loaded[66]; uint32_t available=stream.size;
    stream.position=0; stream.size-=1;
    CHECK(!retdec_call_thiscall2_result(loaded,(void*)g327.e1,PTR(&holder),1));
    CHECK(loaded[66]==before && loaded[67]-loaded[66]==256);
    stream.size=available;
    source[67]=source[66]; stream.position=stream.size=0; stream.reading=0;
    CHECK(retdec_call_thiscall1_result(source,(void*)g327.e0,PTR(&stream))==1);
    CHECK(source[60]==INT_MIN && source[61]==INT_MIN);
    CHECK(source[62]==0 && source[63]==0 && source[64]==0 && source[65]==0);
    g673=saved;
    kinoko_native_buffer_destroy(PTR(loaded)+264);
    kinoko_native_buffer_destroy(PTR(source)+404); kinoko_native_buffer_destroy(PTR(source)+436);
    puts("PASS: map wire format, signed XY order, sparse MCD cache, original bounds and appended records");
    return 0;
}

static int test_dynamic_layer(int32_t vm, int32_t* root) {
    int32_t player[50]={0}, act[60]={0}, holder=PTR(act), player_pair[2]={g483,g484};
    const int top=sq_gettop(kinoko_vm(vm));
    player[4]=PTR(&holder); player[37]=kinoko_sqrat_object_vtable(); player[38]=vm;
    player[39]=root[2]; player[40]=root[3]; player[46]=15;
    retdec_string_assign_cstr(player+41,"dynamicHost");
    InitializeCriticalSection((struct retdec_RTL_CRITICAL_SECTION*)(player+5));
    CHECK(execute_source(vm,root+2,"dynamicHost <- {};"));
    CHECK(retdec_publish_acting_player_class(vm,PTR(root)));
    CHECK(retdec_publish_acting_player(vm,root+2,"dynamicPlayer",PTR(player),player_pair));
    /* The original accepts a borrowed instance pointer and null to clear it.
       Check the actual state change, not only the wrapper's return type. */
    CHECK(execute_source(vm,root+2,
        "if(!dynamicPlayer.SetRenderTarget(dynamicPlayer)) throw \"target set\";"));
    CHECK(player[19]==PTR(player));
    CHECK(execute_source(vm,root+2,
        "if(!dynamicPlayer.SetRenderTarget(null)) throw \"target clear\";"));
    CHECK(player[19]==0);
    CHECK(execute_source(vm,root+2,"inactiveLayer <- dynamicPlayer.CreateLayer2D(\"inactive\");"));
    CHECK(act[52]==0 && act[53]==0);
    ((uint8_t*)player)[8]=1;
    CHECK(execute_source(vm,root+2,
        "dynamicFirst <- dynamicPlayer.CreateLayer2D(\"first\");\n"
        "dynamicSecond <- dynamicPlayer.CreateLayer2D(\"a long dynamically created layer\");\n"
        "if(dynamicFirst.layerID!=1 || dynamicSecond.layerID!=2) throw \"layer ids\";\n"
        "dynamicFirst.alpha=0.375; dynamicSecond.colorR=17;\n"
        "if(dynamicHost.first.alpha!=0.375 || dynamicFirst.layout.alpha!=0.375) throw \"layer alias\";\n"));
    CHECK(act[53]-act[52]==8);
    int32_t* layers=(int32_t*)(intptr_t)act[52];
    for (int i=0;i<2;++i) {
        int32_t layer=layers[i], head=*(int32_t*)(intptr_t)(layer+180);
        int32_t key=*(int32_t*)(intptr_t)(*(int32_t*)(intptr_t)head+8);
        int32_t layout=*(int32_t*)(intptr_t)(key+4);
        CHECK(*(int32_t*)(intptr_t)(layer+184)==1 && *(int32_t*)(intptr_t)key==PTR(&g277));
        CHECK(*(int32_t*)(intptr_t)layout==PTR(&g299));
        CHECK(*(int32_t*)(intptr_t)(layout+304)==layer);
        CHECK(*(float*)(intptr_t)(layout+260)==1.0f);
        CHECK(i ? *(int32_t*)(intptr_t)(layout+292)==17 : *(float*)(intptr_t)(layout+284)==0.375f);
        const char key_name[]="a long key name\0with embedded bytes";
        retdec_string_assign_n((int32_t*)(intptr_t)(key+8),key_name,sizeof(key_name)-1);
        *(uint8_t*)(intptr_t)(key+32)=0xa5;
        *(uint8_t*)(intptr_t)(layout+313)=0xa5;
        int32_t cloned=retdec_call_thiscall0_result((void*)(intptr_t)key,(void*)g277.e5);
        CHECK(cloned && cloned!=key && *(int32_t*)(intptr_t)cloned==PTR(&g277));
        int32_t copied_layout=*(int32_t*)(intptr_t)(cloned+4);
        CHECK(copied_layout && copied_layout!=layout && *(int32_t*)(intptr_t)copied_layout==PTR(&g299));
        CHECK(memcmp((void*)(intptr_t)(copied_layout+8),(void*)(intptr_t)(layout+8),305)==0);
        CHECK(*(uint8_t*)(intptr_t)(copied_layout+313)==0 && *(uint8_t*)(intptr_t)(cloned+32)==0);
        CHECK(*(uint32_t*)(intptr_t)(cloned+24)==sizeof(key_name)-1);
        CHECK(memcmp(retdec_std_string_data(cloned+8),key_name,sizeof(key_name)-1)==0);
        CHECK(retdec_std_string_data(cloned+8)!=retdec_std_string_data(key+8));
        kinoko_string_destroy(cloned+8);
        free((void*)(intptr_t)copied_layout); free((void*)(intptr_t)cloned);
    }
    CHECK(strcmp(retdec_std_string_data(layers[1]+112),"a long dynamically created layer")==0);
    CHECK(execute_source(vm,root+2,
        "if(dynamicPlayer.GetLayerOrder(dynamicFirst)!=0 || dynamicPlayer.GetLayerOrder(dynamicSecond)!=1) throw \"initial order\";\n"
        "if(dynamicPlayer.GetLayerOrder(inactiveLayer)!=-1) throw \"missing layer\";\n"
        "if(dynamicPlayer.SwapLayer(-1,1) || dynamicPlayer.SwapLayer(0,2) || dynamicPlayer.SwapLayer(0,0)) throw \"invalid swap\";\n"
        "dynamicThird <- dynamicPlayer.CreateLayer2D(\"third\");\n"
        "dynamicFourth <- dynamicPlayer.CreateLayer2D(\"fourth\");\n"
        "dynamicFifth <- dynamicPlayer.CreateLayer2D(\"fifth\");\n"
        "dynamicSixth <- dynamicPlayer.CreateLayer2D(\"sixth\");\n"));
    layers=(int32_t*)(intptr_t)act[52];
    int32_t original_layers[6]; memcpy(original_layers,layers,sizeof(original_layers));
    *(int32_t*)(intptr_t)(original_layers[2]+88)=original_layers[0];
    *(int32_t*)(intptr_t)(original_layers[3]+88)=original_layers[1];
    *(int32_t*)(intptr_t)(original_layers[4]+88)=original_layers[2];
    *(int32_t*)(intptr_t)(original_layers[5]+88)=original_layers[0];
    CHECK(execute_source(vm,root+2,
        "if(dynamicPlayer.SwapLayer(0,4) || dynamicPlayer.SwapLayer(4,0)) throw \"ancestor swap\";\n"
        "if(!dynamicPlayer.SwapLayer(0,1)) throw \"root swap\";\n"
        "if(dynamicPlayer.GetLayerOrder(dynamicSecond)!=0 || dynamicPlayer.GetLayerOrder(dynamicFourth)!=1 ||\n"
        "dynamicPlayer.GetLayerOrder(dynamicFirst)!=2 || dynamicPlayer.GetLayerOrder(dynamicThird)!=3 ||\n"
        "dynamicPlayer.GetLayerOrder(dynamicFifth)!=4 || dynamicPlayer.GetLayerOrder(dynamicSixth)!=5) throw \"preorder\";\n"
        "if(!dynamicPlayer.SwapLayer(3,5)) throw \"sibling swap\";\n"
        "if(dynamicPlayer.GetLayerOrder(dynamicSixth)!=3 || dynamicPlayer.GetLayerOrder(dynamicThird)!=4 ||\n"
        "dynamicPlayer.GetLayerOrder(dynamicFifth)!=5) throw \"subtree follows parent\";\n"
        "if(!dynamicPlayer.SwapLayer(0,2)) throw \"second root swap\";\n"));
    const int final_order[]={0,5,2,4,1,3};
    for(int i=0;i<6;++i) CHECK(layers[i]==original_layers[final_order[i]]);
    int32_t* children=(int32_t*)(intptr_t)*(int32_t*)(intptr_t)(original_layers[0]+72);
    CHECK(children[0]==original_layers[5] && children[1]==original_layers[2]);
    CHECK(*(int32_t*)(intptr_t)(original_layers[0]+76)==PTR(children+2));
    {
        const int32_t source=original_layers[0];
        const char text[]="return 17;";
        const char compiled_text[]="/* This script is compiled. Can't read this. Don't edit this.*/";
        free((void*)(intptr_t)*(int32_t*)(intptr_t)(source+296));
        char* raw=(char*)malloc(sizeof(text)); CHECK(raw);
        memcpy(raw,text,sizeof(text));
        *(int32_t*)(intptr_t)(source+296)=PTR(raw);
        *(uint32_t*)(intptr_t)(source+300)=sizeof(text);
        retdec_string_assign_cstr((int32_t*)(intptr_t)(source+268),"a long original script filename.nut");
        const int32_t head=*(int32_t*)(intptr_t)(source+180);
        const int32_t source_key=*(int32_t*)(intptr_t)(*(int32_t*)(intptr_t)head+8);
        const int32_t event=kinoko_act_new_timeline();
        CHECK(event);
        int32_t* timeline=(int32_t*)(intptr_t)event;
        timeline[1]=17; timeline[2]=51;
        int32_t timeline_pairs[4];
        timeline_pairs[0]=3; timeline_pairs[1]=11;
        timeline_pairs[2]=29; timeline_pairs[3]=47;
        kinoko_native_buffer_replace(event+12,timeline_pairs,sizeof(timeline_pairs));
        {
            int32_t methods[6]={0,0,0,PTR(script_io_transfer),0,PTR(script_io_seek)};
            struct script_io_stream stream={0}; stream.vtable=methods;
            const int32_t loaded=kinoko_act_new_timeline(); CHECK(loaded);
            int32_t* restored=(int32_t*)(intptr_t)loaded;
            const unsigned char saved=g673;
            const int32_t* vtable=(const int32_t*)kinoko_act_timeline_vtable();
            for(int compact=0;compact<2;++compact) {
                g673=(unsigned char)compact;
                stream.position=stream.size=0; stream.reading=0;
                CHECK(retdec_call_thiscall1_result(timeline,(void*)(intptr_t)vtable[0],PTR(&stream))==1);
                CHECK(stream.size==(compact?29:68));
                CHECK(stream.bytes[0]==!compact);
                stream.position=0; stream.reading=1;
                CHECK(kinoko_act_load_timeline(loaded,PTR(&stream),1)==1);
                CHECK(stream.position==stream.size && restored[1]==17 && restored[2]==51);
                CHECK(restored[4]-restored[3]==16*(compact+1));
                CHECK(memcmp((void*)(intptr_t)(restored[3]+16*compact),timeline_pairs,16)==0);
                /* Exercise the real ECX virtual entry, including package XOR.
                   A memory-only stream cannot detect a cdecl slot mismatch. */
                for (int package=0;package<2;++package) {
                    char directory[MAX_PATH],path[MAX_PATH];
                    CHECK(GetTempPathA(sizeof(directory),directory));
                    CHECK(GetTempFileNameA(directory,"tli",0,path));
                    HANDLE file=CreateFileA(path,GENERIC_READ|GENERIC_WRITE,0,NULL,
                        OPEN_EXISTING,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,NULL);
                    CHECK(file!=INVALID_HANDLE_VALUE);
                    unsigned char payload[8192],prefix[32]={0}; DWORD written=0;
                    for(uint32_t i=0;i<stream.size;++i) payload[i]=stream.bytes[i]^(package?0xa7:0);
                    if(package) CHECK(WriteFile(file,prefix,sizeof(prefix),&written,NULL) && written==sizeof(prefix));
                    CHECK(WriteFile(file,payload,stream.size,&written,NULL) && written==stream.size);
                    CHECK(SetFilePointer(file,package?32:0,NULL,FILE_BEGIN)!=(DWORD)-1);
                    int32_t reader[7]={0};
                    reader[0]=package?PTR(&kinoko_package_reader_methods):PTR(&kinoko_file_reader_methods); reader[1]=PTR(file);
                    reader[3]=stream.size; reader[4]=reader[5]=package?32:0;
                    ((unsigned char*)reader)[24]=package?0xa7:0;
                    int32_t actual=kinoko_act_new_timeline(); CHECK(actual);
                    CHECK(kinoko_act_load_timeline(actual,PTR(reader),1));
                    int32_t* value=(int32_t*)(intptr_t)actual;
                    CHECK(value[1]==17 && value[2]==51 && value[4]-value[3]==16);
                    CHECK(memcmp((void*)(intptr_t)value[3],timeline_pairs,16)==0);
                    if(package) CHECK(reader[5]==32+stream.size);
                    retdec_destroy_cact_key(actual); CHECK(CloseHandle(file));
                }
            }
            const int32_t before=restored[3];
            stream.position=0; --stream.size;
            CHECK(!kinoko_act_load_timeline(loaded,PTR(&stream),1));
            CHECK(restored[3]==before && restored[4]-restored[3]==32);
            CHECK(!kinoko_act_load_timeline(loaded,PTR(&stream),2));
            g673=saved;
            retdec_destroy_cact_key(loaded);
        }
        CHECK(event && retdec_act_append_list(source+192,event));
        *(int32_t*)(intptr_t)(source+196)=1;
        {
            int32_t methods[6]={0,0,0,PTR(script_io_transfer),0,PTR(script_io_seek)};
            struct script_io_stream stream={0}; stream.vtable=methods;
            const unsigned char saved=g673; const int32_t archives=kinoko_archive_count;
            g673=0; kinoko_archive_count=0;
            CHECK(retdec_call_thiscall1_result((void*)(intptr_t)source,(void*)g252.e0,PTR(&stream)));
            char directory[MAX_PATH],path[MAX_PATH]; DWORD written=0;
            CHECK(GetTempPathA(sizeof(directory),directory) && GetTempFileNameA(directory,"lyr",0,path));
            HANDLE file=CreateFileA(path,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,
                FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,NULL); CHECK(file!=INVALID_HANDLE_VALUE);
            CHECK(WriteFile(file,stream.bytes,stream.size,&written,NULL) && written==stream.size);
            CHECK(SetFilePointer(file,0,NULL,FILE_BEGIN)==0);
            int32_t reader[7]={PTR(&kinoko_file_reader_methods),PTR(file)}, holder=PTR(reader);
            const int32_t loaded=retdec_act_make_layer(); CHECK(loaded);
            CHECK(retdec_call_thiscall2_result((void*)(intptr_t)loaded,(void*)g252.e1,PTR(&holder),1));
            CHECK(SetFilePointer(file,0,NULL,FILE_CURRENT)==stream.size);
            CHECK(strcmp(retdec_std_string_data(loaded+112),retdec_std_string_data(source+112))==0);
            CHECK(*(int32_t*)(intptr_t)(loaded+184)==1 && *(int32_t*)(intptr_t)(loaded+196)==1);
            int32_t key_node=**(int32_t**)(intptr_t)(loaded+180);
            int32_t loaded_key=*(int32_t*)(intptr_t)(key_node+8);
            int32_t loaded_layout=*(int32_t*)(intptr_t)(loaded_key+4);
            CHECK(loaded_layout && *(int32_t*)(intptr_t)(loaded_layout+304)==loaded);
            int32_t event_node=**(int32_t**)(intptr_t)(loaded+192);
            int32_t* restored_event=*(int32_t**)(intptr_t)(event_node+8);
            CHECK(restored_event[0]==PTR(kinoko_act_timeline_vtable()) && restored_event[1]==17);
            CHECK(memcmp((void*)(intptr_t)restored_event[3],timeline_pairs,16)==0);
            retdec_destroy_cact_layer(loaded); free((void*)(intptr_t)loaded);
            CHECK(CloseHandle(file)); kinoko_archive_count=archives; g673=saved;
        }
        for(int compiled=0;compiled<2;++compiled) {
            *(uint8_t*)(intptr_t)(source+305)=(uint8_t)compiled;
            const int32_t copy=retdec_call_thiscall0_result((void*)(intptr_t)source,(void*)g252.e5);
            CHECK(copy && copy!=source);
            CHECK(*(int32_t*)(intptr_t)(copy+72)!=PTR(children));
            CHECK(memcmp((void*)(intptr_t)*(int32_t*)(intptr_t)(copy+72),children,8)==0);
            CHECK(*(int32_t*)(intptr_t)(copy+316)==*(int32_t*)(intptr_t)(source+316));
            CHECK(*(int32_t*)(intptr_t)(copy+320)==*(int32_t*)(intptr_t)(source+320));
            CHECK(*(int32_t*)(intptr_t)(copy+340)==*(int32_t*)(intptr_t)(source+340));
            CHECK(retdec_std_string_data(copy+268)[0]==0);
            CHECK(strcmp((char*)(intptr_t)*(int32_t*)(intptr_t)(copy+296),compiled?compiled_text:text)==0);
            CHECK(*(uint32_t*)(intptr_t)(copy+300)==(compiled?sizeof(compiled_text):sizeof(text)));
            CHECK(*(uint8_t*)(intptr_t)(copy+304)==1 && *(uint8_t*)(intptr_t)(copy+305)==compiled);
            for(int list=0;list<2;++list) {
                const int offset=list?192:180;
                const int32_t copy_head=*(int32_t*)(intptr_t)(copy+offset);
                const int32_t copy_key=*(int32_t*)(intptr_t)(*(int32_t*)(intptr_t)copy_head+8);
                CHECK(copy_key!=source_key && copy_key!=event && *(int32_t*)(intptr_t)(copy+offset+4)==1);
                if(list) {
                    const int32_t* cloned_timeline=(int32_t*)(intptr_t)copy_key;
                    CHECK(cloned_timeline[0]==PTR(kinoko_act_timeline_vtable()));
                    CHECK(cloned_timeline[1]==17 && cloned_timeline[2]==51);
                    CHECK(cloned_timeline[3]!=timeline[3] && cloned_timeline[4]-cloned_timeline[3]==16);
                    CHECK(memcmp((void*)(intptr_t)cloned_timeline[3],timeline_pairs,16)==0);
                } else {
                    const int32_t layout=*(int32_t*)(intptr_t)(copy_key+4);
                    CHECK(*(int32_t*)(intptr_t)(layout+304)==copy);
                    CHECK(*(uint32_t*)(intptr_t)(copy_key+24)==*(uint32_t*)(intptr_t)(source_key+24));
                    CHECK(retdec_std_string_data(copy_key+8)!=retdec_std_string_data(source_key+8));
                }
            }
            retdec_destroy_cact_layer(copy); free((void*)(intptr_t)copy);
        }
        *(uint8_t*)(intptr_t)(source+305)=0;
        CHECK(execute_source(vm,root+2,"if(dynamicFirst.alpha!=0.375) throw \"clone altered source\";"));
    }
    CHECK(execute_source(vm,root+2,
        "dynamicText <- dynamicPlayer.CreateLayerString(\"text\");\n"
        "dynamicText.alpha=0.625; dynamicText.colorR=73;\n"
        "if(dynamicText.layout.alpha!=0.625 || dynamicHost.text.layout.colorR!=73) throw \"text aliases\";\n"
        "if(!dynamicText.layout.PushBack(\"native text\") || dynamicText.layout.queueCount!=11) throw \"text methods\";"));
    layers=(int32_t*)(intptr_t)act[52];CHECK(act[53]-act[52]==28);
    const int32_t text_layer=layers[6],text_head=*(int32_t*)(intptr_t)(text_layer+180);
    const int32_t text_key=*(int32_t*)(intptr_t)(*(int32_t*)(intptr_t)text_head+8);
    const int32_t text_layout=*(int32_t*)(intptr_t)(text_key+4);
    CHECK(*(int32_t*)(intptr_t)text_layout==PTR(g350));
    CHECK(*(int32_t*)(intptr_t)(text_layout+148)==text_layer);
    const int32_t text_copy=retdec_call_thiscall0_result((void*)(intptr_t)text_key,(void*)g277.e5);
    CHECK(text_copy && *(int32_t*)(intptr_t)(text_copy+4)!=text_layout);
    retdec_destroy_cact_key(text_copy);
    CHECK(execute_source(vm,root+2,"delete dynamicText;"));
    ((uint8_t*)player)[8]=0;
    CHECK(execute_source(vm,root+2,
        "if(dynamicPlayer.GetLayerOrder(dynamicSecond)!=0 || dynamicPlayer.SwapLayer(0,1)) throw \"inactive order\";"));
    CHECK(execute_source(vm,root+2,
        "delete inactiveLayer; delete dynamicFirst; delete dynamicSecond; delete dynamicThird; delete dynamicFourth;\n"
        "delete dynamicFifth; delete dynamicSixth; delete dynamicHost; delete dynamicPlayer;"));
    for(int i=0;i<7;++i) { retdec_destroy_cact_layer(layers[i]); free((void*)(intptr_t)layers[i]); }
    kinoko_act_array_destroy(PTR(act)+208); kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), player_pair);
    DeleteCriticalSection((struct retdec_RTL_CRITICAL_SECTION*)(player+5));
    CHECK(sq_gettop(kinoko_vm(vm))==top);
    puts("PASS: dynamic 2D ownership, Sqrat aliases, layer order, ancestor rejection and subtree swaps");
    return 0;
}

static int test_map_set_layer(void) {
    int32_t layout[116]={0}, layer[87]={0}, resource[25]={0}, wrong[25]={0};
    int32_t records[3][8]={{4,7,8},{999,1,2},{2,-3,-4}};
    struct retdec_mcd_chip chips[2]={0};
    struct retdec_mcd_texture textures[1]={{17,123}};
    struct retdec_mcd_data data={2,chips,1,textures};
    chips[0].chip_id=4; chips[1].chip_id=2;
    *(uint32_t*)chips[0].bytes=4; *(uint32_t*)chips[1].bytes=2;
    *(uint32_t*)(chips[0].bytes+4)=17; *(uint32_t*)(chips[1].bytes+4)=999;
    layout[0]=PTR(&g327); resource[0]=PTR(&g313); wrong[0]=PTR(&g365);
    layout[66]=PTR(records); layout[67]=layout[68]=PTR(records+3); layout[113]=-1;
    resource[16]=PTR(&data); layer[25]=PTR(resource);
    CHECK(retdec_call_thiscall1_result(layout,(void*)g327.e6,0)==(int32_t)E_FAIL);
    ((uint8_t*)layout)[460]=1;
    CHECK(retdec_call_thiscall1_result(layout,(void*)g327.e6,PTR(layer))==0);
    CHECK(layout[78]==PTR(layer) && layout[79]==0 && ((uint8_t*)layout)[460]==0);
    CHECK(retdec_call_thiscall1_result(layout,(void*)g327.e6,PTR(layer))==0);
    CHECK(layout[79]==PTR(resource) && layout[71]-layout[70]==12 && layout[75]-layout[74]==12);
    CHECK(records[0][0]==4 && records[2][0]==2); /* Bind never sorts map records. */
    int32_t *chip_refs=(int32_t*)(intptr_t)layout[70], *texture_refs=(int32_t*)(intptr_t)layout[74];
    CHECK(chip_refs[0]==PTR(chips[0].bytes) && chip_refs[1]==0 && chip_refs[2]==PTR(chips[1].bytes));
    CHECK(texture_refs[0]==PTR(textures) && texture_refs[1]==0 && texture_refs[2]==0);
    CHECK(retdec_call_thiscall1_result(layout,(void*)g327.e6,PTR(layer))==0);
    CHECK(layout[71]-layout[70]==12 && layout[75]-layout[74]==24);
    layer[25]=PTR(wrong);
    CHECK(retdec_call_thiscall1_result(layout,(void*)g327.e6,PTR(layer))==(int32_t)E_FAIL);
    CHECK(layout[79]==0); /* Type failure does not reinterpret a texture as MCD. */
    layer[25]=0;
    CHECK(retdec_call_thiscall1_result(layout,(void*)g327.e6,PTR(layer))==0);
    CHECK(layout[71]==layout[70] && layout[75]-layout[74]==24);
    for (int i=0;i<4;++i) { const int slots[]={70,74,101,109}; kinoko_native_buffer_destroy(PTR(layout)+slots[i]*4); }
    puts("PASS: map SetLayer one-shot suppression, type checks, sparse refs and original append behavior");
    return 0;
}

static int test_chip_serialization(void) {
    int32_t methods[6]={0,0,0,PTR(script_io_transfer),0,PTR(script_io_seek)};
    struct script_io_stream stream={0}; stream.vtable=methods;
    int32_t holder=PTR(&stream);
    int32_t *source=(int32_t*)calloc(1,100), *loaded=(int32_t*)calloc(1,100);
    CHECK(source && loaded);
    source[0]=loaded[0]=PTR(&g313);
    source[7]=source[14]=source[23]=loaded[7]=loaded[14]=loaded[23]=15;
    source[1]=719;
    retdec_string_assign_cstr(source+2,"map atlas");
    retdec_string_assign_cstr(source+9,"data/worldmap/worldmap.mcd");
    loaded[24]=0x5a;
    const unsigned char saved=g673;
    for (int compact=0;compact<2;++compact) {
        g673=(unsigned char)compact;
        stream.position=stream.size=0; stream.reading=0;
        CHECK(retdec_call_thiscall1_result(source,(void*)g313.e0,PTR(&stream))==1);
        CHECK(stream.bytes[0]==!compact);
        stream.position=0; stream.reading=1;
        CHECK(retdec_call_thiscall2_result(loaded,(void*)g313.e1,PTR(&holder),1)==1);
        CHECK(stream.position==stream.size && loaded[1]==719);
        CHECK(strcmp(retdec_std_string_data(PTR(loaded+2)),"map atlas")==0);
        CHECK(strcmp(retdec_std_string_data(PTR(loaded+9)),"data/worldmap/worldmap.mcd")==0);
        CHECK(loaded[16]==0 && loaded[17]==0 && loaded[24]==0x5a);
    }
    g673=saved;
    retdec_destroy_cact_resource(PTR(source)); retdec_destroy_cact_resource(PTR(loaded));
    puts("PASS: chip schema IO preserves independent native/MCD lifecycle fields");
    return 0;
}

static int test_serializable_lifetime(void) {
    const int32_t tables[]={PTR(&g231),PTR(&g252),PTR(&g277),PTR(&g285),PTR(&g299),
        PTR(&g327),PTR(&g365),PTR(&g313),PTR(&g379),PTR(kinoko_act_timeline_vtable())};
    const char* names[]={".?AVCActScript@@",".?AVCActLayer@@",".?AVCActKey@@",".?AVCAct@@",
        ".?AVC2DLayout@@",".?AVC2DMapLayout@@",".?AVCActResource2D@@",".?AVCActResourceChip@@",
        ".?AVCActRenderTarget@@",".?AVCActTimeLine@@"};
    for(unsigned i=0;i<sizeof(tables)/sizeof(tables[0]);++i) {
        int32_t object=tables[i],result=123; unsigned char descriptor[96]={0};
        strcpy((char*)descriptor+8,names[i]);
        const int32_t* table=(int32_t*)(intptr_t)tables[i];
        CHECK(retdec_call_thiscall2_result(&object,(void*)(intptr_t)table[2],PTR(descriptor),PTR(&result))==1);
        CHECK(result==PTR(&object));
        descriptor[8]='!'; /* MSVC 4AB2E2 ignores the decorated-name prefix byte. */
        CHECK(retdec_call_thiscall2_result(&object,(void*)(intptr_t)table[2],PTR(descriptor),PTR(&result))==1);
        strcpy((char*)descriptor+8,".?AUISerializable@NamespaceProperty@@");
        CHECK(!retdec_call_thiscall2_result(&object,(void*)(intptr_t)table[2],PTR(descriptor),PTR(&result)) && result==0);
        CHECK(!retdec_call_thiscall2_result(&object,(void*)(intptr_t)table[2],PTR(descriptor),0));
    }
    for(int count=1;count<=2;++count) {
        unsigned char* allocation=(unsigned char*)calloc(1,4+36*count); CHECK(allocation);
        *(uint32_t*)allocation=count;
        for(int i=0;i<count;++i) {
            int32_t* key=(int32_t*)(allocation+4+36*i); key[0]=PTR(&g277); key[7]=15;
            retdec_string_assign_cstr(key+2,"heap callback released by native destructor");
            key[1]=PTR(calloc(1,316)); CHECK(retdec_construct_c2dlayout(key[1]));
        }
        CHECK(retdec_call_thiscall1_result(allocation+4,(void*)g277.e4,2)==PTR(allocation));
        for(int i=0;i<count;++i) {
            int32_t* key=(int32_t*)(allocation+4+36*i);
            CHECK(key[1]==0 && key[6]==0 && key[7]==15 && ((char*)(key+2))[0]==0);
        }
        free(allocation);
    }
    for(int kind=0;kind<3;++kind) {
        int32_t resource[25]={0}; resource[0]=kind==0?PTR(&g365):kind==1?PTR(&g313):PTR(&g379);
        resource[7]=15;
        if(kind==1) { resource[14]=15; resource[23]=15; }
        else resource[15]=15;
        retdec_string_assign_cstr(resource+2,"resource heap name released in place");
        retdec_string_assign_cstr(resource+(kind==1?9:10),"independent long backing filename");
        const int32_t* table=(int32_t*)(intptr_t)resource[0];
        CHECK(retdec_call_thiscall1_result(resource,(void*)(intptr_t)table[4],0)==PTR(resource));
        CHECK(resource[6]==0 && resource[7]==15);
        CHECK(kind==1 ? resource[13]==0 && resource[14]==15 : resource[14]==0 && resource[15]==15);
    }
    const int32_t layer=retdec_act_make_layer(); CHECK(layer);
    CHECK(retdec_call_thiscall1_result((void*)(intptr_t)layer,(void*)g252.e4,0)==layer);
    CHECK(!*(int32_t*)(intptr_t)(layer+180) && !*(int32_t*)(intptr_t)(layer+192));
    free((void*)(intptr_t)layer);
    int32_t* key=(int32_t*)calloc(1,36); CHECK(key); key[0]=PTR(&g277); key[7]=15;
    retdec_call_thiscall0_result(key,(void*)g277.e3); /* Destroy invokes deleting slot four with flags=1. */
    puts("PASS: exact original RTTI name queries and explicit receiver scalar/array destruction");
    return 0;
}

static int32_t __fastcall act_contract_no_script(int32_t self,void* unused,int32_t stream) {
    (void)self; (void)unused; (void)stream; return 1;
}
static int32_t __fastcall act_contract_layer_id(int32_t self,void* unused,int32_t stream) {
    (void)unused;
    return script_io_transfer((struct script_io_stream*)(intptr_t)stream,NULL,(void*)(intptr_t)(self+104),4);
}
static int test_act_serialization(void) {
    int32_t methods[6]={0,0,0,PTR(script_io_transfer),0,PTR(script_io_seek)};
    struct script_io_stream stream={0}; stream.vtable=methods;
    int32_t source[60]={0},loaded[60]={0};
    source[0]=loaded[0]=PTR(&g285); source[9]=source[16]=loaded[9]=loaded[16]=15;
    CHECK(retdec_construct_cact_script(PTR(source+25)) && retdec_construct_cact_script(PTR(loaded+25)));
    source[1]=16; source[2]=640; source[3]=480;
    source[18]=11; source[19]=22; source[20]=33; source[21]=44; ((uint8_t*)source)[96]=1;
    const unsigned char saved=g673; const int32_t archives=kinoko_archive_count; g673=0; kinoko_archive_count=0;
    CHECK(retdec_call_thiscall1_result(source,(void*)g285.e0,PTR(&stream)));
    char directory[MAX_PATH],path[MAX_PATH]; DWORD written=0;
    CHECK(GetTempPathA(sizeof(directory),directory) && GetTempFileNameA(directory,"act",0,path));
    HANDLE file=CreateFileA(path,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,
        FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,NULL); CHECK(file!=INVALID_HANDLE_VALUE);
    CHECK(WriteFile(file,stream.bytes,stream.size,&written,NULL) && written==stream.size);
    CHECK(SetFilePointer(file,0,NULL,FILE_BEGIN)==0);
    int32_t reader[7]={PTR(&kinoko_file_reader_methods),PTR(file)},holder=PTR(reader);
    CHECK(retdec_call_thiscall2_result(loaded,(void*)g285.e1,PTR(&holder),1));
    CHECK(SetFilePointer(file,0,NULL,FILE_CURRENT)==stream.size);
    CHECK(loaded[1]==16 && loaded[2]==640 && loaded[3]==480);
    CHECK(loaded[18]==11 && loaded[19]==22 && loaded[20]==33 && loaded[21]==44);
    CHECK(CloseHandle(file));
    /* Isolate the ACT container writer's original filter from script compilation
       and nested-layer codecs. The stub emits a stable ID to check wire order. */
    int32_t script_methods[1]={PTR(act_contract_no_script)};
    int32_t layer_methods[1]={PTR(act_contract_layer_id)};
    int32_t layers[3][36]={{0}},references[3];
    for(int i=0;i<3;++i) { layers[i][0]=PTR(layer_methods); layers[i][26]=100+i; references[i]=PTR(layers[i]); }
    ((uint8_t*)layers[0])[141]=1; ((uint8_t*)layers[1])[141]=2;
    source[25]=PTR(script_methods); source[52]=PTR(references); source[53]=source[54]=PTR(references+3);
    for(int compact=1;compact<=2;++compact) {
        g673=(unsigned char)compact; stream.position=stream.size=0; stream.reading=0;
        CHECK(retdec_call_thiscall1_result(source,(void*)g285.e0,PTR(&stream)));
        /* Sorted margins: bottom, left, right, top. These golden offsets come
           from 427750, not the former apply_cact helper's incorrect mapping. */
        CHECK(stream.bytes[0]==0 && *(int32_t*)(stream.bytes+1)==44);
        CHECK(*(int32_t*)(stream.bytes+5)==11 && *(int32_t*)(stream.bytes+9)==33);
        CHECK(*(int32_t*)(stream.bytes+13)==22);
        uint32_t count=compact==1?2:3;
        CHECK(*(uint32_t*)(stream.bytes+42)==count && stream.size==50+8*count);
        for(uint32_t i=0;i<count;++i) {
            CHECK(*(uint32_t*)(stream.bytes+46+i*8)==0x2618cf18u);
            CHECK(*(int32_t*)(stream.bytes+50+i*8)==100+i+(compact==1));
        }
        CHECK(*(uint32_t*)(stream.bytes+46+8*count)==0);
    }
    source[52]=source[53]=source[54]=0;
    retdec_destroy_cact_object(PTR(source)); retdec_destroy_cact_object(PTR(loaded));
    g673=saved; kinoko_archive_count=archives;
    puts("PASS: CAct native reader, original margin offsets, debug-only filter and stable layer wire order");
    return 0;
}

static int test_key_string_writers(void) {
    int32_t methods[6]={0,0,0,PTR(script_io_transfer),0,PTR(script_io_seek)};
    struct script_io_stream stream={0}; stream.vtable=methods;
    int32_t key[9]={0},layout[65]={0};
    key[0]=PTR(&g277); key[7]=15;
    layout[0]=PTR(&g350); layout[6]=layout[13]=layout[20]=15;
    ((unsigned char*)layout)[128]=1; layout[33]=0x12345678;
    const unsigned char saved=g673; g673=1;
    CHECK(retdec_call_thiscall1_result(key,(void*)g277.e0,PTR(&stream))==1);
    CHECK(stream.size==6 && stream.bytes[0]==0 && stream.bytes[5]==0);
    stream.position=stream.size=0;
    CHECK(retdec_call_thiscall1_result(layout,(void*)(intptr_t)g350[0],PTR(&stream))==1);
    /* Sorted addEdge/alignment are both bytes from +128, not the int at +132. */
    CHECK(stream.size==75 && stream.bytes[0]==0 && stream.bytes[1]==1 && stream.bytes[2]==1);
    unsigned char expected[8192]; const uint32_t size=stream.size;
    memcpy(expected,stream.bytes,size);
    stream.position=stream.size=0; key[1]=PTR(layout);
    CHECK(retdec_call_thiscall1_result(key,(void*)g277.e0,PTR(&stream))==1);
    CHECK(stream.size==10+size && stream.bytes[5]==1);
    CHECK(memcmp(stream.bytes+10,expected,size)==0);
    int32_t flat[79]={0};
    CHECK(retdec_construct_c2dlayout(PTR(flat)));
    ((float*)flat)[59]=0.625f; ((float*)flat)[71]=0.75f;
    key[1]=PTR(flat);
    retdec_string_assign_cstr(key+2,"independently owned callback name");
    int32_t string_source[65]={0};kinoko_construct_string_layout(PTR(string_source));
    retdec_string_assign_n(string_source+1,"A\0B",3);
    retdec_string_assign_n(string_source+8,"C\0D",3);
    ((uint8_t*)string_source)[128]=1;string_source[33]=2;
    const int32_t archives=kinoko_archive_count; kinoko_archive_count=0;
    for(int kind=0;kind<2;++kind) for(int compact=0;compact<2;++compact) {
        key[1]=kind?PTR(string_source):PTR(flat);
        g673=(unsigned char)compact; stream.position=stream.size=0;
        CHECK(retdec_call_thiscall1_result(key,(void*)g277.e0,PTR(&stream))==1);
        char directory[MAX_PATH],path[MAX_PATH]; DWORD written=0;
        CHECK(GetTempPathA(sizeof(directory),directory) && GetTempFileNameA(directory,"key",0,path));
        HANDLE file=CreateFileA(path,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,
            FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,NULL); CHECK(file!=INVALID_HANDLE_VALUE);
        CHECK(WriteFile(file,stream.bytes,stream.size,&written,NULL) && written==stream.size);
        CHECK(SetFilePointer(file,0,NULL,FILE_BEGIN)==0);
        int32_t reader[7]={PTR(&kinoko_file_reader_methods),PTR(file)}, holder=PTR(reader);
        int32_t* copy=(int32_t*)calloc(1,36); CHECK(copy); copy[0]=PTR(&g277); copy[7]=15;
        CHECK(retdec_call_thiscall2_result(copy,(void*)g277.e1,PTR(&holder),1));
        CHECK(copy[1] && copy[1]!=key[1]);
        CHECK(strcmp(retdec_std_string_data(PTR(copy+2)),retdec_std_string_data(PTR(key+2)))==0);
        if(kind) {
            int32_t* text=(int32_t*)(intptr_t)copy[1];
            CHECK(text[0]==PTR(g350) && text[5]==0 && text[12]==6);
            CHECK(memcmp(retdec_std_string_data(copy[1]+32),"A\0BC\0D",6)==0);
            CHECK(((uint8_t*)text)[128]==1 && text[33]==0); /* wire bool alias, not runtime alignment */
        } else CHECK(*(float*)(intptr_t)(copy[1]+236)==0.625f && *(float*)(intptr_t)(copy[1]+284)==0.75f);
        CHECK(SetFilePointer(file,0,NULL,FILE_CURRENT)==stream.size);
        retdec_destroy_cact_key(PTR(copy)); CHECK(CloseHandle(file));
    }
    kinoko_archive_count=archives;
    kinoko_clear_string_layout(PTR(string_source));
    kinoko_string_destroy(PTR(key)+8);
    g673=saved;
    puts("PASS: key presence byte and CStringLayout original bool alias serialization");
    return 0;
}

static int test_layout_serialization(void) {
    int32_t methods[6]={0,0,0,PTR(script_io_transfer),0,PTR(script_io_seek)};
    struct script_io_stream stream={0}; stream.vtable=methods;
    int32_t holder=PTR(&stream), source[79]={0}, loaded[79]={0};
    source[0]=loaded[0]=PTR(&g299);
    for (int i=59;i<72;++i) ((float*)source)[i]=(float)(i-61)*0.125f;
    source[72]=3; source[73]=255; source[74]=128; source[75]=64;
    const unsigned char saved=g673;
    for (int compact=0;compact<2;++compact) {
        g673=(unsigned char)compact;
        stream.position=stream.size=0; stream.reading=0;
        loaded[24]=0x12345678; loaded[78]=0;
        CHECK(retdec_call_thiscall1_result(source,(void*)g299.e0,PTR(&stream))==1);
        CHECK(stream.bytes[0]==!compact);
        stream.position=0; stream.reading=1;
        CHECK(retdec_call_thiscall2_result(loaded,(void*)g299.e1,PTR(&holder),1)==1);
        CHECK(stream.position==stream.size && memcmp(source+59,loaded+59,17*4)==0);
        CHECK(loaded[78]==1 && loaded[24]==0x12345678);
    }
    g673=saved;
    /* Original 43C6B0 aliases trans.x and roll.x. Sorted reads make trans.x
       the final assignment even when the header lists it before roll.x. */
    stream.position=stream.size=0; stream.reading=0;
    {
        uint8_t has=1; uint32_t count=2,len=7,type=1;
        float roll=1.25f, trans=-3.5f;
        CHECK(script_io_transfer(&stream,NULL,&has,1));
        CHECK(script_io_transfer(&stream,NULL,&count,4));
        CHECK(script_io_transfer(&stream,NULL,&len,4));
        CHECK(script_io_transfer(&stream,NULL,"trans.x",len));
        CHECK(script_io_transfer(&stream,NULL,&type,4));
        len=6;
        CHECK(script_io_transfer(&stream,NULL,&len,4));
        CHECK(script_io_transfer(&stream,NULL,"roll.x",len));
        CHECK(script_io_transfer(&stream,NULL,&type,4));
        CHECK(script_io_transfer(&stream,NULL,&roll,4));
        CHECK(script_io_transfer(&stream,NULL,&trans,4));
    }
    stream.position=0; stream.reading=1;
    int32_t layout3d[13]={0}; layout3d[1]=123; layout3d[7]=456;
    CHECK(retdec_call_thiscall2_result(layout3d,(void*)kinoko_method_layout3d_assign,PTR(&holder),1)==1);
    CHECK(stream.position==stream.size && ((float*)layout3d)[4]==-3.5f);
    CHECK(layout3d[1]==123 && layout3d[7]==456);
    puts("PASS: all 17 layout properties round-trip and restore original transform-dirty state");
    return 0;
}

static int test_texture_serialization(int render_target) {
    int32_t methods[6]={0,0,0,PTR(script_io_transfer),0,PTR(script_io_seek)};
    struct script_io_stream stream={0}; stream.vtable=methods;
    int32_t holder=PTR(&stream);
    int32_t *source=(int32_t*)calloc(1,100), *loaded=(int32_t*)calloc(1,100);
    CHECK(source && loaded);
    source[0]=loaded[0]=render_target ? PTR(&g379) : PTR(&g365);
    void **vtable=(void**)(intptr_t)source[0];
    source[7]=source[15]=loaded[7]=loaded[15]=15;
    source[1]=15; source[18]=source[19]=512;
    ((float*)source)[20]=273; ((float*)source)[21]=0;
    ((float*)source)[22]=136; ((float*)source)[23]=480;
    retdec_string_assign_cstr(source+2,"face_3");
    retdec_string_assign_cstr(source+10,"Data/System/face1");
    const unsigned char saved=g673;
    for (int compact=0;compact<2;++compact) {
        g673=(unsigned char)compact;
        stream.position=stream.size=0; stream.reading=0;
        CHECK(retdec_call_thiscall1_result(source,vtable[0],PTR(&stream))==1);
        CHECK(stream.bytes[0]==!compact);
        stream.position=0; stream.reading=1;
        ((unsigned char*)loaded)[96]=1;
        CHECK(retdec_call_thiscall2_result(loaded,vtable[1],PTR(&holder),1)==1);
        CHECK(stream.position==stream.size && loaded[1]==15);
        CHECK(loaded[18]==512 && loaded[19]==512);
        CHECK(memcmp(source+20,loaded+20,16)==0 && !((unsigned char*)loaded)[96]);
        CHECK(strcmp(retdec_std_string_data(PTR(loaded+2)),"face_3")==0);
        CHECK(strcmp(retdec_std_string_data(PTR(loaded+10)),"Data/System/face1")==0);
        if (render_target) {
            const char name[]=".?AVCActRenderTarget@@";
            const uint32_t type=(uint32_t)kinoko_boost_hash_range(PTR(name),PTR(name+sizeof(name)-1));
            stream.position=0;
            int32_t *factory=(int32_t*)(intptr_t)retdec_act_make_resource(PTR(&stream),type);
            CHECK(factory && factory[0]==PTR(&g379));
            CHECK(stream.position==stream.size && factory[1]==15);
            CHECK(factory[18]==512 && factory[19]==512 && factory[17]==0);
            CHECK(memcmp(source+20,factory+20,16)==0 && !((unsigned char*)factory)[96]);
            CHECK(strcmp(retdec_std_string_data(PTR(factory+10)),"Data/System/face1")==0);
            retdec_destroy_cact_resource(PTR(factory));
        }
    }
    /* Reordered schema: values are still in original std::map key order. */
    stream.position=stream.size=0; stream.reading=0;
    {
        uint8_t has=1; uint32_t count=2,len=5,type=1,id=321; float x=137;
        CHECK(script_io_transfer(&stream,NULL,&has,1));
        CHECK(script_io_transfer(&stream,NULL,&count,4));
        CHECK(script_io_transfer(&stream,NULL,&len,4));
        CHECK(script_io_transfer(&stream,NULL,"src_x",len));
        CHECK(script_io_transfer(&stream,NULL,&type,4));
        len=10; type=0;
        CHECK(script_io_transfer(&stream,NULL,&len,4));
        CHECK(script_io_transfer(&stream,NULL,"resourceID",len));
        CHECK(script_io_transfer(&stream,NULL,&type,4));
        CHECK(script_io_transfer(&stream,NULL,&id,4));
        CHECK(script_io_transfer(&stream,NULL,&x,4));
    }
    stream.position=0; stream.reading=1;
    CHECK(retdec_call_thiscall2_result(loaded,vtable[1],PTR(&holder),1)==1);
    CHECK(stream.position==stream.size && loaded[1]==321 && ((float*)loaded)[20]==137);
    CHECK(((float*)loaded)[22]==136 && ((float*)loaded)[23]==480);
    CHECK(strcmp(retdec_std_string_data(PTR(loaded+10)),"Data/System/face1")==0);
    CHECK(!retdec_call_thiscall2_result(loaded,vtable[1],PTR(&holder),2));
    g673=saved;
    retdec_destroy_cact_resource(PTR(source)); retdec_destroy_cact_resource(PTR(loaded));
    puts("PASS: native texture property IO, full/compact schemas, sorted values, heap strings and crop lifecycle");
    return 0;
}
static int test_script_serialization(int32_t vm, int32_t* root) {
    int32_t methods[6]={0,0,0,PTR(script_io_transfer),0,PTR(script_io_seek)};
    struct script_io_stream stream={0}; stream.vtable=methods;
    int32_t script[26]={0}, loaded[26]={0}, holder=PTR(&stream);
    const char source[]="ioCompiledValue <- 42;\n";
    const char path[]="a-long-script-path-for-serialization.cv4";
    CHECK(retdec_construct_cact_script(PTR(script)) && retdec_construct_cact_script(PTR(loaded)));
    free((void*)(intptr_t)script[23]); script[23]=PTR(malloc(sizeof(source))); CHECK(script[23]);
    memcpy((void*)(intptr_t)script[23],source,sizeof(source)); script[24]=sizeof(source); script[25]=1;
    retdec_string_assign_cstr(script+16,path);
    const unsigned char previous=g673; g673=0;
    CHECK(retdec_call_thiscall1_result(script,(void*)g231.e0,PTR(&stream))==1);
    CHECK(stream.bytes[0]==1 && *(uint32_t*)(stream.bytes+1)==2);
    CHECK(memcmp(stream.bytes+9,"compiled",8)==0);
    CHECK(((unsigned char*)script)[101]==0);
    stream.reading=1; stream.position=0;
    CHECK(retdec_call_thiscall2_result(loaded,(void*)g231.e1,PTR(&holder),1)==1);
    CHECK(stream.position==stream.size && loaded[24]==sizeof(source));
    CHECK(strcmp(retdec_std_string_data(PTR(loaded)+64),path)==0);
    CHECK(memcmp((void*)(intptr_t)loaded[23],source,sizeof(source))==0);
    CHECK(loaded[23]!=script[23] && ((unsigned char*)loaded)[100]==1);
    CHECK(kinoko_method_read_act_script(PTR(loaded),NULL,PTR(&holder),2)==0);
    stream.reading=0; stream.position=stream.size=0; g673=1;
    CHECK(kinoko_method_write_act_script(PTR(script),NULL,PTR(&stream))==1);
    CHECK(stream.bytes[0]==0 && stream.bytes[1]==1 && ((unsigned char*)script)[101]==0);
    {
        uint32_t prefix=1+1+4+(uint32_t)strlen(path);
        CHECK(*(uint32_t*)(stream.bytes+prefix)==stream.size-prefix);
        CHECK(*(uint16_t*)(stream.bytes+prefix+4)==0xfafa);
        int32_t compiled[26]={0};
        compiled[23]=PTR(stream.bytes+prefix+4); compiled[24]=stream.size-prefix-4;
        CHECK(retdec_execute_embedded_act_script(vm,PTR(compiled),root+2));
        CHECK(execute_source(vm,root+2,"if(ioCompiledValue!=42) throw \"serialized closure\";\n"
            "delete ioCompiledValue;\n"));
    }
    g673=previous;
    retdec_destroy_cact_script(PTR(loaded)); retdec_destroy_cact_script(PTR(script));
    puts("PASS: original script virtual read/write, heap path ownership, source compile and inclusive bytecode length");
    return 0;
}

static int test_original_layer_constructor(int32_t vm) {
    int32_t *layer = malloc(348);
    CHECK(layer); memset(layer, 0xcd, 348);
    const int32_t previous = g664; g664 = vm;
    CHECK(function_41e390(PTR(layer)) == PTR(layer)); g664 = previous;
    CHECK(layer[24] == -1 && layer[26] == -1 && layer[27] == -1);
    CHECK(strcmp(retdec_std_string_data(PTR(layer)+112), "Layer_") == 0);
    CHECK(layer[78] == vm && layer[79] == OT_TABLE && layer[83] == vm && layer[84] == OT_NULL);
    CHECK(layer[52] == vm && layer[57] == vm && layer[62] == vm);
    int32_t missing[2] = {g483,g484};
    CHECK(!kinoko_sqrat_get((void *)(intptr_t)(PTR(layer)+308), "CompileFile", (void *)(intptr_t)(PTR(missing))));
    kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), missing);
    {
        int32_t parent[5]={PTR(kinoko_act_host_symbols()->sq_object_vtable),vm,g483,g484,1};
        const char text[]="registered <- thisAct.marker;\nsawLayer <- (\"layer\" in this);\n";
        CHECK(kinoko_sqrat_new_table((struct SQVM *)(intptr_t)(vm), parent+2));
        CHECK(kinoko_sqrat_bind_int((struct SQVM *)(intptr_t)(vm), parent+2, "marker", 73));
        layer[74]=PTR(malloc(sizeof(text))); CHECK(layer[74]);
        memcpy((void*)(intptr_t)layer[74],text,sizeof(text)); layer[76]=1; layer[75]=sizeof(text)-1;
        ((float*)layer)[36]=13.0f; ((float*)layer)[37]=17.0f;
        CHECK(retdec_call_thiscall2_result(layer,(void*)g252.e8,PTR(parent),0)==0);
        CHECK(((float*)layer)[39]==13.0f && ((float*)layer)[40]==17.0f);
        CHECK(execute_source(vm,parent+2,
            "if(Layer_.script.registered!=73 || Layer_.script.sawLayer) throw \"layer order\";\n"
            "if(Layer_.script.thisAct!=this || Layer_.script.layer==Layer_) throw \"layer identity\";\n"
            "if(\"filePath\" in Layer_.script) throw \"invented script field\";\n"));
        CHECK(kinoko_method_register_act_layer(PTR(layer),NULL,0,0)==(int32_t)E_FAIL);
        kinoko_sqrat_object_release((void *)(intptr_t)(PTR(parent)));
    }
    retdec_destroy_cact_layer(PTR(layer)); free(layer);
    int32_t *script = calloc(26, sizeof(int32_t));
    CHECK(script && retdec_construct_cact_script(PTR(script)));
    CHECK(retdec_call_thiscall0_result(script, (void*)g231.e3) == 0);
    puts("PASS: original layer constructor source Table, empty Instance, default name and script deleting ABI");
    return 0;
}

static int test_layout_registration_entries(int32_t vm) {
    int32_t layer[87] = {0}, layout[100] = {0}, environment[2];
    CHECK(retdec_prepare_cact_layer_objects(vm, PTR(layer), environment));
    CHECK(kinoko_sqrat_new_table((struct SQVM *)(intptr_t)(vm), layer+84));
    CHECK(kinoko_method_register_layout(PTR(layout), NULL) == (int32_t)E_FAIL);
    layout[76] = PTR(layer);
    for (int map = 0; map < 2; ++map) {
        if (map) layout[78] = PTR(layer);
        CHECK((map ? kinoko_method_register_map_layout(PTR(layout), NULL) :
            kinoko_method_register_layout(PTR(layout), NULL)) == 0);
        int32_t outer[2] = {g483,g484}, script[2] = {g483,g484};
        CHECK(kinoko_sqrat_get((void *)(intptr_t)(PTR(layer+82)), "layout", (void *)(intptr_t)(PTR(outer))));
        CHECK(kinoko_sqrat_get((void *)(intptr_t)(PTR(layer+77)), "layout", (void *)(intptr_t)(PTR(script))));
        CHECK(outer[0] == OT_INSTANCE && script[0] == OT_INSTANCE && outer[1] != script[1]);
        CHECK(kinoko_sqrat_raw_set_int((struct SQVM *)(intptr_t)(vm), environment, "registrationMarker", 1));
        CHECK(layer[13] == PTR(layout) + (map ? 320 : 284));
        CHECK(layer[14] == PTR(layout) + (map ? 328 : 288));
        CHECK(layer[1] == PTR(layout)+236 && layer[17] == PTR(layout)+300);
        kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), outer); kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), script);
    }
    kinoko_sqrat_object_release((void *)(intptr_t)(PTR(layer+82)));
    kinoko_sqrat_object_release((void *)(intptr_t)(PTR(layer+77)));
    puts("PASS: original layout registrations create distinct wrappers and exact layer aliases");
    return 0;
}

static int test_chip_resource_registration(int32_t vm, int32_t *root) {
    const int top = sq_gettop(kinoko_vm(vm));
    int32_t resource[17] = {0};
    struct retdec_mcd_chip chip = {0};
    struct retdec_mcd_data data = {0};
    chip.chip_id = 7;
    data.chip_count = 1; data.chips = &chip;
    resource[16] = PTR(&data);
    CHECK(function_42f350(0) == (int32_t)E_INVALIDARG);
    CHECK(retdec_call_thiscall1_result(resource, (void*)g313.e6, vm) == 0);
    CHECK(function_42f350(vm) == 0);
    sq_pushroottable(kinoko_vm(vm));
    sq_pushstring(kinoko_vm(vm), "testChipResource", -1);
    sq_pushstring(kinoko_vm(vm), "CActResourceChip", -1);
    CHECK(SQ_SUCCEEDED(sq_get(kinoko_vm(vm), -3)));
    CHECK(SQ_SUCCEEDED(sq_createinstance(kinoko_vm(vm), -1)));
    CHECK(SQ_SUCCEEDED(sq_setinstanceup(kinoko_vm(vm), -1, resource)));
    sq_remove(kinoko_vm(vm), -2);
    CHECK(SQ_SUCCEEDED(sq_newslot(kinoko_vm(vm), -3, SQFalse)));
    sq_settop(kinoko_vm(vm), top);
    CHECK(execute_source(vm, root+2,
        "local info=testChipResource.GetChipInfo(7);\n"
        "info.flag0=6; info.flag1=123;\n"
        "if(!testChipResource.SetChipFlag(7,0,true) || info.flag0!=7 || info.flag1!=123) throw 1;\n"
        "if(testChipResource.SetChipFlag(7,1,true) || testChipResource.SetChipFlag(7,-1,true)) throw 2;\n"
        "if(testChipResource.SetChipFlag(999,0,true)) throw 3;\n"
        "if(!testChipResource.SetChipFlag(7,0,false) || info.flag0!=6) throw 4;\n"
        "delete testChipResource;\n"));
    CHECK(sq_gettop(kinoko_vm(vm)) == top);
    puts("PASS: chip resource virtual ABI, native ChipInfo and original bit-zero-only SetChipFlag");
    return 0;
}

static int test_texture_resource_registration(int32_t vm, int32_t *root) {
    const int top = sq_gettop(kinoko_vm(vm));
    int32_t resource[25] = {0}, texture[2] = {0};
    IDirect3DBaseTexture9Vtbl texture_vtable = {0};
    texture_vtable.Release = count_texture_release;
    texture[0] = PTR(&texture_vtable);
    resource[0] = PTR(&g365); resource[7] = 15; resource[15] = 15;
    CHECK(function_446520(0) == (int32_t)E_INVALIDARG);
    CHECK(function_4495a0(0) == (int32_t)E_INVALIDARG);
    CHECK(retdec_call_thiscall1_result(resource, (void*)g365.e6, vm) == 0);
    CHECK(retdec_call_thiscall1_result(resource, (void*)g379.e6, vm) == 0);
    CHECK(function_446520(vm) == 0 && function_4495a0(vm) == 0);
    const char *slots[] = {"textureResourceA", "textureResourceB"};
    for (int i = 0; i < 2; ++i) {
        sq_pushroottable(kinoko_vm(vm));
        sq_pushstring(kinoko_vm(vm), slots[i], -1);
        sq_pushstring(kinoko_vm(vm), "CActResource2D", -1);
        CHECK(SQ_SUCCEEDED(sq_get(kinoko_vm(vm), -3)));
        CHECK(SQ_SUCCEEDED(sq_createinstance(kinoko_vm(vm), -1)));
        CHECK(SQ_SUCCEEDED(sq_setinstanceup(kinoko_vm(vm), -1, resource)));
        sq_remove(kinoko_vm(vm), -2);
        CHECK(SQ_SUCCEEDED(sq_newslot(kinoko_vm(vm), -3, SQFalse)));
        sq_settop(kinoko_vm(vm), top);
    }
    CHECK(execute_source(vm, root+2,
        "textureResourceA.resourceID=47; textureResourceA.src_width=12.5;\n"
        "textureResourceA.stName=\"shared\";\n"
        "if(textureResourceB.resourceID!=47 || textureResourceB.src_width!=12.5 || textureResourceB.stName!=\"shared\") throw 1;\n"));
    CHECK(resource[1] == 47 && ((float*)resource)[22] == 12.5f);
    resource[18] = 321;
    CHECK(execute_source(vm, root+2,"if(textureResourceB.image_width!=321) throw 2;\n"));
    {
        const int32_t vtables[] = {PTR(&g313), PTR(&g365), PTR(&g379)};
        const char *names[] = {"chipBinding", "textureBinding", "targetBinding"};
        for (int i = 0; i < 3; ++i) {
            resource[0] = vtables[i];
            void **methods = (void**)(intptr_t)vtables[i];
            CHECK(retdec_call_thiscall2_result(resource, methods[7], PTR(root), 0) == (int32_t)E_FAIL);
            CHECK(retdec_call_thiscall2_result(resource, methods[7], PTR(root), PTR(names[i])) == 0);
            CHECK(retdec_call_thiscall2_result(resource, methods[8], PTR(root), 0) == 0);
        }
        CHECK(execute_source(vm, root+2,
            "if(!(chipBinding instanceof CActResourceChip) || !(textureBinding instanceof CActResource2D) || !(targetBinding instanceof CActRenderTarget)) throw 5;\n"
            "if(!(shared instanceof CActRenderTarget)) throw 6;\n"
            "shared.resourceID=57; if(textureResourceA.resourceID!=57) throw 7;\n"
            "delete chipBinding; delete textureBinding; delete targetBinding; delete shared;\n"));
        resource[0] = PTR(&g365);
    }
    IDirect3DDevice9 *old_device = kinoko_graphics.device; kinoko_graphics.device = 0;
    resource[17] = kinoko_texture_register((IDirect3DBaseTexture9*)texture, 64, 64);
    CHECK(execute_source(vm, root+2,"if(textureResourceA.LoadTexture(null)) throw 3;\n"));
    CHECK(resource[17] && texture[1] == 0); /* Empty name preserves ownership. */
    memcpy(resource+10, "missing", 8); resource[14] = 7;
    CHECK(execute_source(vm, root+2,"if(textureResourceA.LoadTexture(\"data\")) throw 4;\n"));
    CHECK(!resource[17] && texture[1] == 1); /* Failed reload releases old owner. */
    int32_t borrowed = kinoko_texture_register((IDirect3DBaseTexture9*)texture, 64, 64);
    resource[17] = borrowed; ((unsigned char*)resource)[36] = 1;
    CHECK(retdec_call_thiscall0_result(resource, (void*)g365.e11) == 1);
    CHECK(!resource[17] && texture[1] == 1);
    CHECK(kinoko_texture_release(borrowed) == 1 && texture[1] == 2);
    kinoko_graphics.device = old_device;
    CHECK(execute_source(vm, root+2,"delete textureResourceA; delete textureResourceB;\n"));
    CHECK(sq_gettop(kinoko_vm(vm)) == top);
    puts("PASS: texture resource native shared fields, original virtual ABI and reload ownership");
    return 0;
}

static int owned_release_count, owned_release_order[2];
static SQInteger owned_release(SQUserPointer payload, SQInteger size) {
    if (owned_release_count < 2) owned_release_order[owned_release_count] = *(int*)payload;
    ++owned_release_count;
    return 0;
}
static void check_owned_exit(void) {
    if (g643 || owned_release_count != 2 || owned_release_order[0] != 2 || owned_release_order[1] != 1)
        abort();
    puts("PASS: CRT exit destroys owned states newest first, once each");
}
static int root_cache_release_count;
static HSQUIRRELVM root_cache_release_vm;
static int32_t root_cache_release_identity;
static SQInteger release_cached_root_probe(SQUserPointer payload, SQInteger size) {
    (void)payload; (void)size;
    ++root_cache_release_count;
    root_cache_release_vm = (HSQUIRRELVM)g644;
    root_cache_release_identity = g645;
    return 0;
}
static int test_owned_states(int at_exit) {
    CHECK(g643 == 0);
    if (at_exit) CHECK(atexit(check_owned_exit) == 0);
    for (int i = 1; i <= 2; ++i) {
        CHECK(kinoko_sqplus_select_vm((struct SQVM *)(intptr_t)(0)) & 1);
        HSQUIRRELVM vm = (HSQUIRRELVM)g644;
        *(int*)sq_newuserdata(vm, sizeof(int)) = i;
        sq_setreleasehook(vm, -1, owned_release);
        kinoko_sqplus_release_vm_wrappers();
        CHECK(owned_release_count == 0 && g644 == NULL && g645 == 0);
    }
    if (at_exit) return 0;
    HSQUIRRELVM external = sq_open(64);
    const int32_t head = g643;
    CHECK(kinoko_sqplus_select_vm((struct SQVM *)(intptr_t)(PTR(external))) & 1);
    CHECK(g643 == head);
    /* Switching VM must destroy the cached external object while its VM is
       still current. A same-VM selection must preserve that cached owner. */
    {
        HSQUIRRELVM next = sq_open(64);
        void *cache = kinoko_sqplus_root_object();
        CHECK(next && cache && PTR(cache) == g645);
        sq_newuserdata(external, 4);
        sq_setreleasehook(external, -1, release_cached_root_probe);
        kinoko_sqplus_object_capture(cache, -1);
        sq_pop(external, 1);
        CHECK(kinoko_sqplus_select_vm(external) & 1);
        CHECK(root_cache_release_count == 0 && PTR(cache) == g645);
        CHECK(kinoko_sqplus_select_vm(next) & 1);
        CHECK(root_cache_release_count == 1 && root_cache_release_vm == external);
        CHECK(root_cache_release_identity == PTR(cache) && g645 == 0);
        kinoko_sqplus_release_vm_wrappers();
        sq_close(next);
        CHECK(kinoko_sqplus_select_vm(external) & 1);
    }
    kinoko_sqplus_release_vm_wrappers();
    kinoko_sq_release_owned_states();
    check_owned_exit();
    kinoko_sq_release_owned_states();
    CHECK(owned_release_count == 2);
    sq_pushinteger(external, 123);
    SQInteger value = 0;
    CHECK(SQ_SUCCEEDED(sq_getinteger(external, -1, &value)) && value == 123);
    sq_close(external);
    puts("PASS: external VM survives owned-state teardown and repeated cleanup");
    return 0;
}

/* Compiled-only until the user runs the contract: full map virtual clone,
   independent cache storage, borrowed scalar pointers and deleting flags. */
static int test_map_virtual_clone(void) {
    const int offsets[]={264,280,296,332,348,364,384,404,420,436};
    const int widths[]={32,4,4,232,4,12,288,48,4,4};
    unsigned char *source=(unsigned char*)calloc(1,464);
    int32_t clone; int i; CHECK(source);
    *(int32_t*)source=PTR(&g327);*(int32_t*)(source+4)=PTR(&g328);
    *(int32_t*)(source+236)=0x12345678;*(int32_t*)(source+452)=37;
    for(i=0;i<10;++i) {
        unsigned char *data=(unsigned char*)malloc(widths[i]*2);CHECK(data);
        memset(data,0x31+i,widths[i]*2);
        kinoko_native_buffer_replace(PTR(source)+offsets[i],data,widths[i]*2);
        free(data);
    }
    clone=retdec_call_thiscall0_result(source,(void*)g327.e5);CHECK(clone);
    CHECK(*(int32_t*)(intptr_t)clone==PTR(&g327));
    CHECK(*(int32_t*)(intptr_t)(clone+4)==PTR(&g328));
    CHECK(*(int32_t*)(intptr_t)(clone+236)==0x12345678);
    CHECK(*(int32_t*)(intptr_t)(clone+452)==37);
    CHECK(*(unsigned char*)(intptr_t)(clone+460)==1);
    for(i=0;i<10;++i) {
        unsigned char *copied=(unsigned char*)(intptr_t)*(int32_t*)(intptr_t)(clone+offsets[i]);
        unsigned char *original=(unsigned char*)(intptr_t)*(int32_t*)(source+offsets[i]);
        int skip=(widths[i]==232 || widths[i]==288)?4:0;
        int length=widths[i]==288?281:widths[i];
        CHECK(copied!=original);
        CHECK(memcmp(copied+skip,original+skip,length-skip)==0);
        CHECK(memcmp(copied+widths[i]+skip,original+widths[i]+skip,length-skip)==0);
        if(skip) CHECK(*(int32_t*)copied==PTR(&g25));
    }
    CHECK(kinoko_delete_map_sprite(clone+4,NULL,0)==clone);
    for(i=0;i<10;++i) CHECK(*(int32_t*)(intptr_t)(clone+offsets[i])==0);
    free((void*)(intptr_t)clone);kinoko_clear_map_layout(PTR(source));free(source);
    {
        unsigned char *array=(unsigned char*)calloc(1,4+2*464);CHECK(array);
        *(uint32_t*)array=2;
        CHECK(kinoko_delete_map_sprite(PTR(array+8),NULL,2)==PTR(array));
        CHECK(*(uint32_t*)array==2);free(array);
    }
    return 0;
}

static int pool_retire_calls, pool_delete_calls;
static int32_t __fastcall pool_actor_delete_probe(int32_t actor, void *unused, unsigned char flags) {
    (void)unused;
    if (flags & 1) { ++pool_delete_calls; free((void*)(intptr_t)actor); }
    else ++pool_retire_calls;
    return actor;
}
static int test_actor_handle_lookup(void) {
    int32_t manager[20]={0}, first=0, second=0, reused=0;
    int32_t probe=PTR(&pool_actor_delete_probe), a, b;
    int32_t value=0x11223344,node;
    CHECK((int32_t)(intptr_t)(kinoko_actor_pool_construct((KinokoActorPool *)(intptr_t)(PTR(manager))))==PTR(manager));
    a=(int32_t)(intptr_t)(kinoko_actor_pool_request((KinokoActorPool *)(intptr_t)(PTR(manager)), (uint32_t *)(intptr_t)(PTR(&first))));
    b=(int32_t)(intptr_t)(kinoko_actor_pool_request((KinokoActorPool *)(intptr_t)(PTR(manager)), (uint32_t *)(intptr_t)(PTR(&second))));
    CHECK(a && b && a!=b && first==0x10000 && second==0x20001);
    CHECK(retdec_call_thiscall0_result(manager,(void*)g29.e4)==2);
    CHECK(retdec_call_thiscall1_result(manager,(void*)g29.e3,first)==a);
    CHECK(retdec_call_thiscall1_result(manager,(void*)g29.e3,second)==b);
    CHECK(retdec_call_thiscall1_result(manager,(void*)g29.e3,0x20000)==0);
    CHECK(retdec_call_thiscall1_result(manager,(void*)g29.e3,0x20002)==0);
    *(int32_t*)(intptr_t)a=PTR(&probe);*(int32_t*)(intptr_t)b=PTR(&probe);
    kinoko_actor_pool_retire((KinokoActorPool *)(intptr_t)(PTR(manager)), first);
    kinoko_actor_pool_retire((KinokoActorPool *)(intptr_t)(PTR(manager)), second);
    CHECK(pool_retire_calls==2);
    CHECK(kinoko_method_lookup_actor(PTR(manager),NULL,first)==0);
    CHECK(kinoko_method_lookup_actor(PTR(manager),NULL,second)==0);
    kinoko_actor_pool_retire((KinokoActorPool *)(intptr_t)(PTR(manager)), second);CHECK(pool_retire_calls==2);
    CHECK((int32_t)(intptr_t)(kinoko_actor_pool_request((KinokoActorPool *)(intptr_t)(PTR(manager)), (uint32_t *)(intptr_t)(PTR(&reused))))==b && reused==0x30001);
    CHECK((int32_t)(intptr_t)(kinoko_actor_pool_request((KinokoActorPool *)(intptr_t)(PTR(manager)), (uint32_t *)(intptr_t)(PTR(&reused))))==a && reused==0x40000);
    CHECK(retdec_call_thiscall0_result(manager,(void*)g29.e4)==2);
    *(int32_t*)(intptr_t)a=PTR(&probe);*(int32_t*)(intptr_t)b=PTR(&probe);
    CHECK(retdec_call_thiscall1_result(manager,(void*)g29.e0,0)==PTR(manager));
    CHECK(pool_delete_calls==2 && manager[0]==PTR(&g28) && manager[1]==0);
    return 0;
}

static int test_actor_owner_list(void) {
    int32_t manager[4]={0}, handle=0, probe=PTR(&pool_actor_delete_probe), a, b;
    int32_t *pool=(int32_t*)calloc(1,80);
    CHECK(pool);pool_retire_calls=pool_delete_calls=0;
    CHECK((int32_t)(intptr_t)(kinoko_actor_pool_construct((KinokoActorPool *)(intptr_t)(PTR(pool))))==PTR(pool));
    manager[0]=PTR(&g31);manager[1]=PTR(pool);
    kinoko_actor_owner_list_construct((KinokoActorManager *)(intptr_t)(PTR(manager)));
    a=(int32_t)(intptr_t)(kinoko_actor_owner_list_acquire((KinokoActorManager *)(intptr_t)(PTR(manager))));b=(int32_t)(intptr_t)(kinoko_actor_owner_list_acquire((KinokoActorManager *)(intptr_t)(PTR(manager))));
    CHECK(a && b && kinoko_actor_owner_list_size((KinokoActorManager *)(intptr_t)(PTR(manager)))==2);
    *(int32_t*)(intptr_t)a=PTR(&probe);*(int32_t*)(intptr_t)b=PTR(&probe);
    CHECK(*(int32_t*)(intptr_t)(a+8)==1 && *(int32_t*)(intptr_t)(b+8)==1);
    *(int32_t*)(intptr_t)(b+8)=2;
    kinoko_actor_owner_list_clear((KinokoActorManager *)(intptr_t)(PTR(manager)));
    CHECK(kinoko_actor_owner_list_size((KinokoActorManager *)(intptr_t)(PTR(manager)))==0 && pool_retire_calls==1);
    CHECK(*(int32_t*)(intptr_t)(b+8)==1);
    CHECK((int32_t)(intptr_t)(kinoko_actor_pool_request((KinokoActorPool *)(intptr_t)(PTR(pool)), (uint32_t *)(intptr_t)(PTR(&handle))))==a);
    *(int32_t*)(intptr_t)a=PTR(&probe);
    CHECK(retdec_call_thiscall1_result(manager,(void*)g31.e0,0)==PTR(manager));
    CHECK(pool_delete_calls==2 && manager[1]==0 && manager[2]==0);
    return 0;
}

static int test_layout_secondary_lifetime(void) {
    unsigned char *layout=(unsigned char*)calloc(1,316);
    unsigned char *array=(unsigned char*)calloc(1,4+2*316);CHECK(layout && array);
    *(int32_t*)layout=PTR(&g299);*(int32_t*)(layout+4)=PTR(&g300);
    CHECK(retdec_call_thiscall1_result(layout+4,(void*)g300.e0,0)==PTR(layout));
    CHECK(*(int32_t*)layout==PTR(&g299));CHECK(*(int32_t*)(layout+4)==PTR(&g23));
    free(layout);*(uint32_t*)array=2;
    CHECK(kinoko_delete_layout_sprite(PTR(array+8),NULL,2)==PTR(array));
    CHECK(*(int32_t*)(array+8)==PTR(&g23));
    CHECK(*(int32_t*)(array+8+316)==PTR(&g23));free(array);
    return 0;
}

#include "act_document_file_contract.h"
#include "act_virtual_clone_contract.h"
#include "map_lazy_binding_contract.h"
#include "collision_lifecycle_contract.h"

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--collision-lifecycle") == 0)
        return test_collision_lifecycle();
    if (argc == 2 && strcmp(argv[1], "--map-lazy-binding") == 0) {
        int32_t vm = function_48a170(1024), root[5];
        CHECK(vm);
        g644 = (char*)(intptr_t)vm;
        CHECK((int32_t)(intptr_t)(kinoko_sqrat_root_construct((void *)(intptr_t)(PTR(root)), (struct SQVM *)(intptr_t)(vm))));
        return test_map_lazy_binding(vm,root);
    }
    if (argc == 2 && strcmp(argv[1], "--act-virtual-clone") == 0)
        return test_act_virtual_clone();
    if (argc == 2 && strcmp(argv[1], "--act-document-lifetime") == 0)
        return test_act_document_file_lifetime();
    CHECK(test_collision_lifecycle()==0);
    CHECK(test_layout_secondary_lifetime()==0);
    CHECK(test_actor_handle_lookup()==0);
    CHECK(test_actor_owner_list()==0);
    CHECK(test_map_virtual_clone()==0);
    if (argc == 2 && strcmp(argv[1], "--owned-state-exit") == 0)
        return test_owned_states(1);
    if (argc == 2 && strcmp(argv[1], "--texture-lifetime-probe") == 0)
        return test_texture_lifetime();
    if (argc == 2 && strcmp(argv[1], "--gc-link-probe") == 0)
        return test_gc_mark_link();
    if (argc == 2 && strcmp(argv[1], "--sound-module") == 0)
        return kinoko_test_sound_module();
    AddVectoredExceptionHandler(1, contract_exception);
    CHECK(test_act_layer_access() == 0);
    CHECK(test_texture_lifetime() == 0);
    CHECK(test_map_camera_fpu() == 0);
    CHECK(test_sprite_geometry() == 0);
    CHECK(test_quad_colors() == 0);
    CHECK(test_actor_state_fields() == 0);
    CHECK(test_animation_timing() == 0);
    CHECK(test_shutdown_tree_cleanup() == 0);
    CHECK(test_global_stage_cleanup() == 0);
    CHECK(test_global_sound_cleanup() == 0);
    CHECK(kinoko_test_bgm_pause() == 0);
    CHECK(test_owned_states(0) == 0);
    CHECK(test_gc_mark_link() == 0);
    int32_t vm = function_48a170(1024);
    int32_t root[5], environment[3], closure[3];
    int32_t target = PTR(kinoko_script_set_init);
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
    CHECK((int32_t)(intptr_t)(kinoko_sqrat_root_construct((void *)(intptr_t)(PTR(root)), (struct SQVM *)(intptr_t)(vm))));
    CHECK(test_string_layout_binding(vm, root)==0);
    CHECK(test_recovered_object_entries(vm, root) == 0);
    CHECK(test_receiver_operations(vm, root) == 0);
    CHECK(test_thread_receivers(vm, root) == 0);
    CHECK(test_csv_receivers(vm, root) == 0);
    CHECK(test_compiler_receivers(vm, root) == 0);
    CHECK(test_act_script_source_registration(vm, root) == 0);
    CHECK(test_native_instance_receivers(vm, root) == 0);
    CHECK(test_global_callback_destructor(vm) == 0);
    CHECK(test_global_script_cleanup(vm, root) == 0);
    CHECK(test_array_pop_values(vm, root) == 0);
    CHECK(test_script_registrations(vm, root) == 0);
    CHECK(test_physical_input() == 0);
    CHECK(test_input_aggregation() == 0);
    CHECK(test_map_manager_copy() == 0);
    CHECK(test_string_glyph_cache() == 0);
    CHECK(test_string_layout_lifetime() == 0);
    CHECK(test_input_copy() == 0);
    CHECK(test_input_configuration() == 0);
    CHECK(test_table_serialization(vm, root) == 0);
    CHECK(test_camera_map_bindings(vm, root) == 0);
    CHECK(test_map_registration(vm, root) == 0);
    CHECK(test_original_layer_constructor(vm) == 0);
    CHECK(test_script_serialization(vm, root) == 0);
    CHECK(test_layout_registration_entries(vm) == 0);
    CHECK(test_chip_resource_registration(vm, root) == 0);
    CHECK(test_texture_resource_registration(vm, root) == 0);
    CHECK(test_chip_shared_ownership() == 0);
    CHECK(test_act_virtual_clone() == 0);
    CHECK(test_map_lazy_binding(vm, root) == 0);
    {
        int32_t before = function_48aa20(vm);
        CHECK(function_41eff0(0) == (int32_t)E_INVALIDARG);
        CHECK(function_41eff0(vm) == 0);
        CHECK(function_41eff0(vm) == 0);
        CHECK(function_42b6d0(0) == (int32_t)E_INVALIDARG);
        CHECK(function_42b6d0(vm) == 0);
        /* Drop the script root and collect before replacing the native cached
           class: source Sqrat references must keep the old class/tables alive. */
        sq_pushroottable(kinoko_vm(vm));
        sq_pushstring(kinoko_vm(vm), "C2DLayout", -1);
        CHECK(SQ_SUCCEEDED(sq_deleteslot(kinoko_vm(vm), -2, SQFalse)));
        sq_pop(kinoko_vm(vm), 1);
        sq_collectgarbage(kinoko_vm(vm));
        CHECK(function_42b6d0(vm) == 0);
        CHECK(function_42b6d0(vm) == 0);
        CHECK(function_48aa20(vm) == before);
        sq_pushroottable(kinoko_vm(vm));
        sq_pushstring(kinoko_vm(vm), "C2DLayout", -1);
        CHECK(SQ_SUCCEEDED(sq_get(kinoko_vm(vm), -2)));
        CHECK(sq_gettype(kinoko_vm(vm), -1) == OT_CLASS);
        sq_pushstring(kinoko_vm(vm), "__getTable", -1);
        CHECK(SQ_SUCCEEDED(sq_get(kinoko_vm(vm), -2)));
        sq_pushstring(kinoko_vm(vm), "coS_z", -1);
        CHECK(SQ_SUCCEEDED(sq_rawget(kinoko_vm(vm), -2)));
        sq_pop(kinoko_vm(vm), 1);
        sq_pushstring(kinoko_vm(vm), "cos_z", -1);
        CHECK(SQ_FAILED(sq_rawget(kinoko_vm(vm), -2)));
        sq_settop(kinoko_vm(vm), before);
    }
    if(argc==3 && strcmp(argv[1],"--act-reentry")==0)
        return test_act_reentry(argv[2]);
    if(argc==3 && strcmp(argv[1],"--portrait-regions")==0)
        return test_portrait_regions(argv[2]);
    if(argc==2 && strcmp(argv[1],"--texture-serialization")==0)
        return test_texture_serialization(0) || test_texture_serialization(1) ||
            test_chip_serialization() || test_layout_serialization() || test_key_string_writers() || test_act_serialization() ||
            test_serializable_lifetime();
    if(argc==2 && strcmp(argv[1],"--map-serialization")==0)
        return test_map_serialization() || test_map_set_layer();
    if(argc==2 && strcmp(argv[1],"--dynamic-layer")==0)
        return test_dynamic_layer(vm,root);
    if (argc == 3 && strcmp(argv[1], "--water-alpha") == 0)
        return test_water_alpha(manager, argv[2]);
    if (argc == 2 && strcmp(argv[1], "--damage-pause") == 0) {
        CHECK(kinoko_actor_manager_construct((KinokoActorManager *)(intptr_t)(manager)));
        kinoko_actor_register_script_class();
        CHECK(execute_source(vm, root + 2, "Actor.funcUpdate <- null;"));
        return test_stage_update_mask(manager, vm, root);
    }
    if (argc == 3 && strcmp(argv[1], "--crystal-countdown") == 0)
        return test_crystal_countdown(vm, root, argv[2]);
    if (argc == 3 && strcmp(argv[1], "--moving-map") == 0)
        return test_moving_map(vm, root, manager, argv[2], 0, 2000, 463);
    if ((argc == 3 || argc == 5) && strcmp(argv[1], "--moving-map-water") == 0)
        return test_moving_map(vm, root, manager, argv[2], 1, argc==5?(float)atof(argv[3]):2000, argc==5?(float)atof(argv[4]):463);
    if (argc == 3 && strcmp(argv[1], "--orange-platform") == 0)
        return test_platform_riding(vm, root, manager, argv[2], 0);
    if (argc == 3 && strcmp(argv[1], "--green-stage5") == 0)
        return test_platform_riding(vm, root, manager, argv[2], 1);
    if (argc == 3 && strcmp(argv[1], "--road-probe") == 0)
        return test_generator_effects(vm, root, argv[2]);
    if ((argc == 3 && strcmp(argv[1], "--enemy-reentry") == 0) ||
        (argc == 4 && strcmp(argv[1], "--entity-stutter") == 0)) {
        char path[MAX_PATH];
        for(char archive='a';archive<='c';++archive) {
            sprintf_s(path,sizeof(path),"%s/6kinoko_%c.dat",argv[2],archive);
            CHECK(kinoko_archive_mount(path));
        }
        CHECK(kinoko_actor_manager_construct((KinokoActorManager *)(intptr_t)(manager)));
        kinoko_actor_register_script_class();
        CHECK(execute_source(vm,root+2,"Actor.funcUpdate <- null;"));
        int32_t compile_target=PTR(kinoko_script_compile_file_argument);
        CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR("CompileFile")), (const void *)(intptr_t)(PTR(&compile_target)), 4, (void *)(intptr_t)(PTR(retdec_compile_file_native)), 0))>=0);
        g874=1;
        CHECK(execute_asset(vm,root+2,"data/script/constant.cv4"));
        if(argc==4) return test_entity_stutter(manager,vm,root,argv[3]);
        return test_enemy_reentry(manager,vm,root);
    }
    CHECK(test_generator_effects(vm, root, NULL) == 0);
    CHECK(test_gc_repeated_collection(vm, root) == 0);
    CHECK(test_script_callback_binding(vm, root) == 0);
    {
        int32_t anonymous[3];
        CHECK(execute_source(vm,root+2,"anonymousError <- function() { throw 17; };"));
        kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root+1)), (void *)(intptr_t)(PTR(anonymous)), "anonymousError");
        int32_t proto=*(int32_t *)(intptr_t)(anonymous[2]+36);
        CHECK(*(int32_t *)(intptr_t)(proto+20)==g483);
        *(int32_t *)(intptr_t)(proto+24)=0;
        expected_vm_error=1;
        CHECK(!execute_source(vm,root+2,"anonymousError();"));
        expected_vm_error=0;
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(anonymous))));
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
    CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR("SetInitFunctionByID")), (const void *)(intptr_t)(PTR(&target)), 4, (void *)(intptr_t)(PTR(function_471d30)), 0)) >= 0);
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
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(environment)), "registry");
    /* root is Sqrat::Object [vtable, vm, type, value, owner], not SquirrelObject. */
    CHECK(environment[1] == 0x0A000020);
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(environment)), (void *)(intptr_t)(PTR(closure)), "Init0443");
    CHECK(closure[1] == 0x08000100);
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(closure))));

    if (argc > 1) {
        CHECK(execute_file(vm, root + 2, argv[1]));
        CHECK(execute_source(vm, root + 2,
            "if (typeof Init0443 != \"function\" || typeof Init0e4c != \"function\") "
            "throw \"block initialization incomplete\";"));
    }
    CHECK(kinoko_actor_manager_construct((KinokoActorManager *)(intptr_t)(manager)));
    kinoko_actor_register_script_class();
    CHECK(execute_source(vm,root+2,
        "if(Actor.step!=null || Actor.user!=null) throw \"Actor null class defaults\";"));
    CHECK(test_delegate_lifetime(vm, root)==0);
    /* Declare the isolated fixture's script-managed callback slot before creating instances. */
    CHECK(execute_source(vm, root + 2, "Actor.funcUpdate <- null;"));
    layout[0] = PTR(&g327);
    layout[66] = PTR(records);
    layout[67] = PTR(records + 4);
    layout[78] = PTR(layer);
    layout[79] = PTR(resource);
    layer[25] = PTR(resource);
    resource[0] = PTR(&g313);
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
    kinoko_map_containers_construct(PTR(g_retdec_map_manager_state));
    *(int32_t *)(g_retdec_map_manager_state + 12) = PTR(act);
    *(int32_t *)(g_retdec_map_manager_state + 16) = PTR(holder);
    *(int32_t *)(g_retdec_map_manager_state + 20) = PTR(act_resource);
    CHECK(function_46f140(PTR("en")) == PTR(layout));
    CHECK(function_46f140(PTR("e")) == 0);
    CHECK(function_46f140(PTR("missing")) == 0);
    {
        /* BeginStage keeps a template and a separate live ACT. Render lookup
           must agree with collision/event lookup, even when names match. */
        int32_t source_act[60] = {0}, source_layer[87] = {0};
        int32_t source_layout[116] = {0}, source_key[9] = {0};
        int32_t source_head[3] = {0}, source_node[3] = {0};
        int32_t source_layers[1] = {PTR(source_layer)}, source_holder = PTR(source_act);
        int32_t saved_head = *(int32_t *)(g_retdec_map_manager_state + 24);
        int32_t render, second;
        memcpy(source_layer + 28, "en", 3);
        source_layer[32] = 2; source_layer[33] = 15;
        source_layer[45] = PTR(source_head); source_layer[46] = 1;
        source_head[0] = PTR(source_node);
        source_node[0] = PTR(source_head); source_node[2] = PTR(source_key);
        source_key[1] = PTR(source_layout);
        source_layout[0] = PTR(&g327); source_layout[78] = PTR(source_layer);
        source_act[52] = PTR(source_layers); source_act[53] = PTR(source_layers + 1);
        *(int32_t *)(g_retdec_map_manager_state + 12) = PTR(source_act);
        *(int32_t *)(g_retdec_map_manager_state + 16) = PTR(&source_holder);
        kinoko_map_containers_construct(PTR(g_retdec_map_manager_state));
        render = function_470030(PTR("en"));
        CHECK(render != 0 && *(int32_t *)(intptr_t)render == PTR(&g37));
        CHECK(*(int32_t *)(intptr_t)(render + 4) == PTR(layout));
        CHECK(*(int32_t *)(intptr_t)(render + 4) != PTR(source_layout));
        CHECK(function_470030(PTR("missing")) == 0);
        CHECK(kinoko_map_render_count(PTR(g_retdec_map_manager_state)) == 1);
        /* A script-side mutation must be visible through the render binding,
           while the template stays independent for the next activation. */
        *(float *)((unsigned char *)layer + 148) = -172.0f;
        CHECK(*(float *)(intptr_t)(*(int32_t *)(intptr_t)(
            *(int32_t *)(intptr_t)(render + 4) + 312) + 148) == -172.0f);
        CHECK(*(float *)((unsigned char *)source_layer + 148) == 0.0f);
        *(float *)((unsigned char *)layer + 148) = 0.0f;
        second = function_470030(PTR("en"));
        CHECK(second != 0 && second != render);
        CHECK(kinoko_map_render_at(PTR(g_retdec_map_manager_state),0)==render);
        CHECK(kinoko_map_render_at(PTR(g_retdec_map_manager_state),1)==second);
        CHECK(kinoko_map_render_count(PTR(g_retdec_map_manager_state))==2);
        kinoko_map_containers_destroy(PTR(g_retdec_map_manager_state));
        *(int32_t *)(g_retdec_map_manager_state + 12) = PTR(act);
        *(int32_t *)(g_retdec_map_manager_state + 16) = PTR(holder);
        *(int32_t *)(g_retdec_map_manager_state + 24) = saved_head;
        puts("PASS: render layers bind live ACT layouts without undoing clone isolation");
    }
    /* R136: spawn from the layer resource before any render-cache binding. */
    layout[79] = 0;
    target = PTR(kinoko_script_create_map_actors);
    CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR("CreateActorFromMap")), (const void *)(intptr_t)(PTR(&target)), 4, (void *)(intptr_t)(PTR(function_471e50)), 0)) >= 0);
    CHECK(execute_source(vm, root + 2,
        "CreateActorFromMap(\"missing\", registry);\n"
        "CreateActorFromMap(\"en\", other);\n"
        "CreateActorFromMap(\"en\", registry);\n"));
    CHECK(layout[79] == 0);
    layout[79] = PTR(resource);
    created = *(int32_t *)(intptr_t)(manager + 92);
    CHECK(created == 3);
    CHECK(function_48aa20(vm) == top);
    CHECK(execute_source(vm, root + 2,
        "if (seen.len() != 3 || seen[0] != 0x443 || seen[1] != 0xc8a || "
        "seen[2] != 0xc9a) throw \"spawn order/id mismatch\";"));
    CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager)) == 3);
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
    CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager)) == 5);
    function_468620_this(PTR(g_514300_storage));
    CHECK(function_468950_this(PTR(g_514300_storage), manager));
    CHECK(g_514300_storage[2] == g_514300_storage[1]);
    CHECK(g_514300_storage[14] == g_514300_storage[13]);
    target = PTR(kinoko_script_create_event);
    CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR("CreateEvent")), (const void *)(intptr_t)(PTR(&target)), 4, (void *)(intptr_t)(PTR(function_471f70)), 0)) >= 0);
    layout[67] = PTR(records + 2);
    layout[79] = 0;
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
    CHECK(layout[79] == 0);
    layout[79] = PTR(resource);
    {
        int32_t map_state = PTR(g_retdec_map_manager_state);
        int32_t query_actor = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, 110, 210, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
        CHECK(query_actor);
        kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root + 1)), "eventProbe", (const void *)(intptr_t)(query_actor + 44));
        layout[60] = 32;
        layout[61] = 48;
        CHECK(kinoko_map_event_count(map_state) == 1);
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
        CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager)) == 5);
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
        CHECK(kinoko_map_create_actors((KinokoActorManager *)(intptr_t)manager, (KinokoActLayout *)layout, (const KinokoSquirrelObject *)environment) == 600);
        CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager)) == 605);
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
        /* R139: front insertion keeps map/weak-parent pairs aligned. */
        int32_t second_layout[116];
        memcpy(second_layout, retired_layout, sizeof(second_layout));
        int32_t first_count = (g_514300_storage[2] - g_514300_storage[1]) / 4;
        int32_t second_proxy = function_4693a0(PTR(second_layout));
        CHECK(second_proxy != 0);
        CHECK((g_514300_storage[2] - g_514300_storage[1]) / 4 == first_count + 1);
        CHECK((g_514300_storage[14] - g_514300_storage[13]) / 8 == first_count + 1);
        CHECK((g_514300_storage[6] - g_514300_storage[5]) / 4 == first_count + 1);
        CHECK(((int32_t*)(intptr_t)g_514300_storage[1])[0] == PTR(second_layout));
        CHECK(((int32_t*)(intptr_t)g_514300_storage[1])[1] == PTR(retired_layout));
        CHECK(((int32_t*)(intptr_t)g_514300_storage[13])[0] == *(int32_t*)(intptr_t)(second_proxy+24));
        CHECK(((int32_t*)(intptr_t)g_514300_storage[13])[2] == *(int32_t*)(intptr_t)(proxy+24));
        CHECK(*(uint32_t*)(intptr_t)(second_proxy+232) == 0x80000000u);
        CHECK(*(uint8_t*)(intptr_t)(second_proxy+40) == 0);
        CHECK(*(uint8_t*)(intptr_t)(second_proxy+20) == 1);
        CHECK(*(float*)(intptr_t)(second_proxy+440) == -65535.0f);
        CHECK(*(float*)(intptr_t)(second_proxy+452) == 65535.0f);
        CHECK(*(int32_t*)(intptr_t)(second_proxy+228) == -1);
        CHECK(((float*)records[0])[3] == (float)records[0][1]);
        CHECK(((float*)records[0])[4] == (float)records[0][2]);
        control = *(int32_t *)(intptr_t)(proxy + 28);
        CHECK(*(int32_t *)(intptr_t)(control + 4) == 1);
        kinoko_script_clear_actors();
        CHECK(*(int32_t *)(intptr_t)(manager + 92) == 0);
        CHECK(*(int32_t *)(intptr_t)(control + 4) == 0);
        kinoko_native_weak_pair_lock(g_514300_storage[13], pair);
        CHECK(pair[0] == 0 && pair[1] == 0);
        CHECK(VirtualProtect(retired_layout, 4096, PAGE_NOACCESS, &old_protection));
        function_468620_this(PTR(g_514300_storage));
        {
            int32_t pool = *(int32_t *)(intptr_t)(manager + 4);
            int32_t pool_count = kinoko_method_actor_pool_count(pool, NULL);
            int32_t (*reused)[8] = (int32_t (*)[8])calloc(600, 32);
            CHECK(reused != NULL);
            for (int i = 0; i < 600; ++i)
                reused[i][0] = 0x443;
            layout[66] = PTR(reused);
            layout[67] = PTR(reused + 600);
            for (int round = 0; round < 4; ++round) {
                CHECK(kinoko_map_create_actors((KinokoActorManager *)(intptr_t)manager, (KinokoActLayout *)layout, (const KinokoSquirrelObject *)environment) == 600);
                CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager)) == 600);
                CHECK(kinoko_method_actor_pool_count(pool, NULL) == pool_count);
                kinoko_native_weak_pair_lock(g_514300_storage[13], pair);
                CHECK(pair[0] == 0 && pair[1] == 0);
                function_468620_this(PTR(g_514300_storage));
                kinoko_script_clear_actors();
                CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager)) == 0);
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
    kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(closure)), "InitContact");
    {
        int32_t pair_actors[3];
        for (int i = 0; i < 3; ++i) {
            pair_actors[i] = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){closure[0], closure[1], closure[2]}, 0, 0, -1, &(const KinokoOwnedObjectWords){PTR(&g16), 0x05000002, i + 1}, (const void *)(intptr_t)(0)));
            CHECK(pair_actors[i] != 0);
            for (int j = 0; j < 4; ++j)
                *(float *)(intptr_t)(pair_actors[i] + 440 + j * 4) =
                    (float)(10 * i + (j >= 2 ? 10 : 0));
        }
        CHECK(kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager)) == 3);
        CHECK(*(int32_t *)(intptr_t)(manager + 124) != 0);
        CHECK(*(int32_t *)(intptr_t)(manager + 128) -
              *(int32_t *)(intptr_t)(manager + 124) >= 3 * 4);
        kinoko_collision_dispatch_all((KinokoActorManager *)(intptr_t)(manager));
        CHECK(execute_source(vm, root + 2,
            "if (hits.len() != 4 || hits[0] != 12 || hits[1] != 21 || "
            "hits[2] != 23 || hits[3] != 32) throw \"collision pair order\";"));
        /* R140: failure clears only the failing callback and restores the VM
           stack before the reciprocal callback. Then restore the fixture. */
        CHECK(execute_source(vm, root + 2,
            "hits.clear(); probe[0].SetCollisionCallbackFunction(function(other) { throw \"collision probe\"; });"));
        kinoko_collision_dispatch_pair((KinokoActor *)(intptr_t)(pair_actors[0]), (KinokoActor *)(intptr_t)(pair_actors[1]));
        CHECK(function_48aa20(vm) == top);
        CHECK(kinoko_sqplus_object_type((void *)(intptr_t)(pair_actors[0]+136)) != 0x08000100);
        CHECK(execute_source(vm, root + 2,
            "if (hits.len()!=1 || hits[0]!=21) throw \"reciprocal callback after failure\";\n"
            "probe[0].SetCollisionCallbackFunction(::Contact);"));
        CHECK(execute_source(vm, root + 2,
            "hits.clear();\nprobe[1].InterrputCollisionCallback();\n"
            "if (hits.len()!=4 || hits[0]!=21 || hits[1]!=12 || hits[2]!=23 || hits[3]!=32) "
            "throw \"immediate collision callback order\";"));
        CHECK(function_48aa20(vm) == top);
        CHECK(execute_source(vm, root + 2,
            "hits.clear();\nprobe[0].callbackMask = 0;\n"));
        kinoko_collision_dispatch_all((KinokoActorManager *)(intptr_t)(manager));
        CHECK(execute_source(vm, root + 2,
            "if (hits.len() != 3 || hits[0] != 21 || hits[1] != 23 || hits[2] != 32) "
            "throw \"directional masks\";\nhits.clear();\n"));
        *(unsigned char *)(intptr_t)(pair_actors[1] + 40) = 0;
        kinoko_collision_dispatch_all((KinokoActorManager *)(intptr_t)(manager));
        CHECK(execute_source(vm, root + 2,
            "if (hits.len() != 0) throw \"inactive/disjoint pair\";"));
        *(unsigned char *)(intptr_t)(pair_actors[1] + 40) = 1;
        CHECK(execute_source(vm, root + 2,
            "probe[0].callbackMask = 1;\n"
            "probe[0].SetCollisionCallbackFunction(function(other) { "
            "::hits.append(user * 10 + other.user); callbackGroup = 0; });\n"));
        kinoko_collision_dispatch_all((KinokoActorManager *)(intptr_t)(manager));
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
        target = PTR(kinoko_script_create_actor);
        CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR("CreateActor")), (const void *)(intptr_t)(PTR(&target)), 4, (void *)(intptr_t)(PTR(kinoko_script_create_actor_entry)), 0)) >= 0);
        *(int32_t *)(intptr_t)(manager + 64) = -1;
        CHECK(kinoko_actor_manager_update((KinokoActorManager *)(intptr_t)(manager), (KinokoCamera *)(intptr_t)(PTR(g_retdec_camera_state))) == 3);
        CHECK(execute_source(vm, root + 2,
            "if (child.x != 105.0 || childSteps != 0) throw \"post-step refresh\";"));
        CHECK(*(int32_t *)(intptr_t)(pair_actors[1] + 28) == 0);
        CHECK(function_48aa20(vm) == top);
    }
    (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(closure))));
    {
        int32_t actor, map_proxy, animation[7] = {0};
        kinoko_script_clear_actors();
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
        map_proxy = function_4693a0(PTR(layout));
        CHECK(map_proxy);
        actor = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, 50, 40, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
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
        kinoko_actor_manager_refresh((KinokoActorManager *)(intptr_t)(manager));
        function_468620_this(PTR(g_514300_storage));
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
        CHECK(*(float *)(intptr_t)(actor + 240) == 52);
        CHECK(*(float *)(intptr_t)(actor + 244) == 45);
        for (int i = 0; i < 12; ++i) {
            function_468620_this(PTR(g_514300_storage));
            kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
        }
        CHECK(*(float *)(intptr_t)(actor + 244) == 100);
        CHECK(*(int32_t *)(intptr_t)(actor + 296) == 1);
        CHECK(*(int32_t *)(intptr_t)(actor + 36) != 0);
        CHECK(*(int32_t *)(intptr_t)g_514300_storage[5] == 1);
        CHECK(*(int32_t *)(intptr_t)(actor+32) == *(int32_t *)(intptr_t)(map_proxy+24));
        CHECK(*(int32_t *)(intptr_t)(*(int32_t *)(intptr_t)(map_proxy+28)+4) == 1);
        *(float *)(intptr_t)(actor + 260) = -6;
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
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
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
        CHECK(*(float *)(intptr_t)(actor + 240) == 102);
        CHECK(*(int32_t *)(intptr_t)(actor + 292) == 1);
        kinoko_actor_move(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor), 40, 0);
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
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
        CHECK(*(float *)(intptr_t)(actor + 244) == 68);
        CHECK(*(int32_t *)(intptr_t)(actor + 288) != 0);
        *(uint32_t *)(chips[0].bytes + 16) = 0x20;
        position_actor(actor, 50, 80);
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
        CHECK(*(float *)(intptr_t)(actor + 244) == 62);
        CHECK(*(int32_t *)(intptr_t)(actor + 288) == 0);
        records[0][2] = 16;
        *(uint32_t *)(chips[0].bytes + 16) = 0;
        *(int16_t *)(chips[0].bytes + 12) = 32;
        *(int16_t *)(chips[0].bytes + 34) = 1;
        position_actor(actor, 16, 40);
        *(float *)(intptr_t)(actor + 260) = 0;
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
        CHECK(*(float *)(intptr_t)(actor + 244) == 32);
        CHECK(*(float *)(intptr_t)(actor + 276) == -1);
        CHECK(*(int32_t *)(intptr_t)(actor + 296) == 1);
        *(int16_t *)(chips[0].bytes + 34) = 2;
        position_actor(actor, 16, 40);
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
        CHECK(*(float *)(intptr_t)(actor + 244) == 32);
        CHECK(*(float *)(intptr_t)(actor + 276) == 1);
        *(uint8_t *)((char *)animation + 25) = 0;
        position_actor(actor, 16, 40);
        *(float *)(intptr_t)(actor + 260) = 5;
        kinoko_actor_update_motion(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor));
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
            CHECK(fixture_collision_query_map(PTR(layout), actor, 0, &count));
            found = (KinokoCollisionRecord *)(intptr_t)g_514300_storage[9];
            CHECK(count == 2 && found[0].index == 1 && found[1].index == 2);
            count = 0;
            position_actor(actor, 16, 100);
            CHECK(fixture_collision_query_map(PTR(layout), actor, 0, &count));
            found = (KinokoCollisionRecord *)(intptr_t)g_514300_storage[9];
            CHECK(count == 1 && found[0].index == 0);
            count = 0;
            position_actor(actor, 144, 100);
            CHECK(fixture_collision_query_map(PTR(layout), actor, 0, &count));
            found = (KinokoCollisionRecord *)(intptr_t)g_514300_storage[9];
            CHECK(count == 1 && found[0].index == 2);
        }
        {
            /* Complete the R139 query chain with two registered layers. */
            int32_t second_map[116];
            memcpy(second_map, layout, sizeof(second_map));
            CHECK(function_4693a0(PTR(second_map)));
            *(int16_t *)(chips[0].bytes + 34) = 0;
            position_actor(actor, 144, 100);
            CHECK(kinoko_collision_move_actor(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor), 0.0f, 0.0f));
            int32_t *ends = (int32_t*)(intptr_t)g_514300_storage[5];
            KinokoCollisionRecord *found = (KinokoCollisionRecord*)(intptr_t)g_514300_storage[9];
            CHECK(ends[0] == 1 && ends[1] == 2);
            CHECK(found[0].index == 2 && found[1].index == 2);
            /* Retire the borrowed stack layout before leaving this scope. */
            kinoko_script_clear_actors();
            CHECK(function_468950_this(PTR(g_514300_storage), manager));
        }
        kinoko_script_clear_actors();
        function_468950_this(PTR(g_514300_storage), manager);
    }
    if (argc > 2) {
        CHECK(execute_file(vm, root + 2, argv[2]));
        target = PTR(kinoko_script_set_global_update);
        CHECK((int32_t)(intptr_t)(kinoko_sqrat_bind_object_function((void *)(intptr_t)(PTR(root)), (const char *)(intptr_t)(PTR("SetGlobalUpdateFunction")), (const void *)(intptr_t)(PTR(&target)), 4, (void *)(intptr_t)(PTR(kinoko_script_global_update_entry)), 0)) >= 0);
        CHECK(execute_source(vm, root + 2,
            "fadeCalls <- [];\n"
            "Fader1 <- { FadeOut = function(a,b,c,d) { ::fadeCalls.append(0); }, "
            "FadeIn = function(a,b,c,d) { ::fadeCalls.append(1); } };\n"
            "StageStart <- { pl = { visible = true } };\n"
            "updateMask <- 0x40000000;\nupdateMaskPause <- -1;\n"
            "function UpdateGlobal() {}\n"
            "stageChangeCount = 120;\nSetGlobalUpdateFunction(UpdateStageStart);\n"));
        for (int i = 0; i < 121; ++i)
            CHECK(kinoko_script_callback_invoke((KinokoScriptCallback *)(intptr_t)(PTR(g612))) >= 0);
        CHECK(execute_source(vm, root + 2,
            "if (stageChangeCount != -1 || StageStart.pl.visible || updateMask != -1 || "
            "fadeCalls.len() != 2) throw \"stage start countdown stalled\";"));
        for (int i = 0; i < 16; ++i)
            CHECK(kinoko_script_callback_invoke((KinokoScriptCallback *)(intptr_t)(PTR(g612))) >= 0);
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
                CHECK(kinoko_script_callback_invoke((KinokoScriptCallback *)(intptr_t)(PTR(g612))) >= 0);
            CHECK(*(uint8_t *)(act + 24) == 0);
            CHECK(function_48aa20(vm) == top);
            kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), player_pair);
        }
    }
    {
        int32_t draw_vtable[9] = {0}, fake_layout[80] = {0};
        draw_vtable[8] = PTR(count_draw);
        fake_layout[0] = PTR(draw_vtable);
        layout_key[1] = PTR(fake_layout);
        act_resource[3] = PTR(act);
        act[24] = 0;
        InitializeCriticalSection((struct retdec_RTL_CRITICAL_SECTION *)(act_resource + 5));
        CHECK(kinoko_act_draw(PTR(act_resource), 0, 0) == 0);
        CHECK(draw_count == 0);
        act[24] = 1;
        CHECK(kinoko_act_draw(PTR(act_resource), 0, 0) == 0);
        CHECK(draw_count == 1);
        DeleteCriticalSection((struct retdec_RTL_CRITICAL_SECTION *)(act_resource + 5));
    }
    {
        int32_t animation[14] = {0};
        int32_t frames[62] = {0};
        int32_t actor = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, 100, 200, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
        CHECK(actor);
        animation[2] = PTR(frames);
        animation[3] = PTR(frames + 62);
        animation[7] = -6;
        animation[8] = -20;
        animation[9] = 10;
        animation[10] = -1;
        animation[11] = 7;
        *((uint8_t *)animation + 25) = 1;
        CHECK(fixture_pat_tree_put(manager, 0x60000001, PTR(animation)));
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
        kinoko_actor_set_take((KinokoActor *)(intptr_t)(actor), 0x60000001);
        CHECK(*(float *)(intptr_t)(actor + 440) == 68.5f);
        CHECK(*(float *)(intptr_t)(actor + 448) == 119.5f);
        CHECK(*(int16_t *)(intptr_t)(actor + 388) == 51);
        kinoko_actor_set_take((KinokoActor *)(intptr_t)(actor), 0x60000002);
        CHECK(*(int32_t *)(intptr_t)(actor + 200) == PTR(animation));
        CHECK(*(float *)(intptr_t)(actor + 440) == 68.5f);
        CHECK(*(int32_t *)(intptr_t)(actor + 208) == 0x60000002);
        CHECK(*(int32_t *)(intptr_t)(actor + 212) == 0);
        CHECK(*(int32_t *)(intptr_t)(actor + 216) == 0);
        *((uint8_t *)animation + 25) = 0;
        kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root + 1)), "animationTakeProbe", (const void *)(intptr_t)(actor + 44));
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
        kinoko_script_clear_actors();
    }
    CHECK(test_pat_records(manager) == 0);
    CHECK(test_actor_step(manager, vm, root) == 0);
    CHECK(test_actor_reset(manager, vm, root) == 0);
    {
        int32_t actor = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, 100, 200, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
        float dx = -40.0f, dy = 0.0f;
        int32_t dx_bits, dy_bits;
        int fault = 0;
        CHECK(actor);
        memcpy(&dx_bits, &dx, sizeof(dx_bits));
        memcpy(&dy_bits, &dy, sizeof(dy_bits));
        __try {
            retdec_call_thiscall2_result((void *)(intptr_t)actor, kinoko_method_actor_move, dx_bits, dy_bits);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fault = 1;
        }
        CHECK(fault == 0);
        CHECK(*(float *)(intptr_t)(actor + 240) == 60);
        CHECK(*(float *)(intptr_t)(actor + 244) == 200);
        __try {
            retdec_call_thiscall0_result((void *)(intptr_t)actor, kinoko_actor_reset_method);
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
            kinoko_actor_refresh_collision_bounds((KinokoActor *)(intptr_t)(actor));
            kinoko_actor_move(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor), 21, -18);
            CHECK(*(float *)(intptr_t)(actor + 240) == 121);
            CHECK(*(float *)(intptr_t)(actor + 244) == 182);
            CHECK(*(float *)(intptr_t)(actor + 248) == 116);
            CHECK(*(float *)(intptr_t)(actor + 252) == 184);
            CHECK(*(float *)(intptr_t)(actor + 256) == 12);
            CHECK(*(float *)(intptr_t)(actor + 260) == -4);
            CHECK(*(float *)(intptr_t)(actor + 264) == 7);
            CHECK(*(float *)(intptr_t)(actor + 268) == 9);
            memcpy(unchanged, (const void *)(intptr_t)actor, sizeof(unchanged));
            kinoko_actor_move(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor), 0, 0);
            CHECK(memcmp(unchanged, (const void *)(intptr_t)actor, sizeof(unchanged)) == 0);
            *((uint8_t *)animation + 25) = 0;
            kinoko_actor_move(kinoko_game_collision_state(), (KinokoActor *)(intptr_t)(actor), -40, 0);
            CHECK(*(float *)(intptr_t)(actor + 240) == 81);
            CHECK(*(float *)(intptr_t)(actor + 248) == 121);
        }
        kinoko_script_clear_actors();
    }
    {
        int32_t constructed[136];
        kinoko_actor_construct((KinokoActor *)(intptr_t)(PTR(constructed)));
        CHECK(constructed[27] == PTR(&g16));
        CHECK(constructed[28] == g483 && constructed[29] == g484);
    }
    {
        int32_t actor = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, 100, 200, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
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
            kinoko_sqplus_object_raw_set_name((void *)(intptr_t)(PTR(root + 1)), "queryProbe", (const void *)(intptr_t)(actor + 44));
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
                int32_t other = (int32_t)(intptr_t)(kinoko_actor_manager_create((KinokoActorManager *)(intptr_t)(manager), &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, 200, 300, -1, &(const KinokoOwnedObjectWords){PTR(&g16), g483, g484}, (const void *)(intptr_t)(0)));
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
        kinoko_script_clear_actors();
    }
    {
        int32_t source[136] = {0}, destination[136] = {0};
        const int offsets[] = {44, 56, 68, 96, 108, 124, 136};
        int32_t* allocation = (int32_t*)malloc(sizeof(int32_t));
        CHECK(allocation != NULL);
        *allocation = PTR(source);
        source[6] = PTR(allocation);
        kinoko_native_control_create(PTR(source + 7), PTR(allocation));
        CHECK(source[7] != 0);
        source[8] = PTR(allocation); source[9] = source[7];
        kinoko_native_add_weak(source[9]);
        source[135] = 1234567;
        destination[93] = 98765; /* original copy skips Actor+372 */
        for (int i = 0; i < 7; ++i) {
            kinoko_sqplus_object_copy_construct((void *)(intptr_t)((int32_t*)((char*)source + offsets[i])), (const void *)(intptr_t)(PTR(root + 1)));
            kinoko_sqplus_object_initialize((void *)(intptr_t)(PTR((char*)destination + offsets[i])));
        }
        CHECK(kinoko_actor_assign_instance(PTR(destination), PTR(source)) == PTR(destination));
        CHECK(destination[135] == 1234567 && destination[93] == 98765);
        CHECK(destination[7] == source[7] && destination[9] == source[9]);
        CHECK(((int32_t*)(intptr_t)source[7])[1] == 2);
        CHECK(((int32_t*)(intptr_t)source[7])[2] == 3);
        CHECK(kinoko_actor_assign_instance(PTR(destination), PTR(destination)) == PTR(destination));
        CHECK(((int32_t*)(intptr_t)source[7])[1] == 2);
        CHECK(((int32_t*)(intptr_t)source[7])[2] == 3);
        for (int i = 0; i < 7; ++i) {
            CHECK(memcmp((char*)destination + offsets[i] + 4,
                         (char*)source + offsets[i] + 4, 8) == 0);
            (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR((char*)destination + offsets[i]))));
            (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR((char*)source + offsets[i]))));
        }
        kinoko_native_release_strong(destination[7]);
        kinoko_native_release_weak(destination[9]);
        kinoko_native_release_strong(source[7]);
        kinoko_native_release_weak(source[9]);
    }
    {
        int32_t camera[128] = {0}, callback[3];
        CHECK(execute_source(vm, root + 2,
            "cameraProbeCount <- 0;\n"
            "function CameraProbeUpdate() { ::cameraProbeCount++; }"));
        kinoko_sqplus_object_copy_construct((void *)(intptr_t)(camera), (const void *)(intptr_t)(PTR(root + 1)));
        kinoko_sqplus_object_get_value((void *)(intptr_t)(PTR(root + 1)), (void *)(intptr_t)(PTR(callback)), "CameraProbeUpdate");
        retdec_call_thiscall3_result(camera, kinoko_camera_set_update_callback,
            callback[0], callback[1], callback[2]);
        int32_t camera_top = function_48aa20(vm);
        retdec_call_thiscall0_result(camera, kinoko_camera_update);
        CHECK(function_48aa20(vm) == camera_top);
        CHECK(execute_source(vm, root + 2,
            "if (cameraProbeCount != 1) throw \"camera update callback skipped\";"));
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(camera + 7))));
        CHECK(kinoko_camera_update((KinokoCamera *)camera, NULL) == g483);
        CHECK(execute_source(vm, root + 2,
            "if (cameraProbeCount != 1) throw \"empty camera callback executed\";"));
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(camera + 4))));
        (int32_t)(intptr_t)(kinoko_sqplus_object_destroy((void *)(intptr_t)(PTR(camera))));
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
    CHECK((int32_t)(intptr_t)(kinoko_actor_manager_clear_resources((KinokoActorManager *)(intptr_t)(manager))) == *(int32_t *)(intptr_t)(manager + 100));
    CHECK((int32_t)(intptr_t)(kinoko_actor_manager_clear_resources((KinokoActorManager *)(intptr_t)(manager))) == *(int32_t *)(intptr_t)(manager + 100));
    kinoko_map_containers_destroy(PTR(g_retdec_map_manager_state));
    puts("PASS: stage lifecycle, terrain motion, start visibility and animation loading/bounds");
    return 0;
}
