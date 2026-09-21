// Production association code and native child container, without a VM/game.
#include "kinoko/act_document_association.hpp"
#include "kinoko/act_layer_access.h"
#include "kinoko/act_array.h"
#include "kinoko/legacy_memory.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace kinoko::act;
using kinoko::legacy::load;
using kinoko::legacy::address;
using LayerView = kinoko::native::RecordView<LayerAssociationRecord>;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "association line %d: %s\n", __LINE__, #x); std::abort(); } } while (0)
static std::vector<KinokoActLayer *> calls;
static KinokoActLayer *change_next;
int32_t __fastcall set_resource(KinokoActLayer *self, void *, KinokoActResource *resource) {
    calls.push_back(self);
    if (change_next) {
        LayerView(change_next).set(&LayerAssociationRecord::resource_id, int32_t{70});
        change_next = nullptr;
    }
    return kinoko_act_layer_set_resource(self, nullptr, resource);
}
struct Layer {
    // The tail stands for the keys/layouts: association must never touch it.
    LayerAssociationRecord prefix{};
    std::array<unsigned char, 348 - sizeof(LayerAssociationRecord)> tail;
    Layer(int32_t id, int32_t parent, const void *table) {
        prefix.vtable = table; prefix.layer_id = id; prefix.parent_id = parent;
        prefix.resource_id = 70; tail.fill(0xa5);
    }
    KinokoActLayer *get() { return reinterpret_cast<KinokoActLayer *>(this); }
    ~Layer() { kinoko_act_array_destroy(address(&prefix.children)); }
};
int main() {
    std::array<void *, 7> table{};
    table[6] = reinterpret_cast<void *>(set_resource);
    Layer child(20, 10, table.data()), first(10, -7, table.data()),
        duplicate(10, -1, table.data()), missing(30, 999, table.data()),
        second_child(40, 10, table.data()), self(50, 50, table.data());
    KinokoActLayer *slots[] = {child.get(), first.get(), duplicate.get(), missing.get(), second_child.get(), self.get()};
    DocumentRecord document{};
    document.layers = {slots, slots + 6, nullptr};
    auto *act = reinterpret_cast<KinokoActDocument *>(&document);
    DocumentLoadAssociations associations;
    for (auto *layer : slots) associations.add_layer(layer);
    associations.bind_loaded_parents(act, 6);
    CHECK(child.prefix.parent == first.get() && second_child.prefix.parent == first.get());
    CHECK(first.prefix.parent_id == -7 && !first.prefix.parent);
    CHECK(!duplicate.prefix.children.storage);
    CHECK(missing.prefix.parent_id == -1 && !missing.prefix.parent);
    CHECK(self.prefix.parent == self.get()); // no invented cycle rejection
    CHECK(first.prefix.children.storage->size() == 2);
    CHECK((*first.prefix.children.storage)[0] == address(child.get()));
    CHECK((*first.prefix.children.storage)[1] == address(second_child.get()));
    // First duplicate wins, signed negative IDs also participate in resources.
    ResourceIdentityRecord resource{nullptr, 70}, duplicate_resource{nullptr, 70}, negative{nullptr, -3};
    auto *r = reinterpret_cast<KinokoActResource *>(&resource);
    associations.begin_resources();
    associations.add_resource(r);
    associations.add_resource(reinterpret_cast<KinokoActResource *>(&duplicate_resource));
    associations.add_resource(reinterpret_cast<KinokoActResource *>(&negative));
    duplicate.prefix.resource_id = -3;
    missing.prefix.resource_id = 12345;
    missing.prefix.resource = reinterpret_cast<KinokoActResource *>(&duplicate_resource);
    first.prefix.resource_id = 12345;
    change_next = first.get(); // callback changes the next layer before its lookup
    associations.bind_resources(act, 6);
    CHECK(calls.size() == 5 && calls[0] == child.get() && calls[1] == first.get());
    CHECK(child.prefix.resource == r && first.prefix.resource == r);
    CHECK(duplicate.prefix.resource == reinterpret_cast<KinokoActResource *>(&negative));
    CHECK(missing.prefix.resource == reinterpret_cast<KinokoActResource *>(&duplicate_resource));
    CHECK(missing.prefix.resource_id == 12345);
    CHECK(kinoko_act_layer_set_resource(child.get(), nullptr, nullptr) == static_cast<int32_t>(0x80004005u));
    CHECK(child.prefix.resource == r && child.prefix.resource_id == 70);
    for (Layer *layer : {&child, &first, &duplicate, &missing, &second_child, &self})
        for (auto byte : layer->tail) CHECK(byte == 0xa5);
    std::puts("PASS: fresh parent resolution, first duplicate IDs, missing references, ordered virtual resource calls, untouched layout tail");
}
