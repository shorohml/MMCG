#pragma once

#include "Models/Mesh.h"
#include "Simulation/PointMass.h"
#include "Simulation/Spring.h"
#include <vector>

struct Cloth {
public:
    Cloth(
        const glm::vec3& lowerLeftCorner_,
        const float width_,
        const float height_,
        const std::uint32_t widthPoints_,
        const std::uint32_t heightPoints_);

    Mesh mesh;

private:
    void createMassesAndSprings();
    Mesh createMesh();

    glm::vec3 lowerLeftCorner;
    float width;
    float height;
    std::uint32_t widthPoints;
    std::uint32_t heightPoints;

    std::vector<PointMass> pointMasses;
    std::vector<Spring> springs;
    std::vector<glm::ivec3> triangles;
};