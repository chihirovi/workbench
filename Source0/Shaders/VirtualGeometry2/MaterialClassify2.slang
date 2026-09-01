#include "Include/GlobalDEscriptor.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"

groupshared uint32_t material_frag[CLASSIFY_MATERIAL_CHUNK_MAX];

void ClassifyPixel(uint2 pos){
	if (all(pos < uint2(frame_render_data.render_target_width, frame_render_data.render_target_height))){
		uint32_t mat_id = material_id[pos];
		if (mat_id < MAX_MATERIAL){
			uint index = mat_id / 32;
			uint bit = mat_id % 32;
			InterlockedOr(material_frag[index], 0x1 << bit);
		}
	}
}


[shader("compute")]
[numthreads(CLASSIFY_THREAD_WIDTH, CLASSIFY_THREAD_WIDTH, 1)]
void main_comp(
    in uint3 global_thread_id   : SV_DispatchThreadID,
    in uint3 local_thread_id    : SV_GroupThreadID,
    in uint3 group_id           : SV_GroupID) {
    
    const uint kMatChunkIndex = local_thread_id.y * CLASSIFY_THREAD_WIDTH + local_thread_id.x;
	material_frag[kMatChunkIndex] = 0x0;
	
	// sync
	GroupMemoryBarrierWithGroupSync();

	// classify material.
	uint2 basePos = group_id.xy * CLASSIFY_TILE_WIDTH + local_thread_id.xy;
	for (uint x = 0; x < 4; x++){
		for (uint y = 0; y < 4; y++){
			ClassifyPixel(basePos + uint2(x, y) * CLASSIFY_THREAD_WIDTH);
		}
	}
    
    // sync
	GroupMemoryBarrierWithGroupSync();
    
    uint32_t tile_num_x = (frame_render_data.render_target_width  + CLASSIFY_TILE_WIDTH -1)/CLASSIFY_TILE_WIDTH;
    uint32_t tile_num_y = (frame_render_data.render_target_height + CLASSIFY_TILE_WIDTH -1)/CLASSIFY_TILE_WIDTH;
    uint32_t tile_num = tile_num_x * tile_num_y;

	uint tile_no = group_id.y * tile_num_x + group_id.x;

    // compute tile draw args.
	if (material_frag[kMatChunkIndex] != 0){
		const uint kMatBaseIndex = kMatChunkIndex * 32;
		uint bits = material_frag[kMatChunkIndex];

		while (bits != 0){
			uint firstBit = firstbitlow(bits);
			uint matIndex = kMatBaseIndex + firstBit;
			bits &= ~(0x1 << firstBit);

			uint store_addr;
            InterlockedAdd(tile_indirect_draw_arg[matIndex][1], 1, store_addr);

			store_addr = ((matIndex * tile_num) + store_addr);
            tile_index[store_addr] = tile_no;
		}
	}
}

/*
struct VkDrawIndirectCommand {
    uint32_t    vertexCount;
    uint32_t    instanceCount;
    uint32_t    firstVertex;
    uint32_t    firstInstance;
}
*/