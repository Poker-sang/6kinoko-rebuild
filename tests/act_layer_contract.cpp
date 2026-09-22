// Real layer/key query source with only its allocator and trace ports controlled.
// Raw fixture offsets are deliberately independent of the production schemas.
#include "kinoko/act_layer_access.h"
#include "kinoko/act_layer_records.hpp"
#include "kinoko/stage_records.hpp"
#include <array>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <type_traits>

namespace test {
int allocation_count, fail_at;
std::vector<std::string> trace;
template<class T> void put(void *p, const T& value) { std::memcpy(p, &value, sizeof(value)); }
template<class T> T get(const void *p) { T value; std::memcpy(&value, p, sizeof(value)); return value; }
template<size_t N> struct Raw {
    std::array<unsigned char, N + 2> data{};
    Raw() { data.front() = data.back() = 0xa5; }
    unsigned char *bytes() { return data.data() + 1; }
    template<class T> T *as() { return reinterpret_cast<T *>(bytes()); }
    bool guarded() const { return data.front() == 0xa5 && data.back() == 0xa5; }
};
}
extern "C" int32_t _3f__3f_2_40_YAPAXI_40_Z(int32_t size) {
    if (++test::allocation_count == test::fail_at) return 0;
    return kinoko::legacy::address(std::malloc(static_cast<size_t>(size)));
}
extern "C" void retdec_trace(const char *label) { test::trace.emplace_back(label); }
extern "C" void retdec_trace_i32(const char *, int32_t) {}
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "layer line %d: %s\n", __LINE__, #c); return 1; } } while (0)

static_assert(std::is_same_v<decltype(kinoko_act_layer_layout(nullptr, 0)), KinokoActLayout *>);
static_assert(std::is_same_v<decltype(kinoko_act_first_key(nullptr, 0)), KinokoActKey *>);

int main() {
    using namespace test;
    Raw<192> runtime;
    Raw<240> document;
    Raw<200> layer;
    Raw<8> key, second_key;
    Raw<12> head, first, second;
    std::array<int, 2> layout{123, 456};
    KinokoActLayer *layers[] = {layer.as<KinokoActLayer>(), nullptr};
    KinokoActSourceHolder source{document.as<KinokoActDocument>()};
    KinokoActLayerHolder borrowed{layer.as<KinokoActLayer>()};
    auto *self = runtime.as<KinokoActRuntime>();
    put(runtime.bytes()+16, &source);
    put(document.bytes()+208, layers+0); put(document.bytes()+212, layers+2);
    put(layer.bytes()+180, head.bytes()); put(layer.bytes()+184, int32_t{2});
    put(head.bytes(), first.bytes()); put(head.bytes()+4, second.bytes());
    put(first.bytes(), second.bytes()); put(first.bytes()+4, head.bytes());
    put(first.bytes()+8, key.as<KinokoActKey>());
    put(second.bytes(), head.bytes()); put(second.bytes()+4, first.bytes());
    put(second.bytes()+8, second_key.as<KinokoActKey>());
    put(key.bytes(), int32_t{7}); put(second_key.bytes(), int32_t{8});
    put(key.bytes()+4, reinterpret_cast<KinokoActLayout *>(layout.data()));
    put(second_key.bytes()+4, reinterpret_cast<KinokoActLayout *>(layout.data()+1));
    CHECK(!kinoko_act_first_key(nullptr, 0));
    CHECK(!kinoko_act_first_key(self, 0));
    runtime.bytes()[8] = 1;
    const auto runtime_before = runtime.data;
    const auto document_before = document.data;
    const auto layer_before = layer.data;
    for (int i=0; i<100; ++i) {
        trace.clear();
        CHECK(kinoko_act_first_key(self, 0) == key.as<KinokoActKey>());
        CHECK((trace == std::vector<std::string>{
            "452040:free-value-holder-before", "452040:free-value-holder-after",
            "452040:free-item-holder-before", "452040:free-item-holder-after"}));
        CHECK(kinoko_act_layer_layout(self, 0) == reinterpret_cast<KinokoActLayout *>(layout.data()));
    }
    CHECK(runtime.data == runtime_before && document.data == document_before && layer.data == layer_before);
    CHECK(get<int32_t>(key.bytes()) == 7 && layout[0] == 123);
    KinokoActKeyHolder *key_result = nullptr;
    CHECK(kinoko_act_key_holder(&borrowed, 1, &key_result) == &key_result);
    CHECK(key_result && key_result->key == second_key.as<KinokoActKey>());
    std::free(key_result);
    CHECK(kinoko_act_key_holder(&borrowed, 2, &key_result) == &key_result && !key_result);
    CHECK(!kinoko_act_layer_holder(reinterpret_cast<KinokoActSourceHolder *>(1), 0, nullptr));
    KinokoActLayerHolder *layer_result = nullptr;
    CHECK(kinoko_act_layer_holder(reinterpret_cast<KinokoActSourceHolder *>(1), -1, &layer_result) == &layer_result && !layer_result);
    CHECK(kinoko_act_key_holder(reinterpret_cast<KinokoActLayerHolder *>(1), -1, &key_result) == &key_result && !key_result);
    CHECK(!kinoko_act_first_key(self, -1) && !kinoko_act_first_key(self, 1) && !kinoko_act_first_key(self, 2));
    put(layer.bytes()+196, int32_t{1}); CHECK(!kinoko_act_first_key(self, 0));
    put(layer.bytes()+196, int32_t{0}); put(layer.bytes()+184, int32_t{0});
    CHECK(!kinoko_act_first_key(self, 0)); put(layer.bytes()+184, int32_t{2});
    for (int fail=1; fail<=2; ++fail) {
        allocation_count = 0; fail_at = fail;
        CHECK(!kinoko_act_first_key(self, 0) && allocation_count == fail);
    }
    fail_at = 0;
    put(first.bytes()+8, static_cast<KinokoActKey *>(nullptr));
    CHECK(!kinoko_act_first_key(self, 0)); put(first.bytes()+8, key.as<KinokoActKey>());
    put(first.bytes(), static_cast<void *>(nullptr));
    CHECK(kinoko_act_key_holder(&borrowed, 1, &key_result) == &key_result && !key_result);
    put(head.bytes(), static_cast<void *>(nullptr)); CHECK(!kinoko_act_first_key(self, 0));
    put(document.bytes()+212, layers+0); CHECK(!kinoko_act_first_key(self, 0));
    put(document.bytes()+208, layers+2); CHECK(!kinoko_act_first_key(self, 0));
    source.document = nullptr; CHECK(!kinoko_act_first_key(self, 0));
    put(runtime.bytes()+16, static_cast<KinokoActSourceHolder *>(nullptr)); CHECK(!kinoko_act_first_key(self, 0));
    CHECK(runtime.guarded() && document.guarded() && layer.guarded() && key.guarded() && second_key.guarded());
    CHECK(head.guarded() && first.guarded() && second.guarded());
    std::puts("PASS: typed ACT layer/key bounds, allocation failures, borrowed records and release trace order");
    return 0;
}
