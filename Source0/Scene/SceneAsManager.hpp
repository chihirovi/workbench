#pragma once

#include "../Core/Common/Barrier2.hpp"
#include "../Core/RayTrace/BLAS.hpp"
#include "../Core/RayTrace/TLAS.hpp"
#include "../Render/FrameRenderData.hpp"
#include "SceneManager.hpp"

#include <unordered_map>

namespace Irori::Scene{

class UniqueSceneAsManager{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueSceneAsManager);

public:
    Core::VulkanContext* vulkan_context = nullptr;
    std::vector<RayTrace::UniqueAS> blas_s;
    RayTrace::UniqueAS tlas;
    uint32_t tlas_instance_count = 0;

    Core::UniqueBuffer tlas_instances_data_buffer;
    Core::UniqueBuffer tlas_instances_data_staging_buffer;
    std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> model_2_blas_idx_RT_render_inst_base_map;    

public:
    UniqueSceneAsManager(){;}
    UniqueSceneAsManager(Core::VulkanContext* vc){vulkan_context = vc;}
    UniqueSceneAsManager(BOOST_RV_REF(UniqueSceneAsManager) rhs) = default;               //move constractor
    UniqueSceneAsManager& operator=(BOOST_RV_REF(UniqueSceneAsManager) rhs) = default;    //move assignment
                                                                                      
    void prepare_tlas_update_data(UniqueSceneManager& scene_manager); //cpu async taskで呼ぶ
    void cmd_update_tlas_with_barrier(vk::CommandBuffer& cmd_buf); //rendering の cmd を積むときに呼ぶ
                                         
    // rendering中に毎フレーム，asをbuildし直すことは普通しないから，
    // onetime submitで，必要があったらbuildするようにしてる.
    // これを呼ぶと多分数ms遅延が出る
    // 基本的には毎フレーム tlasのupdateだけする
    void rebuild_tlas_blas(UniqueSceneManager& scene_manager, Render::CPU_FrameRenderData& cpu_frame_render_data);   //rendering後に呼ぶ
};


}