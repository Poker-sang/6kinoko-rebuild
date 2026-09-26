// Native std::string owns all mutable text. The 24-byte game record exposes
// borrowed data and an opaque owner; no historical SSO/growth allocator remains.
#include "kinoko/legacy_string.h"
#include "kinoko/legacy_string.hpp"
#include <algorithm>
#include <memory>
#include <stdexcept>
namespace kinoko::legacy {
// Active first eight bytes when capacity selects native owned storage.
struct StringOwnerFields { char* data; std::string* owner; };
static_assert(sizeof(StringOwnerFields)==8 && offsetof(StringOwnerFields,owner)==4);

std::string* StringView::owner() const noexcept {
    std::string* result=nullptr;
    if(*this && is_heap()) result=native::RecordView<StringOwnerFields>(storage()).get(&StringOwnerFields::owner);
    return result;
}
void StringView::publish(std::string* value) const noexcept {
    char* bytes=value->data();
    const native::RecordView<StringOwnerFields> fields(storage());
    fields.set(&StringOwnerFields::data,bytes);
    fields.set(&StringOwnerFields::owner,value);
    record_.set(&StringRecord::length,static_cast<uint32_t>(value->size()));
    record_.set(&StringRecord::capacity,(std::max)(16u,static_cast<uint32_t>(value->capacity())));
}
std::string& StringView::ensure_owner() const {
    if(auto* value=owner()) return *value;
    auto value=std::make_unique<std::string>(data(),length());
    auto* result=value.release();publish(result);return *result;
}
void StringView::destroy() const noexcept {
    if(!*this) return;
    delete owner();
    StringRecord empty{};empty.capacity=inline_capacity;
    std::memcpy(storage(),&empty,sizeof(empty));
}
void StringView::assign(const char* source,uint32_t size) const {
    if(!*this || size>maximum_size) return;
    try {
        // Snapshot first: source may be any range in our current buffer,
        // including the old terminator, or our boundary's inline bytes.
        std::string snapshot;
        if(size) {if(source) snapshot.assign(source,size);else snapshot.resize(size);}
        auto& value=ensure_owner();value.assign(snapshot);publish(&value);
    } catch(...) {}
}
void StringView::assign(StringView source,uint32_t position,uint32_t size) const {
    if(!*this || !source || position>source.length()) return;
    assign(source.data()+position,(std::min)(source.length()-position,size));
}
void StringView::append(const char* source,uint32_t size) const {
    if(!*this) return;
    const auto from=reinterpret_cast<uintptr_t>(source),begin=reinterpret_cast<uintptr_t>(data());
    if(source && from>=begin && from<begin+length()) {
        size=(std::min)(size,length()-static_cast<uint32_t>(from-begin));
    }
    if(size>maximum_size-length()) return;
    try {
        std::string snapshot;
        if(size) {if(source) snapshot.assign(source,size);else snapshot.resize(size);}
        auto& value=ensure_owner();value.append(snapshot);publish(&value);
    } catch(...) {}
}
void StringView::append(StringView source,uint32_t position,uint32_t size) const {
    if(!*this || !source || position>source.length()) return;
    append(source.data()+position,(std::min)(source.length()-position,size));
}
bool StringView::reserve(uint32_t requested,bool shrink) const {
    if(!*this || requested==invalid_size) return false;
    try {
        if(shrink && requested<inline_bytes) {
            // Keep the original truncation request, but storage belongs to STL.
            auto& value=ensure_owner();value.resize((std::min)(value.size(),static_cast<size_t>(requested)));
            value.shrink_to_fit();publish(&value);
        } else {
            auto& value=ensure_owner();value.reserve(requested);
            if(!requested) value.clear();publish(&value);
        }
        return requested!=0;
    } catch(...) {return false;}
}
char* StringView::grow(uint32_t requested,uint32_t old_length) const {
    if(!*this || requested==invalid_size || old_length>length()) return 0;
    try {
        auto& value=ensure_owner();value.reserve(requested);value.resize(old_length);publish(&value);
        return value.data();
    } catch(...) {return 0;}
}
} // namespace kinoko::legacy

namespace {
using kinoko::legacy::StringView;
}
extern "C" const char* kinoko_string_data(const void* object) {
    return StringView(const_cast<void*>(object)).data();
}
extern "C" void* kinoko_string_assign_n(void* object, const char* source, uint32_t size) {
    StringView(object).assign(source, size);
    return object;
}
extern "C" void* kinoko_string_assign_cstr(void* object, const char* source) {
    return kinoko_string_assign_n(object, source, kinoko_safe_c_string_length(source));
}
extern "C" void* kinoko_string_assign_substring(void* object, const void* source,
    uint32_t position, uint32_t size) {
    if (!object || !source) return 0;
    StringView(object).assign(StringView(const_cast<void*>(source)), position, size);
    return object;
}
// Address range: 0x4038c0 - 0x4039d3
extern "C" void* kinoko_string_append_n(void* object, const char* source, uint32_t size) {
    StringView(object).append(source, size);
    return object;
}
// Address range: 0x4039e0 - 0x403a8b
extern "C" int32_t kinoko_string_reserve(void* object, uint32_t capacity, int32_t shrink) {
    return StringView(object).reserve(capacity, shrink != 0);
}
// Address range: 0x403bf0 - 0x403cd3
extern "C" void* kinoko_string_append_substring(void* object, const void* source, uint32_t position, uint32_t size) {
    if (!object || !source) return 0;
    StringView(object).append(StringView(const_cast<void*>(source)), position, size);
    return object;
}
// Address range: 0x403ce0 - 0x403e18 (includes original cleanup 403DBC)
extern "C" char* kinoko_string_grow(void* object, uint32_t capacity, uint32_t old_length) {
    return StringView(object).grow(capacity, old_length);
}

extern "C" void kinoko_string_destroy(void* object) {
    StringView(object).destroy();
}
