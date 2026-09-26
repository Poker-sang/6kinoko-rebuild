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

// The reader and its scoped owner share the typed file service. The document
// payload dispatch remains the original virtual ABI.
using ArchiveReader = KinokoArchiveReader;
struct CloseReader {
    void operator()(ArchiveReader *reader) const noexcept {
        kinoko_reader_close(reader);
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
    // 427530 initializes members selectively; padding/unknown words are untouched.
    view.set(&DocumentRecord::vtable, kinoko_act_host_symbols()->act_vtable);
    for (auto member : {&DocumentRecord::name, &DocumentRecord::resource_path}) {
        const auto string = view.view(member);
        string.set(&StringRecord::capacity, uint32_t{15});
        string.set(&StringRecord::length, uint32_t{0});
        *string.bytes(&StringRecord::characters) = 0;
    }
    (int32_t)(intptr_t)kinoko_construct_cact_script((void*)(uintptr_t)(address(view.bytes(&DocumentRecord::script))));
    view.set(&DocumentRecord::layers, kinoko::act::DocumentPointerSpan<KinokoActLayer>{});
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
    // 46615E/46F716 and their unwind entries free the raw constructor storage.
    kinoko::legacy::Allocation<KinokoActDocument> allocation(document);
    auto *result = kinoko_act_document_initialize(document);
    allocation.release();
    return result;
}

extern "C" int32_t kinoko_act_document_load(KinokoActDocument *document, const char *file_name) {
    if (!document || !file_name) return 0;
    KinokoArchiveReader *reader_slot = nullptr;
    const auto opened = kinoko_reader_open(&reader_slot, file_name);
    const ReaderOwner reader(reader_slot);
    if (!opened) return 0;

    // 428000 checks magic and offset reads, but not the seek return value.
    // Version starts at zero to avoid reproducing an indeterminate stack read
    // on truncated input; its value, not the read result, controls acceptance.
    uint32_t magic = 0, version = 0, payload_offset = 0;
    const auto borrowed_reader = reader.get();
    if (!kinoko_reader_read_exact(borrowed_reader, &magic, sizeof(magic)) || magic != 0x31544341u)
        return 0;
    kinoko_reader_read_exact(borrowed_reader, &version, sizeof(version));
    if (version != 1u) return 0;
    if (!kinoko_reader_read_exact(borrowed_reader, &payload_offset, sizeof(payload_offset)))
        return 0;
    kinoko_reader_seek_relative(borrowed_reader, payload_offset);

    // Retain these calls in quiet builds: only the diagnostic sink is silent.
    kinoko_trace_i32("act:header-magic", static_cast<int32_t>(magic));
    kinoko_trace_i32("act:header-version", static_cast<int32_t>(version));
    kinoko_trace_i32("act:header-offset", static_cast<int32_t>(payload_offset));
    using ReadDocument = uint8_t (__thiscall *)(KinokoActDocument *, KinokoArchiveReader **, int32_t);
    const auto *table = static_cast<const unsigned char *>(view_of(document).get(&DocumentRecord::vtable));
    const auto read = kinoko::legacy::load<ReadDocument>(table + sizeof(void *));
    const auto result = read(document, &reader_slot, static_cast<int32_t>(version));
    kinoko_trace_i32("act:load-result", result);
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
