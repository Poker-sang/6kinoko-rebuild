static int32_t function_46cfb0_this(int32_t this_ptr, const char *name,
                                    int32_t parent_ptr) {
    int32_t *state = (int32_t *)(intptr_t)this_ptr;
    int32_t temporary[3] = {0, 0, 0};
    int32_t nested[3] = {0, 0, 0};
    int32_t stack_base;

    if (this_ptr == 0)
        return 0;

    state[0] = (int32_t)(intptr_t)g644;
    state[1] = (int32_t)(intptr_t)name;
    function_4a94e0_this(this_ptr + 8);
    state[5] = parent_ptr;
    function_4a91c0_this(state + 6);
    function_4a91c0_this(state + 9);

    stack_base = sq_gettop(kinoko_vm(state[0]));
    function_4a94e0_this((int32_t)(intptr_t)temporary);
    if ((g628 & 1) == 0) {
        g628 |= 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)(intptr_t)&g36;
        g625 = 0;
    }
    if (function_4aa540(state[0], (int32_t)(intptr_t)temporary, &g623,
                        state[1], state[5]) != 0) {
        function_4a94e0_this((int32_t)(intptr_t)nested);
        sq_pushobject(kinoko_vm((int32_t)g644), kinoko_borrowed_object(*(int32_t *)(intptr_t)(temporary + 1), *(int32_t *)(intptr_t)(temporary + 2)));
        function_4a9660_this((int32_t)(intptr_t)nested, -1);
        kinoko_sq_pop((int32_t)g644, 1);
        function_45f640(nested);
    }
    sq_settop(kinoko_vm(state[0]), (SQInteger)(stack_base));
    function_4a95c0_this(this_ptr + 8, (int32_t)(intptr_t)temporary);
    function_4a9d70_this((int32_t)(intptr_t)temporary);
    return this_ptr;
}