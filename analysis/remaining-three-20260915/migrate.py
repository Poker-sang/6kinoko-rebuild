"""Replace verified operation bodies; leave unresolved receiverless callers visible."""
from pathlib import Path
import re

p = Path('src/decompiled/6kinoko_rebuilt.c')
s = p.read_text(encoding='utf-8')

def body(name, replacement):
    global s
    m = re.search(r'^int32_t ' + name + r'\([^;{}]*\)\s*\{', s, re.M)
    assert m, name
    start = m.end()
    # These selected generated bodies have balanced braces in comments/strings.
    depth, end = 1, start
    while depth:
        depth += (s[end] == '{') - (s[end] == '}')
        end += 1
    s = s[:start] + '\n' + replacement + '\n' + s[end-1:]

body('function_491500', '    return kinoko_sq_get_vararg(retdec_stack_vm(), a1, a2, a3);')
body('function_494990', '    return kinoko_sq_clone(retdec_stack_vm(), a1, a2);')
m = re.search(r'^int32_t function_494da0\([^;{}]*\)\s*\{', s, re.M)
prefix = s[m.end():s.index('    int32_t v1 = (int32_t)a7;', m.end())].strip('\n')
body('function_494da0', prefix + '\n    return kinoko_sq_foreach(vm, (int32_t)(intptr_t)a1,\n'
     '        (int32_t)(intptr_t)a2, (int32_t)(intptr_t)a3, a4, a5, a6, a7);')
assert s.count('static int32_t function_498440_this(') == 2
s = s.replace('static int32_t function_498440_this(', 'int32_t function_498440_this(')
s = s.replace('static int32_t function_48d390_this(', 'int32_t function_48d390_this(')
# Give the live interpreter the same verified vararg operation, including errors.
start = s.index('            case 29: { /* GETVARGV */')
end = s.index('            case 30:', start)
s = s[:start] + '''            case 29: { /* GETVARGV */
                int32_t ci = *(int32_t *)(intptr_t)(vm + 132);
                src = (int32_t *)(intptr_t)retdec_clean_vm_slot(vm, arg1);
                dst = (int32_t *)(intptr_t)retdec_clean_vm_slot(vm, arg0);
                if (src == NULL || dst == NULL || ci == 0 ||
                    !kinoko_sq_get_vararg(vm, (int32_t)(intptr_t)dst,
                                         (int32_t)(intptr_t)src, ci))
                    goto clean_execute_failure;
                break;
            }

''' + s[end:]
p.write_text(s, encoding='utf-8')
