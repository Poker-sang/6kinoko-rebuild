#include "kinoko/act_clone.h"
#include "kinoko/act_document.h"
#include "kinoko/act_document_association.hpp"
#include "kinoko/act_script_text.hpp"
#include "kinoko/act_array.h"
#include "kinoko/act_runtime.h"
#include <memory>

namespace {
using namespace kinoko::act;
using kinoko::legacy::address;
using kinoko::legacy::load;

struct DeleteDocument {
    void operator()(KinokoActDocument *value) const {
        retdec_destroy_cact_with_flags(address(value), 1);
    }
};
struct DeleteResource {
    void operator()(KinokoActResource *value) const {
        retdec_destroy_cact_resource(address(value));
    }
};
struct DeleteLayer {
    void operator()(KinokoActLayer *value) const {
        retdec_destroy_cact_layer(address(value));
        std::free(value);
    }
};

template<class Object, size_t Slot>
Object *clone_virtual(Object *source) {
    using Clone = Object *(__thiscall *)(Object *);
    const auto *table = load<const unsigned char *>(source);
    return load<Clone>(table + Slot * sizeof(void *))(source);
}

KinokoActDocument *clone_document(KinokoActDocument *source) {
    std::unique_ptr<KinokoActDocument, DeleteDocument> result(kinoko_act_document_create());
    if (!result) return nullptr;
    DocumentCloneAssociations associations;
    const DocumentView input(source), output(result.get());
    for (auto member : {&DocumentRecord::resolution_ms, &DocumentRecord::screen_width,
                       &DocumentRecord::screen_height}) output.set(member, input.get(member));
    const kinoko::legacy::StringView name(input.bytes(&DocumentRecord::name));
    kinoko::legacy::StringView(output.bytes(&DocumentRecord::name)).assign(name.data(), name.length());
    output.set(&DocumentRecord::offset_x, input.get(&DocumentRecord::offset_x));
    output.set(&DocumentRecord::offset_y, input.get(&DocumentRecord::offset_y));
    for (auto member : {&DocumentRecord::margin_left, &DocumentRecord::margin_right,
                       &DocumentRecord::margin_top, &DocumentRecord::margin_bottom})
        output.set(member, input.get(member));
    output.set(&DocumentRecord::visible, input.get(&DocumentRecord::visible));
    copy_script_text(ScriptTextView(output.bytes(&DocumentRecord::script)),
                     ScriptTextView(input.bytes(&DocumentRecord::script)));

    // 427B20..427BC3: skip source null slots, clone via +0x24, publish then index.
    for (auto *cursor = reinterpret_cast<unsigned char *>(input.get(&DocumentRecord::resources).begin);
         cursor != reinterpret_cast<unsigned char *>(input.get(&DocumentRecord::resources).end);
         cursor += sizeof(KinokoActResource *)) {
        auto *resource = load<KinokoActResource *>(cursor);
        if (!resource) continue;
        std::unique_ptr<KinokoActResource, DeleteResource> copy(clone_virtual<KinokoActResource, 9>(resource));
        if (!copy) throw std::bad_alloc();
        kinoko_act_array_append(address(output.bytes(&DocumentRecord::resources)), address(copy.get()));
        associations.add_resource(copy.release());
    }
    // 427BF0..427D05: layers dispatch +0x14; their key clones bind layouts.
    for (auto *cursor = reinterpret_cast<unsigned char *>(input.get(&DocumentRecord::layers).begin);
         cursor != reinterpret_cast<unsigned char *>(input.get(&DocumentRecord::layers).end);
         cursor += sizeof(KinokoActLayer *)) {
        auto *layer = load<KinokoActLayer *>(cursor);
        if (!layer) continue;
        std::unique_ptr<KinokoActLayer, DeleteLayer> copy(clone_virtual<KinokoActLayer, 5>(layer));
        if (!copy) throw std::bad_alloc();
        kinoko_act_array_append(address(output.bytes(&DocumentRecord::layers)), address(copy.get()));
        associations.add_layer(copy.release());
    }
    associations.bind(result.get());
    return result.release();
}
}

extern "C" KinokoActDocument *__fastcall kinoko_act_clone(KinokoActDocument *source, void *) {
    if (!source) return nullptr;
    try { return clone_document(source); }
    catch (...) { return nullptr; } // retained native ABI failure boundary
}
