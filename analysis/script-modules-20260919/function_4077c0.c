int32_t __fastcall function_4077c0(int32_t this_ptr) {
    int32_t v1 = 0; // bp-608, 0x4077d1
    int32_t v2 = this_ptr;
    int32_t * v3 = (int32_t *)(v2 + 72); // 0x4077d7
    int32_t * v4 = _memset(v3, 0, 96); // 0x4077d7
    int32_t * v5 = (int32_t *)(v2 + 184); // 0x4077df
    if (*v5 == 0) {
        // 0x4082ab
        return (int32_t)v4;
    }
    char * v6 = (char *)(v2 + 128);
    char * v7 = (char *)(v2 + 192);
    int32_t * v8 = (int32_t *)(v2 + 76);
    char * v9 = (char *)(v2 + 129);
    float32_t * v10 = (float32_t *)(v2 + 144);
    float32_t * v11 = (float32_t *)(v2 + 148);
    float32_t * v12 = (float32_t *)(v2 + 152);
    float32_t * v13 = (float32_t *)(v2 + 156);
    float32_t * v14 = (float32_t *)(v2 + 160);
    float32_t * v15 = (float32_t *)(v2 + 164);
    int32_t v16 = &v1;
    int3_t v17; // 0x4077c0
    int3_t v18 = v17;
    int32_t v19 = 0;
    *(int32_t *)(v16 - 4) = v19;
    int32_t v20; // bp-296, 0x4077c0
    *(int32_t *)(v16 - 8) = (int32_t)&v20;
    int32_t v21 = v16 - 12; // 0x40780c
    int32_t v22; // bp-56, 0x4077c0
    *(int32_t *)v21 = (int32_t)&v22;
    function_408530(v19);
    int32_t v23 = function_408550((int32_t)&g1224, (int32_t)&g1224); // 0x407816
    int32_t v24 = *(int32_t *)v23; // 0x40781b
    int32_t v25 = 0; // 0x40781f
    if (v24 != 0) {
        // 0x407825
        v25 = *(int32_t *)v24;
    }
    uint32_t v26 = *(int32_t *)(v23 + 8); // 0x407827
    uint32_t v27 = *(int32_t *)(v25 + 8); // 0x40782a
    uint32_t v28 = v26 / 4; // 0x40782f
    int32_t v29 = *v3; // 0x40783e
    int32_t v30 = *(int32_t *)(v25 + 4); // 0x407841
    int32_t v31 = *(int32_t *)(4 * (v28 - (v28 < v27 ? 0 : v27)) + v30); // 0x407844
    int32_t v32 = *(int32_t *)(*(int32_t *)(v31 + (4 * v26 & 12)) + 72); // 0x40784a
    int32_t v33; // 0x4077c0
    int32_t v34; // bp-104, 0x4077c0
    int32_t v35; // bp-128, 0x4077c0
    int32_t v36; // bp-344, 0x4077c0
    int32_t v37; // bp-464, 0x4077c0
    int32_t v38; // bp-536, 0x4077c0
    int32_t v39; // bp-80, 0x4077c0
    int32_t v40; // 0x40792b
    int32_t v41; // 0x4078e0
    uint32_t v42; // 0x4078e2
    uint32_t v43; // 0x4078e5
    uint32_t v44; // 0x4078ea
    int32_t v45; // 0x4078f6
    int32_t v46; // 0x4078f9
    int32_t v47; // 0x407935
    uint32_t v48; // 0x407937
    uint32_t v49; // 0x40793a
    uint32_t v50; // 0x40793f
    int32_t v51; // 0x40794b
    int32_t v52; // 0x40794e
    int32_t v53; // 0x40787b
    int32_t v54; // 0x407880
    int32_t v55; // 0x40788a
    uint32_t v56; // 0x40788c
    uint32_t v57; // 0x40788f
    uint32_t v58; // 0x407894
    int32_t v59; // 0x4078a0
    int32_t v60; // 0x4078a3
    int32_t v61; // 0x4078c0
    int32_t v62; // 0x4078d1
    int32_t v63; // 0x4078d6
    int32_t v64; // 0x40791c
    int32_t v65; // 0x407926
    if ((v29 < 0 ? -v29 : v29) < (v32 < 0 ? -v32 : v32)) {
        // 0x407863
        *(int32_t *)(v16 - 16) = v19;
        *(int32_t *)(v16 - 20) = (int32_t)&v37;
        *(int32_t *)(v16 - 24) = (int32_t)&v39;
        function_408530((int32_t)&g1224);
        v53 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v54 = *(int32_t *)v53;
        v55 = 0;
        if (v54 != 0) {
            // 0x40788a
            v55 = *(int32_t *)v54;
        }
        // 0x40788c
        v56 = *(int32_t *)(v53 + 8);
        v57 = *(int32_t *)(v55 + 8);
        v58 = v56 / 4;
        v59 = *(int32_t *)(v55 + 4);
        v60 = *(int32_t *)(4 * (v58 - (v58 < v57 ? 0 : v57)) + v59);
        *(int32_t *)(v16 - 28) = v19;
        *v3 = *(int32_t *)(*(int32_t *)(v60 + (4 * v56 & 12)) + 72);
        *(int32_t *)(v16 - 32) = (int32_t)&v36;
        v61 = v16 - 36;
        *(int32_t *)v61 = (int32_t)&v34;
        *v6 = 0;
        function_408530((int32_t)&g1224);
        v62 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v63 = *(int32_t *)v62;
        v41 = 0;
        if (v63 != 0) {
            // 0x4078e0
            v41 = *(int32_t *)v63;
        }
        // 0x4078e2
        v42 = *(int32_t *)(v62 + 8);
        v43 = *(int32_t *)(v41 + 8);
        v44 = v42 / 4;
        v45 = *(int32_t *)(v41 + 4);
        v46 = *(int32_t *)(4 * (v44 - (v44 < v43 ? 0 : v43)) + v45);
        *v7 = *(char *)(*(int32_t *)(v46 + (4 * v42 & 12)) + 4);
        v33 = v61;
    } else {
        // 0x40790a
        v33 = v21;
        if (v29 == 0) {
            // 0x40790e
            *(int32_t *)(v16 - 16) = v19;
            *(int32_t *)(v16 - 20) = (int32_t)&v38;
            v64 = v16 - 24;
            *(int32_t *)v64 = (int32_t)&v35;
            function_408530((int32_t)&g1224);
            v65 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v40 = *(int32_t *)v65;
            v47 = 0;
            if (v40 != 0) {
                // 0x407935
                v47 = *(int32_t *)v40;
            }
            // 0x407937
            v48 = *(int32_t *)(v65 + 8);
            v49 = *(int32_t *)(v47 + 8);
            v50 = v48 / 4;
            v51 = *(int32_t *)(v47 + 4);
            v52 = *(int32_t *)(4 * (v50 - (v50 < v49 ? 0 : v49)) + v51);
            v33 = v64;
            if (*(char *)(*(int32_t *)(v52 + (4 * v48 & 12)) + 128) != 0) {
                // 0x40795d
                *v6 = 1;
                v33 = v64;
            }
        }
    }
    int32_t v66 = v33;
    *(int32_t *)(v66 - 4) = v19;
    int32_t v67; // bp-368, 0x4077c0
    *(int32_t *)(v66 - 8) = (int32_t)&v67;
    int32_t v68 = v66 - 12; // 0x407978
    int32_t v69; // bp-152, 0x4077c0
    *(int32_t *)v68 = (int32_t)&v69;
    function_408530((int32_t)&g1224);
    int32_t v70 = function_408550((int32_t)&g1224, (int32_t)&g1224); // 0x407982
    int32_t v71 = *(int32_t *)v70; // 0x407987
    int32_t v72 = 0; // 0x40798b
    if (v71 != 0) {
        // 0x407991
        v72 = *(int32_t *)v71;
    }
    uint32_t v73 = *(int32_t *)(v70 + 8); // 0x407993
    uint32_t v74 = *(int32_t *)(v72 + 8); // 0x407996
    uint32_t v75 = v73 / 4; // 0x40799b
    int32_t v76 = *v8; // 0x4079aa
    int32_t v77 = *(int32_t *)(v72 + 4); // 0x4079ad
    int32_t v78 = *(int32_t *)(4 * (v75 - (v75 < v74 ? 0 : v74)) + v77); // 0x4079b0
    int32_t v79 = *(int32_t *)(*(int32_t *)(v78 + (4 * v73 & 12)) + 76); // 0x4079b6
    int32_t v80; // 0x4077c0
    int32_t v81; // bp-176, 0x4077c0
    int32_t v82; // bp-200, 0x4077c0
    int32_t v83; // bp-224, 0x4077c0
    int32_t v84; // bp-392, 0x4077c0
    int32_t v85; // bp-488, 0x4077c0
    int32_t v86; // bp-584, 0x4077c0
    int32_t v87; // 0x4079ea
    int32_t v88; // 0x4079ef
    int32_t v89; // 0x4079f9
    uint32_t v90; // 0x4079fb
    uint32_t v91; // 0x4079fe
    uint32_t v92; // 0x407a03
    int32_t v93; // 0x407a0f
    int32_t v94; // 0x407a12
    int32_t v95; // 0x407a32
    int32_t v96; // 0x407a43
    int32_t v97; // 0x407a48
    int32_t v98; // 0x407a91
    int32_t v99; // 0x407a9b
    int32_t v100; // 0x407aa0
    int32_t v101; // 0x407a52
    uint32_t v102; // 0x407a54
    uint32_t v103; // 0x407a57
    uint32_t v104; // 0x407a5c
    int32_t v105; // 0x407a68
    int32_t v106; // 0x407a6b
    int32_t v107; // 0x407aaa
    uint32_t v108; // 0x407aac
    uint32_t v109; // 0x407aaf
    uint32_t v110; // 0x407ab4
    int32_t v111; // 0x407ac0
    int32_t v112; // 0x407ac3
    if ((v76 < 0 ? -v76 : v76) < (v79 < 0 ? -v79 : v79)) {
        // 0x4079cf
        *(int32_t *)(v66 - 16) = v19;
        *(int32_t *)(v66 - 20) = (int32_t)&v85;
        *(int32_t *)(v66 - 24) = (int32_t)&v81;
        function_408530((int32_t)&g1224);
        v87 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v88 = *(int32_t *)v87;
        v89 = 0;
        if (v88 != 0) {
            // 0x4079f9
            v89 = *(int32_t *)v88;
        }
        // 0x4079fb
        v90 = *(int32_t *)(v87 + 8);
        v91 = *(int32_t *)(v89 + 8);
        v92 = v90 / 4;
        v93 = *(int32_t *)(v89 + 4);
        v94 = *(int32_t *)(4 * (v92 - (v92 < v91 ? 0 : v91)) + v93);
        *(int32_t *)(v66 - 28) = v19;
        *v8 = *(int32_t *)(*(int32_t *)(v94 + (4 * v90 & 12)) + 76);
        *(int32_t *)(v66 - 32) = (int32_t)&v84;
        v95 = v66 - 36;
        *(int32_t *)v95 = (int32_t)&v82;
        *v9 = 0;
        function_408530((int32_t)&g1224);
        v96 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v97 = *(int32_t *)v96;
        v101 = 0;
        if (v97 != 0) {
            // 0x407a52
            v101 = *(int32_t *)v97;
        }
        // 0x407a54
        v102 = *(int32_t *)(v96 + 8);
        v103 = *(int32_t *)(v101 + 8);
        v104 = v102 / 4;
        v105 = *(int32_t *)(v101 + 4);
        v106 = *(int32_t *)(4 * (v104 - (v104 < v103 ? 0 : v103)) + v105);
        *v7 = *(char *)(*(int32_t *)(v106 + (4 * v102 & 12)) + 4);
        v80 = v95;
    } else {
        // 0x407a7c
        v80 = v68;
        if (v76 == 0) {
            // 0x407a80
            *(int32_t *)(v66 - 16) = v19;
            *(int32_t *)(v66 - 20) = (int32_t)&v86;
            v98 = v66 - 24;
            *(int32_t *)v98 = (int32_t)&v83;
            function_408530((int32_t)&g1224);
            v99 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v100 = *(int32_t *)v99;
            v107 = 0;
            if (v100 != 0) {
                // 0x407aaa
                v107 = *(int32_t *)v100;
            }
            // 0x407aac
            v108 = *(int32_t *)(v99 + 8);
            v109 = *(int32_t *)(v107 + 8);
            v110 = v108 / 4;
            v111 = *(int32_t *)(v107 + 4);
            v112 = *(int32_t *)(4 * (v110 - (v110 < v109 ? 0 : v109)) + v111);
            v80 = v98;
            if (*(char *)(*(int32_t *)(v112 + (4 * v108 & 12)) + 129) != 0) {
                // 0x407ad2
                *v9 = 1;
                v80 = v98;
            }
        }
    }
    int32_t v113 = 80;
    int32_t v114 = v80;
    int32_t v115 = 0;
    *(int32_t *)(v114 - 4) = v19;
    int32_t v116; // bp-416, 0x4077c0
    *(int32_t *)(v114 - 8) = (int32_t)&v116;
    int32_t v117 = v114 - 12; // 0x407af5
    int32_t v118; // bp-248, 0x4077c0
    *(int32_t *)v117 = (int32_t)&v118;
    function_408530((int32_t)&g1224);
    int32_t v119 = function_408550((int32_t)&g1224, (int32_t)&g1224); // 0x407b03
    int32_t v120 = *(int32_t *)v119; // 0x407b08
    int32_t v121 = 0; // 0x407b0c
    if (v120 != 0) {
        // 0x407b12
        v121 = *(int32_t *)v120;
    }
    uint32_t v122 = *(int32_t *)(v119 + 8); // 0x407b14
    uint32_t v123 = *(int32_t *)(v121 + 8); // 0x407b17
    uint32_t v124 = v122 / 4; // 0x407b1c
    int32_t v125 = *(int32_t *)(v121 + 4); // 0x407b28
    int32_t v126 = *(int32_t *)(4 * (v124 - (v124 < v123 ? 0 : v123)) + v125); // 0x407b2e
    int32_t * v127 = (int32_t *)(v113 + v2); // 0x407b31
    int32_t v128 = *v127; // 0x407b31
    int32_t v129; // 0x4077c0
    int32_t v130; // bp-272, 0x4077c0
    int32_t v131; // bp-32, 0x4077c0
    int32_t v132; // bp-320, 0x4077c0
    int32_t v133; // bp-440, 0x4077c0
    int32_t v134; // bp-512, 0x4077c0
    int32_t v135; // bp-560, 0x4077c0
    int32_t v136; // 0x407b62
    int32_t v137; // 0x407b67
    int32_t v138; // 0x407b71
    uint32_t v139; // 0x407b73
    uint32_t v140; // 0x407b76
    uint32_t v141; // 0x407b7b
    int32_t v142; // 0x407b87
    int32_t v143; // 0x407b8a
    int32_t v144; // 0x407ba7
    int32_t v145; // 0x407bbd
    int32_t v146; // 0x407bc2
    int32_t v147; // 0x407c0e
    int32_t v148; // 0x407c1f
    int32_t v149; // 0x407c24
    int32_t v150; // 0x407bcc
    uint32_t v151; // 0x407bce
    uint32_t v152; // 0x407bd1
    uint32_t v153; // 0x407bd6
    int32_t v154; // 0x407be2
    int32_t v155; // 0x407be5
    int32_t v156; // 0x407c2e
    uint32_t v157; // 0x407c30
    uint32_t v158; // 0x407c33
    uint32_t v159; // 0x407c38
    int32_t v160; // 0x407c44
    int32_t v161; // 0x407c47
    int32_t v162; // 0x4077c0
    if (v128 < *(int32_t *)(*(int32_t *)(v126 + (4 * v122 & 12)) + v113)) {
        // 0x407b40
        *(int32_t *)(v114 - 16) = v19;
        *(int32_t *)(v114 - 20) = (int32_t)&v134;
        *(int32_t *)(v114 - 24) = (int32_t)&v130;
        function_408530((int32_t)&g1224);
        v136 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v137 = *(int32_t *)v136;
        v138 = 0;
        if (v137 != 0) {
            // 0x407b71
            v138 = *(int32_t *)v137;
        }
        // 0x407b73
        v139 = *(int32_t *)(v136 + 8);
        v140 = *(int32_t *)(v138 + 8);
        v141 = v139 / 4;
        v142 = *(int32_t *)(v138 + 4);
        v143 = *(int32_t *)(4 * (v141 - (v141 < v140 ? 0 : v140)) + v142);
        *(int32_t *)(v114 - 28) = v19;
        *v127 = *(int32_t *)(*(int32_t *)(v143 + (4 * v139 & 12)) + v113);
        *(int32_t *)(v114 - 32) = (int32_t)&v133;
        v144 = v114 - 36;
        *(int32_t *)v144 = (int32_t)&v131;
        *(char *)(v115 + 130 + v2) = 0;
        function_408530((int32_t)&g1224);
        v145 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v146 = *(int32_t *)v145;
        v150 = 0;
        if (v146 != 0) {
            // 0x407bcc
            v150 = *(int32_t *)v146;
        }
        // 0x407bce
        v151 = *(int32_t *)(v145 + 8);
        v152 = *(int32_t *)(v150 + 8);
        v153 = v151 / 4;
        v154 = *(int32_t *)(v150 + 4);
        v155 = *(int32_t *)(4 * (v153 - (v153 < v152 ? 0 : v152)) + v154);
        *v7 = *(char *)(*(int32_t *)(v155 + (4 * v151 & 12)) + 4);
        v129 = v144;
    } else {
        // 0x407bf9
        v129 = v117;
        if (v128 == 0) {
            // 0x407bfd
            *(int32_t *)(v114 - 16) = v19;
            *(int32_t *)(v114 - 20) = (int32_t)&v135;
            v147 = v114 - 24;
            *(int32_t *)v147 = (int32_t)&v132;
            function_408530((int32_t)&g1224);
            v148 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v149 = *(int32_t *)v148;
            v156 = 0;
            if (v149 != 0) {
                // 0x407c2e
                v156 = *(int32_t *)v149;
            }
            // 0x407c30
            v157 = *(int32_t *)(v148 + 8);
            v158 = *(int32_t *)(v156 + 8);
            v159 = v157 / 4;
            v160 = *(int32_t *)(v156 + 4);
            v161 = *(int32_t *)(4 * (v159 - (v159 < v158 ? 0 : v158)) + v160);
            v162 = v115 + 130;
            v129 = v147;
            if (*(char *)(*(int32_t *)(v161 + (4 * v157 & 12)) + v162) != 0) {
                // 0x407c57
                *(char *)(v162 + v2) = 1;
                v129 = v147;
            }
        }
    }
    int32_t v163 = v129;
    int32_t v164 = v115 + 1; // 0x407c65
    int32_t v165 = v113 + 4; // 0x407c6c
    while (v164 != 12) {
        // 0x407ae1
        v113 = v165;
        v114 = v163;
        v115 = v164;
        *(int32_t *)(v114 - 4) = v19;
        *(int32_t *)(v114 - 8) = (int32_t)&v116;
        v117 = v114 - 12;
        *(int32_t *)v117 = (int32_t)&v118;
        function_408530((int32_t)&g1224);
        v119 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v120 = *(int32_t *)v119;
        v121 = 0;
        if (v120 != 0) {
            // 0x407b12
            v121 = *(int32_t *)v120;
        }
        // 0x407b14
        v122 = *(int32_t *)(v119 + 8);
        v123 = *(int32_t *)(v121 + 8);
        v124 = v122 / 4;
        v125 = *(int32_t *)(v121 + 4);
        v126 = *(int32_t *)(4 * (v124 - (v124 < v123 ? 0 : v123)) + v125);
        v127 = (int32_t *)(v113 + v2);
        v128 = *v127;
        if (v128 < *(int32_t *)(*(int32_t *)(v126 + (4 * v122 & 12)) + v113)) {
            // 0x407b40
            *(int32_t *)(v114 - 16) = v19;
            *(int32_t *)(v114 - 20) = (int32_t)&v134;
            *(int32_t *)(v114 - 24) = (int32_t)&v130;
            function_408530((int32_t)&g1224);
            v136 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v137 = *(int32_t *)v136;
            v138 = 0;
            if (v137 != 0) {
                // 0x407b71
                v138 = *(int32_t *)v137;
            }
            // 0x407b73
            v139 = *(int32_t *)(v136 + 8);
            v140 = *(int32_t *)(v138 + 8);
            v141 = v139 / 4;
            v142 = *(int32_t *)(v138 + 4);
            v143 = *(int32_t *)(4 * (v141 - (v141 < v140 ? 0 : v140)) + v142);
            *(int32_t *)(v114 - 28) = v19;
            *v127 = *(int32_t *)(*(int32_t *)(v143 + (4 * v139 & 12)) + v113);
            *(int32_t *)(v114 - 32) = (int32_t)&v133;
            v144 = v114 - 36;
            *(int32_t *)v144 = (int32_t)&v131;
            *(char *)(v115 + 130 + v2) = 0;
            function_408530((int32_t)&g1224);
            v145 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v146 = *(int32_t *)v145;
            v150 = 0;
            if (v146 != 0) {
                // 0x407bcc
                v150 = *(int32_t *)v146;
            }
            // 0x407bce
            v151 = *(int32_t *)(v145 + 8);
            v152 = *(int32_t *)(v150 + 8);
            v153 = v151 / 4;
            v154 = *(int32_t *)(v150 + 4);
            v155 = *(int32_t *)(4 * (v153 - (v153 < v152 ? 0 : v152)) + v154);
            *v7 = *(char *)(*(int32_t *)(v155 + (4 * v151 & 12)) + 4);
            v129 = v144;
        } else {
            // 0x407bf9
            v129 = v117;
            if (v128 == 0) {
                // 0x407bfd
                *(int32_t *)(v114 - 16) = v19;
                *(int32_t *)(v114 - 20) = (int32_t)&v135;
                v147 = v114 - 24;
                *(int32_t *)v147 = (int32_t)&v132;
                function_408530((int32_t)&g1224);
                v148 = function_408550((int32_t)&g1224, (int32_t)&g1224);
                v149 = *(int32_t *)v148;
                v156 = 0;
                if (v149 != 0) {
                    // 0x407c2e
                    v156 = *(int32_t *)v149;
                }
                // 0x407c30
                v157 = *(int32_t *)(v148 + 8);
                v158 = *(int32_t *)(v156 + 8);
                v159 = v157 / 4;
                v160 = *(int32_t *)(v156 + 4);
                v161 = *(int32_t *)(4 * (v159 - (v159 < v158 ? 0 : v158)) + v160);
                v162 = v115 + 130;
                v129 = v147;
                if (*(char *)(*(int32_t *)(v161 + (4 * v157 & 12)) + v162) != 0) {
                    // 0x407c57
                    *(char *)(v162 + v2) = 1;
                    v129 = v147;
                }
            }
        }
        // 0x407c62
        v163 = v129;
        v164 = v115 + 1;
        v165 = v113 + 4;
    }
    int3_t v166 = v18 - 1; // 0x407c75
    __frontend_reg_store_fpr(v166, (float80_t)*v10);
    int32_t v167 = v163 - 8; // 0x407c7b
    __frontend_reg_store_fpr(v166, __frontend_reg_load_fpr(v166));
    *(float64_t *)v167 = (float64_t)__frontend_reg_load_fpr(v166);
    _fabs((float64_t)(int64_t)&g1224);
    __frontend_reg_store_fpr(v18, __frontend_reg_load_fpr(v18));
    *(int32_t *)(v163 - 4) = v19;
    __frontend_reg_store_fpr(v18, __frontend_reg_load_fpr(v18));
    int32_t v168; // bp-44, 0x4077c0
    *(int32_t *)v167 = (int32_t)&v168;
    float80_t v169 = __frontend_reg_load_fpr(v18); // 0x407ca3
    int32_t v170 = v163 - 12; // 0x407ca9
    int32_t v171; // bp-68, 0x4077c0
    *(int32_t *)v170 = (int32_t)&v171;
    function_408530((int32_t)(float32_t)v169);
    int32_t v172 = function_408550((int32_t)&g1224, (int32_t)&g1224); // 0x407cb7
    int32_t v173 = *(int32_t *)v172; // 0x407cbc
    int32_t v174 = 0; // 0x407cc0
    if (v173 != 0) {
        // 0x407cc6
        v174 = *(int32_t *)v173;
    }
    int3_t v175 = v18 + 1; // 0x407ca3
    uint32_t v176 = *(int32_t *)(v172 + 8); // 0x407cc8
    uint32_t v177 = *(int32_t *)(v174 + 8); // 0x407ccb
    uint32_t v178 = v176 / 4; // 0x407cd0
    int32_t v179 = *(int32_t *)(v174 + 4); // 0x407cdc
    int32_t v180 = *(int32_t *)(4 * (v178 - (v178 < v177 ? 0 : v177)) + v179); // 0x407cdf
    float32_t v181 = *(float32_t *)(*(int32_t *)(v180 + (4 * v176 & 12)) + 144); // 0x407ce5
    __frontend_reg_store_fpr(v18, (float80_t)v181);
    int32_t v182 = v163 - 20; // 0x407ceb
    __frontend_reg_store_fpr(v18, __frontend_reg_load_fpr(v18));
    *(float64_t *)v182 = (float64_t)__frontend_reg_load_fpr(v18);
    _fabs((float64_t)(int64_t)&g1224);
    __frontend_reg_store_fpr(v175, __frontend_reg_load_fpr(v175));
    __frontend_reg_store_fpr(v175, __frontend_reg_load_fpr(v175));
    float80_t v183 = __frontend_reg_load_fpr(v175); // 0x407d0e
    __frontend_reg_store_fpr(v175, v169);
    __frontend_reg_store_fpr(v18, v183);
    __frontend_reg_load_fpr(v18);
    __frontend_reg_load_fpr(v175);
    int32_t v184 = v170; // 0x407d1e
    int32_t v185; // bp-116, 0x4077c0
    int32_t v186; // bp-92, 0x4077c0
    int32_t v187; // 0x407d39
    int32_t v188; // 0x407d3e
    int32_t v189; // 0x407d48
    uint32_t v190; // 0x407d4a
    uint32_t v191; // 0x407d4d
    uint32_t v192; // 0x407d52
    int32_t v193; // 0x407d5e
    int32_t v194; // 0x407d61
    float32_t v195; // 0x407d67
    if ((v2 & 0x4100) == 0) {
        // 0x407d20
        *(int32_t *)(v163 - 16) = v19;
        *(int32_t *)v182 = (int32_t)&v186;
        v184 = v163 - 24;
        *(int32_t *)v184 = (int32_t)&v185;
        function_408530((int32_t)&g1224);
        v187 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v188 = *(int32_t *)v187;
        v189 = 0;
        if (v188 != 0) {
            // 0x407d48
            v189 = *(int32_t *)v188;
        }
        // 0x407d4a
        v190 = *(int32_t *)(v187 + 8);
        v191 = *(int32_t *)(v189 + 8);
        v192 = v190 / 4;
        v193 = *(int32_t *)(v189 + 4);
        v194 = *(int32_t *)(4 * (v192 - (v192 < v191 ? 0 : v191)) + v193);
        v195 = *(float32_t *)(*(int32_t *)(v194 + (4 * v190 & 12)) + 144);
        __frontend_reg_store_fpr(v175, (float80_t)v195);
        *v10 = (float32_t)__frontend_reg_load_fpr(v175);
    }
    int3_t v196 = v18 + 2; // 0x407d17
    int32_t v197 = v184;
    __frontend_reg_store_fpr(v175, (float80_t)*v11);
    int32_t v198 = v197 - 8; // 0x407d79
    __frontend_reg_store_fpr(v175, __frontend_reg_load_fpr(v175));
    *(float64_t *)v198 = (float64_t)__frontend_reg_load_fpr(v175);
    _fabs((float64_t)(int64_t)&g1224);
    __frontend_reg_store_fpr(v196, __frontend_reg_load_fpr(v196));
    *(int32_t *)(v197 - 4) = v19;
    __frontend_reg_store_fpr(v196, __frontend_reg_load_fpr(v196));
    int32_t v199; // bp-140, 0x4077c0
    *(int32_t *)v198 = (int32_t)&v199;
    float80_t v200 = __frontend_reg_load_fpr(v196); // 0x407da4
    int32_t v201 = v197 - 12; // 0x407dad
    int32_t v202; // bp-164, 0x4077c0
    *(int32_t *)v201 = (int32_t)&v202;
    function_408530((int32_t)(float32_t)v200);
    int32_t v203 = function_408550((int32_t)&g1224, (int32_t)&g1224); // 0x407dbb
    int32_t v204 = *(int32_t *)v203; // 0x407dc0
    int32_t v205 = 0; // 0x407dc4
    if (v204 != 0) {
        // 0x407dca
        v205 = *(int32_t *)v204;
    }
    int3_t v206 = v18 + 3; // 0x407da4
    uint32_t v207 = *(int32_t *)(v203 + 8); // 0x407dcc
    uint32_t v208 = *(int32_t *)(v205 + 8); // 0x407dcf
    uint32_t v209 = v207 / 4; // 0x407dd4
    int32_t v210 = *(int32_t *)(v205 + 4); // 0x407de0
    int32_t v211 = *(int32_t *)(4 * (v209 - (v209 < v208 ? 0 : v208)) + v210); // 0x407de3
    float32_t v212 = *(float32_t *)(*(int32_t *)(v211 + (4 * v207 & 12)) + 148); // 0x407de9
    __frontend_reg_store_fpr(v196, (float80_t)v212);
    int32_t v213 = v197 - 20; // 0x407def
    __frontend_reg_store_fpr(v196, __frontend_reg_load_fpr(v196));
    *(float64_t *)v213 = (float64_t)__frontend_reg_load_fpr(v196);
    _fabs((float64_t)(int64_t)&g1224);
    __frontend_reg_store_fpr(v206, __frontend_reg_load_fpr(v206));
    __frontend_reg_store_fpr(v206, __frontend_reg_load_fpr(v206));
    float80_t v214 = __frontend_reg_load_fpr(v206); // 0x407e0f
    __frontend_reg_store_fpr(v206, v200);
    __frontend_reg_store_fpr(v196, v214);
    __frontend_reg_load_fpr(v196);
    __frontend_reg_load_fpr(v206);
    int32_t v215 = v201; // 0x407e1f
    int32_t v216; // bp-188, 0x4077c0
    int32_t v217; // bp-212, 0x4077c0
    int32_t v218; // 0x407e40
    int32_t v219; // 0x407e45
    int32_t v220; // 0x407e4f
    uint32_t v221; // 0x407e51
    uint32_t v222; // 0x407e54
    uint32_t v223; // 0x407e59
    int32_t v224; // 0x407e65
    int32_t v225; // 0x407e68
    float32_t v226; // 0x407e6e
    if ((v2 & 0x4100) == 0) {
        // 0x407e21
        *(int32_t *)(v197 - 16) = v19;
        *(int32_t *)v213 = (int32_t)&v216;
        v215 = v197 - 24;
        *(int32_t *)v215 = (int32_t)&v217;
        function_408530((int32_t)&g1224);
        v218 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v219 = *(int32_t *)v218;
        v220 = 0;
        if (v219 != 0) {
            // 0x407e4f
            v220 = *(int32_t *)v219;
        }
        // 0x407e51
        v221 = *(int32_t *)(v218 + 8);
        v222 = *(int32_t *)(v220 + 8);
        v223 = v221 / 4;
        v224 = *(int32_t *)(v220 + 4);
        v225 = *(int32_t *)(4 * (v223 - (v223 < v222 ? 0 : v222)) + v224);
        v226 = *(float32_t *)(*(int32_t *)(v225 + (4 * v221 & 12)) + 148);
        __frontend_reg_store_fpr(v206, (float80_t)v226);
        *v11 = (float32_t)__frontend_reg_load_fpr(v206);
    }
    int3_t v227 = v18 ^ -4; // 0x407e18
    int32_t v228 = v215;
    __frontend_reg_store_fpr(v206, (float80_t)*v12);
    int32_t v229 = v228 - 8; // 0x407e80
    __frontend_reg_store_fpr(v206, __frontend_reg_load_fpr(v206));
    *(float64_t *)v229 = (float64_t)__frontend_reg_load_fpr(v206);
    _fabs((float64_t)(int64_t)&g1224);
    __frontend_reg_store_fpr(v227, __frontend_reg_load_fpr(v227));
    *(int32_t *)(v228 - 4) = v19;
    __frontend_reg_store_fpr(v227, __frontend_reg_load_fpr(v227));
    int32_t v230; // bp-236, 0x4077c0
    *(int32_t *)v229 = (int32_t)&v230;
    float80_t v231 = __frontend_reg_load_fpr(v227); // 0x407eab
    int32_t v232 = v228 - 12; // 0x407eb4
    int32_t v233; // bp-260, 0x4077c0
    *(int32_t *)v232 = (int32_t)&v233;
    function_408530((int32_t)(float32_t)v231);
    int32_t v234 = function_408550((int32_t)&g1224, (int32_t)&g1224); // 0x407ec2
    int32_t v235 = *(int32_t *)v234; // 0x407ec7
    int32_t v236 = 0; // 0x407ecb
    if (v235 != 0) {
        // 0x407ed1
        v236 = *(int32_t *)v235;
    }
    int3_t v237 = v18 - 3; // 0x407eab
    uint32_t v238 = *(int32_t *)(v234 + 8); // 0x407ed3
    uint32_t v239 = *(int32_t *)(v236 + 8); // 0x407ed6
    uint32_t v240 = v238 / 4; // 0x407edb
    int32_t v241 = *(int32_t *)(v236 + 4); // 0x407ee7
    int32_t v242 = *(int32_t *)(4 * (v240 - (v240 < v239 ? 0 : v239)) + v241); // 0x407eea
    float32_t v243 = *(float32_t *)(*(int32_t *)(v242 + (4 * v238 & 12)) + 152); // 0x407ef0
    __frontend_reg_store_fpr(v227, (float80_t)v243);
    int32_t v244 = v228 - 20; // 0x407ef6
    __frontend_reg_store_fpr(v227, __frontend_reg_load_fpr(v227));
    *(float64_t *)v244 = (float64_t)__frontend_reg_load_fpr(v227);
    _fabs((float64_t)(int64_t)&g1224);
    __frontend_reg_store_fpr(v237, __frontend_reg_load_fpr(v237));
    __frontend_reg_store_fpr(v237, __frontend_reg_load_fpr(v237));
    float80_t v245 = __frontend_reg_load_fpr(v237); // 0x407f16
    __frontend_reg_store_fpr(v237, v231);
    __frontend_reg_store_fpr(v227, v245);
    __frontend_reg_load_fpr(v227);
    __frontend_reg_load_fpr(v237);
    int32_t v246 = v232; // 0x407f26
    int32_t v247; // bp-284, 0x4077c0
    int32_t v248; // bp-308, 0x4077c0
    int32_t v249; // 0x407f47
    int32_t v250; // 0x407f4c
    int32_t v251; // 0x407f56
    uint32_t v252; // 0x407f58
    uint32_t v253; // 0x407f5b
    uint32_t v254; // 0x407f60
    int32_t v255; // 0x407f6c
    int32_t v256; // 0x407f6f
    float32_t v257; // 0x407f75
    if ((v2 & 0x4100) == 0) {
        // 0x407f28
        *(int32_t *)(v228 - 16) = v19;
        *(int32_t *)v244 = (int32_t)&v247;
        v246 = v228 - 24;
        *(int32_t *)v246 = (int32_t)&v248;
        function_408530((int32_t)&g1224);
        v249 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v250 = *(int32_t *)v249;
        v251 = 0;
        if (v250 != 0) {
            // 0x407f56
            v251 = *(int32_t *)v250;
        }
        // 0x407f58
        v252 = *(int32_t *)(v249 + 8);
        v253 = *(int32_t *)(v251 + 8);
        v254 = v252 / 4;
        v255 = *(int32_t *)(v251 + 4);
        v256 = *(int32_t *)(4 * (v254 - (v254 < v253 ? 0 : v253)) + v255);
        v257 = *(float32_t *)(*(int32_t *)(v256 + (4 * v252 & 12)) + 152);
        __frontend_reg_store_fpr(v237, (float80_t)v257);
        *v12 = (float32_t)__frontend_reg_load_fpr(v237);
    }
    int3_t v258 = v18 - 2; // 0x407f1f
    int32_t v259 = v246;
    __frontend_reg_store_fpr(v237, (float80_t)*v13);
    int32_t v260 = v259 - 8; // 0x407f87
    __frontend_reg_store_fpr(v237, __frontend_reg_load_fpr(v237));
    *(float64_t *)v260 = (float64_t)__frontend_reg_load_fpr(v237);
    _fabs((float64_t)(int64_t)&g1224);
    __frontend_reg_store_fpr(v258, __frontend_reg_load_fpr(v258));
    *(int32_t *)(v259 - 4) = v19;
    __frontend_reg_store_fpr(v258, __frontend_reg_load_fpr(v258));
    int32_t v261; // bp-332, 0x4077c0
    *(int32_t *)v260 = (int32_t)&v261;
    float80_t v262 = __frontend_reg_load_fpr(v258); // 0x407fb2
    int32_t v263 = v259 - 12; // 0x407fbb
    int32_t v264; // bp-356, 0x4077c0
    *(int32_t *)v263 = (int32_t)&v264;
    function_408530((int32_t)(float32_t)v262);
    int32_t v265 = function_408550((int32_t)&g1224, (int32_t)&g1224); // 0x407fc9
    int32_t v266 = *(int32_t *)v265; // 0x407fce
    int32_t v267 = 0; // 0x407fd2
    if (v266 != 0) {
        // 0x407fd8
        v267 = *(int32_t *)v266;
    }
    uint32_t v268 = *(int32_t *)(v265 + 8); // 0x407fda
    uint32_t v269 = *(int32_t *)(v267 + 8); // 0x407fdd
    uint32_t v270 = v268 / 4; // 0x407fe2
    int32_t v271 = *(int32_t *)(v267 + 4); // 0x407fee
    int32_t v272 = *(int32_t *)(4 * (v270 - (v270 < v269 ? 0 : v269)) + v271); // 0x407ff1
    float32_t v273 = *(float32_t *)(*(int32_t *)(v272 + (4 * v268 & 12)) + 156); // 0x407ff7
    __frontend_reg_store_fpr(v258, (float80_t)v273);
    int32_t v274 = v259 - 20; // 0x407ffd
    __frontend_reg_store_fpr(v258, __frontend_reg_load_fpr(v258));
    *(float64_t *)v274 = (float64_t)__frontend_reg_load_fpr(v258);
    _fabs((float64_t)(int64_t)&g1224);
    __frontend_reg_store_fpr(v166, __frontend_reg_load_fpr(v166));
    __frontend_reg_store_fpr(v166, __frontend_reg_load_fpr(v166));
    float80_t v275 = __frontend_reg_load_fpr(v166); // 0x40801d
    __frontend_reg_store_fpr(v166, v262);
    __frontend_reg_store_fpr(v258, v275);
    __frontend_reg_load_fpr(v258);
    __frontend_reg_load_fpr(v166);
    int32_t v276 = v263; // 0x40802d
    int32_t v277; // bp-380, 0x4077c0
    int32_t v278; // bp-404, 0x4077c0
    int32_t v279; // 0x40804e
    int32_t v280; // 0x408053
    int32_t v281; // 0x40805d
    uint32_t v282; // 0x40805f
    uint32_t v283; // 0x408062
    uint32_t v284; // 0x408067
    int32_t v285; // 0x408073
    int32_t v286; // 0x408076
    float32_t v287; // 0x40807c
    if ((v2 & 0x4100) == 0) {
        // 0x40802f
        *(int32_t *)(v259 - 16) = v19;
        *(int32_t *)v274 = (int32_t)&v277;
        v276 = v259 - 24;
        *(int32_t *)v276 = (int32_t)&v278;
        function_408530((int32_t)&g1224);
        v279 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v280 = *(int32_t *)v279;
        v281 = 0;
        if (v280 != 0) {
            // 0x40805d
            v281 = *(int32_t *)v280;
        }
        // 0x40805f
        v282 = *(int32_t *)(v279 + 8);
        v283 = *(int32_t *)(v281 + 8);
        v284 = v282 / 4;
        v285 = *(int32_t *)(v281 + 4);
        v286 = *(int32_t *)(4 * (v284 - (v284 < v283 ? 0 : v283)) + v285);
        v287 = *(float32_t *)(*(int32_t *)(v286 + (4 * v282 & 12)) + 156);
        __frontend_reg_store_fpr(v166, (float80_t)v287);
        *v13 = (float32_t)__frontend_reg_load_fpr(v166);
    }
    int32_t v288 = v276;
    __frontend_reg_store_fpr(v166, (float80_t)*v14);
    int32_t v289 = v288 - 8; // 0x40808e
    __frontend_reg_store_fpr(v166, __frontend_reg_load_fpr(v166));
    *(float64_t *)v289 = (float64_t)__frontend_reg_load_fpr(v166);
    _fabs((float64_t)(int64_t)&g1224);
    __frontend_reg_store_fpr(v18, __frontend_reg_load_fpr(v18));
    *(int32_t *)(v288 - 4) = v19;
    __frontend_reg_store_fpr(v18, __frontend_reg_load_fpr(v18));
    int32_t v290; // bp-428, 0x4077c0
    *(int32_t *)v289 = (int32_t)&v290;
    float80_t v291 = __frontend_reg_load_fpr(v18); // 0x4080b9
    int32_t v292 = v288 - 12; // 0x4080c2
    int32_t v293; // bp-452, 0x4077c0
    *(int32_t *)v292 = (int32_t)&v293;
    function_408530((int32_t)(float32_t)v291);
    int32_t v294 = function_408550((int32_t)&g1224, (int32_t)&g1224); // 0x4080d0
    int32_t v295 = *(int32_t *)v294; // 0x4080d5
    int32_t v296 = 0; // 0x4080d9
    if (v295 != 0) {
        // 0x4080df
        v296 = *(int32_t *)v295;
    }
    uint32_t v297 = *(int32_t *)(v294 + 8); // 0x4080e1
    uint32_t v298 = *(int32_t *)(v296 + 8); // 0x4080e4
    uint32_t v299 = v297 / 4; // 0x4080e9
    int32_t v300 = *(int32_t *)(v296 + 4); // 0x4080f5
    int32_t v301 = *(int32_t *)(4 * (v299 - (v299 < v298 ? 0 : v298)) + v300); // 0x4080f8
    float32_t v302 = *(float32_t *)(*(int32_t *)(v301 + (4 * v297 & 12)) + 160); // 0x4080fe
    __frontend_reg_store_fpr(v18, (float80_t)v302);
    int32_t v303 = v288 - 20; // 0x408104
    __frontend_reg_store_fpr(v18, __frontend_reg_load_fpr(v18));
    *(float64_t *)v303 = (float64_t)__frontend_reg_load_fpr(v18);
    _fabs((float64_t)(int64_t)&g1224);
    __frontend_reg_store_fpr(v175, __frontend_reg_load_fpr(v175));
    __frontend_reg_store_fpr(v175, __frontend_reg_load_fpr(v175));
    float80_t v304 = __frontend_reg_load_fpr(v175); // 0x408124
    __frontend_reg_store_fpr(v175, v291);
    __frontend_reg_store_fpr(v18, v304);
    __frontend_reg_load_fpr(v18);
    __frontend_reg_load_fpr(v175);
    int32_t v305 = v292; // 0x408134
    int32_t v306; // bp-476, 0x4077c0
    int32_t v307; // bp-500, 0x4077c0
    int32_t v308; // 0x408155
    int32_t v309; // 0x40815a
    int32_t v310; // 0x408164
    uint32_t v311; // 0x408166
    uint32_t v312; // 0x408169
    uint32_t v313; // 0x40816e
    int32_t v314; // 0x40817a
    int32_t v315; // 0x40817d
    float32_t v316; // 0x408183
    if ((v2 & 0x4100) == 0) {
        // 0x408136
        *(int32_t *)(v288 - 16) = v19;
        *(int32_t *)v303 = (int32_t)&v306;
        v305 = v288 - 24;
        *(int32_t *)v305 = (int32_t)&v307;
        function_408530((int32_t)&g1224);
        v308 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v309 = *(int32_t *)v308;
        v310 = 0;
        if (v309 != 0) {
            // 0x408164
            v310 = *(int32_t *)v309;
        }
        // 0x408166
        v311 = *(int32_t *)(v308 + 8);
        v312 = *(int32_t *)(v310 + 8);
        v313 = v311 / 4;
        v314 = *(int32_t *)(v310 + 4);
        v315 = *(int32_t *)(4 * (v313 - (v313 < v312 ? 0 : v312)) + v314);
        v316 = *(float32_t *)(*(int32_t *)(v315 + (4 * v311 & 12)) + 160);
        __frontend_reg_store_fpr(v175, (float80_t)v316);
        *v14 = (float32_t)__frontend_reg_load_fpr(v175);
    }
    int32_t v317 = v305;
    __frontend_reg_store_fpr(v175, (float80_t)*v15);
    int32_t v318 = v317 - 8; // 0x408195
    __frontend_reg_store_fpr(v175, __frontend_reg_load_fpr(v175));
    *(float64_t *)v318 = (float64_t)__frontend_reg_load_fpr(v175);
    _fabs((float64_t)(int64_t)&g1224);
    __frontend_reg_store_fpr(v196, __frontend_reg_load_fpr(v196));
    *(int32_t *)(v317 - 4) = v19;
    __frontend_reg_store_fpr(v196, __frontend_reg_load_fpr(v196));
    int32_t v319; // bp-524, 0x4077c0
    *(int32_t *)v318 = (int32_t)&v319;
    float80_t v320 = __frontend_reg_load_fpr(v196); // 0x4081c0
    int32_t v321 = v317 - 12; // 0x4081c9
    int32_t v322; // bp-548, 0x4077c0
    *(int32_t *)v321 = (int32_t)&v322;
    function_408530((int32_t)(float32_t)v320);
    int32_t v323 = function_408550((int32_t)&g1224, (int32_t)&g1224); // 0x4081d7
    int32_t v324 = *(int32_t *)v323; // 0x4081dc
    int32_t v325 = 0; // 0x4081e0
    if (v324 != 0) {
        // 0x4081e6
        v325 = *(int32_t *)v324;
    }
    uint32_t v326 = *(int32_t *)(v323 + 8); // 0x4081e8
    uint32_t v327 = *(int32_t *)(v325 + 8); // 0x4081eb
    uint32_t v328 = v326 / 4; // 0x4081f0
    int32_t v329 = *(int32_t *)(v325 + 4); // 0x4081fc
    int32_t v330 = *(int32_t *)(4 * (v328 - (v328 < v327 ? 0 : v327)) + v329); // 0x4081ff
    float32_t v331 = *(float32_t *)(*(int32_t *)(v330 + (4 * v326 & 12)) + 164); // 0x408205
    __frontend_reg_store_fpr(v196, (float80_t)v331);
    int32_t v332 = v317 - 20; // 0x40820b
    __frontend_reg_store_fpr(v196, __frontend_reg_load_fpr(v196));
    *(float64_t *)v332 = (float64_t)__frontend_reg_load_fpr(v196);
    _fabs((float64_t)(int64_t)&g1224);
    __frontend_reg_store_fpr(v206, __frontend_reg_load_fpr(v206));
    __frontend_reg_store_fpr(v206, __frontend_reg_load_fpr(v206));
    float80_t v333 = __frontend_reg_load_fpr(v206); // 0x40822b
    __frontend_reg_store_fpr(v206, v320);
    __frontend_reg_store_fpr(v196, v333);
    __frontend_reg_load_fpr(v196);
    __frontend_reg_load_fpr(v206);
    int32_t v334 = v321; // 0x40823b
    int32_t v335; // bp-572, 0x4077c0
    int32_t v336; // bp-596, 0x4077c0
    int32_t v337; // 0x40825c
    int32_t v338; // 0x408261
    int32_t v339; // 0x40826b
    uint32_t v340; // 0x40826d
    uint32_t v341; // 0x408270
    uint32_t v342; // 0x408275
    int32_t v343; // 0x408281
    int32_t v344; // 0x408284
    float32_t v345; // 0x40828a
    if ((v2 & 0x4100) == 0) {
        // 0x40823d
        *(int32_t *)(v317 - 16) = v19;
        *(int32_t *)v332 = (int32_t)&v335;
        v334 = v317 - 24;
        *(int32_t *)v334 = (int32_t)&v336;
        function_408530((int32_t)&g1224);
        v337 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v338 = *(int32_t *)v337;
        v339 = 0;
        if (v338 != 0) {
            // 0x40826b
            v339 = *(int32_t *)v338;
        }
        // 0x40826d
        v340 = *(int32_t *)(v337 + 8);
        v341 = *(int32_t *)(v339 + 8);
        v342 = v340 / 4;
        v343 = *(int32_t *)(v339 + 4);
        v344 = *(int32_t *)(4 * (v342 - (v342 < v341 ? 0 : v341)) + v343);
        v345 = *(float32_t *)(*(int32_t *)(v344 + (4 * v340 & 12)) + 164);
        __frontend_reg_store_fpr(v206, (float80_t)v345);
        *v15 = (float32_t)__frontend_reg_load_fpr(v206);
    }
    int32_t result = v19 + 1; // 0x408299
    while (result < *v5) {
        // 0x4077f5
        v16 = v334;
        v18 = v227;
        v19 = result;
        *(int32_t *)(v16 - 4) = v19;
        *(int32_t *)(v16 - 8) = (int32_t)&v20;
        v21 = v16 - 12;
        *(int32_t *)v21 = (int32_t)&v22;
        function_408530(v19);
        v23 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v24 = *(int32_t *)v23;
        v25 = 0;
        if (v24 != 0) {
            // 0x407825
            v25 = *(int32_t *)v24;
        }
        // 0x407827
        v26 = *(int32_t *)(v23 + 8);
        v27 = *(int32_t *)(v25 + 8);
        v28 = v26 / 4;
        v29 = *v3;
        v30 = *(int32_t *)(v25 + 4);
        v31 = *(int32_t *)(4 * (v28 - (v28 < v27 ? 0 : v27)) + v30);
        v32 = *(int32_t *)(*(int32_t *)(v31 + (4 * v26 & 12)) + 72);
        if ((v29 < 0 ? -v29 : v29) < (v32 < 0 ? -v32 : v32)) {
            // 0x407863
            *(int32_t *)(v16 - 16) = v19;
            *(int32_t *)(v16 - 20) = (int32_t)&v37;
            *(int32_t *)(v16 - 24) = (int32_t)&v39;
            function_408530((int32_t)&g1224);
            v53 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v54 = *(int32_t *)v53;
            v55 = 0;
            if (v54 != 0) {
                // 0x40788a
                v55 = *(int32_t *)v54;
            }
            // 0x40788c
            v56 = *(int32_t *)(v53 + 8);
            v57 = *(int32_t *)(v55 + 8);
            v58 = v56 / 4;
            v59 = *(int32_t *)(v55 + 4);
            v60 = *(int32_t *)(4 * (v58 - (v58 < v57 ? 0 : v57)) + v59);
            *(int32_t *)(v16 - 28) = v19;
            *v3 = *(int32_t *)(*(int32_t *)(v60 + (4 * v56 & 12)) + 72);
            *(int32_t *)(v16 - 32) = (int32_t)&v36;
            v61 = v16 - 36;
            *(int32_t *)v61 = (int32_t)&v34;
            *v6 = 0;
            function_408530((int32_t)&g1224);
            v62 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v63 = *(int32_t *)v62;
            v41 = 0;
            if (v63 != 0) {
                // 0x4078e0
                v41 = *(int32_t *)v63;
            }
            // 0x4078e2
            v42 = *(int32_t *)(v62 + 8);
            v43 = *(int32_t *)(v41 + 8);
            v44 = v42 / 4;
            v45 = *(int32_t *)(v41 + 4);
            v46 = *(int32_t *)(4 * (v44 - (v44 < v43 ? 0 : v43)) + v45);
            *v7 = *(char *)(*(int32_t *)(v46 + (4 * v42 & 12)) + 4);
            v33 = v61;
        } else {
            // 0x40790a
            v33 = v21;
            if (v29 == 0) {
                // 0x40790e
                *(int32_t *)(v16 - 16) = v19;
                *(int32_t *)(v16 - 20) = (int32_t)&v38;
                v64 = v16 - 24;
                *(int32_t *)v64 = (int32_t)&v35;
                function_408530((int32_t)&g1224);
                v65 = function_408550((int32_t)&g1224, (int32_t)&g1224);
                v40 = *(int32_t *)v65;
                v47 = 0;
                if (v40 != 0) {
                    // 0x407935
                    v47 = *(int32_t *)v40;
                }
                // 0x407937
                v48 = *(int32_t *)(v65 + 8);
                v49 = *(int32_t *)(v47 + 8);
                v50 = v48 / 4;
                v51 = *(int32_t *)(v47 + 4);
                v52 = *(int32_t *)(4 * (v50 - (v50 < v49 ? 0 : v49)) + v51);
                v33 = v64;
                if (*(char *)(*(int32_t *)(v52 + (4 * v48 & 12)) + 128) != 0) {
                    // 0x40795d
                    *v6 = 1;
                    v33 = v64;
                }
            }
        }
        // 0x407967
        v66 = v33;
        *(int32_t *)(v66 - 4) = v19;
        *(int32_t *)(v66 - 8) = (int32_t)&v67;
        v68 = v66 - 12;
        *(int32_t *)v68 = (int32_t)&v69;
        function_408530((int32_t)&g1224);
        v70 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v71 = *(int32_t *)v70;
        v72 = 0;
        if (v71 != 0) {
            // 0x407991
            v72 = *(int32_t *)v71;
        }
        // 0x407993
        v73 = *(int32_t *)(v70 + 8);
        v74 = *(int32_t *)(v72 + 8);
        v75 = v73 / 4;
        v76 = *v8;
        v77 = *(int32_t *)(v72 + 4);
        v78 = *(int32_t *)(4 * (v75 - (v75 < v74 ? 0 : v74)) + v77);
        v79 = *(int32_t *)(*(int32_t *)(v78 + (4 * v73 & 12)) + 76);
        if ((v76 < 0 ? -v76 : v76) < (v79 < 0 ? -v79 : v79)) {
            // 0x4079cf
            *(int32_t *)(v66 - 16) = v19;
            *(int32_t *)(v66 - 20) = (int32_t)&v85;
            *(int32_t *)(v66 - 24) = (int32_t)&v81;
            function_408530((int32_t)&g1224);
            v87 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v88 = *(int32_t *)v87;
            v89 = 0;
            if (v88 != 0) {
                // 0x4079f9
                v89 = *(int32_t *)v88;
            }
            // 0x4079fb
            v90 = *(int32_t *)(v87 + 8);
            v91 = *(int32_t *)(v89 + 8);
            v92 = v90 / 4;
            v93 = *(int32_t *)(v89 + 4);
            v94 = *(int32_t *)(4 * (v92 - (v92 < v91 ? 0 : v91)) + v93);
            *(int32_t *)(v66 - 28) = v19;
            *v8 = *(int32_t *)(*(int32_t *)(v94 + (4 * v90 & 12)) + 76);
            *(int32_t *)(v66 - 32) = (int32_t)&v84;
            v95 = v66 - 36;
            *(int32_t *)v95 = (int32_t)&v82;
            *v9 = 0;
            function_408530((int32_t)&g1224);
            v96 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v97 = *(int32_t *)v96;
            v101 = 0;
            if (v97 != 0) {
                // 0x407a52
                v101 = *(int32_t *)v97;
            }
            // 0x407a54
            v102 = *(int32_t *)(v96 + 8);
            v103 = *(int32_t *)(v101 + 8);
            v104 = v102 / 4;
            v105 = *(int32_t *)(v101 + 4);
            v106 = *(int32_t *)(4 * (v104 - (v104 < v103 ? 0 : v103)) + v105);
            *v7 = *(char *)(*(int32_t *)(v106 + (4 * v102 & 12)) + 4);
            v80 = v95;
        } else {
            // 0x407a7c
            v80 = v68;
            if (v76 == 0) {
                // 0x407a80
                *(int32_t *)(v66 - 16) = v19;
                *(int32_t *)(v66 - 20) = (int32_t)&v86;
                v98 = v66 - 24;
                *(int32_t *)v98 = (int32_t)&v83;
                function_408530((int32_t)&g1224);
                v99 = function_408550((int32_t)&g1224, (int32_t)&g1224);
                v100 = *(int32_t *)v99;
                v107 = 0;
                if (v100 != 0) {
                    // 0x407aaa
                    v107 = *(int32_t *)v100;
                }
                // 0x407aac
                v108 = *(int32_t *)(v99 + 8);
                v109 = *(int32_t *)(v107 + 8);
                v110 = v108 / 4;
                v111 = *(int32_t *)(v107 + 4);
                v112 = *(int32_t *)(4 * (v110 - (v110 < v109 ? 0 : v109)) + v111);
                v80 = v98;
                if (*(char *)(*(int32_t *)(v112 + (4 * v108 & 12)) + 129) != 0) {
                    // 0x407ad2
                    *v9 = 1;
                    v80 = v98;
                }
            }
        }
        // 0x407adc
        v113 = 80;
        v114 = v80;
        v115 = 0;
        *(int32_t *)(v114 - 4) = v19;
        *(int32_t *)(v114 - 8) = (int32_t)&v116;
        v117 = v114 - 12;
        *(int32_t *)v117 = (int32_t)&v118;
        function_408530((int32_t)&g1224);
        v119 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v120 = *(int32_t *)v119;
        v121 = 0;
        if (v120 != 0) {
            // 0x407b12
            v121 = *(int32_t *)v120;
        }
        // 0x407b14
        v122 = *(int32_t *)(v119 + 8);
        v123 = *(int32_t *)(v121 + 8);
        v124 = v122 / 4;
        v125 = *(int32_t *)(v121 + 4);
        v126 = *(int32_t *)(4 * (v124 - (v124 < v123 ? 0 : v123)) + v125);
        v127 = (int32_t *)(v113 + v2);
        v128 = *v127;
        if (v128 < *(int32_t *)(*(int32_t *)(v126 + (4 * v122 & 12)) + v113)) {
            // 0x407b40
            *(int32_t *)(v114 - 16) = v19;
            *(int32_t *)(v114 - 20) = (int32_t)&v134;
            *(int32_t *)(v114 - 24) = (int32_t)&v130;
            function_408530((int32_t)&g1224);
            v136 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v137 = *(int32_t *)v136;
            v138 = 0;
            if (v137 != 0) {
                // 0x407b71
                v138 = *(int32_t *)v137;
            }
            // 0x407b73
            v139 = *(int32_t *)(v136 + 8);
            v140 = *(int32_t *)(v138 + 8);
            v141 = v139 / 4;
            v142 = *(int32_t *)(v138 + 4);
            v143 = *(int32_t *)(4 * (v141 - (v141 < v140 ? 0 : v140)) + v142);
            *(int32_t *)(v114 - 28) = v19;
            *v127 = *(int32_t *)(*(int32_t *)(v143 + (4 * v139 & 12)) + v113);
            *(int32_t *)(v114 - 32) = (int32_t)&v133;
            v144 = v114 - 36;
            *(int32_t *)v144 = (int32_t)&v131;
            *(char *)(v115 + 130 + v2) = 0;
            function_408530((int32_t)&g1224);
            v145 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v146 = *(int32_t *)v145;
            v150 = 0;
            if (v146 != 0) {
                // 0x407bcc
                v150 = *(int32_t *)v146;
            }
            // 0x407bce
            v151 = *(int32_t *)(v145 + 8);
            v152 = *(int32_t *)(v150 + 8);
            v153 = v151 / 4;
            v154 = *(int32_t *)(v150 + 4);
            v155 = *(int32_t *)(4 * (v153 - (v153 < v152 ? 0 : v152)) + v154);
            *v7 = *(char *)(*(int32_t *)(v155 + (4 * v151 & 12)) + 4);
            v129 = v144;
        } else {
            // 0x407bf9
            v129 = v117;
            if (v128 == 0) {
                // 0x407bfd
                *(int32_t *)(v114 - 16) = v19;
                *(int32_t *)(v114 - 20) = (int32_t)&v135;
                v147 = v114 - 24;
                *(int32_t *)v147 = (int32_t)&v132;
                function_408530((int32_t)&g1224);
                v148 = function_408550((int32_t)&g1224, (int32_t)&g1224);
                v149 = *(int32_t *)v148;
                v156 = 0;
                if (v149 != 0) {
                    // 0x407c2e
                    v156 = *(int32_t *)v149;
                }
                // 0x407c30
                v157 = *(int32_t *)(v148 + 8);
                v158 = *(int32_t *)(v156 + 8);
                v159 = v157 / 4;
                v160 = *(int32_t *)(v156 + 4);
                v161 = *(int32_t *)(4 * (v159 - (v159 < v158 ? 0 : v158)) + v160);
                v162 = v115 + 130;
                v129 = v147;
                if (*(char *)(*(int32_t *)(v161 + (4 * v157 & 12)) + v162) != 0) {
                    // 0x407c57
                    *(char *)(v162 + v2) = 1;
                    v129 = v147;
                }
            }
        }
        // 0x407c62
        v163 = v129;
        v164 = v115 + 1;
        v165 = v113 + 4;
        while (v164 != 12) {
            // 0x407ae1
            v113 = v165;
            v114 = v163;
            v115 = v164;
            *(int32_t *)(v114 - 4) = v19;
            *(int32_t *)(v114 - 8) = (int32_t)&v116;
            v117 = v114 - 12;
            *(int32_t *)v117 = (int32_t)&v118;
            function_408530((int32_t)&g1224);
            v119 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v120 = *(int32_t *)v119;
            v121 = 0;
            if (v120 != 0) {
                // 0x407b12
                v121 = *(int32_t *)v120;
            }
            // 0x407b14
            v122 = *(int32_t *)(v119 + 8);
            v123 = *(int32_t *)(v121 + 8);
            v124 = v122 / 4;
            v125 = *(int32_t *)(v121 + 4);
            v126 = *(int32_t *)(4 * (v124 - (v124 < v123 ? 0 : v123)) + v125);
            v127 = (int32_t *)(v113 + v2);
            v128 = *v127;
            if (v128 < *(int32_t *)(*(int32_t *)(v126 + (4 * v122 & 12)) + v113)) {
                // 0x407b40
                *(int32_t *)(v114 - 16) = v19;
                *(int32_t *)(v114 - 20) = (int32_t)&v134;
                *(int32_t *)(v114 - 24) = (int32_t)&v130;
                function_408530((int32_t)&g1224);
                v136 = function_408550((int32_t)&g1224, (int32_t)&g1224);
                v137 = *(int32_t *)v136;
                v138 = 0;
                if (v137 != 0) {
                    // 0x407b71
                    v138 = *(int32_t *)v137;
                }
                // 0x407b73
                v139 = *(int32_t *)(v136 + 8);
                v140 = *(int32_t *)(v138 + 8);
                v141 = v139 / 4;
                v142 = *(int32_t *)(v138 + 4);
                v143 = *(int32_t *)(4 * (v141 - (v141 < v140 ? 0 : v140)) + v142);
                *(int32_t *)(v114 - 28) = v19;
                *v127 = *(int32_t *)(*(int32_t *)(v143 + (4 * v139 & 12)) + v113);
                *(int32_t *)(v114 - 32) = (int32_t)&v133;
                v144 = v114 - 36;
                *(int32_t *)v144 = (int32_t)&v131;
                *(char *)(v115 + 130 + v2) = 0;
                function_408530((int32_t)&g1224);
                v145 = function_408550((int32_t)&g1224, (int32_t)&g1224);
                v146 = *(int32_t *)v145;
                v150 = 0;
                if (v146 != 0) {
                    // 0x407bcc
                    v150 = *(int32_t *)v146;
                }
                // 0x407bce
                v151 = *(int32_t *)(v145 + 8);
                v152 = *(int32_t *)(v150 + 8);
                v153 = v151 / 4;
                v154 = *(int32_t *)(v150 + 4);
                v155 = *(int32_t *)(4 * (v153 - (v153 < v152 ? 0 : v152)) + v154);
                *v7 = *(char *)(*(int32_t *)(v155 + (4 * v151 & 12)) + 4);
                v129 = v144;
            } else {
                // 0x407bf9
                v129 = v117;
                if (v128 == 0) {
                    // 0x407bfd
                    *(int32_t *)(v114 - 16) = v19;
                    *(int32_t *)(v114 - 20) = (int32_t)&v135;
                    v147 = v114 - 24;
                    *(int32_t *)v147 = (int32_t)&v132;
                    function_408530((int32_t)&g1224);
                    v148 = function_408550((int32_t)&g1224, (int32_t)&g1224);
                    v149 = *(int32_t *)v148;
                    v156 = 0;
                    if (v149 != 0) {
                        // 0x407c2e
                        v156 = *(int32_t *)v149;
                    }
                    // 0x407c30
                    v157 = *(int32_t *)(v148 + 8);
                    v158 = *(int32_t *)(v156 + 8);
                    v159 = v157 / 4;
                    v160 = *(int32_t *)(v156 + 4);
                    v161 = *(int32_t *)(4 * (v159 - (v159 < v158 ? 0 : v158)) + v160);
                    v162 = v115 + 130;
                    v129 = v147;
                    if (*(char *)(*(int32_t *)(v161 + (4 * v157 & 12)) + v162) != 0) {
                        // 0x407c57
                        *(char *)(v162 + v2) = 1;
                        v129 = v147;
                    }
                }
            }
            // 0x407c62
            v163 = v129;
            v164 = v115 + 1;
            v165 = v113 + 4;
        }
        // 0x407c72
        v166 = v18 - 1;
        __frontend_reg_store_fpr(v166, (float80_t)*v10);
        v167 = v163 - 8;
        __frontend_reg_store_fpr(v166, __frontend_reg_load_fpr(v166));
        *(float64_t *)v167 = (float64_t)__frontend_reg_load_fpr(v166);
        _fabs((float64_t)(int64_t)&g1224);
        __frontend_reg_store_fpr(v18, __frontend_reg_load_fpr(v18));
        *(int32_t *)(v163 - 4) = v19;
        __frontend_reg_store_fpr(v18, __frontend_reg_load_fpr(v18));
        *(int32_t *)v167 = (int32_t)&v168;
        v169 = __frontend_reg_load_fpr(v18);
        v170 = v163 - 12;
        *(int32_t *)v170 = (int32_t)&v171;
        function_408530((int32_t)(float32_t)v169);
        v172 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v173 = *(int32_t *)v172;
        v174 = 0;
        if (v173 != 0) {
            // 0x407cc6
            v174 = *(int32_t *)v173;
        }
        // 0x407cc8
        v175 = v18 + 1;
        v176 = *(int32_t *)(v172 + 8);
        v177 = *(int32_t *)(v174 + 8);
        v178 = v176 / 4;
        v179 = *(int32_t *)(v174 + 4);
        v180 = *(int32_t *)(4 * (v178 - (v178 < v177 ? 0 : v177)) + v179);
        v181 = *(float32_t *)(*(int32_t *)(v180 + (4 * v176 & 12)) + 144);
        __frontend_reg_store_fpr(v18, (float80_t)v181);
        v182 = v163 - 20;
        __frontend_reg_store_fpr(v18, __frontend_reg_load_fpr(v18));
        *(float64_t *)v182 = (float64_t)__frontend_reg_load_fpr(v18);
        _fabs((float64_t)(int64_t)&g1224);
        __frontend_reg_store_fpr(v175, __frontend_reg_load_fpr(v175));
        __frontend_reg_store_fpr(v175, __frontend_reg_load_fpr(v175));
        v183 = __frontend_reg_load_fpr(v175);
        __frontend_reg_store_fpr(v175, v169);
        __frontend_reg_store_fpr(v18, v183);
        __frontend_reg_load_fpr(v18);
        __frontend_reg_load_fpr(v175);
        v184 = v170;
        if ((v2 & 0x4100) == 0) {
            // 0x407d20
            *(int32_t *)(v163 - 16) = v19;
            *(int32_t *)v182 = (int32_t)&v186;
            v184 = v163 - 24;
            *(int32_t *)v184 = (int32_t)&v185;
            function_408530((int32_t)&g1224);
            v187 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v188 = *(int32_t *)v187;
            v189 = 0;
            if (v188 != 0) {
                // 0x407d48
                v189 = *(int32_t *)v188;
            }
            // 0x407d4a
            v190 = *(int32_t *)(v187 + 8);
            v191 = *(int32_t *)(v189 + 8);
            v192 = v190 / 4;
            v193 = *(int32_t *)(v189 + 4);
            v194 = *(int32_t *)(4 * (v192 - (v192 < v191 ? 0 : v191)) + v193);
            v195 = *(float32_t *)(*(int32_t *)(v194 + (4 * v190 & 12)) + 144);
            __frontend_reg_store_fpr(v175, (float80_t)v195);
            *v10 = (float32_t)__frontend_reg_load_fpr(v175);
        }
        // 0x407d73
        v196 = v18 + 2;
        v197 = v184;
        __frontend_reg_store_fpr(v175, (float80_t)*v11);
        v198 = v197 - 8;
        __frontend_reg_store_fpr(v175, __frontend_reg_load_fpr(v175));
        *(float64_t *)v198 = (float64_t)__frontend_reg_load_fpr(v175);
        _fabs((float64_t)(int64_t)&g1224);
        __frontend_reg_store_fpr(v196, __frontend_reg_load_fpr(v196));
        *(int32_t *)(v197 - 4) = v19;
        __frontend_reg_store_fpr(v196, __frontend_reg_load_fpr(v196));
        *(int32_t *)v198 = (int32_t)&v199;
        v200 = __frontend_reg_load_fpr(v196);
        v201 = v197 - 12;
        *(int32_t *)v201 = (int32_t)&v202;
        function_408530((int32_t)(float32_t)v200);
        v203 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v204 = *(int32_t *)v203;
        v205 = 0;
        if (v204 != 0) {
            // 0x407dca
            v205 = *(int32_t *)v204;
        }
        // 0x407dcc
        v206 = v18 + 3;
        v207 = *(int32_t *)(v203 + 8);
        v208 = *(int32_t *)(v205 + 8);
        v209 = v207 / 4;
        v210 = *(int32_t *)(v205 + 4);
        v211 = *(int32_t *)(4 * (v209 - (v209 < v208 ? 0 : v208)) + v210);
        v212 = *(float32_t *)(*(int32_t *)(v211 + (4 * v207 & 12)) + 148);
        __frontend_reg_store_fpr(v196, (float80_t)v212);
        v213 = v197 - 20;
        __frontend_reg_store_fpr(v196, __frontend_reg_load_fpr(v196));
        *(float64_t *)v213 = (float64_t)__frontend_reg_load_fpr(v196);
        _fabs((float64_t)(int64_t)&g1224);
        __frontend_reg_store_fpr(v206, __frontend_reg_load_fpr(v206));
        __frontend_reg_store_fpr(v206, __frontend_reg_load_fpr(v206));
        v214 = __frontend_reg_load_fpr(v206);
        __frontend_reg_store_fpr(v206, v200);
        __frontend_reg_store_fpr(v196, v214);
        __frontend_reg_load_fpr(v196);
        __frontend_reg_load_fpr(v206);
        v215 = v201;
        if ((v2 & 0x4100) == 0) {
            // 0x407e21
            *(int32_t *)(v197 - 16) = v19;
            *(int32_t *)v213 = (int32_t)&v216;
            v215 = v197 - 24;
            *(int32_t *)v215 = (int32_t)&v217;
            function_408530((int32_t)&g1224);
            v218 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v219 = *(int32_t *)v218;
            v220 = 0;
            if (v219 != 0) {
                // 0x407e4f
                v220 = *(int32_t *)v219;
            }
            // 0x407e51
            v221 = *(int32_t *)(v218 + 8);
            v222 = *(int32_t *)(v220 + 8);
            v223 = v221 / 4;
            v224 = *(int32_t *)(v220 + 4);
            v225 = *(int32_t *)(4 * (v223 - (v223 < v222 ? 0 : v222)) + v224);
            v226 = *(float32_t *)(*(int32_t *)(v225 + (4 * v221 & 12)) + 148);
            __frontend_reg_store_fpr(v206, (float80_t)v226);
            *v11 = (float32_t)__frontend_reg_load_fpr(v206);
        }
        // 0x407e7a
        v227 = v18 ^ -4;
        v228 = v215;
        __frontend_reg_store_fpr(v206, (float80_t)*v12);
        v229 = v228 - 8;
        __frontend_reg_store_fpr(v206, __frontend_reg_load_fpr(v206));
        *(float64_t *)v229 = (float64_t)__frontend_reg_load_fpr(v206);
        _fabs((float64_t)(int64_t)&g1224);
        __frontend_reg_store_fpr(v227, __frontend_reg_load_fpr(v227));
        *(int32_t *)(v228 - 4) = v19;
        __frontend_reg_store_fpr(v227, __frontend_reg_load_fpr(v227));
        *(int32_t *)v229 = (int32_t)&v230;
        v231 = __frontend_reg_load_fpr(v227);
        v232 = v228 - 12;
        *(int32_t *)v232 = (int32_t)&v233;
        function_408530((int32_t)(float32_t)v231);
        v234 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v235 = *(int32_t *)v234;
        v236 = 0;
        if (v235 != 0) {
            // 0x407ed1
            v236 = *(int32_t *)v235;
        }
        // 0x407ed3
        v237 = v18 - 3;
        v238 = *(int32_t *)(v234 + 8);
        v239 = *(int32_t *)(v236 + 8);
        v240 = v238 / 4;
        v241 = *(int32_t *)(v236 + 4);
        v242 = *(int32_t *)(4 * (v240 - (v240 < v239 ? 0 : v239)) + v241);
        v243 = *(float32_t *)(*(int32_t *)(v242 + (4 * v238 & 12)) + 152);
        __frontend_reg_store_fpr(v227, (float80_t)v243);
        v244 = v228 - 20;
        __frontend_reg_store_fpr(v227, __frontend_reg_load_fpr(v227));
        *(float64_t *)v244 = (float64_t)__frontend_reg_load_fpr(v227);
        _fabs((float64_t)(int64_t)&g1224);
        __frontend_reg_store_fpr(v237, __frontend_reg_load_fpr(v237));
        __frontend_reg_store_fpr(v237, __frontend_reg_load_fpr(v237));
        v245 = __frontend_reg_load_fpr(v237);
        __frontend_reg_store_fpr(v237, v231);
        __frontend_reg_store_fpr(v227, v245);
        __frontend_reg_load_fpr(v227);
        __frontend_reg_load_fpr(v237);
        v246 = v232;
        if ((v2 & 0x4100) == 0) {
            // 0x407f28
            *(int32_t *)(v228 - 16) = v19;
            *(int32_t *)v244 = (int32_t)&v247;
            v246 = v228 - 24;
            *(int32_t *)v246 = (int32_t)&v248;
            function_408530((int32_t)&g1224);
            v249 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v250 = *(int32_t *)v249;
            v251 = 0;
            if (v250 != 0) {
                // 0x407f56
                v251 = *(int32_t *)v250;
            }
            // 0x407f58
            v252 = *(int32_t *)(v249 + 8);
            v253 = *(int32_t *)(v251 + 8);
            v254 = v252 / 4;
            v255 = *(int32_t *)(v251 + 4);
            v256 = *(int32_t *)(4 * (v254 - (v254 < v253 ? 0 : v253)) + v255);
            v257 = *(float32_t *)(*(int32_t *)(v256 + (4 * v252 & 12)) + 152);
            __frontend_reg_store_fpr(v237, (float80_t)v257);
            *v12 = (float32_t)__frontend_reg_load_fpr(v237);
        }
        // 0x407f81
        v258 = v18 - 2;
        v259 = v246;
        __frontend_reg_store_fpr(v237, (float80_t)*v13);
        v260 = v259 - 8;
        __frontend_reg_store_fpr(v237, __frontend_reg_load_fpr(v237));
        *(float64_t *)v260 = (float64_t)__frontend_reg_load_fpr(v237);
        _fabs((float64_t)(int64_t)&g1224);
        __frontend_reg_store_fpr(v258, __frontend_reg_load_fpr(v258));
        *(int32_t *)(v259 - 4) = v19;
        __frontend_reg_store_fpr(v258, __frontend_reg_load_fpr(v258));
        *(int32_t *)v260 = (int32_t)&v261;
        v262 = __frontend_reg_load_fpr(v258);
        v263 = v259 - 12;
        *(int32_t *)v263 = (int32_t)&v264;
        function_408530((int32_t)(float32_t)v262);
        v265 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v266 = *(int32_t *)v265;
        v267 = 0;
        if (v266 != 0) {
            // 0x407fd8
            v267 = *(int32_t *)v266;
        }
        // 0x407fda
        v268 = *(int32_t *)(v265 + 8);
        v269 = *(int32_t *)(v267 + 8);
        v270 = v268 / 4;
        v271 = *(int32_t *)(v267 + 4);
        v272 = *(int32_t *)(4 * (v270 - (v270 < v269 ? 0 : v269)) + v271);
        v273 = *(float32_t *)(*(int32_t *)(v272 + (4 * v268 & 12)) + 156);
        __frontend_reg_store_fpr(v258, (float80_t)v273);
        v274 = v259 - 20;
        __frontend_reg_store_fpr(v258, __frontend_reg_load_fpr(v258));
        *(float64_t *)v274 = (float64_t)__frontend_reg_load_fpr(v258);
        _fabs((float64_t)(int64_t)&g1224);
        __frontend_reg_store_fpr(v166, __frontend_reg_load_fpr(v166));
        __frontend_reg_store_fpr(v166, __frontend_reg_load_fpr(v166));
        v275 = __frontend_reg_load_fpr(v166);
        __frontend_reg_store_fpr(v166, v262);
        __frontend_reg_store_fpr(v258, v275);
        __frontend_reg_load_fpr(v258);
        __frontend_reg_load_fpr(v166);
        v276 = v263;
        if ((v2 & 0x4100) == 0) {
            // 0x40802f
            *(int32_t *)(v259 - 16) = v19;
            *(int32_t *)v274 = (int32_t)&v277;
            v276 = v259 - 24;
            *(int32_t *)v276 = (int32_t)&v278;
            function_408530((int32_t)&g1224);
            v279 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v280 = *(int32_t *)v279;
            v281 = 0;
            if (v280 != 0) {
                // 0x40805d
                v281 = *(int32_t *)v280;
            }
            // 0x40805f
            v282 = *(int32_t *)(v279 + 8);
            v283 = *(int32_t *)(v281 + 8);
            v284 = v282 / 4;
            v285 = *(int32_t *)(v281 + 4);
            v286 = *(int32_t *)(4 * (v284 - (v284 < v283 ? 0 : v283)) + v285);
            v287 = *(float32_t *)(*(int32_t *)(v286 + (4 * v282 & 12)) + 156);
            __frontend_reg_store_fpr(v166, (float80_t)v287);
            *v13 = (float32_t)__frontend_reg_load_fpr(v166);
        }
        // 0x408088
        v288 = v276;
        __frontend_reg_store_fpr(v166, (float80_t)*v14);
        v289 = v288 - 8;
        __frontend_reg_store_fpr(v166, __frontend_reg_load_fpr(v166));
        *(float64_t *)v289 = (float64_t)__frontend_reg_load_fpr(v166);
        _fabs((float64_t)(int64_t)&g1224);
        __frontend_reg_store_fpr(v18, __frontend_reg_load_fpr(v18));
        *(int32_t *)(v288 - 4) = v19;
        __frontend_reg_store_fpr(v18, __frontend_reg_load_fpr(v18));
        *(int32_t *)v289 = (int32_t)&v290;
        v291 = __frontend_reg_load_fpr(v18);
        v292 = v288 - 12;
        *(int32_t *)v292 = (int32_t)&v293;
        function_408530((int32_t)(float32_t)v291);
        v294 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v295 = *(int32_t *)v294;
        v296 = 0;
        if (v295 != 0) {
            // 0x4080df
            v296 = *(int32_t *)v295;
        }
        // 0x4080e1
        v297 = *(int32_t *)(v294 + 8);
        v298 = *(int32_t *)(v296 + 8);
        v299 = v297 / 4;
        v300 = *(int32_t *)(v296 + 4);
        v301 = *(int32_t *)(4 * (v299 - (v299 < v298 ? 0 : v298)) + v300);
        v302 = *(float32_t *)(*(int32_t *)(v301 + (4 * v297 & 12)) + 160);
        __frontend_reg_store_fpr(v18, (float80_t)v302);
        v303 = v288 - 20;
        __frontend_reg_store_fpr(v18, __frontend_reg_load_fpr(v18));
        *(float64_t *)v303 = (float64_t)__frontend_reg_load_fpr(v18);
        _fabs((float64_t)(int64_t)&g1224);
        __frontend_reg_store_fpr(v175, __frontend_reg_load_fpr(v175));
        __frontend_reg_store_fpr(v175, __frontend_reg_load_fpr(v175));
        v304 = __frontend_reg_load_fpr(v175);
        __frontend_reg_store_fpr(v175, v291);
        __frontend_reg_store_fpr(v18, v304);
        __frontend_reg_load_fpr(v18);
        __frontend_reg_load_fpr(v175);
        v305 = v292;
        if ((v2 & 0x4100) == 0) {
            // 0x408136
            *(int32_t *)(v288 - 16) = v19;
            *(int32_t *)v303 = (int32_t)&v306;
            v305 = v288 - 24;
            *(int32_t *)v305 = (int32_t)&v307;
            function_408530((int32_t)&g1224);
            v308 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v309 = *(int32_t *)v308;
            v310 = 0;
            if (v309 != 0) {
                // 0x408164
                v310 = *(int32_t *)v309;
            }
            // 0x408166
            v311 = *(int32_t *)(v308 + 8);
            v312 = *(int32_t *)(v310 + 8);
            v313 = v311 / 4;
            v314 = *(int32_t *)(v310 + 4);
            v315 = *(int32_t *)(4 * (v313 - (v313 < v312 ? 0 : v312)) + v314);
            v316 = *(float32_t *)(*(int32_t *)(v315 + (4 * v311 & 12)) + 160);
            __frontend_reg_store_fpr(v175, (float80_t)v316);
            *v14 = (float32_t)__frontend_reg_load_fpr(v175);
        }
        // 0x40818f
        v317 = v305;
        __frontend_reg_store_fpr(v175, (float80_t)*v15);
        v318 = v317 - 8;
        __frontend_reg_store_fpr(v175, __frontend_reg_load_fpr(v175));
        *(float64_t *)v318 = (float64_t)__frontend_reg_load_fpr(v175);
        _fabs((float64_t)(int64_t)&g1224);
        __frontend_reg_store_fpr(v196, __frontend_reg_load_fpr(v196));
        *(int32_t *)(v317 - 4) = v19;
        __frontend_reg_store_fpr(v196, __frontend_reg_load_fpr(v196));
        *(int32_t *)v318 = (int32_t)&v319;
        v320 = __frontend_reg_load_fpr(v196);
        v321 = v317 - 12;
        *(int32_t *)v321 = (int32_t)&v322;
        function_408530((int32_t)(float32_t)v320);
        v323 = function_408550((int32_t)&g1224, (int32_t)&g1224);
        v324 = *(int32_t *)v323;
        v325 = 0;
        if (v324 != 0) {
            // 0x4081e6
            v325 = *(int32_t *)v324;
        }
        // 0x4081e8
        v326 = *(int32_t *)(v323 + 8);
        v327 = *(int32_t *)(v325 + 8);
        v328 = v326 / 4;
        v329 = *(int32_t *)(v325 + 4);
        v330 = *(int32_t *)(4 * (v328 - (v328 < v327 ? 0 : v327)) + v329);
        v331 = *(float32_t *)(*(int32_t *)(v330 + (4 * v326 & 12)) + 164);
        __frontend_reg_store_fpr(v196, (float80_t)v331);
        v332 = v317 - 20;
        __frontend_reg_store_fpr(v196, __frontend_reg_load_fpr(v196));
        *(float64_t *)v332 = (float64_t)__frontend_reg_load_fpr(v196);
        _fabs((float64_t)(int64_t)&g1224);
        __frontend_reg_store_fpr(v206, __frontend_reg_load_fpr(v206));
        __frontend_reg_store_fpr(v206, __frontend_reg_load_fpr(v206));
        v333 = __frontend_reg_load_fpr(v206);
        __frontend_reg_store_fpr(v206, v320);
        __frontend_reg_store_fpr(v196, v333);
        __frontend_reg_load_fpr(v196);
        __frontend_reg_load_fpr(v206);
        v334 = v321;
        if ((v2 & 0x4100) == 0) {
            // 0x40823d
            *(int32_t *)(v317 - 16) = v19;
            *(int32_t *)v332 = (int32_t)&v335;
            v334 = v317 - 24;
            *(int32_t *)v334 = (int32_t)&v336;
            function_408530((int32_t)&g1224);
            v337 = function_408550((int32_t)&g1224, (int32_t)&g1224);
            v338 = *(int32_t *)v337;
            v339 = 0;
            if (v338 != 0) {
                // 0x40826b
                v339 = *(int32_t *)v338;
            }
            // 0x40826d
            v340 = *(int32_t *)(v337 + 8);
            v341 = *(int32_t *)(v339 + 8);
            v342 = v340 / 4;
            v343 = *(int32_t *)(v339 + 4);
            v344 = *(int32_t *)(4 * (v342 - (v342 < v341 ? 0 : v341)) + v343);
            v345 = *(float32_t *)(*(int32_t *)(v344 + (4 * v340 & 12)) + 164);
            __frontend_reg_store_fpr(v206, (float80_t)v345);
            *v15 = (float32_t)__frontend_reg_load_fpr(v206);
        }
        // 0x408296
        result = v19 + 1;
    }
    // 0x4082ab
    return result;
}