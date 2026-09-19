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
#include <cstring>

namespace {

using retdec_co_initialize_fn = HRESULT (WINAPI *)(LPVOID);
using retdec_co_uninitialize_fn = void (WINAPI *)(void);
using retdec_co_create_instance_fn = HRESULT (WINAPI *)(
    const GUID *, void *, DWORD, const GUID *, LPVOID *);
using retdec_direct3d_create9_fn = LPVOID (WINAPI *)(UINT);
using retdec_time_get_time_fn = DWORD (WINAPI *)(void);
using retdec_time_begin_period_fn = UINT (WINAPI *)(UINT);

static HMODULE retdec_module(const char *name)
{
    HMODULE module = GetModuleHandleA(name);
    if (module == nullptr) {
        module = LoadLibraryA(name);
    }
    return module;
}

static FARPROC retdec_proc(const char *module_name, const char *proc_name)
{
    HMODULE module = retdec_module(module_name);
    return module == nullptr ? nullptr : GetProcAddress(module, proc_name);
}

/* RetDec retained only the first DWORD of several adjacent GUID objects.
   Supply complete values when one of those truncated objects is passed to
   the COM wrapper, while preserving the caller's GUIDs for other interfaces. */
static const GUID retdec_clsid_direct_input8 = {
    0x25e609e4, 0xb259, 0x11cf,
    { 0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 }
};

static const GUID retdec_iid_direct_input8 = {
    0xbf798030, 0x483a, 0x4da2,
    { 0xaa, 0x99, 0x5d, 0x64, 0xed, 0x36, 0x97, 0x00 }
};

static const GUID retdec_clsid_direct_sound8 = {
    0x3901cc3f, 0x84b5, 0x4fa4,
    { 0xba, 0x35, 0xaa, 0x81, 0x72, 0xb8, 0xa0, 0x9b }
};

static const GUID retdec_iid_direct_sound8 = {
    0xc50a7e93, 0xf395, 0x4834,
    { 0x9e, 0xf6, 0x7f, 0xa9, 0x9d, 0xe5, 0x09, 0x66 }
};

template<class Function>
Function resolve(const char* module, const char* entry) {
    return reinterpret_cast<Function>(retdec_proc(module, entry));
}
} // namespace

// Export the recovered symbol spelling, not C++-mangled names.
extern "C" {

int retdec_valid_range(const void* address, size_t size, int writeable);

int32_t *Direct3DCreate9(int32_t version)
{
    retdec_direct3d_create9_fn create9 =
        resolve<retdec_direct3d_create9_fn>("d3d9.dll", "Direct3DCreate9");
    if (create9 == nullptr) {
        return nullptr;
    }
    return (int32_t *)create9((UINT)version);
}

int32_t CoInitialize(void *reserved)
{
    retdec_co_initialize_fn initialize =
        resolve<retdec_co_initialize_fn>("ole32.dll", "CoInitialize");
    return initialize == nullptr ? (int32_t)E_FAIL : (int32_t)initialize(reserved);
}

void CoUninitialize(void)
{
    retdec_co_uninitialize_fn uninitialize =
        resolve<retdec_co_uninitialize_fn>("ole32.dll", "CoUninitialize");
    if (uninitialize != nullptr) {
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
        resolve<retdec_co_create_instance_fn>("ole32.dll", "CoCreateInstance");
    const GUID *actual_rclsid = (const GUID *)rclsid;
    const GUID *actual_riid = (const GUID *)riid;
    uint32_t clsid_data1 = 0;
    uint32_t iid_data1 = 0;
    if (rclsid) std::memcpy(&clsid_data1, rclsid, sizeof clsid_data1);
    if (riid) std::memcpy(&iid_data1, riid, sizeof iid_data1);

    if (create_instance == nullptr || result == nullptr) {
        return (int32_t)E_FAIL;
    }
    *result = nullptr;
    if (clsid_data1 == retdec_clsid_direct_input8.Data1) {
        actual_rclsid = &retdec_clsid_direct_input8;
    } else if (clsid_data1 == retdec_clsid_direct_sound8.Data1) {
        actual_rclsid = &retdec_clsid_direct_sound8;
    }
    if (iid_data1 == retdec_iid_direct_input8.Data1) {
        actual_riid = &retdec_iid_direct_input8;
    } else if (iid_data1 == retdec_iid_direct_sound8.Data1) {
        actual_riid = &retdec_iid_direct_sound8;
    }
    if (actual_rclsid == nullptr || actual_riid == nullptr) {
        return (int32_t)E_INVALIDARG;
    }
    return (int32_t)create_instance(actual_rclsid, outer, (DWORD)context,
                                    actual_riid, (LPVOID *)result);
}

uint32_t timeGetTime(void)
{
    retdec_time_get_time_fn get_time =
        resolve<retdec_time_get_time_fn>("winmm.dll", "timeGetTime");
    return get_time == nullptr ? GetTickCount() : (uint32_t)get_time();
}

uint32_t timeBeginPeriod(uint32_t period)
{
    retdec_time_begin_period_fn begin_period =
        resolve<retdec_time_begin_period_fn>("winmm.dll", "timeBeginPeriod");
    return begin_period == nullptr ? 0u : (uint32_t)begin_period((UINT)period);
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
    return strtod(text, nullptr);
}

int _atexit(void (*function)(void))
{
    (void)function;
    return 0;
}

/* Remaining C++ ABI/locale adapters are referenced by recovered callers.
   Several signatures and exception semantics are still incomplete. They are
   NOT equivalent to the modern STL or a working native exception runtime;
   keep the boundary until its complete caller/object/unwind contract is known.
   See docs/legacy-library-audit-20260920.md. */
int32_t _3f__3f_2_40_YAPAXI_40_Z(uint32_t size)
{
    return (int32_t)(uintptr_t)malloc(size);
}

int32_t _3f__3f_0exception_40_std_40__40_QAE_40_ABQBD_40_Z(void *result)
{
    return (int32_t)(uintptr_t)result;
}

int32_t _3f__3f_1_Init_locks_40_std_40__40_QAE_40_XZ(void)
{
    return 0;
}

int32_t _3f__3f_8type_info_40__40_QBE_NABV0_40__40_Z(void *value)
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

int32_t _3f___ArrayUnwind_40__40_YGXPAXIHP6EX0_40_Z_40_Z(
    void *object, int32_t count, int32_t size, void (*destroy)(int32_t *))
{
    int32_t i;
    (void)size;
    if (destroy != nullptr) {
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
    if (construct != nullptr) {
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

int32_t ___report_gsfailure(void) { return 0; }
int32_t __CxxThrowException_40_8(void) { return 0; }

int32_t ___RTtypeid(int32_t a1, int32_t a2, int32_t a3, int32_t a4)
{
    (void)a2;
    (void)a3;
    (void)a4;
    return a1;
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

} // extern "C"
