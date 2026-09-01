#pragma once

#include "../Core/Common/Barrier2.hpp"
#include "../Core/RayTrace/BLAS.hpp"
#include "../Core/RayTrace/TLAS.hpp"
#include "../Render/FrameRenderData.hpp"
#include "SceneManager.hpp"
#include "LoadVertexIndex.hpp"

#include <unordered_map>
#include <array>
#include <optional>

namespace Irori::Scene{

struct SceneAsModelLodFile{
    std::unordered_map<std::string, std::array<std::optional<std::string>, 4>> model_name_2_lod_files;
};

class UniqueSceneAsLodManager{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueSceneAsLodManager);

public:
    Core::VulkanContext* vulkan_context = nullptr;
    //std::vector<RayTrace::UniqueAS> lod_blas_s[5];
    std::unordered_map<std::string, std::array<std::pair<uint32_t, std::vector<uint32_t>>, 4>> model_name_2_lod_vi_idx; //lod1,2,3,4
    std::vector<RayTrace::UniqueAS> lod_blas;
    RayTrace::UniqueAS lod_tlas[5];
    uint32_t tlas_instance_count = 0;

    Core::UniqueBuffer lod_tlas_instances_data_buffer[5];
    Core::UniqueBuffer lod_tlas_instances_data_staging_buffer[5];
    std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> model_2_blas_idx_RT_render_inst_base_map;    

    std::vector<Core::UniqueBuffer> vertex_buffers;
    std::vector<Core::UniqueBuffer> index_buffers;
    std::vector<uint64_t> rt_lod1234_vert_idx_addr;//0番目には例外的に1ASあたりのストライドが入ってる

public:
    UniqueSceneAsLodManager(){;}
    UniqueSceneAsLodManager(Core::VulkanContext* vc){vulkan_context = vc;}
    UniqueSceneAsLodManager(BOOST_RV_REF(UniqueSceneAsLodManager) rhs) = default;               //move constractor
    UniqueSceneAsLodManager& operator=(BOOST_RV_REF(UniqueSceneAsLodManager) rhs) = default;    //move assignment
                                                                                      
    void prepare_tlas_update_data(UniqueSceneManager& scene_manager); //cpu async taskで呼ぶ
    void build_cpu_frame_render_data(Render::CPU_FrameRenderData* cpu_frame_render_data); //cpu async taskで呼ぶ
    void cmd_update_tlas_with_barrier(vk::CommandBuffer& cmd_buf); //rendering の cmd を積むときに呼ぶ
                                         
    // rendering中に毎フレーム，asをbuildし直すことは普通しないから，
    // onetime submitで，必要があったらbuildするようにしてる.
    // これを呼ぶと多分数ms遅延が出る
    // 基本的には毎フレーム tlasのupdateだけする
    void rebuild_tlas_blas(UniqueSceneManager& scene_manager, Render::CPU_FrameRenderData& cpu_frame_render_data, SceneAsModelLodFile& lod_files);   //rendering後に呼ぶ
                                                                                                                                                     //
    std::pair<uint32_t, uint32_t> UniqueSceneAsLodManager::get_all_AS_size(UniqueSceneManager& scene_manager);
};


}