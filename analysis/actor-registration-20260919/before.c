int32_t function_460e00(void) {
    int32_t v1 = __readfsdword(0); // bp-16, 0x460e10
    __writefsdword(0, (int32_t)&v1);

    int32_t v2[3] = {0, 0, 0};
    function_460d10_actor(v2, "Actor", 0);
    function_4a95c0_this((int32_t)(intptr_t)&g602,
                         (int32_t)(intptr_t)v2);
    int32_t v3 = (int32_t)(intptr_t)g644;
    function_460e00_register_actor_method(v3, v2, "Release",
                                          (int32_t)(intptr_t)&kinoko_actor_release,
                                          (int32_t)(intptr_t)&function_460b00,
                                          0);
    function_460e00_register_actor_method(v3, v2, "Reset",
                                          (int32_t)(intptr_t)&kinoko_method_actor_destroy_state,
                                          (int32_t)(intptr_t)&function_460b00,
                                          0);
    function_460e00_register_actor_method(v3, v2, "SetUpdateFunction",
                                          (int32_t)(intptr_t)&kinoko_actor_set_update_callback,
                                          (int32_t)(intptr_t)&function_460b50,
                                          0);
    function_460e00_register_actor_method(
        v3, v2, "SetCollisionCallbackFunction",
        (int32_t)(intptr_t)&kinoko_actor_set_collision_callback,
        (int32_t)(intptr_t)&function_460b50, 0);
    function_460e00_register_actor_method(
        v3, v2, "InterrputCollisionCallback",
        (int32_t)(intptr_t)&kinoko_actor_interrupt_collision,
        (int32_t)(intptr_t)&function_460b00, 0);
    function_460e00_register_actor_method(v3, v2, "SetTake",
                                          (int32_t)(intptr_t)&kinoko_actor_set_take_method,
                                          (int32_t)(intptr_t)&function_460bc0,
                                          0);
    function_460e00_register_actor_method(v3, v2, "SetStep",
                                          (int32_t)(intptr_t)&function_4606d0,
                                          (int32_t)(intptr_t)&function_460b50,
                                          0);
    function_460e00_register_actor_method(v3, v2, "SetChipFlag",
                                          (int32_t)(intptr_t)&kinoko_actor_set_chip_flags,
                                          (int32_t)(intptr_t)&function_460bc0,
                                          0);
    function_460e00_register_actor_method(v3, v2, "SetChipBoundType",
                                          (int32_t)(intptr_t)&kinoko_actor_set_chip_bound_type,
                                          (int32_t)(intptr_t)&function_460bc0,
                                          0);
    function_460e00_register_actor_method(v3, v2, "GetChipID",
                                          (int32_t)(intptr_t)&kinoko_actor_get_chip_id,
                                          (int32_t)(intptr_t)&function_460bc0,
                                          0);
    function_460e00_register_actor_method(v3, v2, "GetChipFlag",
                                          (int32_t)(intptr_t)&kinoko_actor_get_chip_flags,
                                          (int32_t)(intptr_t)&function_460c10,
                                          0);
    function_460e00_register_actor_method(v3, v2, "IsExistChip",
                                          (int32_t)(intptr_t)&kinoko_actor_has_chip,
                                          (int32_t)(intptr_t)&function_460c70,
                                          0);
    function_460e00_register_actor_method(v3, v2, "ResetPriority",
                                          (int32_t)(intptr_t)&kinoko_actor_reset_priority_method,
                                          (int32_t)(intptr_t)&function_460bc0,
                                          0);
    function_460e00_register_actor_method(v3, v2, "SyncAnimation",
                                          (int32_t)(intptr_t)&kinoko_actor_sync_animation,
                                          (int32_t)(intptr_t)&function_460b50,
                                          0);
    function_460e00_register_actor_method(v3, v2, "Move",
                                          (int32_t)(intptr_t)&kinoko_method_actor_move,
                                          (int32_t)(intptr_t)&function_460cc0,
                                          0);

    char v21 = g581; // 0x461302
    if ((v21 & 1) == 0) {
        // 0x461310
        g581 = v21 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x46133d
    function_460920(&v2, &g576, 220, "timeTotal", 0);
    char v22 = g581; // 0x461359
    if ((v22 & 1) == 0) {
        // 0x461362
        g581 = v22 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461387
    function_4609c0(&v2, &g576, 156, "px", 0);
    char v23 = g581; // 0x4613a3
    if ((v23 & 1) == 0) {
        // 0x4613ac
        g581 = v23 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x4613d1
    function_4609c0(&v2, &g576, 160, "py", 0);
    char v24 = g581; // 0x4613ed
    if ((v24 & 1) == 0) {
        // 0x4613f6
        g581 = v24 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x46141b
    function_4609c0(&v2, &g576, 172, "sx", 0);
    char v25 = g581; // 0x461437
    if ((v25 & 1) == 0) {
        // 0x461440
        g581 = v25 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461465
    function_4609c0(&v2, &g576, 176, "sy", 0);
    char v26 = g581; // 0x461481
    if ((v26 & 1) == 0) {
        // 0x46148a
        g581 = v26 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x4614af
    function_4609c0(&v2, &g576, 164, "rotate", 0);
    char v27 = g581; // 0x4614cb
    if ((v27 & 1) == 0) {
        // 0x4614d4
        g581 = v27 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x4614f9
    function_4609c0(&v2, &g576, 168, "scale", 0);
    char v28 = g581; // 0x461515
    if ((v28 & 1) == 0) {
        // 0x46151e
        g581 = v28 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461543
    function_460920(&v2, &g576, 180, "a", 0);
    char v29 = g581; // 0x46155f
    if ((v29 & 1) == 0) {
        // 0x461568
        g581 = v29 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x46158d
    function_460920(&v2, &g576, 184, "r", 0);
    char v30 = g581; // 0x4615a9
    if ((v30 & 1) == 0) {
        // 0x4615b2
        g581 = v30 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x4615d7
    function_460920(&v2, &g576, 188, "g", 0);
    char v31 = g581; // 0x4615f3
    if ((v31 & 1) == 0) {
        // 0x4615fc
        g581 = v31 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461621
    function_460920(&v2, &g576, 192, "b", 0);
    char v32 = g581; // 0x46163d
    if ((v32 & 1) == 0) {
        // 0x461646
        g581 = v32 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x46166b
    function_460920(&v2, &g576, 196, "blend", 0);
    char v33 = g581; // 0x461687
    if ((v33 & 1) == 0) {
        // 0x461690
        g581 = v33 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x4616b5
    function_460920(&v2, &g576, 208, "take", 1);
    char v34 = g581; // 0x4616d2
    if ((v34 & 1) == 0) {
        // 0x4616db
        g581 = v34 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461700
    function_460920(&v2, &g576, 224, "id", 0);
    char v35 = g581; // 0x46171c
    if ((v35 & 1) == 0) {
        // 0x461725
        g581 = v35 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x46174a
    function_460920(&v2, &g576, 228, "priority", 0);
    char v36 = g581; // 0x461766
    if ((v36 & 1) == 0) {
        // 0x46176f
        g581 = v36 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461794
    function_460920(&v2, &g576, 232, "updateGroup", 0);
    char v37 = g581; // 0x4617b0
    if ((v37 & 1) == 0) {
        // 0x4617b9
        g581 = v37 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x4617de
    function_460920(&v2, &g576, 236, "flag", 0);
    char v38 = g581; // 0x4617fa
    if ((v38 & 1) == 0) {
        // 0x461803
        g581 = v38 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461828
    function_460a60(&v2, &g576, 40, "isActive", 0);
    char v39 = g581; // 0x461841
    if ((v39 & 1) == 0) {
        // 0x46184a
        g581 = v39 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x46186f
    function_460a60(&v2, &g576, 40, "isStatic", 0);
    char v40 = g581; // 0x461888
    if ((v40 & 1) == 0) {
        // 0x461891
        g581 = v40 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x4618b6
    function_460a60(&v2, &g576, 21, "isVisible", 0);
    char v41 = g581; // 0x4618cf
    if ((v41 & 1) == 0) {
        // 0x4618d8
        g581 = v41 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x4618fd
    function_4609c0(&v2, &g576, 80, "ox", 0);
    char v42 = g581; // 0x461916
    if ((v42 & 1) == 0) {
        // 0x46191f
        g581 = v42 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461944
    function_4609c0(&v2, &g576, 84, "oy", 0);
    char v43 = g581; // 0x46195d
    if ((v43 & 1) == 0) {
        // 0x461966
        g581 = v43 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x46198b
    function_4609c0(&v2, &g576, 240, "x", 0);
    char v44 = g581; // 0x4619a7
    if ((v44 & 1) == 0) {
        // 0x4619b0
        g581 = v44 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x4619d5
    function_4609c0(&v2, &g576, 244, "y", 0);
    char v45 = g581; // 0x4619f1
    if ((v45 & 1) == 0) {
        // 0x4619fa
        g581 = v45 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461a1f
    function_4609c0(&v2, &g576, 304, "freeWidth", 0);
    char v46 = g581; // 0x461a3b
    if ((v46 & 1) == 0) {
        // 0x461a44
        g581 = v46 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461a69
    function_4609c0(&v2, &g576, 308, "freeHeight", 0);
    char v47 = g581; // 0x461a85
    if ((v47 & 1) == 0) {
        // 0x461a8e
        g581 = v47 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461ab3
    function_4609c0(&v2, &g576, 272, "direction", 0);
    char v48 = g581; // 0x461acf
    if ((v48 & 1) == 0) {
        // 0x461ad8
        g581 = v48 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461afd
    function_4609c0(&v2, &g576, 276, "pitch", 0);
    char v49 = g581; // 0x461b19
    if ((v49 & 1) == 0) {
        // 0x461b22
        g581 = v49 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461b47
    function_4609c0(&v2, &g576, 280, "pitchTop", 0);
    char v50 = g581; // 0x461b63
    if ((v50 & 1) == 0) {
        // 0x461b6c
        g581 = v50 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461b91
    function_4609c0(&v2, &g576, 256, "vx", 0);
    char v51 = g581; // 0x461bad
    if ((v51 & 1) == 0) {
        // 0x461bb6
        g581 = v51 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461bdb
    function_4609c0(&v2, &g576, 260, "vy", 0);
    char v52 = g581; // 0x461bf7
    if ((v52 & 1) == 0) {
        // 0x461c00
        g581 = v52 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461c25
    function_460920(&v2, &g576, 472, "collisionFlag", 0);
    char v53 = g581; // 0x461c41
    if ((v53 & 1) == 0) {
        // 0x461c4a
        g581 = v53 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461c6f
    function_460920(&v2, &g576, 284, "hitLeft", 0);
    char v54 = g581; // 0x461c8b
    if ((v54 & 1) == 0) {
        // 0x461c94
        g581 = v54 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461cb9
    function_460920(&v2, &g576, 292, "hitRight", 0);
    char v55 = g581; // 0x461cd5
    if ((v55 & 1) == 0) {
        // 0x461cde
        g581 = v55 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461d03
    function_460920(&v2, &g576, 288, "hitTop", 0);
    char v56 = g581; // 0x461d1f
    if ((v56 & 1) == 0) {
        // 0x461d28
        g581 = v56 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461d4d
    function_460920(&v2, &g576, 296, "hitBottom", 0);
    char v57 = g581; // 0x461d69
    if ((v57 & 1) == 0) {
        // 0x461d72
        g581 = v57 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461d97
    function_4609c0(&v2, &g576, 440, "left", 0);
    char v58 = g581; // 0x461db3
    if ((v58 & 1) == 0) {
        // 0x461dbc
        g581 = v58 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461de1
    function_4609c0(&v2, &g576, 444, "top", 0);
    char v59 = g581; // 0x461dfd
    if ((v59 & 1) == 0) {
        // 0x461e06
        g581 = v59 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461e2b
    function_4609c0(&v2, &g576, 448, "right", 0);
    char v60 = g581; // 0x461e47
    if ((v60 & 1) == 0) {
        // 0x461e50
        g581 = v60 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461e75
    function_4609c0(&v2, &g576, 452, "bottom", 0);
    char v61 = g581; // 0x461e91
    if ((v61 & 1) == 0) {
        // 0x461e9a
        g581 = v61 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461ebf
    function_4609c0(&v2, &g576, 248, "xPrev", 0);
    char v62 = g581; // 0x461edb
    if ((v62 & 1) == 0) {
        // 0x461ee4
        g581 = v62 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461f09
    function_4609c0(&v2, &g576, 252, "yPrev", 0);
    char v63 = g581; // 0x461f25
    if ((v63 & 1) == 0) {
        // 0x461f2e
        g581 = v63 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461f53
    function_4609c0(&v2, &g576, 456, "leftPrev", 0);
    char v64 = g581; // 0x461f6f
    if ((v64 & 1) == 0) {
        // 0x461f78
        g581 = v64 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461f9d
    function_4609c0(&v2, &g576, 460, "topPrev", 0);
    char v65 = g581; // 0x461fb9
    if ((v65 & 1) == 0) {
        // 0x461fc2
        g581 = v65 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x461fe7
    function_4609c0(&v2, &g576, 464, "rightPrev", 0);
    char v66 = g581; // 0x462003
    if ((v66 & 1) == 0) {
        // 0x46200c
        g581 = v66 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x462031
    function_4609c0(&v2, &g576, 468, "bottomPrev", 0);
    char v67 = g581; // 0x46204d
    if ((v67 & 1) == 0) {
        // 0x462056
        g581 = v67 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x46207b
    function_460920(&v2, &g576, 312, "collisionGroup", 0);
    char v68 = g581; // 0x462097
    if ((v68 & 1) == 0) {
        // 0x4620a0
        g581 = v68 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x4620c5
    function_460920(&v2, &g576, 316, "collisionMask", 0);
    char v69 = g581; // 0x4620e1
    if ((v69 & 1) == 0) {
        // 0x4620ea
        g581 = v69 | 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x46210f
    function_460920(&v2, &g576, 320, "callbackGroup", 0);
    if ((g581 & 1) == 0) {
        // 0x462134
        g581 |= 1;
        g577 = 0;
        g579 = 0;
        g580 = -1;
        g576 = (int32_t)&g18;
        g578 = 0;
    }
    // 0x462159
    function_460920(&v2, &g576, 324, "callbackMask", 0);
    /* 462175..462178 constructs the null SquirrelObject used for both
       class defaults. Zero-filled storage has type 0, not OT_NULL. */
    int32_t v72[3] = {0, 0, 0};
    function_4a94e0_this((int32_t)(intptr_t)v72);
    int32_t v70[3] = {0, 0, 0}; // temporary SquirrelObject
    int32_t v71 = (int32_t)(intptr_t)v70;
    function_4a95c0_this((int32_t)(intptr_t)&g601,
                         function_4a9250(v71, (int32_t)"step"));
    function_4a9d70_this(v71);
    function_4a95c0_this((int32_t)(intptr_t)&g600,
                         function_4a9250(v71, (int32_t)"user"));
    function_4a9d70_this(v71);
    int32_t v73 = (int32_t)(intptr_t)v72;
    function_4a97b0_this((int32_t)(intptr_t)&g602,
                         (int32_t)(intptr_t)&g601, v73);
    function_4a97b0_this((int32_t)(intptr_t)&g602,
                         (int32_t)(intptr_t)&g600, v73);
    function_4a9d70_this(v73);
    /* function_460d10_actor already released its three internal temporaries. */
    int32_t result = function_4a9d70_this((int32_t)(intptr_t)v2);
    __writefsdword(0, v1);
    return result;
}