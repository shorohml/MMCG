#pragma once

#include <glm/glm.hpp>

struct PointMass {
public:
    PointMass(const glm::dvec3& startPosition_, const bool pinned_)
        : startPosition(startPosition_)
        , pinned(pinned_)
        , currentPosition(startPosition_)
        , previousPosition(startPosition_)
        , forces(glm::vec3(0.0f))
    {
    }

    //static
    glm::dvec3 startPosition;
    bool pinned;

    //dynamic
    glm::dvec3 currentPosition;
    glm::dvec3 previousPosition;
    glm::dvec3 forces;
};