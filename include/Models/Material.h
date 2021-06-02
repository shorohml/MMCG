#pragma once

#include "ShaderProgram.h"
#include "Models/Texture.h"
#include <string>
#include <unordered_map>
#include <memory>

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

    glm::vec3 ambient = glm::vec3(0.7f);
    glm::vec3 diffuse = glm::vec3(0.4f);
    glm::vec3 specular = glm::vec3(0.5f);
    float shininess = 32.0f;
    float opacity = 1.0f;
    int twosided = 1;

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