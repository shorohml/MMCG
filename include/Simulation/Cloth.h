#pragma once

#include "Models/Mesh.h"
#include "Simulation/PointMass.h"
#include "Simulation/Spring.h"
#include <memory>
#include <vector>

struct Cloth {
public:
    Cloth(
        const glm::dvec3& upperLeftCorner_,
        const glm::dvec3& upperRightCorner_,
        const double height_,
        const std::uint32_t widthPoints_,
        const std::uint32_t heightPoints_);

    std::shared_ptr<Mesh> mesh1;
    std::shared_ptr<Mesh> mesh2;

    void simulate(
        double deltaTime,
        std::uint32_t simulationSteps,
        std::vector<glm::dvec3> accelerations);

    void createMassesAndSprings();
    std::shared_ptr<Mesh> createMesh(glm::dvec3 offset, bool side);
    void recomputePositionsNormals(glm::dvec3 offset, bool side);
    void recomputePositionsNormals();

private:
    glm::dvec3 upperLeftCorner;
    glm::dvec3 upperRightCorner;
    double width;
    double height;
    std::uint32_t widthPoints;
    std::uint32_t heightPoints;

    std::vector<PointMass> pointMasses;
    std::vector<Spring> springs;
    std::vector<glm::ivec3> triangles1;
    std::vector<glm::ivec3> triangles2;

    //TODO: initialize this from config
    double dumping = 0.2; // %, to simulate loss of energy due to friction etc
    double density = 0.015; // kg/cm2
    double ks = 500.0; // H/cm
};