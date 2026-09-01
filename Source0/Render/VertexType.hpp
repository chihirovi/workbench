#pragma once
#include <stdfloat>
#include <iostream>
#include <cmath>

//namaspaceがファイル名でないので注意
namespace Irori::Render{

//4*3 + 2*3 + 2*3 + 2*2 + 1*4 = 26bytes 
//spvの解析結果 -> メンバoffsetは平気だけど，strideがまずい, 
//scalar layoutでも，structのsizeは，メンバーのうち最もalignmentの大きいものの倍数になる．
//今回だと，floatなので，32byteだけど，4の倍数32に切り上げになる．

/*
   OpMemberDecorate %_Array_natural_uint84 0 Offset 0
   OpMemberDecorate %Vertex_natural 0 Offset 0
   OpMemberDecorate %Vertex_natural 1 Offset 12
   OpMemberDecorate %Vertex_natural 2 Offset 18
   OpMemberDecorate %Vertex_natural 3 Offset 26
   OpMemberDecorate %Vertex_natural 4 Offset 30 <-colorのoffset
   OpDecorate %_runtimearr_Vertex_natural ArrayStride 36
*/
    
/* scalar layout 

 	int8		-> 1byte aligment
	float16 	-> 2byte aligment
	float, int 	-> 4byte alignment
	vec2		-> 4byte alignment
	vec3, vec4	-> 4byte alignment
	配列        -> 要素のbase aligmentになる (float f[4] = 4*4 byte)
	構造体のsizeはメンバーのうち，最も大きいalignmentの倍数になる．
*/

struct VertexType0{
    float       position[3];
    //_Float16    normal[3];
    uint16_t    encode_normal;
    //_Float16    tangent[4];       //bitangent = cross(normal.xyz, tangent.xyz) * tangent.w.
    uint16_t    encode_tangent[2];  //
    //_Float16    tex_coord[2];
    uint8_t     padding0;
    uint8_t     padding1;
    uint8_t     color[4];
    uint32_t    packed_tex_coord;
}__attribute__((packed, aligned(1)));

static_assert(sizeof(VertexType0) == 28, "VertexType.hpp : VertexType0 should be 28 bytes!");
    
}