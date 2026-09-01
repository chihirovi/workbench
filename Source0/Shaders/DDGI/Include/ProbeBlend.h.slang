#pragma once
#include "Types.h.slang"

void update_border_texel<let NUM_TEXELS_INCLUDE_BORDER : uint32_t>(
    RWTexture2DArray<float4> tex, 
    int32_t2 texel_coords, 
    uint32_t3 probe_id_3d) {

    bool isCornerTexel = ((texel_coords.x == 0) || (texel_coords.x == (NUM_TEXELS_INCLUDE_BORDER - 1))) 
                        && ((texel_coords.y == 0) || (texel_coords.y == (NUM_TEXELS_INCLUDE_BORDER - 1)));
    bool isRowTexel = ((texel_coords.x > 0) && (texel_coords.x < (NUM_TEXELS_INCLUDE_BORDER - 1)));

    uint32_t3 copyCoordinates = uint32_t3(probe_id_3d.x * NUM_TEXELS_INCLUDE_BORDER, probe_id_3d.y * NUM_TEXELS_INCLUDE_BORDER, probe_id_3d.z);

    if(isCornerTexel){
        copyCoordinates.x += texel_coords.x > 0 ? 1 : (NUM_TEXELS_INCLUDE_BORDER-2);
        copyCoordinates.y += texel_coords.y > 0 ? 1 : (NUM_TEXELS_INCLUDE_BORDER-2);

    }else if(isRowTexel){
        copyCoordinates.x += (NUM_TEXELS_INCLUDE_BORDER - 1) - texel_coords.x;
        copyCoordinates.y += texel_coords.y + ((texel_coords.y > 0) ? -1 : 1);

    }else{ // Column Texel

        copyCoordinates.x += texel_coords.x + ((texel_coords.x > 0) ? -1 : 1);
        copyCoordinates.y += (NUM_TEXELS_INCLUDE_BORDER - 1) - texel_coords.y;
    }
    
    //dst
    int32_t3 dst_texel = int32_t3(   
        texel_coords.x + probe_id_3d.x * NUM_TEXELS_INCLUDE_BORDER,
        texel_coords.y + probe_id_3d.y * NUM_TEXELS_INCLUDE_BORDER,
        probe_id_3d.z
    );

    //copy to border
    float4 value = tex[copyCoordinates];
    tex[dst_texel] = value;
}