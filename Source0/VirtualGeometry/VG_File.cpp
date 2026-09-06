#include "VG_File.hpp"

#include <fstream>
#include <vector>
#include <cstdint>

void Write_VG_File(std::string file_name, std::vector<Irori::Render::Cluster>& clusters, std::vector<uint32_t>& verts, std::vector<uint8_t>& idxs ){
    // "data.bin" をバイナリモードで開く（既に存在していれば上書き）
    std::ofstream file(file_name, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file){
        // 開けなかった場合
        printf("can't open file %s\n", file_name.c_str());
        exit(1);
    }
    
    // create header
    uint32_t cluster_byte_size  = clusters.size()*sizeof(Irori::Render::Cluster);
    uint32_t verts_byte_size    = verts.size()*sizeof(uint32_t);
    uint32_t idxs_byte_size     = idxs.size()*sizeof(uint8_t);
    VG_File_Header vg_header{
        .gltf_mesh_idx          = 0,
        .gltf_primitive_idx     = 0,
        .cluster_byte_offset    = (uint32_t)sizeof(VG_File_Header),
        .cluster_count          = (uint32_t)clusters.size(),
        .vertex_byte_offset     = (uint32_t)sizeof(VG_File_Header) + cluster_byte_size,
        .vertex_count           = (uint32_t)verts.size(),
        .index_byte_offset      = (uint32_t)sizeof(VG_File_Header) + cluster_byte_size + verts_byte_size,
        .index_count            = (uint32_t)idxs.size()
    };
    
    file.write((const char*)&vg_header,         sizeof(VG_File_Header));
    file.write((const char*)clusters.data(),    cluster_byte_size);
    file.write((const char*)verts.data(),       verts_byte_size);
    file.write((const char*)idxs.data(),        idxs_byte_size);

    file.close(); // 明示的に閉じてもよい
    return; 
}


void Read_VG_File(std::string file_name, std::vector<Irori::Render::Cluster>& clusters, std::vector<uint32_t>& verts, std::vector<uint8_t>& idxs){
    std::ifstream file(file_name, std::ios::binary | std::ios::in);
    if (!file){
        printf("can't open file %s\n", file_name.c_str());
        exit(1);
    } 
    
    // header
    file.seekg(0, std::ios::beg);
    VG_File_Header vg_header;
    file.read((char*)(&vg_header), sizeof(VG_File_Header));
    
    // cluster
    file.seekg(vg_header.cluster_byte_offset, std::ios::beg);
    clusters.resize(vg_header.cluster_count);
    file.read((char*)(clusters.data()), clusters.size()*sizeof(Irori::Render::Cluster));
    
    // vertex
    file.seekg(vg_header.vertex_byte_offset, std::ios::beg);
    verts.resize(vg_header.vertex_count);
    file.read((char*)(verts.data()), verts.size()*sizeof(uint32_t));
    
    // index
    file.seekg(vg_header.index_byte_offset, std::ios::beg);
    idxs.resize(vg_header.index_count);
    file.read((char*)(idxs.data()), idxs.size()*sizeof(uint8_t));
    
    file.close(); // 明示的に閉じてもよい
    return; 
}
