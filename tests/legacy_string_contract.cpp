#include "kinoko/legacy_string.h"
#include "kinoko/legacy_string.hpp"
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {
using kinoko::legacy::StringRecord;
using kinoko::legacy::StringView;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
int32_t address(const void* value) {
    return static_cast<int32_t>(reinterpret_cast<uintptr_t>(value));
}
// Own storage only in this fixture. Production StringView is strictly borrowed.
class Fixture final {
    std::array<unsigned char, sizeof(StringRecord) + 2> bytes_{};
public:
    Fixture() {
        bytes_.fill(0x9d);
        StringRecord empty{};
        empty.capacity = StringView::inline_capacity;
        std::memcpy(storage(), &empty, sizeof(empty));
    }
    ~Fixture() { view().destroy(); }
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    void* storage() { return bytes_.data() + 1; }
    int32_t id() { return address(storage()); }
    StringView view() { return StringView(storage()); }
    void assign(const std::string& text) {
        require(retdec_string_assign_n(static_cast<int32_t*>(storage()), text.data(),
            static_cast<uint32_t>(text.size())) == id(), "assign returns its receiver");
    }
    void check(const std::string& text) {
        const auto value = view();
        require(value.length() == text.size(), "logical byte length");
        require(value.capacity() >= value.length(), "capacity covers logical bytes");
        require(std::memcmp(value.data(), text.data(), text.size()) == 0, "logical content");
        require(value.data()[text.size()] == 0, "terminator outside logical content");
        require(bytes_.front() == 0x9d && bytes_.back() == 0x9d, "unaligned record sentinels");
        require(retdec_std_string_data(id()) == value.data(), "shared getter selects same buffer");
    }
};
void growth_and_aliases() {
    Fixture f; f.check("");
    const std::string first = "123456789abcdef";
    f.assign(first); f.check(first);
    require(f.view().capacity() >= 15 && f.view().data() != f.storage(), "text belongs to native string");
    require(kinoko_string_append_n(f.storage(), "g", 1) == f.storage(), "append receiver");
    auto expected = first + 'g'; f.check(expected);
    require(f.view().capacity() >= f.view().length(), "native capacity covers content");
    f.assign(std::string(31, 'x'));
    kinoko_string_append_n(f.storage(), "y", 1); f.check(std::string(31, 'x') + 'y');
    require(f.view().capacity() >= f.view().length(), "native capacity covers content");
    f.assign(std::string(47, 'z'));
    kinoko_string_append_n(f.storage(), "y", 1); f.check(std::string(47, 'z') + 'y');
    require(f.view().capacity() >= f.view().length(), "native capacity covers content");
    f.assign(first);
    kinoko_string_reserve(f.storage(), 15, 1); f.check(first);
    require(f.view().capacity() >= f.view().length(), "native capacity covers content");
    kinoko_string_append_substring(f.storage(), f.storage(), 0, UINT32_MAX); expected = first + first;
    f.check(expected); require(f.view().capacity() >= f.view().length(), "native capacity covers content");
    kinoko_string_append_n(f.storage(), f.view().data() + 5, UINT32_MAX);
    expected += expected.substr(5); f.check(expected);
    const auto self = f.view().data();
    retdec_string_assign_n(static_cast<int32_t*>(f.storage()), self + 3, 11);
    expected = expected.substr(3, 11); f.check(expected);
    retdec_string_assign_n(static_cast<int32_t*>(f.storage()), f.view().data(), f.view().length());
    f.check(expected);
    // Existing assign permits the old terminator as a one-byte source range.
    retdec_string_assign_n(static_cast<int32_t*>(f.storage()), f.view().data() + f.view().length(), 1);
    f.check(std::string(1, '\0'));
    f.assign(std::string("a\0b", 3)); f.check(std::string("a\0b", 3));
    retdec_string_assign_cstr(static_cast<int32_t*>(f.storage()), f.view().data()); f.check("a");
}
void reserve_and_failure_results() {
    Fixture f; f.assign(std::string(40, 'r'));
    const auto allocation = f.view().data(); const auto capacity = f.view().capacity();
    require(kinoko_string_reserve(f.storage(), 0, 0) == 0, "zero capacity returns false even on successful clear");
    f.check(""); require(f.view().data() == allocation && f.view().capacity() == capacity,
        "clear without shrink retains allocation");
    f.assign(std::string(30, 't'));
    kinoko_string_reserve(f.storage(), 20, 1); f.check(std::string(30, 't'));
    require(f.view().capacity() == capacity, "large shrink request does not truncate");
    kinoko_string_reserve(f.storage(), 8, 1); f.check(std::string(8, 't'));
    require(f.view().capacity() >= f.view().length(), "native capacity covers content");
    require(kinoko_string_reserve(f.storage(), UINT32_MAX, 1) == 0, "invalid reserve result");
    require(kinoko_string_grow(f.storage(), UINT32_MAX, f.view().length()) == 0, "invalid grow result");
    require(kinoko_string_append_n(f.storage(), "x", UINT32_MAX) == f.storage(), "overflow append returns receiver");
    require(kinoko_string_append_substring(f.storage(), f.storage(), 900, 1) == f.storage(), "out-of-range append returns receiver");
    require(retdec_string_assign_n(static_cast<int32_t*>(f.storage()), "x", UINT32_MAX) == f.id(),
        "invalid assignment returns receiver");
    f.check(std::string(8, 't'));
    require(!retdec_string_assign_n(nullptr, "x", 1), "null assignment receiver");
    require(!retdec_std_string_data(0) && !kinoko_string_append_n(0, "x", 1), "null string view/append");
    require(!kinoko_string_reserve(0, 10, 0) && !kinoko_string_grow(0, 10, 0), "null reserve/grow");
    require(!kinoko_string_append_substring(f.storage(), 0, 0, 1) && !kinoko_string_append_substring(0, f.storage(), 0, 1),
        "null substring endpoints");
    retdec_string_assign_cstr(static_cast<int32_t*>(f.storage()), nullptr); f.check("");
    // Exercise the recovered opaque return in the unusual zero-capacity case.
    StringRecord zero{};
    require(kinoko_string_grow(&zero, 0, 0) != 0, "short grow returns a borrowed buffer");
    require(StringView(&zero).capacity() >= 16 && StringView(&zero).data()[0] == 0,
        "short grow publishes native storage");
    StringView(&zero).destroy();
}
void deterministic_sequences() {
    Fixture left, right; std::string a, b;
    uint32_t rng = 0x623019d5;
    auto random = [&]() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; };
    for (int i = 0; i < 4000; ++i) {
        const auto n = random() % 90;
        switch (random() % 7) {
        case 0: a = std::string(n, static_cast<char>(random() & 255)); left.assign(a); break;
        case 1: b = std::string(n, static_cast<char>(random() & 255)); right.assign(b); break;
        case 2: {
            const auto pos = random() % (b.size() + 1);
            kinoko_string_append_substring(left.storage(), right.storage(), static_cast<uint32_t>(pos), n);
            a += b.substr(pos, n); break;
        }
        case 3: {
            const auto pos = random() % (a.size() + 1);
            const auto suffix = a.substr(pos, n);
            kinoko_string_append_substring(left.storage(), left.storage(), static_cast<uint32_t>(pos), n);
            a += suffix; break;
        }
        case 4: {
            const auto pos = random() % (a.size() + 1);
            const auto count = (std::min)(static_cast<size_t>(n), a.size() - pos);
            retdec_string_assign_n(static_cast<int32_t*>(left.storage()), left.view().data() + pos,
                static_cast<uint32_t>(count));
            a = a.substr(pos, count); break;
        }
        case 5: kinoko_string_reserve(left.storage(), 0, 0); a.clear(); break;
        case 6:
            kinoko_string_reserve(left.storage(), 8, 1);
            if (a.size() > 8) a.resize(8);
            break;
        }
        left.check(a); right.check(b);
    }
}
void substring_assignment() {
    Fixture source, target;
    const std::string binary("ab\0cdefghijklmnopqrstuvwxyz", 27);
    source.assign(binary);
    for (uint32_t position=0;position<=binary.size();++position) {
        for (uint32_t count : {0u,1u,15u,16u,UINT32_MAX}) {
            target.assign("previous heap allocation must stay independent");
            require(kinoko_string_assign_substring(target.id(),source.id(),position,count)==target.id(),
                "substring returns explicit receiver");
            target.check(binary.substr(position,count)); source.check(binary);
            target.assign(binary);
            const auto buffer=target.view().data(); const auto capacity=target.view().capacity();
            kinoko_string_assign_substring(target.id(),target.id(),position,count);
            target.check(binary.substr(position,count));
            require(target.view().data()==buffer && target.view().capacity()==capacity,
                "self substring erases in place without allocation");
        }
    }
    target.assign("unchanged");
    kinoko_string_assign_substring(target.id(),source.id(),UINT32_MAX,1);
    target.check("unchanged");
    require(!kinoko_string_assign_substring(0,source.id(),0,1),"missing receiver");
}

void scanner_contract() {
    require(retdec_safe_c_string_length(nullptr) == 0, "null missing-length source");
    SYSTEM_INFO system{}; GetSystemInfo(&system);
    const size_t size = 0x100000u + system.dwPageSize;
    auto* memory = static_cast<char*>(VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    require(memory != nullptr, "scanner virtual pages");
    struct Release { void* p; ~Release() { VirtualFree(p, 0, MEM_RELEASE); } } release{memory};
    std::memset(memory, 'a', size);
    memory[system.dwPageSize + 3] = 0;
    DWORD old;
    require(VirtualProtect(memory, system.dwPageSize, PAGE_READONLY, &old) != 0, "read-only first region");
    require(retdec_safe_c_string_length(memory) == system.dwPageSize + 3, "scan across distinct readable regions");
    require(VirtualProtect(memory + system.dwPageSize, system.dwPageSize, PAGE_NOACCESS, &old) != 0,
        "inaccessible second region");
    require(retdec_safe_c_string_length(memory) == 0, "unterminated inaccessible region returns zero");
    require(retdec_safe_c_string_length(memory + system.dwPageSize) == 0, "inaccessible first byte returns zero");
    require(VirtualProtect(memory, size, PAGE_READWRITE, &old) != 0, "restore pages");
    std::memset(memory, 'b', size);
    memory[0x100000u - 1] = 0;
    require(retdec_safe_c_string_length(memory) == 0x100000u - 1, "last permitted terminator");
    memory[0x100000u - 1] = 'b'; memory[0x100000u] = 0;
    require(retdec_safe_c_string_length(memory) == 0, "do not extend the historical one-MiB scan");
}
}
int main() {
    try {
        growth_and_aliases(); reserve_and_failure_results(); deterministic_sequences(); substring_assignment(); scanner_contract();
        std::puts("Legacy string contracts passed: unaligned records, aliases, growth, 4000 sequences, bounded scans");
    } catch (const std::exception& e) { std::fprintf(stderr, "%s\n", e.what()); return 1; }
}
