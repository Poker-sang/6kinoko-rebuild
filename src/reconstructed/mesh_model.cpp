#include "kinoko/mesh_model.hpp"
#include "kinoko/act_runtime.h"
#include "kinoko/act_host.h"
#include "kinoko/legacy_abi.h"
#include <limits>
#include <stdexcept>

namespace kinoko::mesh {
namespace {
class Reader {
    int32_t stream_;
public:
    explicit Reader(int32_t stream) : stream_(stream) {}
    void bytes(void *destination, size_t size) {
        if (size > std::numeric_limits<uint32_t>::max() ||
            (size && !retdec_reader_read_exact(stream_, destination, static_cast<uint32_t>(size))))
            throw std::runtime_error("truncated MSH stream");
    }
    template<class T> void value(T &out) { bytes(&out, sizeof(out)); }
    uint32_t count() { uint32_t n{}; value(n); return n; }
    template<class T> void array(std::vector<T> &out, uint32_t n) {
        if (n > std::numeric_limits<uint32_t>::max() / sizeof(T))
            throw std::length_error("MSH array byte count");
        out.resize(n); bytes(out.data(), sizeof(T) * n);
    }
    template<class T> void array(std::vector<T> &out) { array(out, count()); }
    std::string string() {
        std::string result(count(), '\0');
        bytes(result.data(), result.size()); return result;
    }
};

// 458A20, 459360, 45C7A0, 45C930, 45C0F0 and 45C590.
void read_geometry(Reader &reader, Geometry &mesh, uint32_t version) {
    reader.value(mesh.visible);
    reader.value(mesh.bounds_min); reader.value(mesh.bounds_max);
    reader.value(mesh.origin); reader.value(mesh.radius);
    mesh.index_count = reader.count();
    const uint32_t index_width = mesh.index_count > 65535 ? 4 : 2;
    if (mesh.index_count > std::numeric_limits<uint32_t>::max() / index_width)
        throw std::length_error("MSH index bytes");
    reader.array(mesh.indices, mesh.index_count * index_width);
    reader.array(mesh.positions); reader.array(mesh.normals);
    mesh.material_names.resize(reader.count());
    for (auto &name : mesh.material_names) name = reader.string();
    mesh.attributes.resize(reader.count());
    for (auto &attribute : mesh.attributes) {
        reader.value(attribute.start_index); reader.value(attribute.minimum_vertex);
        reader.value(attribute.vertex_count); reader.value(attribute.primitive_count);
        reader.array(attribute.materials);
    }
    mesh.layers.resize(reader.count());
    for (auto &layer : mesh.layers) {
        reader.array(layer.normals); reader.array(layer.coordinates);
        if (version >= 11) reader.array(layer.colors);
    }
    mesh.skins.resize(reader.count());
    for (auto &skin : mesh.skins) {
        reader.array(skin.influences);
        const auto bones = reader.count();
        skin.bone_names.resize(bones); skin.bone_matrices.resize(bones);
        for (uint32_t i = 0; i != bones; ++i) {
            skin.bone_names[i] = reader.string(); reader.value(skin.bone_matrices[i]);
        }
    }
    mesh.shapes.resize(reader.count());
    for (auto &shape : mesh.shapes) {
        const auto vertices = reader.count();
        reader.array(shape.positions, vertices); reader.array(shape.normals, vertices);
    }
}

// 4585C0: type precedes each child, but the file's root has no type tag.
std::unique_ptr<Node> read_node(Reader &reader, uint32_t version, NodeType type) {
    if (type != NodeType::root && type != NodeType::reference &&
        type != NodeType::mesh && type != NodeType::node)
        return {}; // Original factory has no default type or payload skip.
    auto node = std::make_unique<Node>();
    node->type = type;
    if (type == NodeType::mesh) {
        node->geometry = std::make_unique<Geometry>();
        read_geometry(reader, *node->geometry, version);
    }
    node->name = reader.string(); reader.value(node->transform);
    if (version >= 12) reader.value(node->reference_group);
    if (version >= 14) reader.value(node->flags);
    const auto children = reader.count();
    for (uint32_t i = 0; i != children; ++i) {
        auto child = read_node(reader, version, static_cast<NodeType>(reader.count()));
        if (child) node->children.push_back(std::move(child));
    }
    return node;
}
}

std::unique_ptr<Node> read_model(int32_t archive_reader) {
    if (!archive_reader) return {};
    Reader reader(archive_reader);
    const auto version = reader.count();
    // 4594F0 accepts all versions >= 10, not just the latest writer version.
    return version >= 10 ? read_node(reader, version, NodeType::root) : nullptr;
}
}
