#pragma once

#include "../../Scene/LoadVertexIndex.hpp"
#include "../../Scene/SceneManager.hpp"

namespace Irori::Engine::PathTrace{

std::pair<std::vector<glm::f32vec3>, glm::f32vec4> CalcLightTileInfo(Irori::Scene::UniqueSceneManager& scene_manager, std::string model_inst_name, std::string file_name);

}



