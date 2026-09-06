#pragma once
#include "../External/meshoptimizer/src/meshoptimizer.h"
#include "../Render/Cluster.hpp"
#include <iostream>
#include <vector>
#include <string>

struct VG_File_Header{
    uint32_t gltf_mesh_idx;
    uint32_t gltf_primitive_idx;
    uint32_t cluster_byte_offset;
    uint32_t cluster_count;
    uint32_t vertex_byte_offset;
    uint32_t vertex_count;
    uint32_t index_byte_offset;
    uint32_t index_count;
};

void Write_VG_File(std::string file_name, std::vector<Irori::Render::Cluster>& clusters, std::vector<uint32_t>& verts, std::vector<uint8_t>& idxs );
void Read_VG_File(std::string file_name, std::vector<Irori::Render::Cluster>& clusters, std::vector<uint32_t>& verts, std::vector<uint8_t>& idxs);