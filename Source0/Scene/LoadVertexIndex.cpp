#include "LoadVertexIndex.hpp"

CalcTangents::CalcTangents(std::vector<Irori::Render::VertexType0>* v, std::vector<uint32_t>* i){
    iface.m_getNumFaces = get_num_faces;
    iface.m_getNumVerticesOfFace = get_num_vertices_of_face;

    iface.m_getNormal = get_normal;
    iface.m_getPosition = get_position;
    iface.m_getTexCoord = get_tex_coords;
    iface.m_setTSpaceBasic = set_tspace_basic;

    context.m_pInterface = &iface;
    context.m_pUserData = this;
    
    verts = v;
    idxs = i;
}

void CalcTangents::calc(){
    genTangSpaceDefault(&this->context);
}

int CalcTangents::get_vertex_index(const SMikkTSpaceContext *context, int iFace, int iVert){
    CalcTangents* calc_tangets = (CalcTangents*)context->m_pUserData;
    auto face_size = 3;
    auto indices_index = (iFace * face_size) + iVert;
    int index = int(calc_tangets->idxs->operator[](indices_index));
    return index;
}

int CalcTangents::get_num_faces(const SMikkTSpaceContext *context){
    CalcTangents* calc_tangets = (CalcTangents*)context->m_pUserData;
    return calc_tangets->idxs->size()/3;
}

int CalcTangents::get_num_vertices_of_face(const SMikkTSpaceContext *context, int iFace){
    return 3;
}

void CalcTangents::get_position(const SMikkTSpaceContext *context, float outpos[], int iFace, int iVert){
    CalcTangents* calc_tangets = (CalcTangents*)context->m_pUserData;
    auto index = get_vertex_index(context, iFace, iVert);
    auto& vertex = calc_tangets->verts->operator[](index);
    
    outpos[0] = vertex.position[0];
    outpos[1] = vertex.position[1];
    outpos[2] = vertex.position[2];
}

void CalcTangents::get_normal(const SMikkTSpaceContext *context, float outnormal[], int iFace, int iVert){
    CalcTangents* calc_tangets = (CalcTangents*)context->m_pUserData;
    auto index = get_vertex_index(context, iFace, iVert);
    auto& vertex = calc_tangets->verts->operator[](index);
    
    auto n = decode_spherical_16(vertex.encode_normal);
    outnormal[0] = n[0];
    outnormal[1] = n[1];
    outnormal[2] = n[2];
}

void CalcTangents::get_tex_coords(const SMikkTSpaceContext *context, float outuv[], int iFace, int iVert){
    CalcTangents* calc_tangets = (CalcTangents*)context->m_pUserData;
    auto index = get_vertex_index(context, iFace, iVert);
    auto& vertex = calc_tangets->verts->operator[](index);
    
    //glm::vec2 uv(float(vertex.tex_coord_f32[0]), float(vertex.tex_coord_f32[1]));
    glm::vec2 uv = glm::unpackHalf2x16(vertex.packed_tex_coord);
    outuv[0] = uv[0];
    outuv[1] = uv[1];
}

void CalcTangents::set_tspace_basic(const SMikkTSpaceContext *context, const float tangentu[], float fSign, int iFace, int iVert){
    CalcTangents* calc_tangets = (CalcTangents*)context->m_pUserData;
    auto index = get_vertex_index(context, iFace, iVert);
    auto& vertex = calc_tangets->verts->operator[](index);
    
    uint16_t encode_tan = encode_spherical_16({tangentu[0], tangentu[1], tangentu[2]});
    uint16_t tan_hand = (fSign == 1.0f ? 1 : 0); 
    vertex.encode_tangent[0] = encode_tan;
    vertex.encode_tangent[1] = tan_hand;
}





void load_vertex_index(
    tinygltf::Model& model, 
    tinygltf::Primitive& primitive,
    std::vector<Irori::Render::VertexType0>& vertices,
    std::vector<uint32_t>& indices){
   
    /*utilitys*/
    auto check_attribute_accessor = [&](char* name, int type, int component_type){
        ASSERT_WITH_MSG(primitive.attributes.contains(name), std::format("primitive dosen't containts {}", name));
        auto& accessor = model.accessors[primitive.attributes[name]];
        ASSERT_WITH_MSG((accessor.componentType == component_type), std::format("{} : worng component type [{}], expect [{}]", name, accessor.componentType, component_type));
        ASSERT_WITH_MSG((accessor.type == type), std::format("{} : worng type [{}], expect [{}]", name, accessor.type, type));
        return accessor;
    };
    
    auto data_begin_point = [&](tinygltf::Accessor& accessor){
        auto& buffer_view = model.bufferViews[accessor.bufferView];
        auto& buffer = model.buffers[buffer_view.buffer];
        
        return std::tuple<void*, size_t>{
            (void *)(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]),
            buffer_view.byteStride
        };
    };
    
    auto read_vec_n = [&]<typename SrcT, typename DstT>(tinygltf::Accessor& accessor, uint32_t element_offset, uint32_t vec_n, DstT* p_dst_T){

        auto [p_void, byte_stride] = data_begin_point(accessor);

        // tinygltfではbyte strideは0になりうるらしい
        byte_stride = byte_stride>0 ? byte_stride : sizeof(SrcT) * vec_n;

        uint8_t* p_u8           = (uint8_t*)p_void;
        p_u8                    += byte_stride*element_offset;

        SrcT* p_src_T           = (SrcT*)p_u8;
        for(int i=0; i<vec_n; i++){
            p_dst_T[i] = (DstT)(p_src_T[i]);
        }
    };
    
    ASSERT_WITH_MSG((primitive.mode == TINYGLTF_MODE_TRIANGLES),"Irori only support triangle mode : triangle lists");
    
    // position
    auto position_accessor = check_attribute_accessor("POSITION", TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT);
    vertices.resize(position_accessor.count);

    for(int i=0; i<vertices.size(); i++){
        read_vec_n.operator()<float, float>(position_accessor, i, 3, vertices[i].position);
    }
    
    // normal
    if(primitive.attributes.contains("NORMAL")){
        auto& normal_accessor = model.accessors[primitive.attributes["NORMAL"]];

        if(normal_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE){

            check_attribute_accessor("NORMAL", TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_BYTE);
            
            for(int i=0; i<vertices.size(); i++){
                glm::f32vec3 n_temp;
                read_vec_n.operator()<uint8_t, float>(normal_accessor, i, 3, &n_temp[0]);
                n_temp /= 255.0;
                vertices[i].encode_normal = encode_spherical_16(n_temp);
            }   
            
        }else if(normal_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT){

            check_attribute_accessor("NORMAL", TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT);
               
            for(int i=0; i<vertices.size(); i++){
                glm::f32vec3 tmp;
                read_vec_n.operator()<float, float>(normal_accessor, i, 3, &tmp[0]);
                uint16_t encode_norm = encode_spherical_16(tmp);
                vertices[i].encode_normal = encode_norm;
            }
        }
    }
    
    // texcoord_0, texcoord_1以上の複数のUVを使用するモデルはサポートしない
    bool has_uv = true;
    if(primitive.attributes.contains("TEXCOORD_0")){
        auto uv_accessor = check_attribute_accessor("TEXCOORD_0", TINYGLTF_TYPE_VEC2, TINYGLTF_COMPONENT_TYPE_FLOAT);

        for(int i=0; i<vertices.size(); i++){
            //read_vec_n.operator()<float, _Float16>(uv_accessor, i, 2, vertices[i].tex_coord);
            //read_vec_n.operator()<float, float>(uv_accessor, i, 2, vertices[i].tex_coord_f32);
            
            glm::f32vec2 uv;
            read_vec_n.operator()<float, float>(uv_accessor, i, 2, &uv[0]);
            vertices[i].packed_tex_coord = glm::packHalf2x16(uv);
            
        }   
    }else{
        has_uv = false;
        printf("primitive doesn't contain UV\n");
        
        for(int i=0; i<vertices.size(); i++){
            //vertices[i].tex_coord[0] = _Float16(0.5f);
            //vertices[i].tex_coord[1] = _Float16(0.5f);
            //vertices[i].tex_coord_f32[0] = float(0.5f);
            //vertices[i].tex_coord_f32[1] = float(0.5f);
            glm::f32vec2 uv(0.5, 0.5);
            vertices[i].packed_tex_coord = glm::packHalf2x16(uv);
        }
    }
    
    // color
    if(primitive.attributes.contains("COLOR_0")){
        auto& color_accessor = model.accessors[primitive.attributes["COLOR_0"]];

        if(color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE){

            check_attribute_accessor("COLOR_0", TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE);
            
            for(int i=0; i<vertices.size(); i++){
                read_vec_n.operator()<uint8_t, uint8_t>(color_accessor, i, 4, vertices[i].color);
            }   
            
        }else if(color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT){

            check_attribute_accessor("COLOR_0", TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT);
            
            for(int i=0; i<vertices.size(); i++){
                float c_temp[4] = {1.0, 1.0, 1.0, 1.0};
                read_vec_n.operator()<float, float>(color_accessor, i, 3, c_temp);
                
                vertices[i].color[0] = uint8_t(c_temp[0]*255);
                vertices[i].color[1] = uint8_t(c_temp[1]*255);
                vertices[i].color[2] = uint8_t(c_temp[2]*255);
                vertices[i].color[3] = uint8_t(c_temp[3]*255);
            }   

        }

    }else{
        for(int i=0; i<vertices.size(); i++){
            vertices[i].color[0] = 255;
            vertices[i].color[1] = 255;
            vertices[i].color[2] = 255;
            vertices[i].color[3] = 255;
        }   
    }
    
    // index
    auto index_accessor = model.accessors[primitive.indices];
    ASSERT_WITH_MSG((index_accessor.count > 0), std::format("Irori only support indexed triangle"));
    ASSERT_WITH_MSG((index_accessor.type == TINYGLTF_TYPE_SCALAR), "index type must be SCALAR");
    indices.resize(index_accessor.count);

    if(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT){
        for(int i=0; i<indices.size(); i++){
            read_vec_n.operator()<uint16_t, uint32_t>(index_accessor, i, 1, &indices[i]);
        }
        
    }else if(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT){
        for(int i=0; i<indices.size(); i++){
            read_vec_n.operator()<uint32_t, uint32_t>(index_accessor, i, 1, &indices[i]);
        }
    }
    
    // tangent, ここはオプション，なかったら自分で計算
    if(primitive.attributes.contains("TANGENT")){
        auto tangent_accessor = check_attribute_accessor("TANGENT", TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_FLOAT);

        for(int i=0; i<vertices.size(); i++){
            glm::f32vec4 tmp;
            read_vec_n.operator()<float, float>(tangent_accessor, i, 4, &tmp[0]);
            uint16_t encode_tan = encode_spherical_16({tmp[0], tmp[1], tmp[2]});
            uint16_t tan_hand = (tmp.w == 1.0f ? 1 : 0); 
            vertices[i].encode_tangent[0] = encode_tan;
            vertices[i].encode_tangent[1] = tan_hand;
        }   

    //}else if(has_uv){ // calc tangent
    }else{ // calc tangent
        // ここでタンジェントの計算
        printf("warning : this primitive doesn't has TANGENT\n");
        CalcTangents calc_tangents(&vertices, &indices);
        calc_tangents.calc();

    }
    /*
    else{ // uvがない場合
        printf("warning : this primitive doesn't has TANGENT + UV\n");
        
        std::vector<glm::f32vec3> tangents;
        auto init_v = glm::f32vec3(0.0f, 0.0f, 0.0f);
        tangents.resize(vertices.size(), init_v);
        for(int i=0; i<indices.size()/3; i++){
            int i0 = indices[i*3 + 0];
            int i1 = indices[i*3 + 1];
            int i2 = indices[i*3 + 2];
            
            auto& v0 = vertices[i0];
            auto& v1 = vertices[i1];
            auto& v2 = vertices[i2];
            
            glm::f32vec3 p0(v0.position[0], v0.position[1], v0.position[2]);
            glm::f32vec3 p1(v1.position[0], v1.position[1], v1.position[2]);
            glm::f32vec3 p2(v2.position[0], v2.position[1], v2.position[2]);
            auto n = glm::cross(p2-p1, p1-p0);
            
            glm::f32vec3 a;
            if(abs(n[0]) < 0.9){
                a = glm::f32vec3(1.0f, 0.0f, 0.0);
            }else{
                a = glm::f32vec3(0.0f, 1.0f, 0.0);
            }

            n = glm::normalize(n);
            
            auto t = glm::normalize(glm::cross(a, n));
            tangents[i0] += t;
            tangents[i1] += t;
            tangents[i2] += t;
        }
        
        for(int i=0; i<tangents.size(); i++){
            auto t = glm::normalize(tangents[i]);
            auto n =  decode_spherical_16(vertices[i].encode_normal);
            auto b = glm::cross(n, t);
            //auto w = (glm::dot(glm::cross(n, t), b) < 0.0f) ? -1.0f : 1.0f;
            uint16_t tan_hand = 1;
            uint16_t encode_tan = encode_spherical_16({t[0], t[1], t[2]});
            vertices[i].encode_tangent[0] = encode_tan;
            vertices[i].encode_tangent[1] = tan_hand;
        }
    }
    */
}