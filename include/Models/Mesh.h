#pragma once

#include "ShaderProgram.h"
#include "Texture.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Mesh {
public:
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<uint32_t> indices;

    Mesh(
        std::vector<glm::vec3>&& positions_,
        std::vector<glm::vec3>&& normals_,
        std::vector<glm::vec2>&& texCoords_,
        std::vector<uint32_t>&& indices_)
        : positions(std::move(positions_))
        , normals(std::move(normals_))
        , texCoords(std::move(texCoords_))
        , indices(std::move(indices_))
    {
    }

    Mesh(
        std::vector<float>&& data,
        uint32_t n_vertices);

    void GLSetup();

    void Draw() const;

    void Release();

private:
    bool isLoaded = false;
    GLuint positionsVBO;
    GLuint normalsVBO;
    GLuint texCoordsVBO;
    GLuint VAO;
    GLuint EBO;
};

std::unique_ptr<Mesh> createCube();