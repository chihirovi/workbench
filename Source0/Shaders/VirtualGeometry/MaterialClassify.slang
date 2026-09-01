#include "Include/GlobalDescriptor.h.slang"
#include "Include/VB_Util.h.slang"
#include "../Include/GlobalDescriptor.h.slang"

#define THREAD_NUM_X CLASSIFY_THREAD_WIDTH
#define THREAD_NUM_Y CLASSIFY_THREAD_WIDTH

groupshared uint32_t material_frag[CLASSIFY_MATERIAL_CHUNK_MAX];

void ClassifyPixel(uint2 pos){
	if (all(pos < uint2(frame_render_data.render_target_width, frame_render_data.render_target_height))){

		uint32_t mat_id = VB::get_material_id(frame_render_data, cluster_render_queue, visiblity_buffer, pos);
		if (mat_id < MAX_MATERIAL){
			uint index = mat_id / 32;
			uint bit = mat_id % 32;
			InterlockedOr(material_frag[index], 0x1 << bit);
		}
	}
}

// ============================================================================
// ClassifyPixel (Wave Intrinsics 最適化版)
//
// 変更点:
// 同じマテリアルを持つピクセルが1タイル内に大量にあると、元の実装は
// ピクセル毎に個別の InterlockedOr を groupshared 配列へ発行しており、
// 同一アドレスへの書き込みが大量に重複していた。
//
// 1回の呼び出し(=1回のループ反復)で、Wave内の全レーンが同時に
// ClassifyPixel を実行している点を利用し、
//   1. 同じ index (32マテリアル単位のチャンク) を持つレーンを
//      WaveReadLaneFirst で見つけたリーダーの値と比較してグループ化
//   2. そのグループのbitを WaveActiveBitOr でまとめる
//   3. グループの代表レーン(WaveIsFirstLane)だけが実際に
//      InterlockedOr を発行する
// という手順で、LDSへのatomic発行回数を
// 「タイル内のピクセル数」から「タイル内のユニークなマテリアル数」
// (× ワークグループ内のWave数) まで削減する。
//
// while ループは「Wave内でまだ処理していないレーンがいなくなるまで」
// 繰り返す、ポータブルな(コアのWave Intrinsicsのみを使う)実装。
// SM6.5以降かつVulkan側の対応が確認できるなら WaveMatch や
// (NVIDIA限定だが) subgroupPartitionNV でこのループ自体を1回の
// 呼び出しに置き換えることも可能。
// ============================================================================
void ClassifyPixel2(uint2 pos){

    bool valid = all(pos < uint2(frame_render_data.render_target_width, frame_render_data.render_target_height));

    uint index = 0;
    uint mask  = 0;

    if (valid){
        uint32_t mat_id = VB::get_material_id(frame_render_data, cluster_render_queue, visiblity_buffer, pos);
        if (mat_id < MAX_MATERIAL){
            index = mat_id / 32;
            mask  = 0x1u << (mat_id % 32);
        } else {
            valid = false; // MAX_MATERIALを超えるIDは対象外
        }
    }

    bool done = !valid;

    [loop]
    while (WaveActiveAnyTrue(!done)){
        if (!done){
            // "!done" な(まだ未処理の)レーンの中の先頭レーンのindexを
            // このイテレーションのリーダーとして採用する
            uint leaderIndex = WaveReadLaneFirst(index);
            bool sameIndex = (index == leaderIndex);

            // リーダーと同じindexを持つレーンのbitだけをORでまとめる
            uint combinedMask = WaveActiveBitOr(sameIndex ? mask : 0u);

            if (sameIndex){
                done = true; // このレーンは処理済みにする

                // このグループの代表レーンだけが実際にLDSへ書き込む
                if (WaveIsFirstLane()){
                    InterlockedOr(material_frag[leaderIndex], combinedMask);
                }
            }
        }
    }
}

// ============================================================================
// ClassifyPixel (WaveMatch版 / Shader Model 6.5+)
//
// 前回のポータブル版(while + WaveReadLaneFirstループ)を、
// WaveMatch / WaveMultiPrefixBitOr / WaveMultiPrefixCountBits を使って
// ループ無しの1パスに置き換えたもの。
//
// 注意:
// - HLSL Shader Model 6.5 以降が必要 (DirectX 12なら SM6.5対応ドライバで
//   ベンダー中立に動作)。
// - Vulkan/SPIR-V経由の場合、DXCはこれを NVIDIA限定の拡張
//   (SPV_NV_shader_subgroup_partitioned / OpGroupNonUniformPartitionNV)
//   にマップするため、AMD/Intel の Vulkan ドライバでは動作しない
//   可能性が高い。複数ベンダーのVulkanをサポートする必要があるなら、
//   前回のポータブル版(while + WaveReadLaneFirst)を使うこと。
// ============================================================================
/*
void ClassifyPixel3(uint2 pos){

    bool valid = all(pos < uint2(frame_render_data.render_target_width, frame_render_data.render_target_height));
    if (!valid) return;

    uint32_t mat_id = VB::get_material_id(frame_render_data, cluster_render_queue, visiblity_buffer, pos);
    if (mat_id >= MAX_MATERIAL) return;

    uint index = mat_id / 32;
    uint mask  = 0x1u << (mat_id % 32);

    // 自分と同じ index を持つレーンの集合(パーティション)を取得
    uint4 matchMask = WaveMatch(index);

    // パーティション内で mask をOR集約(自分の値も含めた合計)
    // WaveMultiPrefixBitOr は「自分より前のレーン」だけのORを返すため、
    // 自分の mask を明示的に OR して全体の値にする
    uint combinedMask = WaveMultiPrefixBitOr(mask, matchMask) | mask;

    // パーティション内で自分より前のレーンが0個 = 自分がこのグループの
    // 先頭(代表)レーン。代表レーンだけが実際にLDSへ書き込む。
    if (WaveMultiPrefixCountBits(true, matchMask) == 0){
        InterlockedOr(material_frag[index], combinedMask);
    }
}
*/

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
			//ClassifyPixel(basePos + uint2(x, y) * CLASSIFY_THREAD_WIDTH);
			ClassifyPixel2(basePos + uint2(x, y) * CLASSIFY_THREAD_WIDTH);
			//ClassifyPixel3(basePos + uint2(x, y) * CLASSIFY_THREAD_WIDTH);
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

/*
//groupshared uint32_t material_frag[CLASSIFY_MATERIAL_CHUNK_MAX];


// ------------------------------------------------------------
// Pixel -> Material
// ------------------------------------------------------------
uint GetMaterialID(uint2 pos)
{
    if (any(pos >= uint2(
        frame_render_data.render_target_width,
        frame_render_data.render_target_height)))
    {
        return MAX_MATERIAL;
    }

    return VB::get_material_id(
        frame_render_data,
        cluster_render_queue,
        visiblity_buffer,
        pos);
}


// ------------------------------------------------------------
// Classify
// ------------------------------------------------------------
[shader("compute")]
[numthreads(THREAD_NUM_X, THREAD_NUM_Y, 1)]
void main_comp(
    in uint3 global_thread_id : SV_DispatchThreadID,
    in uint3 local_thread_id  : SV_GroupThreadID,
    in uint3 group_id         : SV_GroupID){

    const uint local_id = local_thread_id.y * THREAD_NUM_X + local_thread_id.x;
    const uint material_word_count = (MAX_MATERIAL + 31) / 32;

    // ------------------------------------------------------------
    // 1. groupshared material bitsetを初期化
    //
    // material_frag[0] = material 0  ~ 31
    // material_frag[1] = material 32 ~ 63
    // ...
    //
    // thread数とword数が一致しない場合も対応
    // ------------------------------------------------------------
    for (uint i = local_id; i < material_word_count; i += THREAD_NUM_X * THREAD_NUM_Y){
        material_frag[i] = 0;
    }

    GroupMemoryBarrierWithGroupSync();


    // ------------------------------------------------------------
    // 2. 各threadが16 pixelを処理
    //
    // atomicはまだ行わない。
    // thread-localでmaterial IDを集める。
    // ------------------------------------------------------------
    uint2 basePos = group_id.xy * CLASSIFY_TILE_WIDTH + local_thread_id.xy;

    uint local_mat[16];
    uint local_mat_count = 0;

    for (uint y = 0; y < 4; ++y){
        for (uint x = 0; x < 4; ++x){
            uint2 pos = basePos + uint2(x, y) * CLASSIFY_THREAD_WIDTH;

            uint mat_id = GetMaterialID(pos);

            if (mat_id < MAX_MATERIAL){
                local_mat[local_mat_count++] = mat_id;
            }
        }
    }


    // ------------------------------------------------------------
    // 3. thread-localで重複materialを除去
    //
    // 同じthread内で
    //
    // A A B A C B
    //
    // -> A B C
    //
    // にする。
    // ------------------------------------------------------------
    for (uint i = 0; i < 16; ++i){
        if (i >= local_mat_count) break;

        uint mat_id = local_mat[i];
        bool duplicate = false;

        for (uint j = 0; j < 16; ++j){
            if (j >= i)break;

            if (local_mat[j] == mat_id){
                duplicate = true;
                break;
            }
        }

        if (duplicate) continue;

        // --------------------------------------------------------
        // 4. Wave内で同じmaterialを持つlaneを検索
        //
        // WaveMatch:
        //
        // lane 0 : material 10
        // lane 1 : material 20
        // lane 2 : material 10
        // lane 3 : material 30
        //
        // material 10:
        //     lane 0,2
        //
        // となる。
        // --------------------------------------------------------
        uint4 match_mask = WaveMatch(mat_id);

        // --------------------------------------------------------
        // 5. Wave内でこのmaterialを持つ最初のlaneだけが
        //    groupsharedへatomic ORする。
        // --------------------------------------------------------
        uint first_lane = firstbitlow(match_mask.x);

        if (WaveGetLaneIndex() == first_lane){
            uint word = mat_id >> 5;
            uint bit  = mat_id & 31;

            InterlockedOr(material_frag[word], 1u << bit);
        }
    }


    // ------------------------------------------------------------
    // 6. Wave leader以外も含めて、全threadが
    //    material_fragの完成を待つ。
    // ------------------------------------------------------------
    GroupMemoryBarrierWithGroupSync();


    // ------------------------------------------------------------
    // 7. Tile情報
    // ------------------------------------------------------------
    uint tile_num_x =
        (frame_render_data.render_target_width +
         CLASSIFY_TILE_WIDTH - 1) /
         CLASSIFY_TILE_WIDTH;

    uint tile_num_y =
        (frame_render_data.render_target_height +
         CLASSIFY_TILE_WIDTH - 1) /
         CLASSIFY_TILE_WIDTH;

    uint tile_num = tile_num_x * tile_num_y;
    uint tile_no = group_id.y * tile_num_x + group_id.x;


    // ------------------------------------------------------------
    // 8. material_fragをWave単位で処理
    //
    // 同じmaterial wordを複数threadが読むので、
    // WaveActiveBallot / WaveReadLaneFirstを利用して
    // 実際に処理するlaneを絞る。
    // ------------------------------------------------------------
    for (uint word = local_id; word < material_word_count; word += THREAD_NUM_X * THREAD_NUM_Y){
        uint bits = material_frag[word];

        while (bits != 0){
            uint firstBit = firstbitlow(bits);

            uint matIndex = word * 32 + firstBit;
            bits &= ~(1u << firstBit);

            // ----------------------------------------------------
            // 同じmaterialを処理するlaneをWave内で1つにする。
            //
            // ここではword単位で各threadが同じbitsを読む構造を
            // 避けているので、materialごとの重複処理は発生しない。
            // ----------------------------------------------------

            uint store_addr;
            InterlockedAdd(tile_indirect_draw_arg[matIndex][1], 1, store_addr);
            store_addr = matIndex * tile_num + store_addr;
            tile_index[store_addr] = tile_no;
        }
    }
}
*/