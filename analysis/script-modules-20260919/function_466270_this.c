static int32_t function_466270_this(int32_t this_ptr) {
    int32_t temporary[3] = { 0, 0, 0 };
    int32_t result;

    if (this_ptr == 0)
        return 0;
    memset((void *)(intptr_t)this_ptr, 0, sizeof(g_retdec_camera_state));
    result = function_4a90c0(temporary, g611);
    function_4a95c0_this(this_ptr, result);
    function_4a9d70_this((int32_t)(intptr_t)temporary);
    function_4a9bb0_this(this_ptr, this_ptr);
    result = function_4a9840_this((int32_t)(intptr_t)&g722,
                                  "camera", this_ptr);
    *(float32_t *)(intptr_t)(this_ptr + 44) = 0.0f;
    *(float32_t *)(intptr_t)(this_ptr + 40) = 0.0f;
    *(float32_t *)(intptr_t)(this_ptr + 52) = 0.0f;
    *(float32_t *)(intptr_t)(this_ptr + 48) = 0.0f;
    *(float32_t *)(intptr_t)(this_ptr + 84) = 0.0f;
    *(float32_t *)(intptr_t)(this_ptr + 80) = 0.0f;
    *(float32_t *)(intptr_t)(this_ptr + 76) = 0.0f;
    *(float32_t *)(intptr_t)(this_ptr + 72) = 0.0f;
    *(float32_t *)(intptr_t)(this_ptr + 56) = 0.0f;
    *(float32_t *)(intptr_t)(this_ptr + 60) = 0.0f;
    retdec_trace_i32("actor:camera-init-left",
                     *(int32_t *)(intptr_t)(this_ptr + 72));
    retdec_trace_i32("actor:camera-init-right",
                     *(int32_t *)(intptr_t)(this_ptr + 80));
    return result;
}