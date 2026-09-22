/* R134/R135/R136/R137 regression source. Builds with stage_contract; execution is user-owned.
   Exercise actual document/layer/key/map virtual clones, without manually
   performing the second SetLayer that masked the missing consumer behavior. */
static int test_map_lazy_binding(int32_t vm, int32_t *root) {
    const int32_t previous_default_vm = g664;
    g664 = vm;
    int32_t source[60] = {0};
    int32_t *resource = (int32_t*)calloc(1,100);
    struct retdec_mcd_data *data = (struct retdec_mcd_data*)calloc(1,sizeof(*data));
    int32_t layer = retdec_act_make_layer();
    int32_t *key = (int32_t*)calloc(1,36);
    int32_t *map = (int32_t*)calloc(1,464);
    int32_t records[1][8] = {{4,7,8,0,0,0,1,0}};
    CHECK(resource && data && layer && key && map);
    CHECK(kinoko_act_document_initialize((KinokoActDocument*)source));
    retdec_string_assign_n(source+4,"RegistrationProbe",17);
    retdec_string_assign_n((int32_t*)(intptr_t)(layer+112),"mapProbe",8);
    ((float*)map)[80] = 1.0f;
    map[60] = map[61] = 16;
    data->chip_count = 1;
    data->chips = (struct retdec_mcd_chip*)calloc(1,sizeof(*data->chips));
    CHECK(data->chips);
    data->chips[0].chip_id = 4;
    *(uint32_t*)data->chips[0].bytes = 4;
    *(int16_t*)(data->chips[0].bytes+12) = 16;
    *(int16_t*)(data->chips[0].bytes+14) = 16;
    resource[0] = PTR(&g313); resource[1] = 42;
    resource[7] = resource[14] = resource[23] = 15;
    resource[16] = PTR(data);
    *(int32_t*)(intptr_t)(layer+96) = 42;
    *(int32_t*)(intptr_t)(layer+100) = PTR(resource);
    *(int32_t*)(intptr_t)(layer+104) = 9;
    map[0] = PTR(&g327); map[1] = PTR(&g328); map[113] = -1;
    kinoko_native_buffer_replace(PTR(map)+264,records,sizeof(records));
    key[0] = PTR(&g277); key[1] = PTR(map); key[7] = 15;
    CHECK(retdec_act_append_list(layer+180,PTR(key)));
    *(int32_t*)(intptr_t)(layer+184) = 1;
    kinoko_act_array_append(PTR(source)+224,PTR(resource));
    kinoko_act_array_append(PTR(source)+208,layer);

    for (int query = 0; query != 5; ++query) {
        KinokoActDocument *copy = kinoko_act_clone((KinokoActDocument*)source,NULL);
        CHECK(copy);
        int32_t *doc = (int32_t*)copy;
        int32_t cloned_layer = *(int32_t*)(intptr_t)doc[52];
        int32_t cloned_resource = *(int32_t*)(intptr_t)doc[56];
        int32_t head = *(int32_t*)(intptr_t)(cloned_layer+180);
        int32_t node = *(int32_t*)(intptr_t)head;
        int32_t cloned_key = *(int32_t*)(intptr_t)(node+8);
        int32_t *layout = *(int32_t**)(intptr_t)(cloned_key+4);
        CHECK(cloned_resource != PTR(resource));
        CHECK(*(int32_t*)(intptr_t)(cloned_layer+100) == cloned_resource);
        CHECK(layout[78] == cloned_layer && layout[79] == 0);
        CHECK(((uint8_t*)layout)[460] == 0);
        CHECK(kinoko_map_layer_chip_data((KinokoActLayout*)layout) == data);
        CHECK(kinoko_map_cached_chip_data((KinokoActLayout*)layout) == NULL);
        /* Creation consults the owner even when a different cache is present.
           Query preserves a non-null cache; only Update rejects stale binding. */
        {
            struct retdec_mcd_data other_data = {0};
            int32_t other_resource[17] = {0};
            other_resource[16] = PTR(&other_data);
            layout[79] = PTR(other_resource);
            CHECK(kinoko_map_layer_chip_data((KinokoActLayout*)layout) == data);
            CHECK(kinoko_map_cached_chip_data((KinokoActLayout*)layout) == &other_data);
            CHECK(kinoko_map_query_chip_data((KinokoActLayout*)layout) == &other_data);
            CHECK(layout[79] == PTR(other_resource));
            layout[79] = 0;
        }
        CHECK(layout[79] == 0); /* Layer lookup must not prime render caches. */
        *(int32_t*)(intptr_t)(cloned_layer+100) = 0;
        CHECK(kinoko_map_layer_chip_data((KinokoActLayout*)layout) == NULL);
        *(int32_t*)(intptr_t)(cloned_layer+100) = cloned_resource;
        if (!query) {
            /* Invisible Update must not consume the pending binding. */
            *(uint8_t*)(intptr_t)(cloned_layer+140) = 0;
            CHECK(kinoko_map_update(PTR(layout),0,0,100,100) == 0);
            CHECK(layout[79] == 0);
            *(uint8_t*)(intptr_t)(cloned_layer+140) = 1;
            CHECK(kinoko_map_update(PTR(layout),0,0,100,100) == 0);
            CHECK(layout[79] == cloned_resource);
            /* A non-null stale resource must fail without rebinding. */
            layout[79] = PTR(resource);
            CHECK(kinoko_map_update(PTR(layout),0,0,100,100) == (int32_t)E_FAIL);
            CHECK(layout[79] == PTR(resource));
            layout[79] = 0;
            *(int32_t*)(intptr_t)(cloned_layer+100) = 0;
            CHECK(kinoko_map_update(PTR(layout),0,0,100,100) == (int32_t)E_FAIL);
            CHECK(layout[79] == 0);
            *(int32_t*)(intptr_t)(cloned_layer+100) = cloned_resource;
        } else if (query == 1) {
            int32_t klass[2] = {g483,g484}, instance[2] = {g483,g484};
            *(uint8_t*)(intptr_t)(cloned_layer+140) = 0;
            CHECK(retdec_publish_c2dmaplayout_class(vm,PTR(root),klass));
            CHECK(retdec_create_bound_instance(vm,root+2,"LazyMapProbe",klass,PTR(layout),instance));
            CHECK(execute_source(vm,root+2,
                "if (LazyMapProbe.GetChipByPosition(8,9) != 0) throw \"unbound event map\";"
                "if (LazyMapProbe.GetChipByPosition(100,100) != -1) throw \"outside map\";"));
            CHECK(layout[79] == cloned_resource);
            layout[79] = 0;
            *(int32_t*)(intptr_t)(cloned_layer+100) = 0;
            CHECK(execute_source(vm,root+2,
                "if (LazyMapProbe.GetChipByPosition(8,9) != -1) throw \"missing resource\";"));
            CHECK(layout[79] == 0);
            *(int32_t*)(intptr_t)(cloned_layer+100) = cloned_resource;
            CHECK(execute_source(vm,root+2,"delete LazyMapProbe;"));
            kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), instance);
            kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), klass);
        }
        if (query == 2 || query == 3) {
            int32_t runtime[48] = {0}, parent[2] = {g483,g484}, active = 0;
            CHECK(*(int32_t*)(intptr_t)(cloned_layer+52) == 0);
            CHECK(*(int32_t*)(intptr_t)(cloned_layer+56) == 0);
            CHECK(retdec_publish_cact_layer_class(vm,PTR(root)));
            CHECK(kinoko_sqrat_new_table((struct SQVM *)(intptr_t)(vm), parent));
            CHECK(kinoko_sqrat_set_pair((struct SQVM *)(intptr_t)(vm), root+2, "RegistrationProbe", parent));
            CHECK(execute_source(vm,parent,"resource <- {};"));
            runtime[39] = root[2]; runtime[40] = root[3];
            /* Original 452040 excludes layers with timeline extras. */
            if (query == 3) *(int32_t*)(intptr_t)(cloned_layer+196) = 1;
            CHECK(retdec_publish_act_layers(vm,PTR(copy),PTR(runtime),&active));
            CHECK(active == 1);
            if (query == 2) {
                CHECK(*(int32_t*)(intptr_t)(cloned_layer+52) == PTR(layout)+320);
                CHECK(*(int32_t*)(intptr_t)(cloned_layer+56) == PTR(layout)+328);
                CHECK(layout[79] == 0); /* Register does not force SetLayer. */
                CHECK(execute_source(vm,parent,
                    "if (mapProbe.layout == mapProbe.script.layout) throw \"shared wrapper\";"
                    "mapProbe.alpha -= 0.25; mapProbe.blend = 1;"
                    "if (mapProbe.layout.alpha != 0.75 || mapProbe.script.layout.alpha != 0.75) throw \"alpha alias\";"
                    "if (mapProbe.layout.blend != 1) throw \"blend alias\";"
                    "mapProbe.script.layout.alpha = 0.5;"
                    "if (mapProbe.alpha != 0.5) throw \"reverse alpha alias\";"));
                CHECK(((float*)map)[80] == 1.0f);
            } else {
                CHECK(*(int32_t*)(intptr_t)(cloned_layer+52) == 0);
                CHECK(*(int32_t*)(intptr_t)(cloned_layer+56) == 0);
                *(int32_t*)(intptr_t)(cloned_layer+196) = 0;
            }
            CHECK(execute_source(vm,root+2,"delete RegistrationProbe;"));
            kinoko_sqrat_release_pair((struct SQVM *)(intptr_t)(vm), parent);
        }
        if (query == 4) {
            int32_t scratch[12] = {0}, cached = 0, hits = 0;
            *(uint8_t*)(intptr_t)(cloned_layer+140) = 0;
            CHECK(fixture_collision_query_rect(PTR(scratch),PTR(layout),&cached,0,0,100,100,&hits));
            CHECK(layout[79] == cloned_resource && hits == 1);
            KinokoCollisionRecord *hit = (KinokoCollisionRecord*)(intptr_t)scratch[9];
            CHECK(hit[0].index == 0 && hit[0].chip == data->chips[0].bytes);
            /* R138: order, inclusive bounds, fractional coordinates and buffer
               cursor reuse. Source only: execution remains user-owned. */
            {
                int32_t scan_records[5][8] = {
                    {4,-20,8,0,0,0,1,0}, {4,0,8,0,0,0,1,0},
                    {4,20,8,0,0,0,1,0}, {4,40,24,0,0,0,1,0},
                    {4,60,8,0,0,0,1,0}};
                kinoko_native_buffer_replace(PTR(layout)+264,scan_records,sizeof(scan_records));
                *(float*)(intptr_t)(cloned_layer+144) = 0.5f;
                *(float*)(intptr_t)(cloned_layer+148) = -0.5f;
                cached = 2; hits = 0;
                CHECK(fixture_collision_query_rect(PTR(scratch),PTR(layout),&cached,16,24,40,24,&hits));
                hit = (KinokoCollisionRecord*)(intptr_t)scratch[9];
                CHECK(hits == 3 && cached == 1);
                CHECK(hit[0].index == 2 && hit[1].index == 3 && hit[2].index == 1);
                CHECK(hit[0].layout == (void*)(intptr_t)(layout[66]+2*32));
                CHECK(((const float*)hit[0].layout)[3] == 20.5f);
                CHECK(((const float*)hit[0].layout)[4] == 7.5f);
                cached = 4; hits = 0;
                CHECK(fixture_collision_query_rect(PTR(scratch),PTR(layout),&cached,16,24,40,24,&hits));
                hit = (KinokoCollisionRecord*)(intptr_t)scratch[9];
                CHECK(hits == 3 && cached == 1);
                CHECK(hit[0].index == 3 && hit[1].index == 2 && hit[2].index == 1);
                cached = 0; hits = 0;
                CHECK(fixture_collision_query_rect(PTR(scratch),PTR(layout),&cached,56,24,56,24,&hits));
                hit = (KinokoCollisionRecord*)(intptr_t)scratch[9];
                CHECK(hits == 1 && cached == 3 && hit[0].index == 3);
                CHECK(scratch[10] == PTR(hit+3)); /* Published end is retained. */
                cached = 0;
                CHECK(fixture_collision_query_rect(PTR(scratch),PTR(layout),&cached,0,24,0,24,&hits));
                hit = (KinokoCollisionRecord*)(intptr_t)scratch[9];
                CHECK(hits == 2 && hit[0].index == 3 && hit[1].index == 1);
                /* Existing internal empty-map contract leaves both cursors. */
                int32_t saved_end = layout[67];
                layout[67] = layout[66]; cached = 99;
                CHECK(fixture_collision_query_rect(PTR(scratch),PTR(layout),&cached,0,0,100,100,&hits));
                CHECK(hits == 2 && cached == 99);
                layout[67] = saved_end;
            }
            kinoko_native_buffer_destroy(PTR(scratch)+36);
            layout[79] = 0;
            *(int32_t*)(intptr_t)(cloned_layer+100) = 0;
            hits = 0;
            CHECK(!fixture_collision_query_rect(PTR(scratch),PTR(layout),&cached,0,0,100,100,&hits));
            CHECK(hits == 0 && layout[79] == 0);
            *(int32_t*)(intptr_t)(cloned_layer+100) = cloned_resource;
        }
        CHECK(map[79] == 0); /* No mutation of the source layout's cache. */
        retdec_destroy_cact_with_flags(PTR(copy),1);
    }
    retdec_destroy_cact_object(PTR(source));
    g664 = previous_default_vm;
    puts("PASS: virtual ACT clone lazy map binding and publication property aliases");
    return 0;
}
