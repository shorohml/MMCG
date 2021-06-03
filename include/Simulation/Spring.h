#pragma once

#include "Simulation/PointMass.h"
#include <glm/glm.hpp>
#include <memory>

enum Constraint {
    STRUCTURAL = 0,
    SHEARING,
    BENDING,
};

struct Spring {
public:
    //TODO: change to indexes (or to smart pointers)
    Spring(PointMass *start_, PointMass *end_, const Constraint constraint_)
        : constraint(constraint_)
    {
        start = start_;
        end = end_;
        restLength = glm::length(start->startPosition - end->startPosition);
    };

    float restLength;
    PointMass *start;
    PointMass *end;
    Constraint constraint;
};