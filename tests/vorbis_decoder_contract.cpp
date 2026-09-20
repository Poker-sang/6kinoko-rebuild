#include "kinoko/vorbis_decoder.hpp"
#include <ogg/ogg.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdint>
#include <utility>
#include <iterator>

#define REQUIRE(value) do { if (!(value)) { \
    std::fprintf(stderr, "vorbis contract line %d: %s\n", __LINE__, #value); std::abort(); \
} } while (0)

namespace {
using kinoko::audio::VorbisDecoder;
std::vector<unsigned char> encode(int channels, int rate, int count, int serial) {
    vorbis_info info{}; vorbis_info_init(&info);
    REQUIRE(vorbis_encode_init_vbr(&info, channels, rate, 0.25f) == 0);
    vorbis_comment comment{}; vorbis_comment_init(&comment);
    vorbis_dsp_state dsp{}; REQUIRE(vorbis_analysis_init(&dsp, &info) == 0);
    vorbis_block block{}; REQUIRE(vorbis_block_init(&dsp, &block) == 0);
    ogg_stream_state stream{}; REQUIRE(ogg_stream_init(&stream, serial) == 0);
    std::vector<unsigned char> bytes;
    auto page = [&](const ogg_page& value) {
        bytes.insert(bytes.end(), value.header, value.header + value.header_len);
        bytes.insert(bytes.end(), value.body, value.body + value.body_len);
    };
    ogg_packet header{}, comments{}, books{};
    REQUIRE(vorbis_analysis_headerout(&dsp, &comment, &header, &comments, &books) == 0);
    ogg_stream_packetin(&stream, &header); ogg_stream_packetin(&stream, &comments);
    ogg_stream_packetin(&stream, &books);
    ogg_page output{};
    while (ogg_stream_flush(&stream, &output)) page(output);
    auto drain = [&] {
        while (vorbis_analysis_blockout(&dsp, &block) == 1) {
            REQUIRE(vorbis_analysis(&block, nullptr) == 0);
            REQUIRE(vorbis_bitrate_addblock(&block) == 0);
            ogg_packet packet{};
            while (vorbis_bitrate_flushpacket(&dsp, &packet)) {
                REQUIRE(ogg_stream_packetin(&stream, &packet) == 0);
                while (ogg_stream_pageout(&stream, &output)) page(output);
            }
        }
    };
    for (int first = 0; first < count; first += 256) {
        const int frames = (std::min)(count - first, 256);
        float** buffer = vorbis_analysis_buffer(&dsp, frames);
        REQUIRE(buffer != nullptr);
        for (int channel = 0; channel < channels; ++channel) {
            for (int frame = 0; frame < frames; ++frame) {
                buffer[channel][frame] = static_cast<float>(0.3 * std::sin(
                    (first + frame) * (channel + 1) * 0.073));
            }
        }
        REQUIRE(vorbis_analysis_wrote(&dsp, frames) == 0);
        drain();
    }
    REQUIRE(vorbis_analysis_wrote(&dsp, 0) == 0); drain();
    while (ogg_stream_flush(&stream, &output)) page(output);
    ogg_stream_clear(&stream); vorbis_block_clear(&block); vorbis_dsp_clear(&dsp);
    vorbis_comment_clear(&comment); vorbis_info_clear(&info);
    return bytes;
}
std::vector<short> read_all(VorbisDecoder& decoder, int chunk) {
    std::vector<short> pcm;
    std::vector<short> buffer(static_cast<std::size_t>(chunk) * decoder.format().channels);
    int iterations = 0;
    for (;;) {
        REQUIRE(++iterations < 100000);
        const int got = decoder.read_frames(buffer.data(), chunk);
        REQUIRE(got >= 0 && got <= chunk);
        if (!got) break;
        pcm.insert(pcm.end(), buffer.begin(), buffer.begin() + got * decoder.format().channels);
    }
    return pcm;
}
// Independent upstream FILE-callback decoding, in the same CRT as this test.
// It is an oracle for the memory callback adapter, not an original-game oracle.
std::vector<short> upstream_pcm(const std::vector<unsigned char>& bytes) {
    FILE* file = std::tmpfile(); REQUIRE(file != nullptr);
    REQUIRE(std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size());
    std::rewind(file);
    OggVorbis_File stream{}; REQUIRE(ov_open(file, &stream, nullptr, 0) == 0);
    std::vector<short> result;
    short pcm[2048]{};
    for (;;) {
        int section = 0;
        const auto got = ov_read(&stream, reinterpret_cast<char*>(pcm), sizeof pcm, 0, 2, 1, &section);
        REQUIRE(got >= 0);
        if (!got) break;
        result.insert(result.end(), pcm, pcm + got / sizeof(short));
    }
    REQUIRE(ov_clear(&stream) == 0); // closes the test FILE
    return result;
}
void check_stream(int channels, int rate) {
    constexpr int frames = 12000;
    const auto bytes = encode(channels, rate, frames, 91 + channels);
    int error = 777;
    auto decoder = VorbisDecoder::open(bytes.data(), bytes.size(), error);
    REQUIRE(decoder && error == 0);
    REQUIRE(decoder->format().channels == channels && decoder->format().sample_rate == static_cast<unsigned>(rate));
    REQUIRE(decoder->frame_count() == frames);
    const auto expected = upstream_pcm(bytes);
    REQUIRE(expected.size() == static_cast<std::size_t>(frames * channels));
    REQUIRE(read_all(*decoder, 2048) == expected);
    short sentinel[32]; std::fill(std::begin(sentinel), std::end(sentinel), short{12345});
    REQUIRE(decoder->read_frames(sentinel, 1) == 0 && sentinel[0] == 12345);
    REQUIRE(!decoder->seek_frame(-1) && !decoder->seek_frame(INT64_MAX));
    REQUIRE(!decoder->seek_frame(frames + 1));
    REQUIRE(decoder->read_frames(nullptr, 2) == OV_EINVAL);
    REQUIRE(decoder->read_frames(sentinel, -1) == OV_EINVAL);
    REQUIRE(decoder->read_frames(sentinel, 0) == 0);
    REQUIRE(decoder->seek_frame(0));
    REQUIRE(read_all(*decoder, 7) == expected); // repeated partial packet reads
    for (int at : {0, 1, 129, 4096, 11999, 12000}) {
        REQUIRE(decoder->seek_frame(at));
        auto moved = std::move(decoder); REQUIRE(!decoder);
        const auto tail = read_all(*moved, 173);
        REQUIRE(tail == std::vector<short>(expected.begin() + at * channels, expected.end()));
        decoder = std::move(moved);
    }
}
}
int main() {
    int error = 0; unsigned char garbage[64]{};
    REQUIRE(!VorbisDecoder::open(nullptr, 1, error) && error == OV_EINVAL);
    REQUIRE(!VorbisDecoder::open(garbage, 0, error));
    REQUIRE(!VorbisDecoder::open(garbage, static_cast<std::size_t>(INT32_MAX) + 1, error));
    REQUIRE(!VorbisDecoder::open(garbage, sizeof garbage, error) && error < 0);
    check_stream(1, 44100); check_stream(2, 44100); check_stream(2, 22050);
    auto first = encode(2, 44100, 900, 700);
    const auto truncated = VorbisDecoder::open(first.data(), 4, error);
    REQUIRE(!truncated);
    const auto second = encode(2, 44100, 1500, 701);
    first.insert(first.end(), second.begin(), second.end());
    auto chain = VorbisDecoder::open(first.data(), first.size(), error);
    REQUIRE(chain && chain->frame_count() == 2400);
    REQUIRE(read_all(*chain, 31) == upstream_pcm(first));
    const auto wrong_format = encode(1, 44100, 200, 702);
    first.insert(first.end(), wrong_format.begin(), wrong_format.end());
    REQUIRE(!VorbisDecoder::open(first.data(), first.size(), error) && error == OV_EBADLINK);
    std::puts("PASS: upstream 1.2.0 PCM, memory callbacks, format, seek, EOF, partial reads, ownership and chains");
}
