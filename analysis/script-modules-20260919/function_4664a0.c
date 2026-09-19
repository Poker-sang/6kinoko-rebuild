int32_t function_4664a0(int32_t a1) {
    // 0x4664a0
    function_4a95c0(a1);
    int32_t result; // 0x4664a0
    *(int32_t *)(result + 12) = *(int32_t *)(a1 + 12);
    function_4a95c0(a1 + 16);
    function_4a95c0(a1 + 28);
    *(int32_t *)(result + 40) = *(int32_t *)(a1 + 40);
    *(int32_t *)(result + 44) = *(int32_t *)(a1 + 44);
    *(int32_t *)(result + 48) = *(int32_t *)(a1 + 48);
    *(int32_t *)(result + 52) = *(int32_t *)(a1 + 52);
    *(int32_t *)(result + 56) = *(int32_t *)(a1 + 56);
    *(int32_t *)(result + 60) = *(int32_t *)(a1 + 60);
    *(int32_t *)(result + 64) = *(int32_t *)(a1 + 64);
    *(int32_t *)(result + 68) = *(int32_t *)(a1 + 68);
    int32_t * v1 = (int32_t *)(a1 + 72);
    int32_t * v2 = (int32_t *)(result + 72);
    *v2 = *v1;
    int32_t * v3 = (int32_t *)(a1 + 76);
    int32_t * v4 = (int32_t *)(result + 76);
    *v4 = *v3;
    int32_t * v5 = (int32_t *)(a1 + 80);
    int32_t * v6 = (int32_t *)(result + 80);
    *v6 = *v5;
    int32_t * v7 = (int32_t *)(a1 + 84);
    int32_t * v8 = (int32_t *)(result + 84);
    *v8 = *v7;
    *v2 = *v1;
    *v4 = *v3;
    *v6 = *v5;
    *v8 = *v7;
    return result;
}