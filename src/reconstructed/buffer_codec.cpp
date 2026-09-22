#include "kinoko/base_utilities.h"
#include <zlib.h>

namespace {
class Codec final {
public:
    z_stream stream{};
    explicit Codec(bool compress) : compress_(compress) {}
    ~Codec() { if (initialized_) finish(); }
    bool initialize() {
        initialized_ = (compress_ ? deflateInit(&stream, Z_DEFAULT_COMPRESSION)
                                  : inflateInit(&stream)) == Z_OK;
        return initialized_;
    }
    int finish() {
        initialized_ = false;
        return compress_ ? deflateEnd(&stream) : inflateEnd(&stream);
    }
    Codec(const Codec&) = delete;
private:
    bool compress_;
    bool initialized_ = false;
};
int32_t transform(bool compress, const void* input, int32_t input_size,
    void* output, int32_t capacity) {
    // Retain the reconstruction's invalid-input checks and error cleanup.
    if (!input || input_size < 0 || (!compress && input_size == 0) ||
        !output || capacity <= 0) return 0;
    Codec codec(compress);
    if (!codec.initialize()) return 0;
    auto& stream = codec.stream;
    stream.next_in = static_cast<Bytef*>(const_cast<void*>(input));
    stream.avail_in = static_cast<uInt>(input_size);
    stream.next_out = static_cast<Bytef*>(output);
    stream.avail_out = static_cast<uInt>(capacity);
    const auto status = compress ? deflate(&stream, Z_FINISH) : inflate(&stream, Z_NO_FLUSH);
    // 404390/404430 accept both END and OK with spare output. In particular,
    // do not replace this with uncompress() or demand END for partial streams.
    if (status != Z_STREAM_END && (status != Z_OK || stream.avail_out == 0)) return 0;
    const auto written = capacity - static_cast<int32_t>(stream.avail_out);
    return codec.finish() == Z_OK ? written : 0;
}
}
extern "C" int32_t kinoko_compress_buffer(const void* input, int32_t size, void* output, int32_t capacity) {
    return transform(true, input, size, output, capacity);
}
extern "C" int32_t kinoko_decompress_buffer(const void* input, int32_t size, void* output, int32_t capacity) {
    return transform(false, input, size, output, capacity);
}
