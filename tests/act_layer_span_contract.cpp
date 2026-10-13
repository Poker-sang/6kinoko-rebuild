#include "kinoko/act_layer_records.hpp"
#include <array>
#include <cstdio>

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "layer span line %d: %s\n", __LINE__, #condition); \
    return 1; } } while (0)

int main() {
    using namespace kinoko::act;
    std::array<unsigned char, 4> objects{};
    auto* first = reinterpret_cast<KinokoActLayer*>(&objects[0]);
    auto* second = reinterpret_cast<KinokoActLayer*>(&objects[1]);
    auto* outside = reinterpret_cast<KinokoActLayer*>(&objects[2]);
    auto* absent = reinterpret_cast<KinokoActLayer*>(&objects[3]);
    // The actual ACT array owns void* slots. The third entry lies immediately
    // outside this document's two-layer span and must never become a layer.
    std::array<void*, 3> slots{first, second, outside};
    DocumentPointerSpan<KinokoActLayer> span{
        reinterpret_cast<KinokoActLayer**>(slots.data()),
        reinterpret_cast<KinokoActLayer**>(slots.data() + 2), nullptr};
    int32_t count = -1;
    CHECK(checked_layer_count(span, count) && count == 2);
    CHECK(find_layer_order(span, first) == 0);
    CHECK(find_layer_order(span, second) == 1);
    CHECK(find_layer_order(span, outside) == -1);
    CHECK(find_layer_order(span, absent) == -1);
    CHECK(layer_at(span, 0) == first && layer_at(span, 1) == second);

    // An empty span, either null or allocated, has no readable slots.
    CHECK(checked_layer_count({}, count) && count == 0);
    CHECK(find_layer_order({}, nullptr) == -1);
    auto empty = span;
    empty.end = empty.begin;
    CHECK(checked_layer_count(empty, count) && count == 0);
    CHECK(find_layer_order(empty, first) == -1);

    auto malformed = span;
    malformed.begin = span.end;
    malformed.end = span.begin;
    CHECK(!checked_layer_count(malformed, count) && count == 0);
    CHECK(find_layer_order(malformed, first) == -1);
    malformed = span;
    malformed.begin = nullptr;
    CHECK(!checked_layer_count(malformed, count) && count == 0);
    malformed = span;
    malformed.end = reinterpret_cast<KinokoActLayer**>(
        reinterpret_cast<unsigned char*>(slots.data()) + sizeof(void*) - 1);
    CHECK(!checked_layer_count(malformed, count) && count == 0);
    CHECK(find_layer_order(malformed, first) == -1);
    std::puts("PASS: native-width ACT layer counts, order and malformed span bounds");
    return 0;
}
