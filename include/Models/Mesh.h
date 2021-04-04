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
    std::string name = "default";

    bool hasDiffuseMap = false;
    std::string diffuseMapPath;
    GLuint diffuseMap;
    bool hasSpecularMap = false;
    std::string specularMapPath;
    GLuint specularMap;
    bool hasNormalMap = false;
    std::string normalMapPath;
    GLuint normalMap;

    glm::vec3 ambient = glm::vec3(0.05f);
    glm::vec3 diffuse = glm::vec3(0.8f);
    glm::vec3 specular = glm::vec3(1.0f);
    float shininess = 32.0f;

    void Setup(
        ShaderProgram& program,
        std::unordered_map<std::string, std::unique_ptr<Texture>>& textures,
        const GLenum diffuseTextureId,
        const GLenum specularTextureId,
        const GLenum normalTextureId,
        int diffuseIdx,
        int specularIdx,
        int normalIdx);

    void SetMaps(
        const std::string& diffuseMapPath_,
        const std::string& specularMapPath_,
        const std::string& normalMapPath_,
        std::unordered_map<std::string, std::unique_ptr<Texture>>& textures);
};

class Mesh {
public:
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<uint32_t> indices;
    std::uint32_t matId;
    glm::mat4 model;
    bool isStatic;
    bool hasTangentsBitangents;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;

    Mesh(
        std::vector<glm::vec3>&& positions_,
        std::vector<glm::vec3>&& normals_,
        std::vector<glm::vec2>&& texCoords_,
        std::vector<uint32_t>&& indices_,
        uint32_t matId_ = 0,
        glm::mat4 model_ = glm::mat4(1.0f),
        bool isStatic_ = true)
        : positions(std::move(positions_))
        , normals(std::move(normals_))
        , texCoords(std::move(texCoords_))
        , indices(std::move(indices_))
        , matId(matId_)
        , model(model_)
        , isStatic(isStatic_)
        , hasTangentsBitangents(false)
    {
    }

    Mesh(
        std::vector<glm::vec3>& positions_,
        std::vector<glm::vec3>& normals_,
        std::vector<glm::vec2>& texCoords_,
        std::vector<uint32_t>& indices_,
        uint32_t matId_ = 0,
        glm::mat4 model_ = glm::mat4(1.0f),
        bool isStatic_ = true)
        : positions(std::move(positions_))
        , normals(std::move(normals_))
        , texCoords(std::move(texCoords_))
        , indices(std::move(indices_))
        , matId(matId_)
        , model(model_)
        , isStatic(isStatic_)
        , hasTangentsBitangents(false)
    {
    }

    Mesh(
        std::vector<glm::vec3>& positions_,
        std::vector<glm::vec3>& normals_,
        std::vector<glm::vec2>& texCoords_,
        std::vector<uint32_t>& indices_,
        std::vector<glm::vec3>& tangents_,
        std::vector<glm::vec3>& bitangents_,
        uint32_t matId_ = 0,
        glm::mat4 model_ = glm::mat4(1.0f),
        bool isStatic_ = true)
        : positions(std::move(positions_))
        , normals(std::move(normals_))
        , texCoords(std::move(texCoords_))
        , indices(std::move(indices_))
        , matId(matId_)
        , model(model_)
        , isStatic(isStatic_)
        , hasTangentsBitangents(true)
        , tangents(std::move(tangents_))
        , bitangents(std::move(bitangents_))
    {
    }

    Mesh(
        std::vector<float>&& data,
        uint32_t n_vertices,
        std::uint32_t matId_ = 0,
        bool isStatic_ = true);

    std::uint32_t numberOfVertices() const
    {
        return positions.size();
    }

    std::uint32_t numberOfFaces() const
    {
        return indices.size() / 3;
    }

    void GLLoad();

    void Draw() const;
    void Draw(const std::vector<glm::mat4>& modelMatrices) const;

    void Release();

    bool IsLoaded() const
    {
        return isLoaded;
    }

private:
    bool isLoaded = false;
    GLuint positionsVBO;
    GLuint normalsVBO;
    GLuint texCoordsVBO;
    GLuint modelsVBO;
    GLuint tangentsVBO;
    GLuint bitangentsVBO;
    GLuint VAO;
    GLuint EBO;
};

std::unique_ptr<Mesh> createCube();