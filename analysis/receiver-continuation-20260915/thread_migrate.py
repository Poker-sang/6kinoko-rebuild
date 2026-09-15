from pathlib import Path
import re
p=Path('src/decompiled/6kinoko_rebuilt.c')
s=p.read_text(encoding='utf-8')
lex=re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
def span(name):
    mask=lex.sub(lambda m: ''.join('\n' if c=='\n' else ' ' for c in m[0]),s)
    m=re.search(r'^(?:static )?int32_t '+name+r'\([^;{}]*\)\s*\{',mask,re.M)
    assert m,name
    e=m.end(); d=1
    while d: d+=(mask[e]=='{')-(mask[e]=='}');e+=1
    return m.start(),m.end(),e
def body(name,code):
    global s
    _,b,e=span(name)
    s=s[:b]+'\n'+code+'\n'+s[e-1:]
body('function_48adb0','    return kinoko_sq_wakeup(a1, a2, a3, a4);')
body('function_490c40','    return kinoko_sq_suspend(retdec_stack_vm());')
body('function_48ada0','    return kinoko_sq_suspend(a1);')
body('function_4a1c60','    return kinoko_sq_suspend(retdec_stack_vm());')
body('function_4a1c10','    return kinoko_sq_newthread(a1);')
body('function_4a2c80','    return kinoko_sq_thread_call(a1);')
body('function_4a2df0','    return kinoko_sq_thread_wakeup(a1);')
body('function_4a2fa0','    return kinoko_sq_thread_status(a1);')

# The only external old Execute call was in the replaced sq_wakeupvm.
a,b,e=span('function_495360')
outside=s[:a]+s[e:]
assert len(re.findall(r'\bfunction_495360\s*\(',outside))==1 # prototype only
s=outside
s=re.sub(r'^int32_t function_495360\([^;{}]*\);\n','',s,flags=re.M)

a,b,e=span('retdec_execute_clean_vm')
segment=s[a:e]
segment=segment.replace('int32_t stackbase, int32_t outres, int32_t raiseerror)',
                        'int32_t stackbase, int32_t outres, int32_t raiseerror, int32_t resume)')
segment=segment.replace('if (vm == 0 || closure == NULL || closure[0] != 0x08000100 ||\n'
    '        closure[1] == 0 || nargs < 0 || stackbase < 0)',
    'if (vm == 0 || (!resume && (closure == NULL || closure[0] != 0x08000100 ||\n'
    '        closure[1] == 0 || nargs < 0 || stackbase < 0)))')
start=segment.index('    result = function_494120(')
end=segment.index('\nclean_execute_loop:',start)
original=segment[start:end]
segment=segment[:start]+'''    if (resume) {
        current_ci = *(int32_t *)(intptr_t)(vm + 132);
        if (!current_ci || !*(int32_t *)(intptr_t)(vm + 148)) {
            *(int32_t *)(intptr_t)(vm + 144) -= 1;
            return 0;
        }
        /* ET_RESUME_VM: root, traps and varargs belong to the suspended frame. */
        traps = *(int32_t *)(intptr_t)(vm + 160);
        *(int32_t *)(intptr_t)(current_ci + 40) = *(int32_t *)(intptr_t)(vm + 152);
        memcpy((void *)(intptr_t)(current_ci + 44), (void *)(intptr_t)(vm + 164), 4);
        *(int32_t *)(intptr_t)(vm + 148) = 0;
    } else {
'''+original+'''    }
'''+segment[end:]
segment=segment.replace('arg2, arg3, raiseerror);','arg2, arg3, raiseerror, outres, traps);')
segment=segment.replace('                if (result == 2)', '''                if (result == 3) {
                    *(int32_t *)(intptr_t)(vm + 144) -= 1;
                    return 1;
                }
                if (result == 2)''',1)
segment=segment.replace('                if (result == 2 || result == 1)', '''                if (result == 3) {
                    *(int32_t *)(intptr_t)(vm + 144) -= 1;
                    return 1;
                }
                if (result == 2 || result == 1)''',1)
s=s[:a]+segment+s[e:]
# Regular call remains ET_CALL.
s=s.replace('a3, a4, a5, a6);\n            /* function_495360',
            'a3, a4, a5, a6, 0);\n            /* function_495360')
a,b,e=span('retdec_clean_vm_call')
segment=s[a:e].replace('int32_t raiseerror)', 'int32_t raiseerror, int32_t outres, int32_t traps)')
old='''                *(int32_t *)(intptr_t)(vm + 148) = 1;
                *(int32_t *)(intptr_t)(vm + 152) = arg0;
                *(int32_t *)(intptr_t)(vm + 156) = 0;'''
new='''                int32_t ci = *(int32_t *)(intptr_t)(vm + 132);
                *(int32_t *)(intptr_t)(vm + 148) = 1;
                *(int32_t *)(intptr_t)(vm + 152) = *(int32_t *)(intptr_t)(ci + 40);
                *(int32_t *)(intptr_t)(vm + 156) = arg0;
                *(int32_t *)(intptr_t)(vm + 160) = traps;
                memcpy((void *)(intptr_t)(vm + 164), (void *)(intptr_t)(ci + 44), 4);
                retdec_squirrel_assign((int32_t *)(intptr_t)outres, native_result);
                retdec_release_squirrel_value(native_result);
                return 3; /* return to the API, without executing another opcode */'''
assert old in segment
segment=segment.replace(old,new).replace('return suspend != 0 ? 1 : 1;', 'return 1;')
s=s[:a]+segment+s[e:]
s+='''
/* Explicit VM boundaries for C++ coroutine adapters. Nested calls restore the
   caller VM; the same reconstructed interpreter handles call and resume. */
int32_t kinoko_call_game_vm(int32_t vm, int32_t nargs, int32_t retval, int32_t raiseerror) {
    int32_t previous = retdec_active_vm;
    retdec_active_vm = vm;
    int32_t result = function_48ace0(vm, nargs, retval, raiseerror);
    retdec_active_vm = previous;
    return result;
}

int32_t kinoko_resume_game_vm(int32_t vm, int32_t out, int32_t raiseerror) {
    int32_t previous = retdec_active_vm;
    retdec_active_vm = vm;
    int32_t result = retdec_execute_clean_vm(0, 0, 0, 0, out, raiseerror, 1);
    retdec_active_vm = previous;
    return result;
}
'''
p.write_text(s,encoding='utf-8')
