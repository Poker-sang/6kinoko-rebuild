#include "kinoko/bitmap.h"
#include "kinoko/file_io.h"
#include <cstdlib>
#include <memory>

namespace {
struct CloseReader {
    void operator()(KinokoArchiveReader* reader) const { kinoko_reader_close(reader); }
};
}

extern "C" void kinoko_bitmap_release_pixels(KinokoBitmap* bitmap) {
    if (!bitmap) return;
    std::free(bitmap->pixels);
    bitmap->pixels = nullptr;
}

// 414010 receives CBitmapData in ECX. The 17-byte file header is read field by
// field; it is not the 32-byte in-memory record. Keep inherited exact-read and
// allocation limits distinct from the original unchecked virtual reads.
extern "C" int32_t kinoko_bitmap_load_cv2(KinokoBitmap* bitmap, const char* path) {
    if (!bitmap || !path) return 0;
    KinokoArchiveReader* opened = nullptr;
    if (!kinoko_reader_open(&opened, path)) return 0;
    const std::unique_ptr<KinokoArchiveReader, CloseReader> reader(opened);
    uint8_t depth;
    uint32_t width, height, row_width, encoded_size;
    if (!kinoko_reader_read_exact(reader.get(), &depth, sizeof(depth)) ||
        !kinoko_reader_read_exact(reader.get(), &width, sizeof(width)) ||
        !kinoko_reader_read_exact(reader.get(), &height, sizeof(height)) ||
        !kinoko_reader_read_exact(reader.get(), &row_width, sizeof(row_width)) ||
        !kinoko_reader_read_exact(reader.get(), &encoded_size, sizeof(encoded_size)))
        return 0;

    bitmap->bit_depth = depth;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->row_width = row_width;
    bitmap->encoded_size = encoded_size;
    uint32_t allocation_size;
    if (encoded_size) allocation_size = encoded_size;
    else if (depth < 24)
        allocation_size = static_cast<uint32_t>(uint64_t(row_width) * height * depth / 8u);
    else allocation_size = static_cast<uint32_t>(uint64_t(row_width) * height * 4u);
    if (!allocation_size || allocation_size > 256u * 1024u * 1024u ||
        uint64_t(width) * height > 64u * 1024u * 1024u)
        return 0;

    // Preserve replacement timing: once the header is accepted, failed payload
    // allocation/read leaves no old pixels. Metadata and the borrowed palette
    // survive failure. malloc/free is the existing reconstruction allocator.
    kinoko_bitmap_release_pixels(bitmap);
    bitmap->pixels = static_cast<uint8_t*>(std::malloc(allocation_size));
    if (!bitmap->pixels || !kinoko_reader_read_exact(reader.get(), bitmap->pixels, allocation_size)) {
        kinoko_bitmap_release_pixels(bitmap);
        return 0;
    }
    return 1;
}
