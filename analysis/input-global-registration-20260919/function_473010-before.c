int32_t function_473010(void) {
    int32_t v1 = __readfsdword(0); // bp-16, 0x473020
    __writefsdword(0, (int32_t)&v1);
    retdec_trace("473010:enter");
    function_402aa0();
    int32_t v2 = (int32_t)g644; // 0x47303c
    g664 = v2;
    // IDA's stack frame is a 0x14-byte Sqrat::RootTable: Object vtable,
    // VM, SquirrelObject value, and the Object validity flag.  The vtable
    // changes to RootTable only after the value has been initialized.
    int32_t root_table[5]; // 0x47304e, recovered Sqrat::RootTable object
    root_table[0] = (int32_t)(intptr_t)&g39;
    root_table[1] = v2;
    root_table[4] = 1;
    sq_resetobject((HSQOBJECT*)kinoko_pointer((int32_t)(intptr_t)&root_table[2]));
    root_table[0] = (int32_t)(intptr_t)&g40;
    sq_pushroottable(kinoko_vm(v2));
    sq_getstackobj(kinoko_vm(v2), -1, (HSQOBJECT*)((int32_t *)(intptr_t)&root_table[2]));
    function_48a400(v2, (int32_t)(intptr_t)&root_table[2]);
    kinoko_sq_pop(v2, 1);
    int32_t v5; // bp-20, 0x473010
    int32_t v6 = &v5; // 0x4730a3
    v5 = (int32_t)(intptr_t)function_402af0;
    function_415550_this((int32_t)(intptr_t)root_table,
                         (int32_t)(intptr_t)"ShowCallStack", v6, 4,
                         (int32_t)(intptr_t)function_470ee0, 0);
    v5 = (int32_t)(intptr_t)function_471b30;
    function_415550_this((int32_t)(intptr_t)root_table,
                         (int32_t)(intptr_t)"CompileFile", v6, 4,
                         (int32_t)(intptr_t)retdec_compile_file_native, 0);
    int32_t v7 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v7));
    char * v8 = "PostQuitMessage"; // bp-116, 0x4730e9
    char * v9; // bp-120, 0x473010
    *(int32_t *)&v9 = v7;
    sq_pushstring(kinoko_vm(v7), (const SQChar*)kinoko_pointer((int32_t)"PostQuitMessage"), -1);
    char * v10 = (char *)4; // bp-124, 0x4730f4
    int32_t v11 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v7), 4)); // 0x4730f7
    char * v12 = (char *)1; // bp-132, 0x4730fc
    char * v13 = (char *)(intptr_t)function_471bc0; // bp-136, 0x4730fe
    char * v14; // bp-140, 0x473010
    *(int32_t *)&v14 = v7;
    *(int32_t *)v11 = (int32_t)(intptr_t)function_471080;
    sq_newclosure(kinoko_vm((int32_t)v14), (SQFUNCTION)kinoko_pointer((int32_t)v13), (int32_t)v12);
    char * v15 = NULL; // bp-144, 0x47310f
    char * v16; // bp-152, 0x473010
    *(int32_t *)&v16 = v7;
    sq_newslot(kinoko_vm(v7), -3, ((0) != 0));
    char * v17; // bp-156, 0x473010
    *(int32_t *)&v17 = v7;
    kinoko_sq_pop(v7, 1);
    int32_t v18 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v18));
    char * v19 = "ReadCSV"; // bp-168, 0x47312c
    char * v20; // bp-172, 0x473010
    *(int32_t *)&v20 = v18;
    sq_pushstring(kinoko_vm(v18), (const SQChar*)kinoko_pointer((int32_t)"ReadCSV"), -1);
    int32_t v21 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v18), 4)); // 0x47313d
    v8 = (char *)1;
    v9 = (char *)(intptr_t)function_471c10;
    *(int32_t *)&v10 = v18;
    *(int32_t *)v21 = (int32_t)(intptr_t)function_403000;
    sq_newclosure(kinoko_vm((int32_t)v10), (SQFUNCTION)kinoko_pointer((int32_t)v9), (int32_t)v8);
    v12 = (char *)-3;
    *(int32_t *)&v13 = v18;
    sq_newslot(kinoko_vm(v18), -3, ((0) != 0));
    *(int32_t *)&v14 = v18;
    kinoko_sq_pop(v18, 1);
    int32_t v22 = (int32_t)g644;
    *(int32_t *)&v15 = v22;
    sq_pushroottable(kinoko_vm(v22));
    v16 = "LoadTable";
    *(int32_t *)&v17 = v22;
    sq_pushstring(kinoko_vm(v22), (const SQChar*)kinoko_pointer((int32_t)"LoadTable"), -1);
    int32_t v23 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v22), 4)); // 0x473180
    v19 = (char *)1;
    v20 = (char *)(intptr_t)function_471c10;
    char * v24; // bp-176, 0x473010
    *(int32_t *)&v24 = v22;
    *(int32_t *)v23 = (int32_t)(intptr_t)function_472c90;
    sq_newclosure(kinoko_vm((int32_t)v24), (SQFUNCTION)kinoko_pointer((int32_t)v20), (int32_t)v19);
    *(int32_t *)&v8 = v22;
    sq_newslot(kinoko_vm(v22), -3, ((0) != 0));
    *(int32_t *)&v9 = v22;
    kinoko_sq_pop(v22, 1);
    int32_t v25 = (int32_t)g644;
    *(int32_t *)&v10 = v25;
    sq_pushroottable(kinoko_vm(v25));
    v12 = "SaveTable";
    *(int32_t *)&v13 = v25;
    sq_pushstring(kinoko_vm(v25), (const SQChar*)kinoko_pointer((int32_t)"SaveTable"), -1);
    v14 = (char *)4;
    *(int32_t *)&v15 = v25;
    int32_t v26 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v25), 4)); // 0x4731c6
    v16 = (char *)(intptr_t)function_471c10;
    *(int32_t *)&v17 = v25;
    *(int32_t *)v26 = (int32_t)(intptr_t)function_472e50;
    sq_newclosure(kinoko_vm((int32_t)v17), (SQFUNCTION)kinoko_pointer((int32_t)v16), 1);
    *(int32_t *)&v19 = v25;
    sq_newslot(kinoko_vm(v25), -3, ((0) != 0));
    kinoko_sq_pop(v25, 1);
    int32_t v27 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v27));
    v8 = (char *)-1;
    v9 = "SetGlobalUpdateFunction";
    *(int32_t *)&v10 = v27;
    sq_pushstring(kinoko_vm(v27), (const SQChar*)kinoko_pointer((int32_t)"SetGlobalUpdateFunction"), -1);
    *(int32_t *)&v12 = v27;
    int32_t v28 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v27), 4)); // 0x47320c
    v13 = (char *)1;
    v14 = (char *)(intptr_t)function_471c70;
    *(int32_t *)&v15 = v27;
    *(int32_t *)v28 = (int32_t)(intptr_t)function_469a20;
    sq_newclosure(kinoko_vm((int32_t)v15), (SQFUNCTION)kinoko_pointer((int32_t)v14), (int32_t)v13);
    v16 = (char *)-3;
    *(int32_t *)&v17 = v27;
    sq_newslot(kinoko_vm(v27), -3, ((0) != 0));
    kinoko_sq_pop(v27, 1);
    int32_t v29 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v29));
    v19 = (char *)-1;
    v20 = "SetInitFunctionByID";
    *(int32_t *)&v24 = v29;
    sq_pushstring(kinoko_vm(v29), (const SQChar*)kinoko_pointer((int32_t)"SetInitFunctionByID"), -1);
    int32_t v30 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v29), 4)); // 0x473252
    v8 = (char *)1;
    v9 = (char *)(intptr_t)function_471d30;
    *(int32_t *)&v10 = v29;
    *(int32_t *)v30 = (int32_t)(intptr_t)function_470fa0;
    sq_newclosure(kinoko_vm((int32_t)v10), (SQFUNCTION)kinoko_pointer((int32_t)v9), (int32_t)v8);
    v12 = (char *)-3;
    *(int32_t *)&v13 = v29;
    sq_newslot(kinoko_vm(v29), -3, ((0) != 0));
    *(int32_t *)&v14 = v29;
    kinoko_sq_pop(v29, 1);
    int32_t v31 = (int32_t)g644;
    *(int32_t *)&v15 = v31;
    sq_pushroottable(kinoko_vm(v31));
    v16 = "LoadAnimationData";
    *(int32_t *)&v17 = v31;
    sq_pushstring(kinoko_vm(v31), (const SQChar*)kinoko_pointer((int32_t)"LoadAnimationData"), -1);
    int32_t v32 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v31), 4)); // 0x473295
    v19 = (char *)1;
    v20 = (char *)(intptr_t)function_471d90;
    *(int32_t *)&v24 = v31;
    *(int32_t *)v32 = (int32_t)(intptr_t)function_4696b0;
    sq_newclosure(kinoko_vm((int32_t)v24), (SQFUNCTION)kinoko_pointer((int32_t)v20), (int32_t)v19);
    *(int32_t *)&v8 = v31;
    sq_newslot(kinoko_vm(v31), -3, ((0) != 0));
    *(int32_t *)&v9 = v31;
    kinoko_sq_pop(v31, 1);
    int32_t v33 = (int32_t)g644;
    *(int32_t *)&v10 = v33;
    sq_pushroottable(kinoko_vm(v33));
    v12 = "CreateActor";
    *(int32_t *)&v13 = v33;
    sq_pushstring(kinoko_vm(v33), (const SQChar*)kinoko_pointer((int32_t)"CreateActor"), -1);
    v14 = (char *)4;
    *(int32_t *)&v15 = v33;
    int32_t v34 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v33), 4)); // 0x4732db
    v16 = (char *)(intptr_t)function_471df0;
    *(int32_t *)&v17 = v33;
    *(int32_t *)v34 = (int32_t)(intptr_t)function_469b40;
    sq_newclosure(kinoko_vm((int32_t)v17), (SQFUNCTION)kinoko_pointer((int32_t)v16), 1);
    *(int32_t *)&v19 = v33;
    sq_newslot(kinoko_vm(v33), -3, ((0) != 0));
    kinoko_sq_pop(v33, 1);
    int32_t v35 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v35));
    v8 = (char *)-1;
    v9 = "CreateActorFromMap";
    *(int32_t *)&v10 = v35;
    sq_pushstring(kinoko_vm(v35), (const SQChar*)kinoko_pointer((int32_t)"CreateActorFromMap"), -1);
    *(int32_t *)&v12 = v35;
    int32_t v36 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v35), 4)); // 0x473321
    v13 = (char *)1;
    v14 = (char *)(intptr_t)function_471e50;
    *(int32_t *)&v15 = v35;
    *(int32_t *)v36 = (int32_t)(intptr_t)function_469d10;
    sq_newclosure(kinoko_vm((int32_t)v15), (SQFUNCTION)kinoko_pointer((int32_t)v14), (int32_t)v13);
    v16 = (char *)-3;
    *(int32_t *)&v17 = v35;
    sq_newslot(kinoko_vm(v35), -3, ((0) != 0));
    kinoko_sq_pop(v35, 1);
    int32_t v37 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v37));
    v19 = (char *)-1;
    v20 = "ClearActor";
    *(int32_t *)&v24 = v37;
    sq_pushstring(kinoko_vm(v37), (const SQChar*)kinoko_pointer((int32_t)"ClearActor"), -1);
    int32_t v38 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v37), 4)); // 0x473367
    v8 = (char *)1;
    v9 = (char *)(intptr_t)function_471bc0;
    *(int32_t *)&v10 = v37;
    *(int32_t *)v38 = (int32_t)(intptr_t)function_469700;
    sq_newclosure(kinoko_vm((int32_t)v10), (SQFUNCTION)kinoko_pointer((int32_t)v9), (int32_t)v8);
    v12 = (char *)-3;
    *(int32_t *)&v13 = v37;
    sq_newslot(kinoko_vm(v37), -3, ((0) != 0));
    *(int32_t *)&v14 = v37;
    kinoko_sq_pop(v37, 1);
    int32_t v39 = (int32_t)g644;
    *(int32_t *)&v15 = v39;
    sq_pushroottable(kinoko_vm(v39));
    v16 = "MoveActor";
    *(int32_t *)&v17 = v39;
    sq_pushstring(kinoko_vm(v39), (const SQChar*)kinoko_pointer((int32_t)"MoveActor"), -1);
    int32_t v40 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v39), 4)); // 0x4733aa
    v19 = (char *)1;
    v20 = (char *)(intptr_t)function_471eb0;
    *(int32_t *)&v24 = v39;
    *(int32_t *)v40 = (int32_t)(intptr_t)function_469710;
    sq_newclosure(kinoko_vm((int32_t)v24), (SQFUNCTION)kinoko_pointer((int32_t)v20), (int32_t)v19);
    *(int32_t *)&v8 = v39;
    sq_newslot(kinoko_vm(v39), -3, ((0) != 0));
    *(int32_t *)&v9 = v39;
    kinoko_sq_pop(v39, 1);
    int32_t v41 = (int32_t)g644;
    *(int32_t *)&v10 = v41;
    sq_pushroottable(kinoko_vm(v41));
    v12 = "ClearCollision";
    *(int32_t *)&v13 = v41;
    sq_pushstring(kinoko_vm(v41), (const SQChar*)kinoko_pointer((int32_t)"ClearCollision"), -1);
    v14 = (char *)4;
    *(int32_t *)&v15 = v41;
    int32_t v42 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v41), 4)); // 0x4733f0
    v16 = (char *)(intptr_t)function_471bc0;
    *(int32_t *)&v17 = v41;
    *(int32_t *)v42 = (int32_t)(intptr_t)function_469870;
    sq_newclosure(kinoko_vm((int32_t)v17), (SQFUNCTION)kinoko_pointer((int32_t)v16), 1);
    *(int32_t *)&v19 = v41;
    sq_newslot(kinoko_vm(v41), -3, ((0) != 0));
    kinoko_sq_pop(v41, 1);
    int32_t v43 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v43));
    v8 = (char *)-1;
    v9 = "CreateCollision";
    *(int32_t *)&v10 = v43;
    sq_pushstring(kinoko_vm(v43), (const SQChar*)kinoko_pointer((int32_t)"CreateCollision"), -1);
    *(int32_t *)&v12 = v43;
    int32_t v44 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v43), 4)); // 0x473436
    v13 = (char *)1;
    *(int32_t *)v44 = (int32_t)(intptr_t)function_469880;
    v14 = (char *)(intptr_t)function_471f10;
    *(int32_t *)&v15 = v43;
    sq_newclosure(kinoko_vm(v43), (SQFUNCTION)kinoko_pointer((int32_t)(intptr_t)function_471f10), (int32_t)v13);
    v16 = (char *)-3;
    *(int32_t *)&v17 = v43;
    sq_newslot(kinoko_vm(v43), -3, ((0) != 0));
    kinoko_sq_pop(v43, 1);
    int32_t v45 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v45));
    v19 = (char *)-1;
    v20 = "CreateEvent";
    *(int32_t *)&v24 = v45;
    sq_pushstring(kinoko_vm(v45), (const SQChar*)kinoko_pointer((int32_t)"CreateEvent"), -1);
    int32_t v46 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v45), 4)); // 0x47347c
    v8 = (char *)1;
    v9 = (char *)(intptr_t)function_471f70;
    *(int32_t *)&v10 = v45;
    *(int32_t *)v46 = (int32_t)(intptr_t)function_469dd0;
    sq_newclosure(kinoko_vm((int32_t)v10), (SQFUNCTION)kinoko_pointer((int32_t)v9), (int32_t)v8);
    v12 = (char *)-3;
    *(int32_t *)&v13 = v45;
    sq_newslot(kinoko_vm(v45), -3, ((0) != 0));
    *(int32_t *)&v14 = v45;
    kinoko_sq_pop(v45, 1);
    int32_t v47 = (int32_t)g644;
    *(int32_t *)&v15 = v47;
    sq_pushroottable(kinoko_vm(v47));
    v16 = "ClearRenderLayer";
    *(int32_t *)&v17 = v47;
    sq_pushstring(kinoko_vm(v47), (const SQChar*)kinoko_pointer((int32_t)"ClearRenderLayer"), -1);
    int32_t v48 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v47), 4)); // 0x4734bf
    v19 = (char *)1;
    v20 = (char *)(intptr_t)function_471bc0;
    *(int32_t *)&v24 = v47;
    *(int32_t *)v48 = (int32_t)(intptr_t)function_46a1d0;
    sq_newclosure(kinoko_vm((int32_t)v24), (SQFUNCTION)kinoko_pointer((int32_t)v20), (int32_t)v19);
    *(int32_t *)&v8 = v47;
    sq_newslot(kinoko_vm(v47), -3, ((0) != 0));
    *(int32_t *)&v9 = v47;
    kinoko_sq_pop(v47, 1);
    int32_t v49 = (int32_t)g644;
    *(int32_t *)&v10 = v49;
    sq_pushroottable(kinoko_vm(v49));
    v12 = "CreateRenderLayer";
    *(int32_t *)&v13 = v49;
    sq_pushstring(kinoko_vm(v49), (const SQChar*)kinoko_pointer((int32_t)"CreateRenderLayer"), -1);
    v14 = (char *)4;
    *(int32_t *)&v15 = v49;
    int32_t v50 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v49), 4)); // 0x473505
    v16 = (char *)(intptr_t)function_471f10;
    *(int32_t *)&v17 = v49;
    *(int32_t *)v50 = (int32_t)(intptr_t)retdec_create_render_layer_fixed;
    sq_newclosure(kinoko_vm((int32_t)v17), (SQFUNCTION)kinoko_pointer((int32_t)v16), 1);
    *(int32_t *)&v19 = v49;
    sq_newslot(kinoko_vm(v49), -3, ((0) != 0));
    kinoko_sq_pop(v49, 1);
    int32_t v51 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v51));
    v8 = (char *)-1;
    v9 = "LoadAct";
    *(int32_t *)&v10 = v51;
    sq_pushstring(kinoko_vm(v51), (const SQChar*)kinoko_pointer((int32_t)"LoadAct"), -1);
    *(int32_t *)&v12 = v51;
    int32_t v52 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v51), 4)); // 0x47354b
    v13 = (char *)1;
    v14 = (char *)(intptr_t)function_471d90;
    *(int32_t *)&v15 = v51;
    *(int32_t *)v52 = (int32_t)(intptr_t)function_469820;
    sq_newclosure(kinoko_vm((int32_t)v15), (SQFUNCTION)kinoko_pointer((int32_t)v14), (int32_t)v13);
    v16 = (char *)-3;
    *(int32_t *)&v17 = v51;
    sq_newslot(kinoko_vm(v51), -3, ((0) != 0));
    kinoko_sq_pop(v51, 1);
    int32_t v53 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v53));
    v19 = (char *)-1;
    v20 = "LoadMap";
    *(int32_t *)&v24 = v53;
    sq_pushstring(kinoko_vm(v53), (const SQChar*)kinoko_pointer((int32_t)"LoadMap"), -1);
    int32_t v54 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v53), 4)); // 0x473591
    v8 = (char *)1;
    v9 = (char *)(intptr_t)function_471d90;
    *(int32_t *)&v10 = v53;
    *(int32_t *)v54 = (int32_t)(intptr_t)function_469840;
    sq_newclosure(kinoko_vm((int32_t)v10), (SQFUNCTION)kinoko_pointer((int32_t)v9), (int32_t)v8);
    v12 = (char *)-3;
    *(int32_t *)&v13 = v53;
    sq_newslot(kinoko_vm(v53), -3, ((0) != 0));
    *(int32_t *)&v14 = v53;
    kinoko_sq_pop(v53, 1);
    int32_t v55 = (int32_t)g644;
    *(int32_t *)&v15 = v55;
    sq_pushroottable(kinoko_vm(v55));
    v16 = "LoadSE";
    *(int32_t *)&v17 = v55;
    sq_pushstring(kinoko_vm(v55), (const SQChar*)kinoko_pointer((int32_t)"LoadSE"), -1);
    int32_t v56 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v55), 4)); // 0x4735d4
    v19 = (char *)1;
    v20 = (char *)(intptr_t)function_471f10;
    *(int32_t *)&v24 = v55;
    *(int32_t *)v56 = (int32_t)(intptr_t)function_470ab0;
    sq_newclosure(kinoko_vm((int32_t)v24), (SQFUNCTION)kinoko_pointer((int32_t)v20), (int32_t)v19);
    *(int32_t *)&v8 = v55;
    sq_newslot(kinoko_vm(v55), -3, ((0) != 0));
    *(int32_t *)&v9 = v55;
    kinoko_sq_pop(v55, 1);
    int32_t v57 = (int32_t)g644;
    *(int32_t *)&v10 = v57;
    sq_pushroottable(kinoko_vm(v57));
    v12 = "ReleaseMap";
    *(int32_t *)&v13 = v57;
    sq_pushstring(kinoko_vm(v57), (const SQChar*)kinoko_pointer((int32_t)"ReleaseMap"), -1);
    v14 = (char *)4;
    *(int32_t *)&v15 = v57;
    int32_t v58 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v57), 4)); // 0x47361a
    v16 = (char *)(intptr_t)function_471bc0;
    *(int32_t *)&v17 = v57;
    *(int32_t *)v58 = (int32_t)(intptr_t)function_469860;
    sq_newclosure(kinoko_vm((int32_t)v17), (SQFUNCTION)kinoko_pointer((int32_t)v16), 1);
    *(int32_t *)&v19 = v57;
    sq_newslot(kinoko_vm(v57), -3, ((0) != 0));
    kinoko_sq_pop(v57, 1);
    int32_t v59 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v59));
    v8 = (char *)-1;
    v9 = "MessageBox";
    *(int32_t *)&v10 = v59;
    sq_pushstring(kinoko_vm(v59), (const SQChar*)kinoko_pointer((int32_t)"MessageBox"), -1);
    *(int32_t *)&v12 = v59;
    int32_t v60 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v59), 4)); // 0x473660
    v13 = (char *)1;
    v14 = (char *)(intptr_t)function_471f10;
    *(int32_t *)&v15 = v59;
    *(int32_t *)v60 = (int32_t)(intptr_t)function_470f60;
    sq_newclosure(kinoko_vm((int32_t)v15), (SQFUNCTION)kinoko_pointer((int32_t)v14), (int32_t)v13);
    v16 = (char *)-3;
    *(int32_t *)&v17 = v59;
    sq_newslot(kinoko_vm(v59), -3, ((0) != 0));
    kinoko_sq_pop(v59, 1);
    int32_t v61 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v61));
    v19 = (char *)-1;
    v20 = "dprint";
    *(int32_t *)&v24 = v61;
    sq_pushstring(kinoko_vm(v61), (const SQChar*)kinoko_pointer((int32_t)"dprint"), -1);
    int32_t v62 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v61), 4)); // 0x4736a6
    v8 = (char *)1;
    v9 = (char *)(intptr_t)function_471f10;
    *(int32_t *)&v10 = v61;
    *(int32_t *)v62 = (int32_t)(intptr_t)function_43e100;
    sq_newclosure(kinoko_vm((int32_t)v10), (SQFUNCTION)kinoko_pointer((int32_t)v9), (int32_t)v8);
    v12 = (char *)-3;
    *(int32_t *)&v13 = v61;
    sq_newslot(kinoko_vm(v61), -3, ((0) != 0));
    *(int32_t *)&v14 = v61;
    kinoko_sq_pop(v61, 1);
    int32_t v63 = (int32_t)g644;
    *(int32_t *)&v15 = v63;
    sq_pushroottable(kinoko_vm(v63));
    v16 = "Sleep";
    *(int32_t *)&v17 = v63;
    sq_pushstring(kinoko_vm(v63), (const SQChar*)kinoko_pointer((int32_t)"Sleep"), -1);
    int32_t v64 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v63), 4)); // 0x4736e9
    v19 = (char *)1;
    v20 = (char *)(intptr_t)function_471fd0;
    *(int32_t *)&v24 = v63;
    *(int32_t *)v64 = (int32_t)(intptr_t)function_470f80;
    sq_newclosure(kinoko_vm((int32_t)v24), (SQFUNCTION)kinoko_pointer((int32_t)v20), (int32_t)v19);
    *(int32_t *)&v8 = v63;
    sq_newslot(kinoko_vm(v63), -3, ((0) != 0));
    *(int32_t *)&v9 = v63;
    kinoko_sq_pop(v63, 1);
    int32_t v65 = (int32_t)g644;
    *(int32_t *)&v10 = v65;
    sq_pushroottable(kinoko_vm(v65));
    v12 = "timeGetTime";
    *(int32_t *)&v13 = v65;
    sq_pushstring(kinoko_vm(v65), (const SQChar*)kinoko_pointer((int32_t)"timeGetTime"), -1);
    v14 = (char *)4;
    *(int32_t *)&v15 = v65;
    int32_t v66 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v65), 4)); // 0x47372f
    v16 = (char *)(intptr_t)function_472030;
    *(int32_t *)&v17 = v65;
    *(int32_t *)v66 = (int32_t)(intptr_t)function_470f90;
    sq_newclosure(kinoko_vm((int32_t)v17), (SQFUNCTION)kinoko_pointer((int32_t)v16), 1);
    *(int32_t *)&v19 = v65;
    sq_newslot(kinoko_vm(v65), -3, ((0) != 0));
    kinoko_sq_pop(v65, 1);
    int32_t v67 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v67));
    v8 = (char *)-1;
    v9 = "PlaySE";
    *(int32_t *)&v10 = v67;
    sq_pushstring(kinoko_vm(v67), (const SQChar*)kinoko_pointer((int32_t)"PlaySE"), -1);
    *(int32_t *)&v12 = v67;
    int32_t v68 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v67), 4)); // 0x473775
    v13 = (char *)1;
    v14 = (char *)(intptr_t)function_471fd0;
    *(int32_t *)&v15 = v67;
    *(int32_t *)v68 = (int32_t)(intptr_t)function_470980;
    sq_newclosure(kinoko_vm((int32_t)v15), (SQFUNCTION)kinoko_pointer((int32_t)v14), (int32_t)v13);
    v16 = (char *)-3;
    *(int32_t *)&v17 = v67;
    sq_newslot(kinoko_vm(v67), -3, ((0) != 0));
    kinoko_sq_pop(v67, 1);
    int32_t v69 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v69));
    v19 = (char *)-1;
    v20 = "PlayBgm";
    *(int32_t *)&v24 = v69;
    sq_pushstring(kinoko_vm(v69), (const SQChar*)kinoko_pointer((int32_t)"PlayBgm"), -1);
    int32_t v70 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v69), 4)); // 0x4737bb
    v8 = (char *)1;
    v9 = (char *)(intptr_t)function_472080;
    *(int32_t *)&v10 = v69;
    *(int32_t *)v70 = (int32_t)(intptr_t)function_470220;
    sq_newclosure(kinoko_vm((int32_t)v10), (SQFUNCTION)kinoko_pointer((int32_t)v9), (int32_t)v8);
    v12 = (char *)-3;
    *(int32_t *)&v13 = v69;
    sq_newslot(kinoko_vm(v69), -3, ((0) != 0));
    *(int32_t *)&v14 = v69;
    kinoko_sq_pop(v69, 1);
    int32_t v71 = (int32_t)g644;
    *(int32_t *)&v15 = v71;
    sq_pushroottable(kinoko_vm(v71));
    v16 = "PlayBgmMargin";
    *(int32_t *)&v17 = v71;
    sq_pushstring(kinoko_vm(v71), (const SQChar*)kinoko_pointer((int32_t)"PlayBgmMargin"), -1);
    int32_t v72 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v71), 4)); // 0x4737fe
    v19 = (char *)1;
    v20 = (char *)(intptr_t)function_4720e0;
    *(int32_t *)&v24 = v71;
    *(int32_t *)v72 = (int32_t)(intptr_t)function_470290;
    sq_newclosure(kinoko_vm((int32_t)v24), (SQFUNCTION)kinoko_pointer((int32_t)v20), (int32_t)v19);
    *(int32_t *)&v8 = v71;
    sq_newslot(kinoko_vm(v71), -3, ((0) != 0));
    *(int32_t *)&v9 = v71;
    kinoko_sq_pop(v71, 1);
    int32_t v73 = (int32_t)g644;
    *(int32_t *)&v10 = v73;
    sq_pushroottable(kinoko_vm(v73));
    v12 = "FadeBgm";
    *(int32_t *)&v13 = v73;
    sq_pushstring(kinoko_vm(v73), (const SQChar*)kinoko_pointer((int32_t)"FadeBgm"), -1);
    v14 = (char *)4;
    *(int32_t *)&v15 = v73;
    *(int32_t *)(int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v73), 4)) = (int32_t)(intptr_t)function_470320;
    v16 = (char *)(intptr_t)function_472140;
    *(int32_t *)&v17 = v73;
    sq_newclosure(kinoko_vm(v73), (SQFUNCTION)kinoko_pointer((int32_t)(intptr_t)function_472140), 1);
    *(int32_t *)&v19 = v73;
    sq_newslot(kinoko_vm(v73), -3, ((0) != 0));
    kinoko_sq_pop(v73, 1);
    int32_t v74 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v74));
    v8 = (char *)-1;
    v9 = "StopBgm";
    *(int32_t *)&v10 = v74;
    sq_pushstring(kinoko_vm(v74), (const SQChar*)kinoko_pointer((int32_t)"StopBgm"), -1);
    *(int32_t *)&v12 = v74;
    int32_t v75 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v74), 4)); // 0x47388a
    v13 = (char *)1;
    v14 = (char *)(intptr_t)function_471bc0;
    *(int32_t *)&v15 = v74;
    *(int32_t *)v75 = (int32_t)(intptr_t)function_470360;
    sq_newclosure(kinoko_vm((int32_t)v15), (SQFUNCTION)kinoko_pointer((int32_t)v14), (int32_t)v13);
    v16 = (char *)-3;
    *(int32_t *)&v17 = v74;
    sq_newslot(kinoko_vm(v74), -3, ((0) != 0));
    kinoko_sq_pop(v74, 1);
    int32_t v76 = (int32_t)g644;
    sq_pushroottable(kinoko_vm(v76));
    v19 = (char *)-1;
    v20 = "PauseBgm";
    sq_pushstring(kinoko_vm(v76), (const SQChar*)kinoko_pointer((int32_t)"PauseBgm"), -1);
    int32_t v77 = (int32_t)(intptr_t)(sq_newuserdata(kinoko_vm(v76), 4)); // 0x4738d0
    v8 = (char *)1;
    v9 = (char *)(intptr_t)function_471bc0;
    *(int32_t *)&v10 = v76;
    *(int32_t *)v77 = (int32_t)(intptr_t)function_470300;
    sq_newclosure(kinoko_vm((int32_t)v10), (SQFUNCTION)kinoko_pointer((int32_t)v9), (int32_t)v8);
    v12 = (char *)-3;
    *(int32_t *)&v13 = v76;
    sq_newslot(kinoko_vm(v76), -3, ((0) != 0));
    *(int32_t *)&v14 = v76;
    kinoko_sq_pop(v76, 1);
    int32_t v78[3]; // Complete 12-byte SquirrelObject temporary.
    function_4a9500_this(&v78, function_4a8cc0());
    v8 = "updateMask";
    v9 = (char *)&g459;
    v10 = (char *)&v78;
    {
        int32_t update_bind_result =
            function_4721a0(&v78, &g459, "updateMask", 0);
        retdec_trace_i32("473010:update-bind-result", update_bind_result);
        retdec_trace_i32("473010:update-storage", g459);
        retdec_trace_i32("473010:update-object-type",
                         *(int32_t *)((unsigned char *)&v78 + 4));
        retdec_trace_i32("473010:update-object-data",
                         *(int32_t *)((unsigned char *)&v78 + 8));
        retdec_trace_i32("473010:update-present",
                         function_4aa1a0((int32_t)(intptr_t)&v78,
                                         "updateMask"));
    }
    function_4a9d70_this((int32_t)(intptr_t)&v78);
    function_4a9500_this(&v78, function_4a8cc0());
    v8 = NULL;
    v9 = "renderMask";
    v10 = (char *)&g460;
    {
        int32_t render_bind_result =
            function_4721a0(&v78, &g460, "renderMask", 0);
        retdec_trace_i32("473010:render-bind-result", render_bind_result);
        retdec_trace_i32("473010:render-storage", g460);
        retdec_trace_i32("473010:render-object-type",
                         *(int32_t *)((unsigned char *)&v78 + 4));
        retdec_trace_i32("473010:render-object-data",
                         *(int32_t *)((unsigned char *)&v78 + 8));
        retdec_trace_i32("473010:render-present",
                         function_4aa1a0((int32_t)(intptr_t)&v78,
                                         "renderMask"));
    }
    function_4a9d70_this((int32_t)(intptr_t)&v78);
    int32_t v79 = function_4a8cc0(); // 0x473962
    v8 = (char *)v79;
    function_4a9500_this(&v78, v79);
    v9 = "GP_CAMERA";
    v10 = (char *)0x20000000;
    function_472240(&v78, 0x20000000, "GP_CAMERA");
    function_4a9d70_this((int32_t)(intptr_t)&v78);
    int32_t v80 = function_4a8cc0(); // 0x473995
    v9 = (char *)v80;
    function_4a9500_this(&v78, v80);
    v10 = "GP_ACT";
    v12 = (char *)&v78;
    function_472240(&v78, 0x40000000, "GP_ACT");
    function_4a9d70_this((int32_t)(intptr_t)&v78);
    int32_t v81 = function_4a8cc0(); // 0x4739c8
    v10 = (char *)v81;
    function_4a9500_this(&v78, v81);
    v12 = (char *)-0x80000000;
    v13 = (char *)&v78;
    function_472240(&v78, -0x80000000, "GP_BACKGROUND");
    function_4a9d70_this((int32_t)(intptr_t)&v78);
    function_4a9500_this(&v78, function_4a8cc0());
    v12 = "PR_BACK";
    v13 = (char *)-1;
    v14 = (char *)&v78;
    function_472240(&v78, -1, "PR_BACK");
    function_4a9d70_this((int32_t)(intptr_t)&v78);
    int32_t v82 = function_4a8cc0(); // 0x473a2b
    v12 = (char *)v82;
    function_4a9500_this(&v78, v82);
    v13 = "PR_FRONT";
    v14 = (char *)0xffff;
    v15 = (char *)&v78;
    function_472240(&v78, 0xffff, "PR_FRONT");
    function_4a9d70_this((int32_t)(intptr_t)&v78);
    int32_t v83 = function_4a8cc0(); // 0x473a5e
    v13 = (char *)v83;
    function_4a9500_this(&v78, v83);
    v14 = "PR_WATER";
    v15 = (char *)0x10000;
    function_472240(&v78, 0x10000, "PR_WATER");
    function_4a9d70_this((int32_t)(intptr_t)&v78);
    function_460e00();
    function_46d950();
    function_4669d0();
    function_46fac0();
    int32_t v84 = function_402d30(); // 0x473aa5
    v14 = (char *)v84;
    v15 = "data/script/class_def.nut";
    function_402d40("data/script/class_def.nut", v84);
    int32_t v85 = (int32_t)g644;
    v14 = (char *)&root_table[2];
    *(int32_t *)&v15 = v85;
    int32_t result = function_48a430(
        v85, (int32_t)(intptr_t)&root_table[2]); // 0x473acc
    __writefsdword(0, v1);
    return result;
}