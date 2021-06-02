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
    Spring(PointMass& start_, PointMass& end_, const Constraint constraint_) : contraint(constraint_)
    {
        start = std::make_shared<PointMass>(start_);
        end = std::make_shared<PointMass>(end_);
    };

private:
    std::shared_ptr<PointMass> start;
    std::shared_ptr<PointMass> end;
    Constraint contraint;
};