int32_t function_46d950(void) {
    int32_t v1 = __readfsdword(0); // bp-16, 0x46d960
    __writefsdword(0, (int32_t)&v1);
    int32_t root_object[3] = {0, 0, 0};
    int32_t input_class[12] = {0};
    int32_t *field_object = input_class + 2;
    int32_t vm;

    function_4a9500_this(root_object, function_4a8cc0());
    function_46cfb0_this((int32_t)(intptr_t)input_class, "Input", 0);
    vm = input_class[0];
    sq_pushobject(kinoko_vm(vm), kinoko_borrowed_object(input_class[3], input_class[4]));
    sq_pushstring(kinoko_vm(vm), (const SQChar*)kinoko_pointer((int32_t)"Save"), -1);
    *(int32_t *)(int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(vm), 4)) =
        (int32_t)(intptr_t)function_46b7c0;
    sq_newclosure(kinoko_vm(vm), (SQFUNCTION)kinoko_pointer((int32_t)(intptr_t)function_46ce70), 1);
    sq_newslot(kinoko_vm(vm), -3, ((0) != 0));
    kinoko_sq_pop(vm, 1);
    sq_pushobject(kinoko_vm(vm), kinoko_borrowed_object(input_class[3], input_class[4]));
    sq_pushstring(kinoko_vm(vm), (const SQChar*)kinoko_pointer((int32_t)"Load"), -1);
    *(int32_t *)(int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(vm), 4)) =
        (int32_t)(intptr_t)function_46b880;
    sq_newclosure(kinoko_vm(vm), (SQFUNCTION)kinoko_pointer((int32_t)(intptr_t)function_46ce70), 1);
    sq_newslot(kinoko_vm(vm), -3, ((0) != 0));
    kinoko_sq_pop(vm, 1);
    sq_pushobject(kinoko_vm(vm), kinoko_borrowed_object(input_class[3], input_class[4]));
    sq_pushstring(kinoko_vm(vm), (const SQChar*)kinoko_pointer((int32_t)"SetAssign"), -1);
    *(int32_t *)(int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(vm), 4)) =
        (int32_t)(intptr_t)function_46bbe0;
    sq_newclosure(kinoko_vm(vm), (SQFUNCTION)kinoko_pointer((int32_t)(intptr_t)function_46cec0), 1);
    sq_newslot(kinoko_vm(vm), -3, ((0) != 0));
    kinoko_sq_pop(vm, 1);
    sq_pushobject(kinoko_vm(vm), kinoko_borrowed_object(input_class[3], input_class[4]));
    sq_pushstring(kinoko_vm(vm), (const SQChar*)kinoko_pointer((int32_t)"WaitAssign"), -1);
    *(int32_t *)(int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(vm), 4)) =
        (int32_t)(intptr_t)function_46bc90;
    sq_newclosure(kinoko_vm(vm), (SQFUNCTION)kinoko_pointer((int32_t)(intptr_t)function_46cf10), 1);
    sq_newslot(kinoko_vm(vm), -3, ((0) != 0));
    kinoko_sq_pop(vm, 1);
    sq_pushobject(kinoko_vm(vm), kinoko_borrowed_object(input_class[3], input_class[4]));
    sq_pushstring(kinoko_vm(vm), (const SQChar*)kinoko_pointer((int32_t)"GetAssign"), -1);
    *(int32_t *)(int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(vm), 4)) =
        (int32_t)(intptr_t)function_46be40;
    sq_newclosure(kinoko_vm(vm), (SQFUNCTION)kinoko_pointer((int32_t)(intptr_t)function_46cf60), 1);
    sq_newslot(kinoko_vm(vm), -3, ((0) != 0));
    kinoko_sq_pop(vm, 1);
    char v3 = g628; // 0x46db12
    if ((v3 & 1) == 0) {
        // 0x46db20
        g628 = v3 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46db45
    function_460920(field_object, &g623, 1436, "x", 0);
    char v5 = g628; // 0x46db61
    if ((v5 & 1) == 0) {
        // 0x46db6a
        g628 = v5 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46db8f
    function_460920(field_object, &g623, 1440, "y", 0);
    char v6 = g628; // 0x46dbab
    if ((v6 & 1) == 0) {
        // 0x46dbb4
        g628 = v6 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46dbd9
    function_460a60(field_object, &g623, 1468, "br0", 0);
    char v7 = g628; // 0x46dbf5
    if ((v7 & 1) == 0) {
        // 0x46dbfe
        g628 = v7 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46dc23
    function_460a60(field_object, &g623, 1469, "br1", 0);
    char v8 = g628; // 0x46dc3f
    if ((v8 & 1) == 0) {
        // 0x46dc48
        g628 = v8 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46dc6d
    function_460a60(field_object, &g623, 1470, "br2", 0);
    char v9 = g628; // 0x46dc89
    if ((v9 & 1) == 0) {
        // 0x46dc92
        g628 = v9 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46dcb7
    function_460a60(field_object, &g623, 1471, "br3", 0);
    char v10 = g628; // 0x46dcd3
    if ((v10 & 1) == 0) {
        // 0x46dcdc
        g628 = v10 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46dd01
    function_460920(field_object, &g623, 1444, "b0", 0);
    char v11 = g628; // 0x46dd1d
    if ((v11 & 1) == 0) {
        // 0x46dd26
        g628 = v11 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46dd4b
    function_460920(field_object, &g623, 1448, "b1", 0);
    char v12 = g628; // 0x46dd67
    if ((v12 & 1) == 0) {
        // 0x46dd70
        g628 = v12 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46dd95
    function_460920(field_object, &g623, 1452, "b2", 0);
    char v13 = g628; // 0x46ddb1
    if ((v13 & 1) == 0) {
        // 0x46ddba
        g628 = v13 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46dddf
    function_460920(field_object, &g623, 1456, "b3", 0);
    char v14 = g628; // 0x46ddfb
    if ((v14 & 1) == 0) {
        // 0x46de04
        g628 = v14 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46de29
    function_460920(field_object, &g623, 1460, "b4", 0);
    char v15 = g628; // 0x46de45
    if ((v15 & 1) == 0) {
        // 0x46de4e
        g628 = v15 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46de73
    function_460a60(field_object, &g623, 1468, "kr0", 0);
    char v16 = g628; // 0x46de8f
    if ((v16 & 1) == 0) {
        // 0x46de98
        g628 = v16 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46debd
    function_460a60(field_object, &g623, 1469, "kr1", 0);
    char v17 = g628; // 0x46ded9
    if ((v17 & 1) == 0) {
        // 0x46dee2
        g628 = v17 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46df07
    function_460a60(field_object, &g623, 1470, "kr2", 0);
    char v18 = g628; // 0x46df23
    if ((v18 & 1) == 0) {
        // 0x46df2c
        g628 = v18 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46df51
    function_460a60(field_object, &g623, 1471, "kr3", 0);
    char v19 = g628; // 0x46df6d
    if ((v19 & 1) == 0) {
        // 0x46df76
        g628 = v19 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46df9b
    function_460920(field_object, &g623, 1444, "k0", 0);
    char v20 = g628; // 0x46dfb7
    if ((v20 & 1) == 0) {
        // 0x46dfc0
        g628 = v20 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46dfe5
    function_460920(field_object, &g623, 1448, "k1", 0);
    char v21 = g628; // 0x46e001
    if ((v21 & 1) == 0) {
        // 0x46e00a
        g628 = v21 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e02f
    function_460920(field_object, &g623, 1452, "k2", 0);
    char v22 = g628; // 0x46e04b
    if ((v22 & 1) == 0) {
        // 0x46e054
        g628 = v22 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e079
    function_460920(field_object, &g623, 1456, "k3", 0);
    char v23 = g628; // 0x46e095
    if ((v23 & 1) == 0) {
        // 0x46e09e
        g628 = v23 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e0c3
    function_460920(field_object, &g623, 1460, "k4", 0);
    char v24 = g628; // 0x46e0df
    if ((v24 & 1) == 0) {
        // 0x46e0e8
        g628 = v24 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e10d
    function_460920(field_object, &g623, 1464, "k5", 0);
    char v25 = g628; // 0x46e129
    if ((v25 & 1) == 0) {
        // 0x46e132
        g628 = v25 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e157
    function_460920(field_object, &g623, 1476, "s1", 0);
    char v26 = g628; // 0x46e173
    if ((v26 & 1) == 0) {
        // 0x46e17c
        g628 = v26 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e1a1
    function_460920(field_object, &g623, 1480, "s2", 0);
    char v27 = g628; // 0x46e1bd
    if ((v27 & 1) == 0) {
        // 0x46e1c6
        g628 = v27 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e1eb
    function_460920(field_object, &g623, 1484, "s3", 0);
    char v28 = g628; // 0x46e207
    if ((v28 & 1) == 0) {
        // 0x46e210
        g628 = v28 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e235
    function_460920(field_object, &g623, 1488, "s4", 0);
    char v29 = g628; // 0x46e251
    if ((v29 & 1) == 0) {
        // 0x46e25a
        g628 = v29 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e27f
    function_460920(field_object, &g623, 1492, "s5", 0);
    char v30 = g628; // 0x46e29b
    if ((v30 & 1) == 0) {
        // 0x46e2a4
        g628 = v30 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e2c9
    function_460920(field_object, &g623, 1496, "s6", 0);
    char v31 = g628; // 0x46e2e5
    if ((v31 & 1) == 0) {
        // 0x46e2ee
        g628 = v31 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e313
    function_460920(field_object, &g623, 1500, "s7", 0);
    char v32 = g628; // 0x46e32f
    if ((v32 & 1) == 0) {
        // 0x46e338
        g628 = v32 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e35d
    function_460920(field_object, &g623, 1504, "s8", 0);
    char v33 = g628; // 0x46e379
    if ((v33 & 1) == 0) {
        // 0x46e382
        g628 = v33 | 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e3a7
    function_460920(field_object, &g623, 1508, "s9", 0);
    if ((g628 & 1) == 0) {
        // 0x46e3cc
        g628 |= 1;
        g624 = 0;
        g626 = 0;
        g627 = -1;
        g623 = (int32_t)&g36;
        g625 = 0;
    }
    // 0x46e3f1
    function_460920(field_object, &g623, 1472, "s0", 0);
    int32_t v34[3] = {0, 0, 0}; // bp-40, 0x46d950
    int32_t *v35 = function_4aa3a0_this(
        (int32_t)(intptr_t)root_object, (int32_t)(intptr_t)v34, "Input");
    function_4a95c0_this((int32_t)(intptr_t)&g629,
                         (int32_t)(intptr_t)v35);
    function_4a9d70_this((int32_t)(intptr_t)v34);
    function_4a9d70_this((int32_t)(intptr_t)(input_class + 9));
    function_4a9d70_this((int32_t)(intptr_t)(input_class + 6));
    function_4a9d70_this((int32_t)(intptr_t)(input_class + 2));
    int32_t result = function_4a9d70_this(
        (int32_t)(intptr_t)root_object); // 0x46e462
    __writefsdword(0, v1);
    return result;
}