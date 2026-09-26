/* R140 source-only regression; execution belongs to the user. */
static int test_collision_lifecycle(void) {
    int32_t state[32] = {0}, manager[40] = {0}, replacement[40] = {0};
    int32_t proxy[136] = {0}, a[136] = {0}, b[136] = {0}, ignored[136] = {0};
    int32_t layout[116] = {0}, layer[40] = {0};
    int32_t *slot = (int32_t*)malloc(sizeof(int32_t)), control = 0;
    int32_t actors[4] = {PTR(ignored), PTR(a), 0, PTR(b)};
    int32_t layouts[1] = {PTR(layout)}, pair[2];
    CHECK(slot);
    *slot = PTR(proxy);
    CHECK(kinoko_native_control_create(PTR(&control), PTR(slot)) && control);
    pair[0] = PTR(slot); pair[1] = control;
    kinoko_native_add_weak(control); /* map owns one weak */
    kinoko_native_add_weak(control); /* test witness survives expiry and reset */
    kinoko_native_buffer_replace(PTR(state)+4, layouts, sizeof(layouts));
    kinoko_native_buffer_replace(PTR(state)+52, pair, sizeof(pair));
    state[0] = PTR(manager);
    manager[25] = PTR(actors); manager[29] = 4;
    a[78] = 2; b[78] = 4;
    layout[78] = PTR(layer);
    ((float*)layer)[36] = 15.25f; ((float*)layer)[37] = -6.5f;
    ((float*)proxy)[60] = 3.0f; ((float*)proxy)[61] = 8.0f;
    CHECK(kinoko_collision_refresh_abi(PTR(state)) == PTR(b));
    CHECK(((float*)proxy)[62] == 3.0f && ((float*)proxy)[63] == 8.0f);
    CHECK(((float*)proxy)[60] == 15.25f && ((float*)proxy)[61] == -6.5f);
    CHECK(state[21] == 2);
    CHECK(((int32_t*)(intptr_t)state[17])[0] == PTR(a));
    CHECK(((int32_t*)(intptr_t)state[17])[1] == PTR(b));
    CHECK(*(int32_t*)(intptr_t)(control+4) == 1);
    int32_t capacity_end = state[18], candidate_owner = state[19];
    manager[29] = 1;
    CHECK(kinoko_collision_refresh_abi(PTR(state)) == PTR(manager));
    CHECK(state[21] == 0 && state[18] == capacity_end && state[19] == candidate_owner);
    manager[29] = 0;
    CHECK(kinoko_collision_refresh_abi(PTR(state)) == PTR(manager));
    /* Losing the strong owner destroys its slot. Never follow the stale layout. */
    kinoko_native_release_strong(control);
    CHECK(*(int32_t*)(intptr_t)(control+4) == 0);
    *(int32_t*)(intptr_t)state[1] = 1;
    CHECK(kinoko_collision_refresh_abi(PTR(state)) == PTR(manager));
    int32_t weak_before = *(int32_t*)(intptr_t)(control+8);
    state[6] = 101; state[10] = 202; /* reset must preserve scratch ends */
    CHECK(!kinoko_collision_reset_abi(PTR(state), 0));
    CHECK(state[2] != state[1] && state[14] != state[13]);
    CHECK(kinoko_collision_reset_abi(PTR(state), PTR(replacement)));
    CHECK(state[0] == PTR(replacement) && state[21] == 0);
    CHECK(state[2] == state[1] && state[14] == state[13]);
    CHECK(state[6] == 101 && state[10] == 202 && state[18] == capacity_end);
    CHECK(*(int32_t*)(intptr_t)(control+8) == weak_before-1);
    CHECK(kinoko_collision_reset_abi(PTR(state), PTR(replacement)));
    CHECK(*(int32_t*)(intptr_t)(control+8) == weak_before-1);
    state[0] = 0; state[21] = 7;
    CHECK(kinoko_collision_refresh_abi(PTR(state)) == 0 && state[21] == 0);
    CHECK(kinoko_collision_refresh_abi(0) == 0 && !kinoko_collision_reset_abi(0, PTR(manager)));
    kinoko_native_release_weak(control);
    kinoko_native_buffer_destroy(PTR(state)+4);
    kinoko_native_buffer_destroy(PTR(state)+52);
    kinoko_native_buffer_destroy(PTR(state)+68);
    return 0;
}
