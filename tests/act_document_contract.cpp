// Compile the real ACT creation/loading boundary, with deterministic archive
// ports. This is not a substitute for DAT/gameplay testing.
#include "kinoko/act_document.h"
#include "kinoko/act_document_records.hpp"
#include "kinoko/act_host.h"
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_memory.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using kinoko::act::DocumentRecord;
using kinoko::act::DocumentView;
using kinoko::legacy::StringView;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
void require(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
const int vtable_identity = 7;
KinokoActHostSymbols host{};
int script_constructions = 0;
void *last_script = nullptr;
struct Fixture {
    std::array<unsigned char, sizeof(DocumentRecord) + 2> bytes;
    Fixture() {
        bytes.fill(0xa5);
        require(kinoko_act_document_initialize(document()) == document(), "initialize returns receiver");
    }
    ~Fixture() {
        StringView(view().bytes(&DocumentRecord::name)).destroy();
        StringView(view().bytes(&DocumentRecord::resource_path)).destroy();
    }
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    KinokoActDocument *document() { return reinterpret_cast<KinokoActDocument *>(bytes.data() + 1); }
    DocumentView view() { return DocumentView(document()); }
};
struct ReaderFixture {
    std::vector<unsigned char> data;
    size_t position = 0;
} archive;
struct State {
    bool open_ok = true, seek_ok = true, throw_payload = false, live = false;
    int opens = 0, closes = 0, reads = 0, seeks = 0, payloads = 0, traces = 0;
    int32_t result = 1;
    KinokoActDocument *document = nullptr;
    const char *path = nullptr;
    uint32_t skip = 0;
    size_t payload_position = 0;
    std::string events;
} state;
void reset(KinokoActDocument *document) {
    require(!state.live, "previous reader leaked");
    state = State{};
    state.document = document;
    archive = ReaderFixture{};
    const uint32_t header[] = {0x31544341u, 1u, 3u};
    archive.data.resize(sizeof(header) + 3);
    std::memcpy(archive.data.data(), header, sizeof(header));
}
void valid_reader(int32_t reader) {
    require(reader == address(&archive) && state.live, "reader is borrowed and alive");
}
void write_word(size_t offset, uint32_t value) {
    std::memcpy(archive.data.data() + offset, &value, sizeof(value));
}
void initialization() {
    require(!kinoko_act_document_initialize(nullptr), "null constructor");
    require(!kinoko_act_document_name(nullptr), "null name");
    require(!kinoko_act_document_screen_width(nullptr) && !kinoko_act_document_screen_height(nullptr), "null dimensions");
    const int before = script_constructions;
    Fixture f;
    const auto v = f.view();
    require(script_constructions == before + 1 && last_script == v.bytes(&DocumentRecord::script), "embedded script constructed once");
    require(v.get(&DocumentRecord::vtable) == &vtable_identity, "preserve host virtual table");
    require(v.get(&DocumentRecord::resolution_ms) == 16, "resolution default");
    require(kinoko_act_document_screen_width(f.document()) == 1280, "width default");
    require(kinoko_act_document_screen_height(f.document()) == 720, "height default");
    require(std::strcmp(kinoko_act_document_name(f.document()), "act") == 0, "native owned document name");
    require(StringView(v.bytes(&DocumentRecord::resource_path)).length() == 0 &&
        StringView(v.bytes(&DocumentRecord::resource_path)).capacity() == 15, "empty resource path");
    require(v.get(&DocumentRecord::margin_left) == 128 && v.get(&DocumentRecord::margin_top) == 128 &&
        v.get(&DocumentRecord::margin_right) == 128 && v.get(&DocumentRecord::margin_bottom) == 128, "four margins");
    require(v.get(&DocumentRecord::offset_x) == 0.0f && v.get(&DocumentRecord::offset_y) == 0.0f, "offsets");
    require(v.get(&DocumentRecord::visible) == 1 && v.get(&DocumentRecord::resources_suspended) == 0, "flags");
    const auto layers = v.get(&DocumentRecord::layers);
    const auto resources = v.get(&DocumentRecord::resources);
    require(!layers.begin && !layers.end && !layers.capacity && !resources.begin && !resources.end && !resources.capacity, "empty pointer spans");
    require(!v.get(&DocumentRecord::unknown40) && !v.get(&DocumentRecord::unknown68) &&
        !v.get(&DocumentRecord::unknown220) && !v.get(&DocumentRecord::unknown236), "unidentified bytes still zero");
    for (auto b : v.get(&DocumentRecord::script)) require(b == 0x39, "do not overwrite script constructor result");
    for (auto b : v.get(&DocumentRecord::padding97)) require(b == 0, "first padding zero");
    for (auto b : v.get(&DocumentRecord::padding205)) require(b == 0, "second padding zero");
    require(f.bytes.front() == 0xa5 && f.bytes.back() == 0xa5, "unaligned fixture guards intact");
    auto *created = kinoko_act_document_create();
    require(created != nullptr && kinoko_act_document_screen_height(created) == 720, "factory returns initialized storage");
    StringView(DocumentView(created).bytes(&DocumentRecord::name)).destroy();
    StringView(DocumentView(created).bytes(&DocumentRecord::resource_path)).destroy();
    std::free(created); // production deletion uses the ACT deleting destructor
}
void header_failures() {
    Fixture f;
    reset(f.document());
    require(!kinoko_act_document_load(nullptr, "a") && !kinoko_act_document_load(f.document(), nullptr), "invalid arguments");
    require(state.opens == 0, "invalid arguments do not open");
    state.open_ok = false;
    require(!kinoko_act_document_load(f.document(), "missing.act"), "open failure");
    require(state.opens == 1 && state.closes == 0 && !state.live, "failed open owns no reader");
    for (size_t length = 0; length < 12; ++length) {
        reset(f.document()); archive.data.resize(length);
        require(!kinoko_act_document_load(f.document(), "truncated.act"), "reject every truncated header prefix");
        require(state.closes == 1 && !state.live && state.payloads == 0 && state.traces == 0, "header failure closes exactly once");
    }
    for (int word = 0; word != 2; ++word) {
        reset(f.document()); write_word(word * 4, word == 0 ? 0x32544341u : 2u);
        require(!kinoko_act_document_load(f.document(), "unsupported.act"), "reject magic/version");
        require(state.closes == 1 && state.seeks == 0 && !state.live, "unsupported header closes before seek");
    }
    reset(f.document()); state.seek_ok = false;
    require(!kinoko_act_document_load(f.document(), "bad-offset.act"), "seek failure");
    require(state.seeks == 1 && state.closes == 1 && state.payloads == 0, "seek failure closes once");
    reset(f.document()); write_word(8, UINT32_MAX);
    require(!kinoko_act_document_load(f.document(), "overflow-offset.act"), "offset out of range");
    require(state.skip == UINT32_MAX && state.closes == 1, "offset forwarded without truncation");
}
void payload_results() {
    Fixture f;
    for (const int result : {0, 1, 23, -7}) {
        reset(f.document()); state.result = result;
        const char path[] = "Source/stage.act";
        require(kinoko_act_document_load(f.document(), path) == result, "forward payload result unchanged");
        require(state.path == path && state.opens == 1, "path borrowed unchanged; no directory fallback");
        require(state.payload_position == 15 && state.skip == 3 && state.reads == 3, "seek is relative to twelve-byte header");
        require(state.events == "ORRRSTTTPTC", "read/trace/parse/close order");
        require(state.closes == 1 && !state.live, "payload path releases reader once");
        require(f.view().get(&DocumentRecord::screen_width) == 777, "failed payload remains caller-owned and partially modified");
    }
    reset(f.document()); state.throw_payload = true;
    bool caught = false;
    try { kinoko_act_document_load(f.document(), "throw.act"); }
    catch (const std::runtime_error& e) { caught = std::strcmp(e.what(), "injected payload exception") == 0; }
    require(caught && state.closes == 1 && !state.live, "reader released while exception propagates");
    require(state.events == "ORRRSTTTPC", "no result trace after exception; close still runs");
}
}
extern "C" const KinokoActHostSymbols *kinoko_act_host_symbols() { return &host; }
extern "C" int32_t retdec_construct_cact_script(int32_t script) {
    ++script_constructions; last_script = pointer<void>(script);
    std::memset(last_script, 0x39, 104); return script;
}
extern "C" int32_t function_407370(int32_t slot, const char *path) {
    ++state.opens; state.events += 'O'; state.path = path;
    require(*pointer<int32_t>(slot) == 0, "new reader slot starts null");
    if (!state.open_ok) return 0;
    state.live = true; *pointer<int32_t>(slot) = address(&archive); return 1;
}
extern "C" void retdec_destroy_reader(int32_t *reader) {
    // Called by a noexcept owner: do not throw assertions out of a destructor.
    if (reader != reinterpret_cast<int32_t *>(&archive) || !state.live) std::abort();
    ++state.closes; state.live = false; state.events += 'C';
}
extern "C" int32_t retdec_reader_read_exact(int32_t reader, void *out, uint32_t size) {
    valid_reader(reader); ++state.reads; state.events += 'R';
    require(size == 4, "header uses three four-byte reads");
    if (size > archive.data.size() - archive.position) return 0;
    std::memcpy(out, archive.data.data() + archive.position, size); archive.position += size; return 1;
}
extern "C" int32_t retdec_reader_seek_relative(int32_t reader, uint32_t offset) {
    valid_reader(reader); ++state.seeks; state.events += 'S'; state.skip = offset;
    require(archive.position == 12, "relative seek begins after header");
    if (!state.seek_ok || offset > archive.data.size() - archive.position) return 0;
    archive.position += offset; return 1;
}
extern "C" int32_t retdec_act_load(int32_t document, int32_t reader, int32_t version) {
    valid_reader(reader); ++state.payloads; state.events += 'P'; state.payload_position = archive.position;
    require(document == address(state.document) && version == 1, "typed document handed to existing payload parser");
    DocumentView(state.document).set(&DocumentRecord::screen_width, int32_t{777});
    if (state.throw_payload) throw std::runtime_error("injected payload exception");
    return state.result;
}
extern "C" void retdec_trace_i32(const char *label, int32_t value) {
    require(state.live, "reader remains alive through final diagnostic call");
    constexpr const char *labels[] = {"act:header-magic", "act:header-version", "act:header-offset", "act:load-result"};
    require(state.traces < 4 && std::strcmp(label, labels[state.traces]) == 0, "diagnostic call sites preserved");
    const int32_t values[] = {0x31544341, 1, 3, state.result};
    require(value == values[state.traces], "diagnostic values unchanged");
    ++state.traces; state.events += 'T';
}
int main() {
    try {
        host.act_vtable = &vtable_identity;
        initialization(); header_failures(); payload_results();
        std::puts("ACT document contract: layout, factory, 12 truncations, header errors, payload results and unwind passed");
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "ACT document contract: %s\n", e.what()); return 1;
    }
}
