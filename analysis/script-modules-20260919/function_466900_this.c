static int32_t function_466900_this(int32_t this_ptr, const char *name,
                                    int32_t parent_ptr) {
    int32_t *state;
    int32_t temporary[3] = { 0, 0, 0 };

    if (this_ptr == 0)
        return 0;
    state = (int32_t *)(intptr_t)this_ptr;
    state[0] = (int32_t)(intptr_t)g644;
    state[1] = (int32_t)(intptr_t)name;
    function_4a94e0_this(this_ptr + 8);
    state[5] = parent_ptr;
    function_4a91c0_this(state + 6);
    function_4a91c0_this(state + 9);

    function_466770(temporary, state[0], state[1], state[5]);
    function_4a95c0_this(this_ptr + 8,
                         (int32_t)(intptr_t)temporary);
    function_4a9d70_this((int32_t)(intptr_t)temporary);
    return this_ptr;
}