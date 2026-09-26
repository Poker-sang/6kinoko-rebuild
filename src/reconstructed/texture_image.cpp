#include "kinoko/texture_image.h"
#include "kinoko/bitmap.hpp"
#include "kinoko/graphics_device.h"
#include "kinoko/graphics_lock.hpp"
#include "kinoko/com_owner.hpp"
#include "kinoko/diagnostics.h"
#include <algorithm>
#include <cstring>
#include <type_traits>

extern "C" {
HRESULT WINAPI D3DXCreateTexture(IDirect3DDevice9*,UINT,UINT,UINT,DWORD,
    D3DFORMAT,D3DPOOL,IDirect3DTexture9**);
void retdec_trace_i32(const char*,int32_t);
}

namespace {
bool copy_indexed_raw(const KinokoBitmap& bitmap, uint32_t source_pitch,
    const D3DLOCKED_RECT& locked) {
    if (!bitmap.palette || locked.Pitch < 0 || size_t(locked.Pitch) < size_t(bitmap.width) * 2u)
        return false;
    for (uint32_t row = 0; row < bitmap.height; ++row) {
        auto* destination = static_cast<uint16_t*>(static_cast<void*>(
            static_cast<uint8_t*>(locked.pBits) + size_t(row) * locked.Pitch));
        const auto* source = bitmap.pixels + size_t(row) * source_pitch;
        for (uint32_t column = 0; column < bitmap.width; ++column)
            destination[column] = bitmap.palette[source[column]];
    }
    return true;
}

bool copy_raw_16(const KinokoBitmap& bitmap, uint32_t source_pitch,
    const D3DLOCKED_RECT& locked) {
    if (locked.Pitch < 0 || size_t(locked.Pitch) < size_t(bitmap.width) * 2u)
        return false;
    for (uint32_t row = 0; row < bitmap.height; ++row) {
        auto* destination = static_cast<uint8_t*>(locked.pBits) + size_t(row) * locked.Pitch;
        const auto* source = bitmap.pixels + size_t(row) * source_pitch;
        // The original raw branch copies complete 32-bit pairs only.
        std::memcpy(destination, source, size_t(bitmap.width / 2u) * 4u);
    }
    return true;
}

bool copy_raw_32(const KinokoBitmap& bitmap, uint32_t source_pitch,
    const D3DLOCKED_RECT& locked) {
    if (locked.Pitch < 0 || size_t(locked.Pitch) < size_t(bitmap.width) * 4u)
        return false;
    for (uint32_t row = 0; row < bitmap.height; ++row) {
        auto* destination = static_cast<uint8_t*>(locked.pBits) + size_t(row) * locked.Pitch;
        const auto* source = bitmap.pixels + size_t(row) * source_pitch;
        std::memcpy(destination, source, size_t(bitmap.width) * 4u);
    }
    return true;
}

template <typename T, typename Convert>
bool decode_rle(const KinokoBitmap& bitmap, const D3DLOCKED_RECT& locked, Convert convert) {
    if (locked.Pitch < 0 || size_t(locked.Pitch) < size_t(bitmap.width) * sizeof(T))
        return false;
    const auto* cursor = bitmap.pixels;
    const auto* end = bitmap.pixels + bitmap.encoded_size;
    uint32_t run_left = 0;
    T run_value{};
    using Count = std::conditional_t<sizeof(T) == 4, uint32_t, uint16_t>;
    for (uint32_t row = 0; row < bitmap.height; ++row) {
        auto* destination = reinterpret_cast<T*>(static_cast<uint8_t*>(locked.pBits) +
            size_t(row) * locked.Pitch);
        uint32_t column = 0;
        while (column < bitmap.width) {
            if (!run_left) {
                if (cursor + sizeof(Count) + sizeof(T) > end) return false;
                Count count;
                std::memcpy(&count, cursor, sizeof(count)); cursor += sizeof(count);
                std::memcpy(&run_value, cursor, sizeof(run_value)); cursor += sizeof(run_value);
                if (!count) return false;
                run_left = count;
            }
            const auto amount = (std::min)(run_left, bitmap.width - column);
            for (uint32_t i = 0; i < amount; ++i)
                destination[column + i] = convert(run_value);
            column += amount;
            run_left -= amount;
        }
    }
    return true;
}

bool copy_surface(const KinokoBitmap& bitmap, uint32_t source_pitch, const D3DLOCKED_RECT& locked) {
    if (bitmap.encoded_size) {
        if (bitmap.bit_depth == 8) {
            if (!bitmap.palette) return false;
            return decode_rle<uint16_t>(bitmap, locked,
                [&bitmap](uint16_t value) { return bitmap.palette[value]; });
        }
        if (bitmap.bit_depth == 16)
            return decode_rle<uint16_t>(bitmap, locked, [](uint16_t value) { return value; });
        if (bitmap.bit_depth == 24 || bitmap.bit_depth == 32)
            return decode_rle<uint32_t>(bitmap, locked, [](uint32_t value) { return value; });
        return false;
    }
    if (bitmap.bit_depth == 8)
        return copy_indexed_raw(bitmap, source_pitch, locked);
    if (bitmap.bit_depth == 16)
        return copy_raw_16(bitmap, source_pitch, locked);
    if (bitmap.bit_depth == 24 || bitmap.bit_depth == 32)
        return copy_raw_32(bitmap, source_pitch, locked);
    return false;
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
    const auto format = bitmap.bit_depth<=16 ? D3DFMT_A1R5G5B5 : D3DFMT_A8R8G8B8;
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
    const HRESULT creation_result = result;
    D3DLOCKED_RECT locked{};
    const HRESULT lock_result = texture->LockRect(0,&locked,nullptr,0);
    retdec_trace_hresult("texture:lock-hr",lock_result);
    retdec_trace_i32("texture:lock-object",diagnostic_address(texture.get()));
    retdec_trace_i32("texture:lock-bits",diagnostic_address(locked.pBits));
    retdec_trace_i32("texture:lock-pitch",locked.Pitch);
    // 40E7E0 tests exactly zero. A failed or nonzero-success lock skips upload,
    // but 40E705 still returns the creation status and hands off the texture.
    if (lock_result != D3D_OK) {
        *output = texture.detach();
        return creation_result;
    }
    if (!locked.pBits || locked.Pitch<=0) {
        retdec_trace("texture:lock-invalid-surface");
        texture->UnlockRect(0);
        return E_FAIL;
    }
    if (!copy_surface(bitmap,source_pitch,locked)) {
        texture->UnlockRect(0);
        return E_FAIL;
    }
    retdec_trace_i32("texture:unlock-object",diagnostic_address(texture.get()));
    texture->UnlockRect(0); // 40E815 ignores this HRESULT.
    *output=texture.detach();
    return creation_result;
}
