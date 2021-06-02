#pragma once

#include <glm/glm.hpp>

struct PointMass {
public:
    PointMass(const glm::vec3& startPosition_, const bool pinned_)
        : startPosition(startPosition_)
        , pinned(pinned_)
        , currentPosition(startPosition_)
        , previousPosition(startPosition_)
        , forces(glm::vec3(0.0f))
    {
    }

    //static
    glm::vec3 startPosition;
    bool pinned;

    //dynamic
    glm::vec3 currentPosition;
    glm::vec3 previousPosition;
    glm::vec3 forces;
};