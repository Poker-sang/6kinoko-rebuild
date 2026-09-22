#include "kinoko/texture_image.h"
#include "kinoko/bitmap.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/graphics_lock.hpp"
#include "kinoko/com_owner.hpp"
#include "kinoko/diagnostics.h"
#include <algorithm>
#include <cstring>

extern "C" {
HRESULT WINAPI D3DXCreateTexture(IDirect3DDevice9*,UINT,UINT,UINT,DWORD,
    D3DFORMAT,D3DPOOL,IDirect3DTexture9**);
void retdec_trace_i32(const char*,int32_t);
}

namespace {
// Existing raw upload compatibility path. Original 4141E0 additionally decodes
// RLE and indexed palette data; those are explicitly outside this extraction.
void copy_surface(const KinokoBitmap& bitmap, uint32_t source_pitch, const D3DLOCKED_RECT& locked) {
    for (uint32_t row=0; row<bitmap.height; ++row) {
        auto* destination = static_cast<uint8_t*>(locked.pBits) + size_t(row)*locked.Pitch;
        const auto* source = bitmap.pixels + size_t(row)*source_pitch;
        if (bitmap.bit_depth==16) {
            // Original raw 414482 copies pairs only, retaining an odd last
            // pixel in the destination and stepping by the stored row width.
            std::memcpy(destination,source,size_t(bitmap.width/2u)*4u);
        } else if (bitmap.bit_depth==24 || bitmap.bit_depth==32) {
            std::memcpy(destination,source,size_t(bitmap.width)*4u);
        } else {
            // Inherited fallback, not a recovered original palette algorithm.
            for (uint32_t column=0; column<bitmap.width; ++column) {
                const auto value = source[column];
                destination[column*4u] = value;
                destination[column*4u+1] = value;
                destination[column*4u+2] = value;
                destination[column*4u+3] = 0xff;
            }
        }
    }
}
int32_t diagnostic_address(const void* value) { return static_cast<int32_t>(reinterpret_cast<intptr_t>(value)); }
}

extern "C" HRESULT kinoko_texture_load_image(const char* path, IDirect3DTexture9** output,
    uint32_t* width, uint32_t* height) {
    if (!path || !output || !kinoko_graphics.device) return D3DERR_INVALIDCALL;
    const auto length = std::strlen(path);
    char lookup[MAX_PATH];
    if (length<3 || length+1>sizeof(lookup)) return D3DERR_INVALIDCALL;
    std::memcpy(lookup,path,length+1);
    std::memcpy(lookup+length-3,"cv2",3);
    kinoko::Bitmap owner;
    if (!kinoko_bitmap_load_cv2(owner.get(),lookup)) return D3DERR_INVALIDCALL;
    const auto& bitmap = *owner.get();
    const auto source_pitch = bitmap.bit_depth==16 ? (bitmap.row_width/2u)*4u :
        bitmap.bit_depth>=24 ? bitmap.row_width*4u : bitmap.row_width;
    if (!bitmap.width || !bitmap.height || bitmap.row_width<bitmap.width || !source_pitch ||
        uint64_t(source_pitch)*bitmap.height>256u*1024u*1024u)
        return D3DERR_INVALIDCALL;
    if (width) *width=bitmap.width;
    if (height) *height=bitmap.height;

    auto allocation_width=bitmap.width, allocation_height=bitmap.height;
    // 40E73D and 40E793: report source dimensions, then square allocation if
    // required by caps. The original graphics lock encloses texture creation.
    if (kinoko_graphics.capabilities.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
        allocation_width=allocation_height=(std::max)(allocation_width,allocation_height);
    const auto format = bitmap.bit_depth==16 ? D3DFMT_A1R5G5B5 : D3DFMT_A8R8G8B8;
    kinoko::ComOwner<IDirect3DTexture9> texture;
    HRESULT result;
    {
        kinoko::graphics::Lock lock;
        result=D3DXCreateTexture(kinoko_graphics.device,allocation_width,allocation_height,
            1,0,format,D3DPOOL_MANAGED,texture.put());
    }
    retdec_trace_hresult("texture:create-hr",result);
    retdec_trace_i32("texture:create-object",diagnostic_address(texture.get()));
    if (FAILED(result) || !texture) return result;
    if (!*reinterpret_cast<void***>(texture.get())) {
        retdec_trace("texture:create-no-vtable");
        texture.detach(); // inherited invalid-interface boundary: Release is unavailable
        return E_FAIL;
    }
    D3DLOCKED_RECT locked{};
    result=texture->LockRect(0,&locked,nullptr,0);
    retdec_trace_hresult("texture:lock-hr",result);
    retdec_trace_i32("texture:lock-object",diagnostic_address(texture.get()));
    retdec_trace_i32("texture:lock-bits",diagnostic_address(locked.pBits));
    retdec_trace_i32("texture:lock-pitch",locked.Pitch);
    if (FAILED(result)) return result;
    if (!locked.pBits || locked.Pitch<=0) {
        retdec_trace("texture:lock-invalid-surface");
        texture->UnlockRect(0);
        return E_FAIL;
    }
    copy_surface(bitmap,source_pitch,locked);
    retdec_trace_i32("texture:unlock-object",diagnostic_address(texture.get()));
    texture->UnlockRect(0); // existing contract returns LockRect status, not UnlockRect status
    *output=texture.detach();
    return result;
}
