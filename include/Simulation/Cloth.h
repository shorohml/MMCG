#pragma once

#include "Models/Mesh.h"
#include "Simulation/PointMass.h"
#include "Simulation/Spring.h"
#include <vector>
#include <memory>

struct Cloth {
public:
    Cloth(
        const glm::vec3& lowerLeftCorner_,
        const float width_,
        const float height_,
        const std::uint32_t widthPoints_,
        const std::uint32_t heightPoints_);

    std::shared_ptr<Mesh> mesh;

    void simulate(
        float deltaTime,
        std::uint32_t simulationSteps,
        std::vector<glm::vec3> accelerations
    );

    void createMassesAndSprings();
    std::shared_ptr<Mesh> createMesh();
    void recomputePositionsNormals();

private:

    glm::vec3 lowerLeftCorner;
    float width;
    float height;
    std::uint32_t widthPoints;
    std::uint32_t heightPoints;

    std::vector<PointMass> pointMasses;
    std::vector<Spring> springs;
    std::vector<glm::ivec3> triangles;

    float dumping = 0.2f;
    float density = 150.0f;
    float ks = 5000.0f;
};