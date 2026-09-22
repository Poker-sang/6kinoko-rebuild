/* Included after stage_contract.c's byte-stream fixture. This mode exercises
   the actual file reader/payload parser, not the isolated I/O contract mocks. */
static const char *act_file_probe_path;
static int act_file_deletes, act_file_close_error;
static int act_file_layout_binds;
static int32_t __fastcall act_file_set_layer_probe(int32_t layout, void *unused, int32_t layer) {
    ++act_file_layout_binds;
    return kinoko_method_layout_set_layer(layout, unused, layer);
}
static int32_t __fastcall act_file_delete_probe(int32_t self, void *unused, unsigned char flags) {
    HANDLE exclusive = CreateFileA(act_file_probe_path, GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ++act_file_deletes;
    if (exclusive == INVALID_HANDLE_VALUE) act_file_close_error = 1;
    else CloseHandle(exclusive);
    return kinoko_method_destroy_act(self, unused, flags);
}
static int act_file_write(const char *path, const void *bytes, DWORD size) {
    DWORD written = 0;
    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    const int ok = WriteFile(file, bytes, size, &written, NULL) && written == size;
    return CloseHandle(file) && ok;
}
static int test_act_document_file_lifetime(void) {
    int32_t methods[6] = {0, 0, 0, PTR(script_io_transfer), 0, PTR(script_io_seek)};
    struct script_io_stream stream = {0}; stream.vtable = methods;
    const unsigned char saved_compact = g673;
    const int32_t saved_archives = kinoko_archive_count;
    char *saved_vm = g644;
    int32_t (*saved_delete)(unsigned char) = g285.e4;
    const char *valid_path = "act-document-generated.bin";
    const char *short_path = "act-document-truncated.bin";
    unsigned char encoded[8192 + 19] = {0};
    const uint32_t header[] = {0x31544341u, 1u, 7u};
    g673 = 0; kinoko_archive_count = 0; g644 = NULL;
    KinokoActDocument *source = kinoko_act_document_create();
    CHECK(source);
    const int32_t layer = retdec_act_make_layer();
    CHECK(layer);
    kinoko_act_array_append(PTR(source) + 208, layer);
    /* Forward parent reference: child is serialized before its parent. */
    *(int32_t *)(intptr_t)(layer + 104) = 20;
    *(int32_t *)(intptr_t)(layer + 108) = 10;
    const int32_t parent = retdec_act_make_layer();
    CHECK(parent);
    *(int32_t *)(intptr_t)(parent + 104) = 10;
    kinoko_act_array_append(PTR(source) + 208, parent);
    const int32_t key = PTR(calloc(1, 36));
    const int32_t layout = PTR(calloc(1, 316));
    CHECK(key && layout && retdec_construct_c2dlayout(layout));
    *(int32_t *)(intptr_t)key = PTR(kinoko_act_host_symbols()->key_vtable);
    *(int32_t *)(intptr_t)(key + 4) = layout;
    *(int32_t *)(intptr_t)(key + 28) = 15;
    CHECK(retdec_act_append_list(layer + 180, key));
    ++*(int32_t *)(intptr_t)(layer + 184);
    kinoko_string_assign_cstr((int32_t *)(intptr_t)(layer + 112),
        "heap-owned layer from generated ACT");
    CHECK(kinoko_method_write_act(PTR(source), NULL, PTR(&stream)));
    memcpy(encoded, header, sizeof(header));
    memset(encoded + sizeof(header), 0xa5, 7); /* relative skip, not an absolute offset */
    memcpy(encoded + 19, stream.bytes, stream.size);
    const DWORD total = 19 + stream.size;
    CHECK(act_file_write(valid_path, encoded, total));
    retdec_destroy_cact_with_flags(PTR(source), 1);

    /* Observe the real virtual deleting destructor, forwarding to its normal
       implementation. An exclusive file open proves the reader closed first. */
    g285.e4 = (int32_t (*)(unsigned char))act_file_delete_probe;
    act_file_deletes = act_file_close_error = 0;
    act_file_probe_path = valid_path;
    int32_t (*saved_set_layer)(int32_t) = g299.e6;
    g299.e6 = (int32_t (*)(int32_t))act_file_set_layer_probe;
    act_file_layout_binds = 0;
    KinokoActDocument *loaded = kinoko_act_document_create();
    CHECK(loaded && kinoko_act_document_load(loaded, valid_path));
    CHECK(act_file_layout_binds == 1); /* 41F8B9 only, no second document pass */
    g299.e6 = saved_set_layer;
    const int32_t *loaded_layers = *(int32_t **)((char *)loaded + 208);
    CHECK(*(int32_t *)(intptr_t)(loaded_layers[0] + 88) == loaded_layers[1]);
    CHECK(*(int32_t *)(intptr_t)(loaded_layers[0] + 108) == 10);
    CHECK(**(int32_t **)(intptr_t)(loaded_layers[1] + 72) == loaded_layers[0]);
    CHECK(strcmp(kinoko_act_document_name(loaded), "act") == 0);
    CHECK(kinoko_act_document_screen_width(loaded) == 1280);
    CHECK(kinoko_act_document_screen_height(loaded) == 720);
    CHECK(retdec_call_thiscall1_result(loaded, (void *)g285.e4, 1));
    CHECK(act_file_deletes == 1 && !act_file_close_error);

    kinoko_stage_list_construct();
    act_file_probe_path = short_path;
    for (DWORD length = 0; length < total; ++length) {
        CHECK(act_file_write(short_path, encoded, length));
        const int before = act_file_deletes;
        /* 466100 continues after failed parse; cleanup occurs only on clear. */
        CHECK(kinoko_stage_load(short_path));
        CHECK(act_file_deletes == before && g604 == 1);
        kinoko_clear_global_stages();
        CHECK(act_file_deletes == before + 1 && !act_file_close_error);
        CHECK(g604 == 0 && kinoko_stage_list_first() == kinoko_stage_list_end());
    }
    /* The entire accepted document remains owned after list publication; a
       clear destroys it exactly once, including its layer/script allocations. */
    act_file_probe_path = valid_path;
    const int before = act_file_deletes;
    KinokoStageOwner *published = kinoko_stage_load(valid_path);
    CHECK(published && g604 == 1 && act_file_deletes == before);
    CHECK(kinoko_stage_list_value(kinoko_stage_list_first()) == published);
    kinoko_clear_global_stages();
    CHECK(g604 == 0 && act_file_deletes == before + 1 && !act_file_close_error);
    kinoko_clear_global_stages();
    CHECK(act_file_deletes == before + 1);
    kinoko_stage_list_destroy();

    /* With no global list, the caller owns the same complete cleanup path. */
    KinokoStageOwner *unpublished = kinoko_stage_load(valid_path);
    CHECK(unpublished && g603 == 0 && act_file_deletes == before + 1);
    kinoko_stage_owner_destroy(unpublished);
    CHECK(act_file_deletes == before + 2 && !act_file_close_error);
    kinoko_stage_owner_destroy(NULL);
    g285.e4 = saved_delete; g673 = saved_compact; kinoko_archive_count = saved_archives; g644 = saved_vm;
    printf("PASS: real ACT file/payload, %lu truncated prefixes, reader-before-document release, stage publication and caller ownership\n", (unsigned long)total);
    return 0;
}
