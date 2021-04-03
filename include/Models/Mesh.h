#pragma once

#include "ShaderProgram.h"
#include "Texture.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Material {
public:
    uint32_t id = 0;

    bool hasDiffuseMap = false;
    std::string diffuseMapPath;
    GLuint diffuseMap;
    bool hasSpecularMap = false;
    std::string specularMapPath;
    GLuint specularMap;

    glm::vec3 ambient = glm::vec3(0.05f);
    glm::vec3 diffuse = glm::vec3(0.8f);
    glm::vec3 specular = glm::vec3(1.0f);
    float shininess = 32.0f;

    void Setup(
        ShaderProgram& program,
        std::unordered_map<std::string, std::unique_ptr<Texture>>& textures,
        const GLenum diffuseTextureId,
        const GLenum specularTextureId,
        int diffuseIdx,
        int specularIdx);

    void SetMaps(
        const std::string &diffuseMapPath_,
        const std::string &specularMapPath_,
        std::unordered_map<std::string, std::unique_ptr<Texture>> &textures
    );
};

class Mesh {
public:
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<uint32_t> indices;
    std::uint32_t matId;

    Mesh(
        std::vector<glm::vec3>&& positions_,
        std::vector<glm::vec3>&& normals_,
        std::vector<glm::vec2>&& texCoords_,
        std::vector<uint32_t>&& indices_,
        uint32_t matId_ = 0)
        : positions(std::move(positions_))
        , normals(std::move(normals_))
        , texCoords(std::move(texCoords_))
        , indices(std::move(indices_))
        , matId(matId_)
    {
    }

    Mesh(
        std::vector<glm::vec3>& positions_,
        std::vector<glm::vec3>& normals_,
        std::vector<glm::vec2>& texCoords_,
        std::vector<uint32_t>& indices_,
        uint32_t matId_ = 0)
        : positions(std::move(positions_))
        , normals(std::move(normals_))
        , texCoords(std::move(texCoords_))
        , indices(std::move(indices_))
        , matId(matId_)
    {
    }

    Mesh(
        std::vector<float>&& data,
        uint32_t n_vertices,
        std::uint32_t matId_ = 0);

    void GLSetup();

    void Draw() const;
    void Draw(const std::vector<glm::mat4>& modelMatrices) const;

    void Release();

private:
    bool isLoaded = false;
    GLuint positionsVBO;
    GLuint normalsVBO;
    GLuint texCoordsVBO;
    GLuint modelsVBO;
    GLuint VAO;
    GLuint EBO;
};

std::unique_ptr<Mesh> createCube();