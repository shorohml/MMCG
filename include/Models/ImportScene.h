#pragma once

#include "Models/Mesh.h"
#include "Models/Texture.h"
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

void importSceneFromFile(
    const std::string& path,
    std::vector<std::unique_ptr<Mesh>>& scene,
    std::unordered_map<uint32_t, Material>& materials,
    std::unordered_map<std::string, std::unique_ptr<Texture>>& textures);