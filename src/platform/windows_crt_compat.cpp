/*
 * Compatibility entry points for the old MSVC runtime names emitted by
 * RetDec. The decompiled translation unit uses the original symbol spelling
 * as a C identifier, so the leading underscore is intentional here.
 */

#define CoInitialize kinoko_decl_CoInitialize
#define CoUninitialize kinoko_decl_CoUninitialize
#define CoCreateInstance kinoko_decl_CoCreateInstance
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

using kinoko_co_initialize_fn = HRESULT (WINAPI *)(LPVOID);
using kinoko_co_uninitialize_fn = void (WINAPI *)(void);
using kinoko_co_create_instance_fn = HRESULT (WINAPI *)(
    const GUID *, void *, DWORD, const GUID *, LPVOID *);
using kinoko_direct3d_create9_fn = LPVOID (WINAPI *)(UINT);
using kinoko_time_get_time_fn = DWORD (WINAPI *)(void);
using kinoko_time_begin_period_fn = UINT (WINAPI *)(UINT);

static HMODULE kinoko_module(const char *name)
{
    HMODULE module = GetModuleHandleA(name);
    if (module == nullptr) {
        module = LoadLibraryA(name);
    }
    return module;
}

static FARPROC kinoko_proc(const char *module_name, const char *proc_name)
{
    HMODULE module = kinoko_module(module_name);
    return module == nullptr ? nullptr : GetProcAddress(module, proc_name);
}

/* RetDec retained only the first DWORD of several adjacent GUID objects.
   Supply complete values when one of those truncated objects is passed to
   the COM wrapper, while preserving the caller's GUIDs for other interfaces. */
static const GUID kinoko_clsid_direct_input8 = {
    0x25e609e4, 0xb259, 0x11cf,
    { 0xbf, 0xc7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 }
};

static const GUID kinoko_iid_direct_input8 = {
    0xbf798030, 0x483a, 0x4da2,
    { 0xaa, 0x99, 0x5d, 0x64, 0xed, 0x36, 0x97, 0x00 }
};

static const GUID kinoko_clsid_direct_sound8 = {
    0x3901cc3f, 0x84b5, 0x4fa4,
    { 0xba, 0x35, 0xaa, 0x81, 0x72, 0xb8, 0xa0, 0x9b }
};

static const GUID kinoko_iid_direct_sound8 = {
    0xc50a7e93, 0xf395, 0x4834,
    { 0x9e, 0xf6, 0x7f, 0xa9, 0x9d, 0xe5, 0x09, 0x66 }
};

template<class Function>
Function resolve(const char* module, const char* entry) {
    return reinterpret_cast<Function>(kinoko_proc(module, entry));
}
} // namespace

// Export the recovered symbol spelling, not C++-mangled names.
extern "C" {

int kinoko_valid_range(const void* address, size_t size, int writeable);

int32_t *Direct3DCreate9(int32_t version)
{
    kinoko_direct3d_create9_fn create9 =
        resolve<kinoko_direct3d_create9_fn>("d3d9.dll", "Direct3DCreate9");
    if (create9 == nullptr) {
        return nullptr;
    }
    return (int32_t *)create9((UINT)version);
}

int32_t CoInitialize(void *reserved)
{
    kinoko_co_initialize_fn initialize =
        resolve<kinoko_co_initialize_fn>("ole32.dll", "CoInitialize");
    return initialize == nullptr ? (int32_t)E_FAIL : (int32_t)initialize(reserved);
}

void CoUninitialize(void)
{
    kinoko_co_uninitialize_fn uninitialize =
        resolve<kinoko_co_uninitialize_fn>("ole32.dll", "CoUninitialize");
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
    kinoko_co_create_instance_fn create_instance =
        resolve<kinoko_co_create_instance_fn>("ole32.dll", "CoCreateInstance");
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
    if (clsid_data1 == kinoko_clsid_direct_input8.Data1) {
        actual_rclsid = &kinoko_clsid_direct_input8;
    } else if (clsid_data1 == kinoko_clsid_direct_sound8.Data1) {
        actual_rclsid = &kinoko_clsid_direct_sound8;
    }
    if (iid_data1 == kinoko_iid_direct_input8.Data1) {
        actual_riid = &kinoko_iid_direct_input8;
    } else if (iid_data1 == kinoko_iid_direct_sound8.Data1) {
        actual_riid = &kinoko_iid_direct_sound8;
    }
    if (actual_rclsid == nullptr || actual_riid == nullptr) {
        return (int32_t)E_INVALIDARG;
    }
    return (int32_t)create_instance(actual_rclsid, outer, (DWORD)context,
                                    actual_riid, (LPVOID *)result);
}

uint32_t timeGetTime(void)
{
    kinoko_time_get_time_fn get_time =
        resolve<kinoko_time_get_time_fn>("winmm.dll", "timeGetTime");
    return get_time == nullptr ? GetTickCount() : (uint32_t)get_time();
}

uint32_t timeBeginPeriod(uint32_t period)
{
    kinoko_time_begin_period_fn begin_period =
        resolve<kinoko_time_begin_period_fn>("winmm.dll", "timeBeginPeriod");
    return begin_period == nullptr ? 0u : (uint32_t)begin_period((UINT)period);
}

/* The RetDec output passes a caller-owned vararg area to this old helper. A
   normal vararg view is sufficient for the diagnostic strings used here. */
int _vsprintf_compat(char *buffer, const char *format, va_list args)
{
    return vsprintf_s(buffer, 0x7fffffff, format, args);
}



/* Legacy allocation spelling remains malloc-compatible with the recovered
   callers and their free-based cleanup. This is not modern operator new. */
int32_t _3f__3f_2_40_YAPAXI_40_Z(uint32_t size)
{
    return (int32_t)(uintptr_t)malloc(size);
}






















} // extern "C"
