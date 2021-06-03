#pragma once

#include "Models/Mesh.h"
#include "Simulation/PointMass.h"
#include "Simulation/Spring.h"
#include <memory>
#include <vector>

struct Cloth {
public:
    Cloth(
        const glm::dvec3& lowerLeftCorner_,
        const double width_,
        const double height_,
        const std::uint32_t widthPoints_,
        const std::uint32_t heightPoints_);

    std::shared_ptr<Mesh> mesh;

    void simulate(
        double deltaTime,
        std::uint32_t simulationSteps,
        std::vector<glm::dvec3> accelerations);

    void createMassesAndSprings();
    std::shared_ptr<Mesh> createMesh();
    void recomputePositionsNormals();

private:
    glm::dvec3 lowerLeftCorner;
    double width;
    double height;
    std::uint32_t widthPoints;
    std::uint32_t heightPoints;

    std::vector<PointMass> pointMasses;
    std::vector<Spring> springs;
    std::vector<glm::ivec3> triangles;

    double dumping = 0.2;
    double density = 150.0;
    double ks = 5000.0;
};