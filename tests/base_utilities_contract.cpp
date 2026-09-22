// Compiled with the batch; execution is reserved for the user.
#include "kinoko/base_utilities.h"
#include "kinoko/critical_section.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

extern "C" { char* g767 = nullptr; }
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void codecs() {
    // Independent zlib-format fixture for the five bytes "hello".
    const unsigned char compressed[]{0x78,0x9c,0xcb,0x48,0xcd,0xc9,0xc9,0x07,0x00,0x06,0x2c,0x02,0x15};
    std::array<unsigned char, 80> output{};
    require(kinoko_decompress_buffer(compressed, sizeof(compressed), output.data(), 5) == 5,
        "complete stream may exactly fill output");
    require(std::memcmp(output.data(), "hello", 5) == 0, "decode known zlib fixture");
    require(kinoko_decompress_buffer(compressed, sizeof(compressed), output.data(), 4) == 0,
        "insufficient output is rejected");
    require(kinoko_decompress_buffer(compressed, sizeof(compressed) - 4, output.data(), output.size()) == 5,
        "original permits OK with spare output when checksum is absent");
    auto corrupted = std::array<unsigned char, sizeof(compressed)>{};
    std::memcpy(corrupted.data(), compressed, sizeof(compressed)); corrupted.back() ^= 1;
    require(kinoko_decompress_buffer(corrupted.data(), corrupted.size(), output.data(), output.size()) == 0,
        "bad checksum is rejected");
    std::array<unsigned char, 128> encoded{};
    const auto size = kinoko_compress_buffer("hello", 5, encoded.data(), encoded.size());
    require(size > 0 && kinoko_decompress_buffer(encoded.data(), size, output.data(), output.size()) == 5,
        "matching original codec roundtrip");
    require(std::memcmp(output.data(), "hello", 5) == 0, "roundtrip bytes");
    require(kinoko_compress_buffer("", 0, encoded.data(), encoded.size()) > 0, "empty compression accepted");
    require(!kinoko_compress_buffer(nullptr, 5, encoded.data(), encoded.size()), "null input rejected");
    require(!kinoko_compress_buffer("hello", -1, encoded.data(), encoded.size()), "negative length rejected");
    require(!kinoko_compress_buffer("hello", 5, encoded.data(), 1), "compressed output exhaustion rejected");
    require(!kinoko_decompress_buffer(compressed, 0, output.data(), output.size()), "empty decompression rejected");
}
void paths() {
    struct Case { const char* path; const char* directory; const char* file; };
    for (const auto& entry : {
        Case{"C:\\game\\data\\test.pat", "C:\\game\\data\\", "test.pat"},
        Case{"data/actor/test.pat", "data/actor/", "test.pat"},
        Case{"marisaA.dat", "", "marisaA.dat"},
        Case{"C:test.nut", "C:", "test.nut"},
        Case{"data/actor/", "data/actor/", ""}}) {
        char directory[MAX_PATH]{}, file[MAX_PATH]{};
        require(kinoko_path_split(entry.path, directory, file) == 0, "split status");
        require(std::string(directory) == entry.directory && std::string(file) == entry.file,
            "drive/directory and name/extension decomposition");
        std::memset(directory, 0, sizeof(directory));
        require(kinoko_path_split(entry.path, directory, nullptr) == 0 &&
            std::string(directory) == entry.directory, "directory-only path");
    }
    char directory[MAX_PATH]{};
    require(kinoko_path_split(nullptr, directory, nullptr) != 0, "invalid path status");
}
void locks() {
    struct Storage { unsigned before; KinokoCriticalSection lock; unsigned after; } storage{0xa7a7a7a7, {}, 0xb8b8b8b8};
    auto* self = kinoko_critical_section_construct(&storage.lock);
    require(self == &storage.lock && self->methods == &kinoko_critical_section_methods,
        "constructor publishes original-layout receiver and methods");
    EnterCriticalSection(&self->native);
    require(TryEnterCriticalSection(&self->native) != 0, "Win32 recursion is preserved");
    LeaveCriticalSection(&self->native); LeaveCriticalSection(&self->native);
    // Only bit zero releases storage. Bit one alone must leave this stack object intact.
    require(self->methods->destroy(self, nullptr, 2) == self, "nondeleting virtual destructor receiver");
    require(storage.before == 0xa7a7a7a7 && storage.after == 0xb8b8b8b8, "lock bounds canaries");
    kinoko_critical_section_construct(self);
    kinoko_critical_section_destruct(self);
    auto* heap = static_cast<KinokoCriticalSection*>(std::malloc(sizeof(KinokoCriticalSection)));
    require(heap != nullptr, "heap lock allocation");
    kinoko_critical_section_construct(heap);
    require(heap->methods->destroy(heap, nullptr, 1) == heap, "deleting virtual destructor receiver");
}
}
int main() {
    try { codecs(); paths(); locks(); return 0; }
    catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
