#pragma once

#include <glm/glm.hpp>

struct PointMass {
public:
    PointMass(const glm::vec3& startPosition_, const bool pinned_)
        : startPosition(startPosition_)
        , pinned(pinned_)
        , currentPosition(startPosition_)
    {
    }

    //static
    glm::vec3 startPosition;
    bool pinned;

    //dynamic
    glm::vec3 currentPosition;
};