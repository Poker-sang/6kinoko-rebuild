#include "kinoko/file_io.h"
#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace kinoko::mesh {
using Vector3 = std::array<float, 3>;
using Matrix = std::array<float, 16>;
struct Attribute {
    uint32_t start_index{}, minimum_vertex{}, vertex_count{}, primitive_count{};
    std::vector<int32_t> materials;
};
struct Layer {
    std::vector<Vector3> normals;
    std::vector<std::array<float, 2>> coordinates;
    std::vector<uint32_t> colors;
};
struct Skin {
    // 45C200 transfers each 16-byte influence without conversion.
    std::vector<std::array<uint32_t, 4>> influences;
    std::vector<std::string> bone_names;
    std::vector<Matrix> bone_matrices;
};
struct Shape {
    std::vector<Vector3> positions, normals;
    std::vector<int32_t> vertex_indices, normal_indices;
};
struct Material {
    std::array<std::string,5> names;
    std::array<float,16> colors{};
};
enum class NodeType : uint32_t { root = 0, reference = 1, mesh = 2, node = 3 };
struct Geometry {
    uint8_t visible{};
    Vector3 bounds_min{}, bounds_max{}, origin{};
    float radius{};
    uint32_t index_count{};
    // Original file width depends on index COUNT, not vertex count.
    std::vector<uint8_t> indices;
    std::vector<Vector3> positions, normals;
    std::vector<std::string> material_names;
    std::vector<Attribute> attributes;
    std::vector<Layer> layers;
    std::vector<Skin> skins;
    std::vector<Shape> shapes;
};
struct Node {
    NodeType type{};
    std::string name;
    Matrix transform{};
    int32_t reference_group{}, flags{};
    std::unique_ptr<Geometry> geometry;
    std::vector<std::unique_ptr<Node>> children;
};
// Borrowed archive reader; caller retains ownership. This decodes the original
// versioned MSH stream, not a D3DX/X-file replacement format.
std::unique_ptr<Node> read_model(KinokoArchiveReader *archive_reader);
std::unique_ptr<Material> read_material(KinokoArchiveReader *archive_reader);
}
