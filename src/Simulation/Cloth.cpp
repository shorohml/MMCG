#include "Simulation/Cloth.h"

void Cloth::createMassesAndSprings()
{
    //create point masses
    pointMasses.reserve(widthPoints * heightPoints);
    double leftHeight = height;
    double rightHeight = height;
    if (upperLeftCorner.y > upperRightCorner.y) {
        leftHeight += upperLeftCorner.y - upperRightCorner.y;        
    } else {
        rightHeight += upperRightCorner.y - upperLeftCorner.y;        
    }
    glm::dvec3 left = upperLeftCorner;
    glm::dvec3 right = upperRightCorner;
    for (std::uint32_t i = 0; i < heightPoints; ++i) {
        glm::dvec3 dir = right - left;
        double len = glm::length(dir);
        dir = glm::normalize(dir);
        for (std::uint32_t j = 0; j < widthPoints; ++j) {
            glm::dvec3 pos = left + len / (widthPoints - 1) * j * dir;
            bool pinned = i == 0 && (j == 0 || j == widthPoints - 1);
            pointMasses.emplace_back(pos, pinned);
        }
        left.y -= leftHeight / (heightPoints - 1);
        right.y -= rightHeight / (heightPoints - 1);
    }

    //create springs
    //structural (upper)
    for (std::uint32_t i = 1; i < heightPoints; ++i) {
        for (std::uint32_t j = 0; j < widthPoints; ++j) {
            springs.emplace_back(
                &pointMasses[(i - 1) * widthPoints + j],
                &pointMasses[i * widthPoints + j],
                Constraint::STRUCTURAL);
        }
    }
    //structural (left)
    for (std::uint32_t i = 0; i < heightPoints; ++i) {
        for (std::uint32_t j = 1; j < widthPoints; ++j) {
            springs.emplace_back(
                &pointMasses[i * widthPoints + j - 1],
                &pointMasses[i * widthPoints + j],
                Constraint::STRUCTURAL);
        }
    }
    //shearing (upper)
    for (std::uint32_t i = 2; i < heightPoints; ++i) {
        for (std::uint32_t j = 0; j < widthPoints; ++j) {
            springs.emplace_back(
                &pointMasses[(i - 2) * widthPoints + j],
                &pointMasses[i * widthPoints + j],
                Constraint::SHEARING);
        }
    }
    //shearing (left)
    for (std::uint32_t i = 0; i < heightPoints; ++i) {
        for (std::uint32_t j = 2; j < widthPoints; ++j) {
            springs.emplace_back(
                &pointMasses[i * widthPoints + j - 2],
                &pointMasses[i * widthPoints + j],
                Constraint::SHEARING);
        }
    }
    //bending (upper left)
    for (std::uint32_t i = 1; i < heightPoints; ++i) {
        for (std::uint32_t j = 1; j < widthPoints; ++j) {
            springs.emplace_back(
                &pointMasses[(i - 1) * widthPoints + j - 1],
                &pointMasses[i * widthPoints + j],
                Constraint::BENDING);
        }
    }
    //bending (upper right)
    for (std::uint32_t i = 1; i < heightPoints; ++i) {
        for (std::uint32_t j = 0; j < widthPoints - 1; ++j) {
            springs.emplace_back(
                &pointMasses[(i - 1) * widthPoints + j + 1],
                &pointMasses[i * widthPoints + j],
                Constraint::BENDING);
        }
    }
}

std::shared_ptr<Mesh> Cloth::createMesh()
{
    //positions and texture coordinates
    std::vector<glm::vec3> positions(pointMasses.size());
    std::vector<glm::vec2> texCoords(pointMasses.size());
    for (std::uint32_t i = 0; i < pointMasses.size(); ++i) {
        positions[i] = pointMasses[i].currentPosition;
        //compute texture coords (in [0, 1])
        std::uint32_t row = i / widthPoints;
        std::uint32_t col = i % widthPoints;
        texCoords[i] = glm::vec2(
            1.0f / widthPoints * col,
            1.0f / heightPoints * row);
    }

    //indices
    std::uint32_t numIndices = 6 * (heightPoints - 1) * (widthPoints - 1);
    std::vector<std::uint32_t> indices;
    indices.reserve(numIndices);
    triangles.reserve(numIndices / 3);
    for (std::uint32_t i = 0; i < heightPoints - 1; ++i) {
        for (std::uint32_t j = 0; j < widthPoints - 1; ++j) {
            std::uint32_t offset = i * widthPoints + j;

            glm::ivec3 triangle_1(offset, offset + 1 + widthPoints, offset + 1);
            glm::ivec3 triangle_2(offset, offset + widthPoints, offset + 1 + widthPoints);

            triangles.push_back(triangle_1);
            triangles.push_back(triangle_2);

            indices.push_back(triangle_1.x);
            indices.push_back(triangle_1.y);
            indices.push_back(triangle_1.z);
            indices.push_back(triangle_2.x);
            indices.push_back(triangle_2.y);
            indices.push_back(triangle_2.z);
        }
    }

    //compute normals
    std::vector<glm::vec3> normalsTmp(pointMasses.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> normals(pointMasses.size());
    for (std::uint32_t i = 0; i < triangles.size(); ++i) {
        glm::vec3 v1 = positions[triangles[i].x];
        glm::vec3 v2 = positions[triangles[i].y];
        glm::vec3 v3 = positions[triangles[i].z];

        glm::vec3 edge1(glm::normalize(v2 - v1));
        glm::vec3 edge2(glm::normalize(v3 - v1));

        glm::vec3 faceNormal = glm::cross(edge1, edge2);

        normalsTmp.at(triangles[i].x) += faceNormal;
        normalsTmp.at(triangles[i].y) += faceNormal;
        normalsTmp.at(triangles[i].z) += faceNormal;
    }
    for (std::uint32_t i = 0; i < normalsTmp.size(); ++i) {
        normals[i] = glm::normalize(normalsTmp[i]);
    }

    return std::make_shared<Mesh>(
        positions,
        normals,
        texCoords,
        indices,
        0,
        glm::mat4(1.0f),
        false);
}

Cloth::Cloth(
    const glm::dvec3& upperLeftCorner_,
    const glm::dvec3& upperRightCorner_,
    const double height_,
    const std::uint32_t widthPoints_,
    const std::uint32_t heightPoints_)
    : upperLeftCorner(upperLeftCorner_)
    , upperRightCorner(upperRightCorner_)
    , height(height_)
    , widthPoints(widthPoints_)
    , heightPoints(heightPoints_)
{
    if (widthPoints_ < 2 || heightPoints_ < 2) {
        throw std::runtime_error("widthPoints and heightPoints must be >= 2");
    }
    width = glm::length(upperLeftCorner_ - upperRightCorner_);
    createMassesAndSprings();
    mesh = createMesh();
}

void Cloth::recomputePositionsNormals()
{
    //set positions from point masses
    for (std::uint32_t i = 0; i < pointMasses.size(); ++i) {
        mesh->positions[i] = pointMasses[i].currentPosition;
    }

    //compute normals
    std::vector<glm::vec3> normalsTmp(pointMasses.size(), glm::vec3(0.0f));
    for (std::uint32_t i = 0; i < triangles.size(); ++i) {
        glm::vec3 v1 = mesh->positions[triangles[i].x];
        glm::vec3 v2 = mesh->positions[triangles[i].y];
        glm::vec3 v3 = mesh->positions[triangles[i].z];

        glm::vec3 edge1(glm::normalize(v2 - v1));
        glm::vec3 edge2(glm::normalize(v3 - v1));

        glm::vec3 faceNormal = glm::cross(edge1, edge2);

        normalsTmp.at(triangles[i].x) += faceNormal;
        normalsTmp.at(triangles[i].y) += faceNormal;
        normalsTmp.at(triangles[i].z) += faceNormal;
    }
    for (std::uint32_t i = 0; i < normalsTmp.size(); ++i) {
        mesh->normals[i] = glm::normalize(normalsTmp[i]);
    }
}

void Cloth::simulate(
    double dt,
    std::uint32_t simulationSteps,
    std::vector<glm::dvec3> accelerations)
{
    double mass = density * width * height / widthPoints / heightPoints;
    dt /= simulationSteps;

    //compute total force acting on point masses
    //external forces
    glm::dvec3 externalForce(0.0); //same for each mass
    for (std::uint32_t i = 0; i < accelerations.size(); ++i) {
        externalForce += mass * accelerations[i];
    }
    for (std::uint32_t i = 0; i < pointMasses.size(); ++i) {
        pointMasses[i].forces = externalForce;
    }
    //spring correction forces (Hooke's law)
    for (std::uint32_t i = 0; i < springs.size(); ++i) {
        glm::dvec3 dir = springs[i].start->currentPosition - springs[i].end->currentPosition;
        double dist = glm::length(dir);
        dir = glm::normalize(dir);
        double magnitude = std::abs(ks * (dist - springs[i].restLength));
        if (springs[i].constraint == Constraint::BENDING) {
            magnitude *= 0.2;
        }
        if (dist > springs[i].restLength) {
            springs[i].start->forces -= magnitude * dir;
            springs[i].end->forces += magnitude * dir;
        } else {
            springs[i].start->forces += magnitude * dir;
            springs[i].end->forces -= magnitude * dir;
        }
    }

    //verlet integration
    for (std::uint32_t i = 0; i < pointMasses.size(); ++i) {
        if (pointMasses[i].pinned) {
            continue;
        }
        glm::dvec3 diff = pointMasses[i].currentPosition - pointMasses[i].previousPosition;
        diff *= 1 - dumping / 100;
        pointMasses[i].previousPosition = pointMasses[i].currentPosition;
        pointMasses[i].currentPosition += diff + pointMasses[i].forces / mass * dt * dt;
    }
}