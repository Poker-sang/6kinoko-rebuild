/*
 * Compatibility entry points for the old MSVC runtime names emitted by
 * RetDec. The decompiled translation unit uses the original symbol spelling
 * as a C identifier, so the leading underscore is intentional here.
 */

#define CoInitialize retdec_decl_CoInitialize
#define CoUninitialize retdec_decl_CoUninitialize
#define CoCreateInstance retdec_decl_CoCreateInstance
#include <windows.h>
#undef CoInitialize
#undef CoUninitialize
#undef CoCreateInstance

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#define RETDEC_NOINLINE __declspec(noinline)
#else
#define RETDEC_NOINLINE
#endif

typedef HRESULT (WINAPI *retdec_co_initialize_fn)(LPVOID);
typedef void (WINAPI *retdec_co_uninitialize_fn)(void);
typedef HRESULT (WINAPI *retdec_co_create_instance_fn)(
    const GUID *, void *, DWORD, const GUID *, LPVOID *);
typedef LPVOID (WINAPI *retdec_direct3d_create9_fn)(UINT);
typedef DWORD (WINAPI *retdec_time_get_time_fn)(void);
typedef UINT (WINAPI *retdec_time_begin_period_fn)(UINT);

static HMODULE retdec_module(const char *name)
{
    HMODULE module = GetModuleHandleA(name);
    if (module == NULL) {
        module = LoadLibraryA(name);
    }
    return module;
}

static FARPROC retdec_proc(const char *module_name, const char *proc_name)
{
    HMODULE module = retdec_module(module_name);
    return module == NULL ? NULL : GetProcAddress(module, proc_name);
}

/* The original binary uses DirectInput8. RetDec retained only the first
   DWORD of each adjacent GUID, so the wrapper supplies the complete values. */
static const GUID retdec_clsid_direct_input8 = {
    0x25e609e4, 0xb259, 0x11cf,
    { 0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 }
};

static const GUID retdec_iid_direct_input8 = {
    0xbf798030, 0x483a, 0x4da2,
    { 0xaa, 0x99, 0x5d, 0x64, 0xed, 0x36, 0x97, 0x00 }
};

int32_t *Direct3DCreate9(int32_t version)
{
    retdec_direct3d_create9_fn create9 =
        (retdec_direct3d_create9_fn)retdec_proc("d3d9.dll", "Direct3DCreate9");
    if (create9 == NULL) {
        return NULL;
    }
    return (int32_t *)create9((UINT)version);
}

int32_t CoInitialize(void *reserved)
{
    retdec_co_initialize_fn initialize =
        (retdec_co_initialize_fn)retdec_proc("ole32.dll", "CoInitialize");
    return initialize == NULL ? (int32_t)E_FAIL : (int32_t)initialize(reserved);
}

void CoUninitialize(void)
{
    retdec_co_uninitialize_fn uninitialize =
        (retdec_co_uninitialize_fn)retdec_proc("ole32.dll", "CoUninitialize");
    if (uninitialize != NULL) {
        uninitialize();
    }
}

int32_t CoCreateInstance(
    const void *rclsid,
    void *outer,
    uint32_t context,
    const void *riid,
    void **result)
{
    retdec_co_create_instance_fn create_instance =
        (retdec_co_create_instance_fn)retdec_proc("ole32.dll", "CoCreateInstance");
    (void)rclsid;
    (void)outer;
    (void)riid;
    if (create_instance == NULL || result == NULL) {
        return (int32_t)E_FAIL;
    }
    return (int32_t)create_instance(
        &retdec_clsid_direct_input8,
        NULL,
        (DWORD)context,
        &retdec_iid_direct_input8,
        (LPVOID *)result);
}

uint32_t timeGetTime(void)
{
    retdec_time_get_time_fn get_time =
        (retdec_time_get_time_fn)retdec_proc("winmm.dll", "timeGetTime");
    return get_time == NULL ? GetTickCount() : (uint32_t)get_time();
}

uint32_t timeBeginPeriod(uint32_t period)
{
    retdec_time_begin_period_fn begin_period =
        (retdec_time_begin_period_fn)retdec_proc("winmm.dll", "timeBeginPeriod");
    return begin_period == NULL ? 0u : (uint32_t)begin_period((UINT)period);
}

void *_malloc(size_t size)
{
    return malloc(size);
}

void *_calloc(size_t count, size_t size)
{
    return calloc(count, size);
}

void *_realloc(void *memory, size_t size)
{
    return realloc(memory, size);
}

void _free(void *memory)
{
    free(memory);
}

void *_memcpy(void *destination, const void *source, size_t count)
{
    return memmove(destination, source, count);
}

void *_memset(void *destination, int value, size_t count)
{
    return memset(destination, value, count);
}

void *_memchr(const void *buffer, int value, size_t count)
{
    return (void *)memchr(buffer, value, count);
}

int _memcpy_s(void *destination, size_t destination_size,
              const void *source, size_t source_size)
{
    return memcpy_s(destination, destination_size, source, source_size);
}

int _memmove_s(void *destination, size_t destination_size,
               const void *source, size_t source_size)
{
    return memmove_s(destination, destination_size, source, source_size);
}

int _strcpy_s(char *destination, size_t destination_size, const char *source)
{
    return strcpy_s(destination, destination_size, source);
}

int _strcat_s(char *destination, size_t destination_size, const char *source)
{
    return strcat_s(destination, destination_size, source);
}

int _strncpy_s(char *destination, size_t destination_size,
               const char *source, size_t source_count)
{
    return strncpy_s(destination, destination_size, source, source_count);
}

char *_strchr(const char *text, int value)
{
    return (char *)strchr(text, value);
}

char *_strrchr(const char *text, int value)
{
    return (char *)strrchr(text, value);
}

size_t _strcspn(const char *text, const char *reject)
{
    return strcspn(text, reject);
}

char *_strtok(char *text, const char *delimiters)
{
    return strtok(text, delimiters);
}

char *_strtok_s(char *text, const char *delimiters, char **context)
{
    return strtok_s(text, delimiters, context);
}

int __stricmp(const char *left, const char *right)
{
    return _stricmp(left, right);
}

int _isalnum(int value) { return isalnum((unsigned char)value); }
int _isalpha(int value) { return isalpha((unsigned char)value); }
int _iscntrl(int value) { return iscntrl((unsigned char)value); }
int _isdigit(int value) { return isdigit((unsigned char)value); }
int _isprint(int value) { return isprint((unsigned char)value); }
int _ispunct(int value) { return ispunct((unsigned char)value); }
int _isspace(int value) { return isspace((unsigned char)value); }
int _islower(int value) { return islower((unsigned char)value); }
int _isupper(int value) { return isupper((unsigned char)value); }
int _isxdigit(int value) { return isxdigit((unsigned char)value); }

struct lconv *_localeconv(void)
{
    return localeconv();
}

size_t _fread(void *buffer, size_t size, size_t count, FILE *stream)
{
    return fread(buffer, size, count, stream);
}

size_t _fwrite(const void *buffer, size_t size, size_t count, FILE *stream)
{
    return fwrite(buffer, size, count, stream);
}

int _fseek(FILE *stream, long offset, int origin)
{
    return fseek(stream, offset, origin);
}

long _ftell(FILE *stream)
{
    return ftell(stream);
}

int _fflush(FILE *stream)
{
    return fflush(stream);
}

int _fclose(FILE *stream)
{
    return fclose(stream);
}

void _qsort(void *base, size_t count, size_t size,
            int (__cdecl *compare)(const void *, const void *))
{
    qsort(base, count, size, compare);
}

int _rand(void)
{
    return rand();
}

void _srand(unsigned int seed)
{
    srand(seed);
}

long _atol(const char *text)
{
    return atol(text);
}

long _strtol(const char *text, char **end, int base)
{
    return strtol(text, end, base);
}

double _strtod(const char *text, char **end)
{
    return strtod(text, end);
}

double _frexp(double value, int *exponent)
{
    return frexp(value, exponent);
}

double _fabs(double value)
{
    return fabs(value);
}

double _acos(double value)
{
    return acos(value);
}

double _floor(double value)
{
    return floor(value);
}

double _ceil(double value)
{
    return ceil(value);
}

int _sprintf_s(char *buffer, size_t buffer_size, const char *format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = vsprintf_s(buffer, buffer_size, format, args);
    va_end(args);
    return result;
}

int _vsprintf(char *buffer, const char *format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = vsprintf_s(buffer, 0x7fffffff, format, args);
    va_end(args);
    return result;
}

int __snprintf(char *buffer, size_t buffer_size, const char *format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = _vsnprintf(buffer, buffer_size, format, args);
    va_end(args);
    return result;
}

int _printf(const char *format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = vprintf(format, args);
    va_end(args);
    return result;
}

int _puts(const char *text)
{
    return puts(text);
}

/* The RetDec output passes a caller-owned vararg area to this old helper. A
   normal vararg view is sufficient for the diagnostic strings used here. */
int _vsprintf_compat(char *buffer, const char *format, va_list args)
{
    return vsprintf_s(buffer, 0x7fffffff, format, args);
}

double __atof_l(const char *text, void *locale, void *unused)
{
    (void)locale;
    (void)unused;
    return strtod(text, NULL);
}

static int retdec_valid_range(const void *address, size_t size, int writeable)
{
    MEMORY_BASIC_INFORMATION info;
    uintptr_t start = (uintptr_t)address;
    uintptr_t end;
    uintptr_t region_end;
    DWORD protection;

    if (address == NULL) {
        return size == 0;
    }
    if (VirtualQuery(address, &info, sizeof(info)) == 0 ||
        info.State != MEM_COMMIT) {
        return 0;
    }
    if (size > (uintptr_t)-1 - start) {
        return 0;
    }
    end = start + size;
    region_end = (uintptr_t)info.BaseAddress + info.RegionSize;
    if (end > region_end) {
        return 0;
    }
    protection = info.Protect & 0xffu;
    if (protection == PAGE_NOACCESS || (info.Protect & PAGE_GUARD) != 0) {
        return 0;
    }
    if (writeable) {
        return protection == PAGE_READWRITE ||
               protection == PAGE_WRITECOPY ||
               protection == PAGE_EXECUTE_READWRITE ||
               protection == PAGE_EXECUTE_WRITECOPY;
    }
    return 1;
}

static int32_t retdec_memcpy2_from_frame(uint32_t *caller_frame)
{
    int best_score = -1;
    void *best_destination = NULL;
    const void *best_source = NULL;
    size_t best_size = 0;
    int offset;

    if (caller_frame == NULL) {
        return 0;
    }

    /* RetDec writes [destination, source, count] into adjacent words and
       emits a zero-argument call. Search the caller frame for that pattern. */
    for (offset = -0x800; offset <= 0x100; offset += 4) {
        uint32_t *slot = (uint32_t *)((unsigned char *)caller_frame + offset);
        uintptr_t destination_value;
        uintptr_t source_value;
        size_t count;
        int score;

        if (!retdec_valid_range(slot, 12, 0)) {
            continue;
        }
        destination_value = slot[0];
        source_value = slot[1];
        count = (size_t)slot[2];
        if (destination_value == 0 || source_value == 0 || count > 0x04000000u) {
            continue;
        }
        if (!retdec_valid_range((const void *)source_value, count == 0 ? 1 : count, 0) ||
            !retdec_valid_range((void *)destination_value, count == 0 ? 1 : count, 1)) {
            continue;
        }
        score = 1000 - abs(offset + 0x14);
        if (count <= 0x1000u) {
            score += 30;
        }
        if (destination_value == source_value) {
            score -= 10;
        }
        if (score > best_score) {
            best_score = score;
            best_destination = (void *)destination_value;
            best_source = (const void *)source_value;
            best_size = count;
        }
    }

    if (best_destination == NULL) {
        return 0;
    }
    if (best_size != 0) {
        memmove(best_destination, best_source, best_size);
    }
    return (int32_t)(uintptr_t)best_destination;
}

#if defined(_M_IX86)
__declspec(naked) int32_t _memcpy2(void)
{
    __asm {
        push ebp
        call retdec_memcpy2_from_frame
        add esp, 4
        ret
    }
}
#else
int32_t _memcpy2(void)
{
    return 0;
}
#endif

long double llvm_log2_f80(long double value)
{
    return log2l(value);
}

long double llvm_round_f80(long double value)
{
    return roundl(value);
}

long double llvm_exp2_f80(long double value)
{
    return exp2l(value);
}

uint32_t llvm_bswap_i32(uint32_t value)
{
    return _byteswap_ulong(value);
}

uint8_t llvm_ctpop_i8(uint8_t value)
{
    uint8_t count = 0;
    while (value != 0) {
        count = (uint8_t)(count + (value & 1u));
        value = (uint8_t)(value >> 1);
    }
    return count;
}

int64_t __alldiv(int64_t left, int64_t right)
{
    return right == 0 ? 0 : left / right;
}

int64_t __allmul(int64_t left, int64_t right)
{
    return left * right;
}

int32_t __allshr(void)
{
    return 0;
}

int32_t __ftol(void)
{
#if defined(_M_IX86)
    int32_t result = 0;
    __asm {
        fistp result
    }
    return result;
#else
    return 0;
#endif
}

int32_t _acos2(long double value)
{
    (void)value;
    return 0;
}

int32_t __CIacos(void) { return 0; }
int32_t __CIasin(void) { return 0; }
int32_t __CIatan(void) { return 0; }
int32_t __CIcos(void) { return 0; }
int32_t __CIexp(void) { return 0; }
int32_t __CIfmod(void) { return 0; }
int32_t __CIlog(void) { return 0; }
int32_t __CIlog10(void) { return 0; }
int32_t __CIpow(void) { return 0; }
int32_t __CIsin(void) { return 0; }
int32_t __CIsqrt(void) { return 0; }
int32_t __CItan(void) { return 0; }

int32_t __controlfp_s(void)
{
    return 0;
}

int32_t __chkstk(void)
{
    return 0;
}

int32_t _longjmp(void *environment, int32_t value)
{
    (void)environment;
    (void)value;
    return 0;
}

int _atexit(void (*function)(void))
{
    (void)function;
    return 0;
}

int32_t _doexit(int32_t code, int32_t quick, int32_t retcaller)
{
    (void)code;
    (void)quick;
    (void)retcaller;
    return 0;
}

int32_t __splitpath_s(
    const char *path,
    char *drive,
    size_t drive_count,
    char *directory,
    size_t directory_count,
    char *filename,
    size_t filename_count,
    char *extension,
    size_t extension_count,
    ...)
{
    if (path == NULL) {
        return EINVAL;
    }
    _splitpath_s(path, drive, drive_count, directory, directory_count,
                 filename, filename_count, extension, extension_count);
    return 0;
}

void *__fsopen(const char *path, const char *mode, int share)
{
    FILE *stream;
    (void)share;
    stream = fopen(path, mode);
    return stream;
}

int32_t _flsall(int32_t flush)
{
    (void)flush;
    fflush(NULL);
    return 0;
}

static int32_t retdec_zero(void)
{
    return 0;
}

int32_t __Getctype(int32_t *unused)
{
    static int32_t ctype_data[4];
    (void)unused;
    return (int32_t)(uintptr_t)ctype_data;
}

int32_t __Getcvt(void)
{
    return 0;
}

int32_t __Tolower(void)
{
    return 0;
}

int32_t __Toupper(void)
{
    return 0;
}

static int retdec_valid_text(const char *text)
{
    return text != NULL && retdec_valid_range(text, 1, 0);
}

long __Stolx(const char *text, char **end, int base)
{
    return retdec_valid_text(text) ? strtol(text, end, base) : 0;
}

unsigned long __Stoulx(const char *text, char **end, int base)
{
    return retdec_valid_text(text) ? strtoul(text, end, base) : 0;
}

long long __Stollx(const char *text, char **end, int base)
{
    return retdec_valid_text(text) ? _strtoi64(text, end, base) : 0;
}

unsigned long long __Stoullx(const char *text, char **end, int base)
{
    return retdec_valid_text(text) ? _strtoui64(text, end, base) : 0;
}

float __Stofx(const char *text, char **end, int flags)
{
    (void)flags;
    return retdec_valid_text(text) ? (float)strtod(text, end) : 0.0f;
}

double __Stodx(const char *text, char **end, int flags)
{
    (void)flags;
    return retdec_valid_text(text) ? strtod(text, end) : 0.0;
}

/* Safe fallbacks for old C++ ABI and locale entry points. The decompiled
   source contains the corresponding object layouts and performs the useful
   field initialization itself; these functions only need to preserve the
   call contract and avoid entering a mismatched modern ABI. */
int32_t _3f__3f_2_40_YAPAXI_40_Z(uint32_t size)
{
    return (int32_t)(uintptr_t)malloc(size);
}

void _3f__3f_0_Lockit_40_std_40__40_QAE_40_H_40_Z(int32_t value)
{
    (void)value;
}

int32_t _3f__3f_1_Lockit_40_std_40__40_QAE_40_XZ(void)
{
    return 0;
}

void _3f__3f_0_Mutex_40_std_40__40_QAE_40_XZ(void)
{
}

int32_t _3f__3f_1_Mutex_40_std_40__40_QAE_40_XZ(void)
{
    return 0;
}

int32_t _3f__3f_0bad_alloc_40_std_40__40_QAE_40_PBD_40_Z(char *message)
{
    (void)message;
    return 0;
}

int32_t _3f__3f_0exception_40_std_40__40_QAE_40_ABQBD_40_Z(void *result)
{
    return (int32_t)(uintptr_t)result;
}

int32_t _3f__3f_0exception_40_std_40__40_QAE_40_ABV01_40__40_Z(void *result)
{
    return (int32_t)(uintptr_t)result;
}

int32_t _3f__3f_1_Fac_tidy_reg_t_40_std_40__40_QAE_40_XZ(void)
{
    return 0;
}

int32_t _3f__3f_1_Init_atexit_40__40_QAE_40_XZ(void)
{
    return 0;
}

int32_t _3f__3f_1_Init_locks_40_std_40__40_QAE_40_XZ(void)
{
    return 0;
}

int32_t _3f__3f_8type_info_40__40_QBE_NABV0_40__40_Z(void *value)
{
    return (int32_t)(uintptr_t)value;
}

int32_t _3f__Facet_Register_40_facet_40_locale_40_std_40__40_CAXPAV123_40__40_Z(void *value)
{
    return (int32_t)(uintptr_t)value;
}

int32_t _3f__Init_40_locale_40_std_40__40_CAPAV_Locimp_40_12_40_XZ(void)
{
    return 0;
}

int32_t _3f__Ios_base_dtor_40_ios_base_40_std_40__40_CAXPAV12_40__40_Z(void *value)
{
    return (int32_t)(uintptr_t)value;
}

int32_t _3f__Locinfo_ctor_40__Locinfo_40_std_40__40_SAXPAV12_40_PBD_40_Z(void *value, char *name)
{
    (void)name;
    return (int32_t)(uintptr_t)value;
}

int32_t _3f__Locinfo_dtor_40__Locinfo_40_std_40__40_SAXPAV12_40__40_Z(void *value)
{
    return (int32_t)(uintptr_t)value;
}

int32_t _3f__Tidy_40_exception_40_std_40__40_AAEXXZ(void)
{
    return 0;
}

int32_t _3f__Xinvalid_argument_40_std_40__40_YAXPBD_40_Z(char *message)
{
    (void)message;
    return 0;
}

int32_t _3f___uncaught_exception_40__40_YA_NXZ(void)
{
    return 0;
}

int32_t _3f___ArrayUnwind_40__40_YGXPAXIHP6EX0_40_Z_40_Z(
    void *object, int32_t count, int32_t size, void (*destroy)(int32_t *))
{
    int32_t i;
    (void)size;
    if (destroy != NULL) {
        for (i = count - 1; i >= 0; --i) {
            destroy((int32_t *)((unsigned char *)object + (size_t)i * (size_t)size));
        }
    }
    return 0;
}

int32_t _3f__3f__L_40_YGXPAXIHP6EX0_40_Z1_40_Z(
    void *object, int32_t count, int32_t size,
    void (*construct)(int32_t *), void (*destroy)(int32_t *))
{
    int32_t i;
    (void)destroy;
    if (construct != NULL) {
        for (i = 0; i < count; ++i) {
            construct((int32_t *)((unsigned char *)object + (size_t)i * (size_t)size));
        }
    }
    return 0;
}

int32_t _3f__3f__M_40_YGXPAXIHP6EX0_40_Z_40_Z(
    void *object, int32_t count, int32_t size, void (*destroy)(int32_t *))
{
    return _3f___ArrayUnwind_40__40_YGXPAXIHP6EX0_40_Z_40_Z(
        object, count, size, destroy);
}

int32_t _3f_ExFilterRethrow_40__40_YAHPAU_EXCEPTION_POINTERS_40__40__40_Z(void *value)
{
    (void)value;
    return 0;
}

int32_t _3f_terminate_40__40_YAXXZ(void)
{
    return 0;
}

int32_t __87except(int32_t a1, int32_t a2, int32_t a3)
{
    (void)a1;
    (void)a2;
    (void)a3;
    return 0;
}

int32_t ___alldiv_placeholder(void)
{
    return 0;
}

int32_t __amsg_exit(int32_t code)
{
    (void)code;
    return 0;
}

int32_t __calloc_impl(void)
{
    return 0;
}

int32_t __check_range_exit(void)
{
    return 0;
}

int32_t __cintrindisp1(void) { return 0; }
int32_t __cintrindisp2(void) { return 0; }
int32_t __convertTOStoQNaN(void) { return 0; }
int32_t __ctrandisp1(void) { return 0; }
int32_t __ctrandisp2(void) { return 0; }
int32_t __FindAndUnlinkFrame(int32_t value) { return value; }
int32_t __fload_withFB(void) { return 0; }
int32_t __getptd(void) { return 0; }
int32_t __IsExceptionObjectToBeDestroyed(int32_t value) { return value; }
int32_t __load_CW(void) { return 0; }
int32_t __local_unwind4(int32_t a1, int32_t a2, int32_t a3, int32_t a4)
{
    (void)a1;
    (void)a2;
    (void)a3;
    (void)a4;
    return 0;
}
int32_t __lock(int32_t value) { return value; }
int32_t __math_exit(void) { return 0; }
int32_t __Mtxlock(int32_t value) { return value; }
int32_t __Mtxunlock(int32_t value) { return value; }
int32_t __powhlp(int64_t value) { return (int32_t)value; }
int32_t __SEH_epilog4(void) { return 0; }
int32_t __startOneArgErrorHandling(void) { return 0; }
int32_t __startTwoArgErrorHandling(void) { return 0; }
int32_t __unlock(int32_t value) { return value; }
int32_t __unlock_fhandle(int32_t value) { return value; }
int32_t __unlock_file(int32_t value) { return value; }
int32_t __unlock_file2(int32_t a1, int32_t a2) { return a1 + a2; }
int32_t __XcptFilter(int32_t a1, int32_t a2) { return a1 + a2; }
int32_t __twoToTOS(void) { return 0; }
int32_t ___security_init_cookie(void) { return 0; }
int32_t ___tmainCRTStartup(void) { return 0; }
int32_t ___report_gsfailure(void) { return 0; }
int32_t __CxxThrowException_40_8(void) { return 0; }
int32_t ___CxxFrameHandler(void) { return 0; }
int32_t ___DestructExceptionObject(int32_t a1, int32_t a2) { return a1 + a2; }
int32_t ___FrameUnwindToState(void) { return 0; }
int32_t ___freetlocinfo(int32_t value) { return value; }
int32_t ___libm_error_support(void) { return 0; }
int32_t ___removelocaleref(int32_t value) { return value; }
int32_t ___RTtypeid(int32_t a1, int32_t a2, int32_t a3, int32_t a4)
{
    (void)a2;
    (void)a3;
    (void)a4;
    return a1;
}

int32_t retdec_msvc_0bad_alloc_std__QAE_ABV01__Z(int32_t *value)
{
    return (int32_t)(uintptr_t)value;
}

int32_t retdec_Xinvalid_argument(char *message)
{
    (void)message;
    return 0;
}

int32_t unknown_fcd53371(void)
{
    return 0;
}
