#include <vector>
#include <fstream>
#include <cstdint>
#include <cstring>

#include "VG2.hpp"

namespace Irori::VirtualGeometry2{

    VG2File load_vg2_file(const char *filename){
        std::ifstream file(filename, std::ios::binary);

        file.seekg(0, std::ios::end);
        size_t file_size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> bytes(file_size);
        file.read(reinterpret_cast<char *>(bytes.data()), file_size);

        VG2File vg2{};

        // header
        std::memcpy(&vg2.header, bytes.data(), sizeof(VG2Header));

        // clusters
        {
            size_t begin = vg2.header.cluster_byte_offset;
            size_t count = vg2.header.cluster_count;

            vg2.clusters.resize(count);

            std::memcpy(
                vg2.clusters.data(),
                bytes.data() + begin,
                count * sizeof(ClusterVG2));
        }

        // bvh8 nodes
        {
            size_t begin = vg2.header.bvh8_node_byte_offset;
            size_t count = vg2.header.bvh8_node_count;

            vg2.bvh8_nodes.resize(count);

            std::memcpy(
                vg2.bvh8_nodes.data(),
                bytes.data() + begin,
                count * sizeof(BVH8NodeVG2));
        }

        // vertices
        {
            size_t begin = vg2.header.vertex_byte_offset;
            size_t count = vg2.header.vertex_count * 3;

            vg2.vertex_positions.resize(count);

            std::memcpy(
                vg2.vertex_positions.data(),
                bytes.data() + begin,
                count * sizeof(float));
        }

        // indices
        {
            size_t begin = vg2.header.index_byte_offset;
            size_t count = vg2.header.index_count;

            vg2.indices.resize(count);

            std::memcpy(
                vg2.indices.data(),
                bytes.data() + begin,
                count * sizeof(uint8_t));
        }

        return vg2;
    }
}