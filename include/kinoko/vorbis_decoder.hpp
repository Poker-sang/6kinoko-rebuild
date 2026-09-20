#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>

namespace kinoko::audio {
// A libvorbisfile stream over borrowed immutable archive bytes, never FILE*.
// The byte owner must outlive this object. The opaque codec state stays at one
// address while the unique_ptr owning this decoder can be moved between tracks.
class VorbisDecoder final {
public:
    struct Format { int channels; std::uint32_t sample_rate; };
    static std::unique_ptr<VorbisDecoder> open(const unsigned char* bytes,
                                             std::size_t size, int& error);
    ~VorbisDecoder();
    VorbisDecoder(const VorbisDecoder&) = delete;
    VorbisDecoder& operator=(const VorbisDecoder&) = delete;
    Format format() const noexcept;
    std::int64_t frame_count() const noexcept;
    // One ov_read call, not an aggregate fill. Returns interleaved PCM frames,
    // zero for EOF, or the negative libvorbisfile error. Caller controls retry.
    int read_frames(short* destination, int capacity_frames) noexcept;
    bool seek_frame(std::int64_t frame) noexcept;
private:
    struct State;
    explicit VorbisDecoder(std::unique_ptr<State> state) noexcept;
    std::unique_ptr<State> state_;
};
} // namespace kinoko::audio
