#include "kinoko/vorbis_decoder.hpp"
#include <vorbis/vorbisfile.h>
#include <algorithm>
#include <climits>
#include <cstring>
#include <limits>
#include <new>
#include <utility>

namespace kinoko::audio {
namespace {
struct MemorySource {
    const unsigned char* bytes;
    std::size_t size;
    std::size_t position;

    static std::size_t read(void* destination, std::size_t item_size,
                            std::size_t count, void* context) noexcept {
        auto& source = *static_cast<MemorySource*>(context);
        if (!item_size || !count) return 0;
        // fread semantics: return complete items; no multiplication overflow.
        const auto items = (std::min)(count, (source.size - source.position) / item_size);
        const auto bytes = items * item_size;
        if (bytes) std::memcpy(destination, source.bytes + source.position, bytes);
        source.position += bytes;
        return items;
    }
    static int seek(void* context, ogg_int64_t offset, int origin) noexcept {
        auto& source = *static_cast<MemorySource*>(context);
        std::int64_t base;
        switch (origin) {
        case SEEK_SET: base = 0; break;
        case SEEK_CUR: base = static_cast<std::int64_t>(source.position); break;
        case SEEK_END: base = static_cast<std::int64_t>(source.size); break;
        default: return -1;
        }
        // Sources are capped at Win32 LONG_MAX, so the subtraction cannot
        // overflow. Check offset before addition, including INT64_MIN.
        if (offset < -base || offset > static_cast<std::int64_t>(source.size) - base) return -1;
        source.position = static_cast<std::size_t>(base + offset);
        return 0;
    }
    static long tell(void* context) noexcept {
        return static_cast<long>(static_cast<MemorySource*>(context)->position);
    }
};
}
struct VorbisDecoder::State {
    MemorySource source{};
    OggVorbis_File file{};
    Format format{};
    std::int64_t frames = 0;
    bool opened = false;
    ~State() { if (opened) ov_clear(&file); }
};
VorbisDecoder::VorbisDecoder(std::unique_ptr<State> state) noexcept : state_(std::move(state)) {}
VorbisDecoder::~VorbisDecoder() = default;

std::unique_ptr<VorbisDecoder> VorbisDecoder::open(const unsigned char* bytes,
                                                 std::size_t size, int& error) {
    error = OV_EINVAL;
    // ov_callbacks::tell_func uses a Win32 long in the production ABI.
    if (!bytes || !size || size > static_cast<std::size_t>(INT32_MAX)) return {};
    auto state = std::unique_ptr<State>(new (std::nothrow) State);
    if (!state) { error = OV_EFAULT; return {}; }
    state->source = {bytes, size, 0};
    const ov_callbacks callbacks{MemorySource::read, MemorySource::seek, nullptr, MemorySource::tell};
    error = ov_open_callbacks(&state->source, &state->file, nullptr, 0, callbacks);
    if (error < 0) return {};
    state->opened = true;
    const auto* info = ov_info(&state->file, 0);
    if (!info || info->channels <= 0 || info->rate <= 0 ||
        static_cast<unsigned long>(info->rate) > UINT32_MAX) {
        error = OV_EBADHEADER;
        return {};
    }
    state->format = {info->channels, static_cast<std::uint32_t>(info->rate)};
    // DirectSound has one fixed format per track. Do not decode a later link
    // using a stale block alignment and miscount its PCM frames.
    for (int link = 1; link < ov_streams(&state->file); ++link) {
        const auto* next = ov_info(&state->file, link);
        if (!next || next->channels != info->channels || next->rate != info->rate) {
            error = OV_EBADLINK;
            return {};
        }
    }
    state->frames = ov_pcm_total(&state->file, -1);
    if (state->frames < 0) { error = static_cast<int>(state->frames); return {}; }
    auto result = std::unique_ptr<VorbisDecoder>(new (std::nothrow) VorbisDecoder(std::move(state)));
    if (!result) { error = OV_EFAULT; return {}; }
    error = 0;
    return result;
}
VorbisDecoder::Format VorbisDecoder::format() const noexcept { return state_->format; }
std::int64_t VorbisDecoder::frame_count() const noexcept { return state_->frames; }
int VorbisDecoder::read_frames(short* destination, int capacity_frames) noexcept {
    static_assert(sizeof(short) == 2, "Original output is signed 16-bit PCM");
    if (!destination || capacity_frames < 0) return OV_EINVAL;
    if (!capacity_frames) return 0;
    const int channels = state_->format.channels;
    if (channels > INT_MAX / 2) return OV_EINVAL;
    const int frame_bytes = channels * 2;
    const int frames = (std::min)(capacity_frames, INT_MAX / frame_bytes);
    int link = 0;
    const auto bytes = ov_read(&state_->file, reinterpret_cast<char*>(destination),
                              frames * frame_bytes, 0, 2, 1, &link);
    return bytes <= 0 ? static_cast<int>(bytes) : static_cast<int>(bytes / frame_bytes);
}
bool VorbisDecoder::seek_frame(std::int64_t frame) noexcept {
    return frame >= 0 && frame <= state_->frames && ov_pcm_seek(&state_->file, frame) == 0;
}
} // namespace kinoko::audio
