// Actual 4289C0 implementation with virtual resource endpoints as fixtures.
#include "kinoko/act_document.h"
#include "kinoko/act_document_records.hpp"
#include "kinoko/legacy_memory.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace kinoko::act;
struct Resource {
    void **vtable;
    unsigned char padding[68];
    int32_t width, height;
    const char *type;
    int id;
    uint8_t result;
};
static_assert(offsetof(Resource, width) == 72);
static std::string calls;
static DocumentRecord document{};
static bool shorten;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "resource pass line %d: %s\n", __LINE__, #x); std::abort(); } } while (0)
uint8_t __fastcall query(Resource *self, void *, const void *type, Resource **output) {
    const auto *name = static_cast<const char *>(type) + 8;
    calls += std::to_string(self->id) + name + ";";
    *output = std::strcmp(name, self->type) == 0 ? self : nullptr;
    return *output != nullptr;
}
uint8_t __fastcall load_resource(Resource *self, void *, const char *prefix) {
    CHECK(std::strcmp(prefix, "assets") == 0);
    calls += "L" + std::to_string(self->id) + ";";
    if (shorten) document.resources.end = document.resources.begin + 1;
    return self->result;
}
uint8_t __fastcall create(Resource *self, void *, int width, int height) {
    CHECK(width == 320 && height == 200);
    calls += "C" + std::to_string(self->id) + ";";
    return self->result;
}
int main() {
    std::array<void *, 13> resource_table{};
    resource_table[2] = reinterpret_cast<void *>(query);
    resource_table[10] = reinterpret_cast<void *>(load_resource);
    resource_table[12] = reinterpret_cast<void *>(create);
    std::array<void *, 7> document_table{};
    document_table[6] = reinterpret_cast<void *>(kinoko_method_load_act_resources);
    document.vtable = document_table.data();
    document.resource_path.capacity = 15;
    Resource resources[] = {
        {resource_table.data(), {}, 0, 0, ".?AVCActResource2D@@", 1, 0},
        {resource_table.data(), {}, 320, 200, ".?AVCActRenderTarget@@", 2, 1},
        {resource_table.data(), {}, 0, 0, ".?AVCActResourceMesh@@", 3, 1},
        {resource_table.data(), {}, 0, 0, ".?AVCActResourceChip@@", 4, 1},
        {resource_table.data(), {}, 0, 0, "unknown", 5, 1},
    };
    ResourceRecord *slots[5];
    for (int i = 0; i != 5; ++i) slots[i] = reinterpret_cast<ResourceRecord *>(&resources[i]);
    document.resources.begin = slots;
    document.resources.end = slots + 5;
    auto *act = reinterpret_cast<KinokoActDocument *>(&document);
    CHECK(kinoko_act_document_load_resources(act, "assets") == 0);
    CHECK(calls.find("L1;") < calls.find("C2;"));
    CHECK(calls.find("C2;") < calls.find("L3;"));
    CHECK(calls.find("L3;") < calls.find("L4;"));
    CHECK(calls.find("L5;") == std::string::npos);
    CHECK(calls.find("4.?AVCActResource2D@@;4.?AVCActRenderTarget@@;4.?AVCActResourceMesh@@;4.?AVCActResourceChip@@;L4;") != std::string::npos);
    calls.clear();
    resources[0].result = 1;
    shorten = true;
    CHECK(kinoko_act_document_load_resources(act, nullptr) == 1);
    CHECK(calls == "1.?AVCActResource2D@@;L1;");
    document.resources.end = slots;
    calls.clear();
    CHECK(kinoko_act_document_load_resources(act, nullptr) == 1 && calls.empty());
    kinoko::legacy::StringView(&document.resource_path).destroy();
    std::puts("PASS: resource virtual dispatch, query order, prefix reuse, accumulated failure and callback end reload");
}
