#include "kinoko/bitmap.hpp"
#include "kinoko/file_io.h"
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <type_traits>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while (0)
struct KinokoArchiveReader { size_t position = 0; };
static std::vector<uint8_t> input;
static unsigned opens, closes;
extern "C" int32_t kinoko_reader_open(KinokoArchiveReader** slot, const char* path) {
    if (!std::strcmp(path,"missing")) return 0;
    *slot = new KinokoArchiveReader; ++opens; return 1;
}
extern "C" void kinoko_reader_close(KinokoArchiveReader* reader) {
    if (reader) { delete reader; ++closes; }
}
extern "C" int32_t kinoko_reader_read_exact(KinokoArchiveReader* reader, void* data, uint32_t size) {
    if (size > input.size() - reader->position) return 0;
    std::memcpy(data,input.data()+reader->position,size); reader->position += size; return 1;
}
static void header(uint8_t depth,uint32_t width,uint32_t height,uint32_t row_width,uint32_t encoded) {
    input = {depth};
    for (uint32_t word : {width,height,row_width,encoded})
        for (unsigned shift=0;shift<32;shift+=8) input.push_back(static_cast<uint8_t>(word>>shift));
}
int main() {
    static_assert(!std::is_copy_constructible_v<kinoko::Bitmap>);
    kinoko::Bitmap owner;
    auto* bitmap = owner.get();
    const uint16_t palette[]{0x8000,0xffff};
    bitmap->palette = palette;
    for (auto depth : {8,16,24,32}) {
        header(static_cast<uint8_t>(depth),2,2,4,0);
        const size_t bytes = depth<24 ? 4*2*depth/8 : 4*2*4;
        input.resize(17+bytes,0x5a);
        CHECK(kinoko_bitmap_load_cv2(bitmap,"sample.cv2") == 1);
        CHECK(bitmap->bit_depth==depth && bitmap->width==2 && bitmap->height==2 && bitmap->row_width==4);
        CHECK(bitmap->palette==palette && bitmap->pixels[bytes-1]==0x5a && opens==closes);
    }
    auto* previous = bitmap->pixels;
    CHECK(!kinoko_bitmap_load_cv2(bitmap,"missing") && bitmap->pixels==previous);
    input={16,1,2};
    CHECK(!kinoko_bitmap_load_cv2(bitmap,"short-header") && bitmap->pixels==previous && bitmap->bit_depth==32);
    header(16,2,2,4,0); input.resize(18);
    CHECK(!kinoko_bitmap_load_cv2(bitmap,"short-payload") && !bitmap->pixels && bitmap->bit_depth==16);
    CHECK(bitmap->palette==palette && opens==closes);
    header(8,200,100,200,4); input.insert(input.end(),{2,1,3,0});
    CHECK(kinoko_bitmap_load_cv2(bitmap,"encoded") && bitmap->encoded_size==4);
    CHECK(bitmap->pixels[0]==2 && bitmap->pixels[3]==0); // loader preserves encoded bytes
    previous=bitmap->pixels;
    header(32,1,1,1,256u*1024u*1024u+1);
    CHECK(!kinoko_bitmap_load_cv2(bitmap,"limit") && bitmap->pixels==previous);
    CHECK(bitmap->encoded_size==256u*1024u*1024u+1 && opens==closes);
    kinoko_bitmap_release_pixels(bitmap); kinoko_bitmap_release_pixels(bitmap);
    CHECK(!bitmap->pixels && bitmap->palette==palette);
    std::puts("PASS: CV2 header, row storage, encoded payload, replacement failure and reader ownership");
}
