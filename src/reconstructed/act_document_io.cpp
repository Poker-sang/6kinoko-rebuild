#include "kinoko/act_document.h"
#include "kinoko/act_document_records.hpp"
#include "kinoko/act_host.h"
#include "kinoko/act_runtime.h"
#include "kinoko/legacy_memory.hpp"
#include <cstdlib>
#include <memory>

namespace {
using kinoko::act::DocumentRecord;
using kinoko::act::DocumentView;
using kinoko::legacy::address;
using kinoko::legacy::pointer;
using kinoko::legacy::StringRecord;
using kinoko::legacy::StringView;

// Only the unconverted reader host ports exchange integer address slots.
// The ACT interface and the scoped owner never represent ownership as int32_t.
struct ArchiveReader;
struct CloseReader {
    void operator()(ArchiveReader *reader) const noexcept {
        retdec_destroy_reader(reinterpret_cast<int32_t *>(reader));
    }
};
using ReaderOwner = std::unique_ptr<ArchiveReader, CloseReader>;

DocumentView view_of(const KinokoActDocument *document) {
    // RecordView is an alignment-independent byte view, not a C++ overlay.
    return DocumentView(const_cast<KinokoActDocument *>(document));
}
}

extern "C" KinokoActDocument *kinoko_act_document_initialize(KinokoActDocument *document) {
    if (!document) return nullptr;
    const DocumentView view(document);
    // Preserve R127's whole-record zeroing, including still-unidentified bytes.
    view.clear();
    view.set(&DocumentRecord::vtable, kinoko_act_host_symbols()->act_vtable);
    StringRecord empty{};
    empty.capacity = StringView::inline_capacity;
    view.set(&DocumentRecord::name, empty);
    view.set(&DocumentRecord::resource_path, empty);
    retdec_construct_cact_script(address(view.bytes(&DocumentRecord::script)));
    view.set(&DocumentRecord::layers, kinoko::act::DocumentPointerSpan<kinoko::act::LayerRecord>{});
    view.set(&DocumentRecord::resources, kinoko::act::DocumentPointerSpan<kinoko::act::ResourceRecord>{});
    view.set(&DocumentRecord::resolution_ms, int32_t{16});
    view.set(&DocumentRecord::screen_width, int32_t{1280});
    view.set(&DocumentRecord::screen_height, int32_t{720});
    StringView(view.bytes(&DocumentRecord::name)).assign("act", 3);
    view.set(&DocumentRecord::offset_x, 0.0f);
    view.set(&DocumentRecord::margin_left, int32_t{128});
    view.set(&DocumentRecord::offset_y, 0.0f);
    view.set(&DocumentRecord::margin_top, int32_t{128});
    view.set(&DocumentRecord::margin_right, int32_t{128});
    view.set(&DocumentRecord::margin_bottom, int32_t{128});
    view.set(&DocumentRecord::visible, uint8_t{1});
    view.set(&DocumentRecord::resources_suspended, uint8_t{0});
    return document;
}

extern "C" KinokoActDocument *kinoko_act_document_create() {
    auto *document = static_cast<KinokoActDocument *>(std::malloc(sizeof(DocumentRecord)));
    return kinoko_act_document_initialize(document);
}

extern "C" int32_t kinoko_act_document_load(KinokoActDocument *document, const char *file_name) {
    if (!document || !file_name) return 0;
    int32_t legacy_reader_slot = 0;
    const auto opened = function_407370(address(&legacy_reader_slot), file_name);
    const ReaderOwner reader(pointer<ArchiveReader>(legacy_reader_slot));
    if (!opened) return 0;

    uint32_t magic = 0, version = 0, payload_offset = 0;
    const auto borrowed_reader = address(reader.get());
    if (!retdec_reader_read_exact(borrowed_reader, &magic, sizeof(magic)) ||
        magic != 0x31544341u ||
        !retdec_reader_read_exact(borrowed_reader, &version, sizeof(version)) ||
        version != 1u ||
        !retdec_reader_read_exact(borrowed_reader, &payload_offset, sizeof(payload_offset)) ||
        !retdec_reader_seek_relative(borrowed_reader, payload_offset))
        return 0;

    // Retain these calls in quiet builds: only the diagnostic sink is silent.
    retdec_trace_i32("act:header-magic", static_cast<int32_t>(magic));
    retdec_trace_i32("act:header-version", static_cast<int32_t>(version));
    retdec_trace_i32("act:header-offset", static_cast<int32_t>(payload_offset));
    const auto result = retdec_act_load(address(document), borrowed_reader, static_cast<int32_t>(version));
    retdec_trace_i32("act:load-result", result);
    return result; // reader closes after the payload loader/last trace, once
}

extern "C" const char *kinoko_act_document_name(const KinokoActDocument *document) {
    return document ? StringView(view_of(document).bytes(&DocumentRecord::name)).data() : nullptr;
}
extern "C" int32_t kinoko_act_document_screen_width(const KinokoActDocument *document) {
    return document ? view_of(document).get(&DocumentRecord::screen_width) : 0;
}
extern "C" int32_t kinoko_act_document_screen_height(const KinokoActDocument *document) {
    return document ? view_of(document).get(&DocumentRecord::screen_height) : 0;
}
