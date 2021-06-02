#pragma once

#include "Models/Mesh.h"
#include "Models/Texture.h"
#include "Models/Material.h"
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

void importSceneFromFile(
    const std::string& path,
    std::vector<std::shared_ptr<Mesh>>& scene,
    std::unordered_map<uint32_t, Material>& materials,
    std::unordered_map<std::string, std::unique_ptr<Texture>>& textures);

std::vector<std::shared_ptr<Mesh>> unifyStaticMeshes(
    std::vector<std::shared_ptr<Mesh>>& scene,
    const std::unordered_map<uint32_t, Material>& materials);