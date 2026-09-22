#include "kinoko/act_document.h"
#include "kinoko/act_document_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include <cstring>

namespace {
using namespace kinoko::act;
using kinoko::legacy::load;
// The existing QueryType port compares the decorated name at descriptor+8.
// These descriptors are data for that ABI, never compiler type_info objects.
template<size_t N> struct TypeDescriptor { void *table, *cache; char name[N]; };
const TypeDescriptor<sizeof(".?AVCActResource2D@@")> texture{{}, {}, ".?AVCActResource2D@@"};
const TypeDescriptor<sizeof(".?AVCActRenderTarget@@")> target{{}, {}, ".?AVCActRenderTarget@@"};
const TypeDescriptor<sizeof(".?AVCActResourceMesh@@")> mesh{{}, {}, ".?AVCActResourceMesh@@"};
const TypeDescriptor<sizeof(".?AVCActResourceChip@@")> chip{{}, {}, ".?AVCActResourceChip@@"};
using Query = uint8_t (__thiscall *)(ResourceRecord *, const void *, ResourceRecord **);
using Load = uint8_t (__thiscall *)(ResourceRecord *, const char *);
using Unload = uint8_t (__thiscall *)(ResourceRecord *);
using Create = uint8_t (__thiscall *)(ResourceRecord *, int32_t, int32_t);
struct ResourceMethods {
    void *write, *read;
    Query query;
    void *middle[7];
    Load load;
    Unload unload;
    Create create;
};
struct ResourcePrefix { const ResourceMethods *vtable; };
struct TargetDimensions { unsigned char prefix[72]; int32_t width, height; };
using LoadDocument = uint8_t (__thiscall *)(KinokoActDocument *, const char *);
struct DocumentMethods { void *preceding[6]; LoadDocument load_resources; };
static_assert(offsetof(ResourceMethods, query) == 8);
static_assert(offsetof(ResourceMethods, load) == 40);
static_assert(offsetof(ResourceMethods, unload) == 44);
static_assert(offsetof(ResourceMethods, create) == 48);
static_assert(offsetof(DocumentMethods, load_resources) == 24);
template<class Method> Method method(ResourceRecord *resource, size_t offset) {
    const auto table = load<ResourcePrefix>(resource).vtable;
    return load<Method>(reinterpret_cast<const unsigned char *>(table) + offset);
}
}

extern "C" int32_t kinoko_act_document_load_resources(KinokoActDocument *document, const char *prefix) {
    const auto table = DocumentView(document).get(&DocumentRecord::vtable);
    return load<DocumentMethods>(table).load_resources(document, prefix);
}

// 4289C0: preserve type query order, virtual receivers, non-short-circuit
// accumulation and re-reading the resource end after each resource callback.
extern "C" int32_t __fastcall kinoko_method_load_act_resources(
    KinokoActDocument *document, void *, const char *prefix) {
    const DocumentView view(document);
    const kinoko::legacy::StringView path(view.bytes(&DocumentRecord::resource_path));
    if (prefix) path.assign(prefix, static_cast<uint32_t>(std::strlen(prefix)));
    else prefix = path.data();
    uint8_t result = 1;
    auto *cursor = reinterpret_cast<const unsigned char *>(view.get(&DocumentRecord::resources).begin);
    while (cursor != reinterpret_cast<const unsigned char *>(view.get(&DocumentRecord::resources).end)) {
        auto *resource = load<ResourceRecord *>(cursor);
        ResourceRecord *converted = nullptr;
        const auto query = [resource](const void *type, ResourceRecord **output) {
            return method<Query>(resource, offsetof(ResourceMethods, query))(resource, type, output);
        };
        if (query(&texture, &converted) && converted) {
            result &= method<Load>(converted, offsetof(ResourceMethods, load))(converted, prefix);
        } else if (query(&target, &converted) && converted) {
            const auto dimensions = load<TargetDimensions>(converted);
            result &= method<Create>(converted, offsetof(ResourceMethods, create))(converted, dimensions.width, dimensions.height);
        } else if ((query(&mesh, &converted) && converted) ||
                   (query(&chip, &converted) && converted)) {
            result &= method<Load>(converted, offsetof(ResourceMethods, load))(converted, prefix);
        } else {
            result = 0;
        }
        cursor += sizeof(ResourceRecord *);
    }
    return result;
}

namespace {
// 428AF0/428BD0: only a real CActResource2D unloads/reloads here. Targets,
// Mesh and Chip are recognized but skipped, not coerced to a texture resource.
uint8_t transition_resources(KinokoActDocument *document,bool suspend) {
    const DocumentView view(document);
    const auto old=view.get(&DocumentRecord::resources_suspended);
    if(suspend ? old==1 : old==0) return 0;
    view.set(&DocumentRecord::resources_suspended,static_cast<uint8_t>(suspend));
    const auto *prefix=kinoko::legacy::StringView(view.bytes(&DocumentRecord::resource_path)).data();
    uint8_t result=1;
    auto *cursor=reinterpret_cast<const unsigned char *>(view.get(&DocumentRecord::resources).begin);
    while(cursor!=reinterpret_cast<const unsigned char *>(view.get(&DocumentRecord::resources).end)) {
        ResourceRecord *converted=nullptr;
        const auto query=[cursor](const void *type,ResourceRecord **output) {
            auto *resource=load<ResourceRecord*>(cursor);
            return method<Query>(resource,offsetof(ResourceMethods,query))(resource,type,output);
        };
        if(query(&texture,&converted) && converted) {
            // Do not short-circuit subsequent callbacks after a failure.
            result &= suspend
                ? method<Unload>(converted,offsetof(ResourceMethods,unload))(converted)
                : method<Load>(converted,offsetof(ResourceMethods,load))(converted,prefix);
        } else if(!((query(&target,&converted) && converted) ||
                    (query(&mesh,&converted) && converted) ||
                    (query(&chip,&converted) && converted))) result=0;
        cursor+=sizeof(ResourceRecord*);
    }
    return result;
}
}
extern "C" int32_t __fastcall kinoko_method_suspend_act_resources(KinokoActDocument *document,void *) {
    return transition_resources(document,true);
}
extern "C" int32_t __fastcall kinoko_method_resume_act_resources(KinokoActDocument *document,void *) {
    return transition_resources(document,false);
}
