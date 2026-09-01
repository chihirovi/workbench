#pragma once
#include "../Render/VertexType.hpp"
#include "../Utility/Error.hpp"
#include "LoadVertIdxUtil.hpp"
#include <tinygltf/tiny_gltf.h>

#include <MikkTSpace/mikktspace.h>


class CalcTangents{
public:
    SMikkTSpaceInterface iface{};
    SMikkTSpaceContext context{};
    std::vector<Irori::Render::VertexType0>* verts;
    std::vector<uint32_t>* idxs;

public:

    CalcTangents(std::vector<Irori::Render::VertexType0>* v, std::vector<uint32_t>* i);
    void calc();
    static int get_vertex_index(const SMikkTSpaceContext *context, int iFace, int iVert);
    static int get_num_faces(const SMikkTSpaceContext *context);
    static int get_num_vertices_of_face(const SMikkTSpaceContext *context, int iFace);
    static void get_position(const SMikkTSpaceContext *context, float outpos[], int iFace, int iVert);
    static void get_normal(const SMikkTSpaceContext *context, float outnormal[], int iFace, int iVert);
    static void get_tex_coords(const SMikkTSpaceContext *context, float outuv[], int iFace, int iVert);
    static void set_tspace_basic(const SMikkTSpaceContext *context, const float tangentu[], float fSign, int iFace, int iVert);
};

void load_vertex_index(
    tinygltf::Model& model, 
    tinygltf::Primitive& primitive,
    std::vector<Irori::Render::VertexType0>& vertices,
    std::vector<uint32_t>& indices
);
 