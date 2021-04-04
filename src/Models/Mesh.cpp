#include "Models/Mesh.h"
#include "common.h"

void Material::Setup(
    ShaderProgram& program,
    std::unordered_map<std::string, std::unique_ptr<Texture>>& textures,
    const GLenum diffuseTextureId,
    const GLenum specularTextureId,
    const GLenum normalTextureId,
    int diffuseIdx,
    int specularIdx,
    int normalIdx)
{
    program.SetUniform("material.hasDiffuseMap", hasDiffuseMap && (textures.count(diffuseMapPath) > 0));
    program.SetUniform("material.hasSpecularMap", hasSpecularMap && (textures.count(specularMapPath) > 0));
    program.SetUniform("material.hasNormalMap", hasNormalMap && (textures.count(normalMapPath) > 0));
    if (hasDiffuseMap && textures.count(diffuseMapPath)) {
        glActiveTexture(diffuseTextureId);
        GL_CHECK_ERRORS;
        textures[diffuseMapPath]->GLBind();
        GL_CHECK_ERRORS;
        program.SetUniform("material.diffuseMap", diffuseIdx);
        GL_CHECK_ERRORS;
    }
    if (hasSpecularMap && textures.count(specularMapPath)) {
        glActiveTexture(specularTextureId);
        GL_CHECK_ERRORS;
        textures[specularMapPath]->GLBind();
        GL_CHECK_ERRORS;
        program.SetUniform("material.specularMap", specularIdx);
        GL_CHECK_ERRORS;
    }
    if (hasNormalMap && (textures.count(normalMapPath) > 0)) {
        glActiveTexture(normalTextureId);
        GL_CHECK_ERRORS;
        textures[normalMapPath]->GLBind();
        GL_CHECK_ERRORS;
        program.SetUniform("material.normalMap", normalIdx);
        GL_CHECK_ERRORS;
    }
    program.SetUniform("material.ambient", ambient);
    GL_CHECK_ERRORS;
    program.SetUniform("material.diffuse", diffuse);
    GL_CHECK_ERRORS;
    program.SetUniform("material.specular", specular);
    GL_CHECK_ERRORS;
    program.SetUniform("material.shininess", shininess);
    GL_CHECK_ERRORS;
}

void Material::SetMaps(
    const std::string& diffuseMapPath_,
    const std::string& specularMapPath_,
    const std::string& normalMapPath_,
    std::unordered_map<std::string, std::unique_ptr<Texture>>& textures)
{
    if (textures.count(diffuseMapPath_)) {
        hasDiffuseMap = true;
        diffuseMapPath = diffuseMapPath_;
        diffuseMap = textures[diffuseMapPath]->GetTextureID();
    }
    if (textures.count(specularMapPath_)) {
        hasSpecularMap = true;
        specularMapPath = specularMapPath_;
        specularMap = textures[specularMapPath]->GetTextureID();
    }
    if (textures.count(normalMapPath_)) {
        hasSpecularMap = true;
        specularMapPath = normalMapPath_;
        specularMap = textures[specularMapPath]->GetTextureID();
    }
}

Mesh::Mesh(
    std::vector<float>&& data,
    uint32_t nVertices,
    std::uint32_t matId_,
    bool isStatic_)
    : matId(matId_)
    , isStatic(isStatic_)
{
    if (data.size() != nVertices * 8) {
        throw std::runtime_error("Data array size should be nVertices * 8");
    }
    if (nVertices % 3 != 0) {
        throw std::runtime_error("nVerticers should be dividable by 3");
    }
    positions.resize(nVertices);
    normals.resize(nVertices);
    texCoords.resize(nVertices);
    indices.resize(nVertices);
    for (uint32_t i = 0; i < nVertices; ++i) {
        positions[i] = glm::vec3(
            data[i * 8],
            data[i * 8 + 1],
            data[i * 8 + 2]);
        normals[i] = glm::vec3(
            data[i * 8 + 3],
            data[i * 8 + 4],
            data[i * 8 + 5]);
        texCoords[i] = glm::vec2(
            data[i * 8 + 6],
            data[i * 8 + 7]);
        indices[i] = i;
    }
}

void Mesh::GLLoad()
{
    //generate buffers
    glGenVertexArrays(1, &VAO);
    GL_CHECK_ERRORS;
    glGenBuffers(1, &positionsVBO);
    GL_CHECK_ERRORS;
    glGenBuffers(1, &normalsVBO);
    GL_CHECK_ERRORS;
    glGenBuffers(1, &texCoordsVBO);
    GL_CHECK_ERRORS;
    glGenBuffers(1, &modelsVBO);
    GL_CHECK_ERRORS;
    glGenBuffers(1, &tangentsVBO);
    GL_CHECK_ERRORS;
    glGenBuffers(1, &bitangentsVBO);
    GL_CHECK_ERRORS;
    glGenBuffers(1, &EBO);
    GL_CHECK_ERRORS;

    //VAO
    glBindVertexArray(VAO);
    GL_CHECK_ERRORS;

    {
        //vertex positions
        glBindBuffer(GL_ARRAY_BUFFER, positionsVBO);
        GL_CHECK_ERRORS;
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GL_FLOAT) * 3, positions.data(), GL_STATIC_DRAW);
        GL_CHECK_ERRORS;
        glEnableVertexAttribArray(0);
        GL_CHECK_ERRORS;
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 3, (GLvoid*)0);
        GL_CHECK_ERRORS;

        //normals
        glBindBuffer(GL_ARRAY_BUFFER, normalsVBO);
        GL_CHECK_ERRORS;
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT) * 3, normals.data(), GL_STATIC_DRAW);
        GL_CHECK_ERRORS;
        glEnableVertexAttribArray(1);
        GL_CHECK_ERRORS;
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 3, (GLvoid*)0);
        GL_CHECK_ERRORS;

        //texture coordinates
        glBindBuffer(GL_ARRAY_BUFFER, texCoordsVBO);
        GL_CHECK_ERRORS;
        glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(GL_FLOAT) * 2, texCoords.data(), GL_STATIC_DRAW);
        GL_CHECK_ERRORS;
        glEnableVertexAttribArray(2);
        GL_CHECK_ERRORS;
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 2, (GLvoid*)0);
        GL_CHECK_ERRORS;

        //tangents and bitangents for normal mapping
        if (hasTangentsBitangents) {
            glBindBuffer(GL_ARRAY_BUFFER, tangentsVBO);
            GL_CHECK_ERRORS;
            glBufferData(GL_ARRAY_BUFFER, tangents.size() * sizeof(GL_FLOAT) * 3, tangents.data(), GL_STATIC_DRAW);
            GL_CHECK_ERRORS;
            glEnableVertexAttribArray(3);
            GL_CHECK_ERRORS;
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 3, (GLvoid*)0);
            GL_CHECK_ERRORS;

            glBindBuffer(GL_ARRAY_BUFFER, bitangentsVBO);
            GL_CHECK_ERRORS;
            glBufferData(GL_ARRAY_BUFFER, bitangents.size() * sizeof(GL_FLOAT) * 3, bitangents.data(), GL_STATIC_DRAW);
            GL_CHECK_ERRORS;
            glEnableVertexAttribArray(4);
            GL_CHECK_ERRORS;
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 3, (GLvoid*)0);
            GL_CHECK_ERRORS;
        }

        //indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        GL_CHECK_ERRORS;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
        GL_CHECK_ERRORS;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    GL_CHECK_ERRORS;
    glBindVertexArray(0);
    GL_CHECK_ERRORS;

    isLoaded = true;
}

//draw without instancing - select vertexPhong.glsl vertex shader
void Mesh::Draw() const
{
    if (!isLoaded) {
        return;
    }
    glBindVertexArray(VAO);
    GL_CHECK_ERRORS;
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
    GL_CHECK_ERRORS;
    glBindVertexArray(0);
    GL_CHECK_ERRORS;
}

//draw with instancing - select vertexPhong_ins.glsl vertex shader
void Mesh::Draw(const std::vector<glm::mat4>& modelMatrices) const
{
    glBindVertexArray(VAO);
    GL_CHECK_ERRORS;
    glBindBuffer(GL_ARRAY_BUFFER, modelsVBO);
    GL_CHECK_ERRORS;
    glBufferData(GL_ARRAY_BUFFER, modelMatrices.size() * sizeof(GL_FLOAT) * 16, modelMatrices.data(), GL_STATIC_DRAW);
    GL_CHECK_ERRORS;
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(i + 5);
        GL_CHECK_ERRORS;
        glVertexAttribPointer(i + 5, 4, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 16, (GLvoid*)(sizeof(GLfloat) * i * 4));
        GL_CHECK_ERRORS;
        glVertexAttribDivisor(i + 5, 1);
        GL_CHECK_ERRORS;
    }
    glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, modelMatrices.size());
}

void Mesh::Release()
{
    glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &positionsVBO);
    glDeleteBuffers(1, &normalsVBO);
    glDeleteBuffers(1, &texCoordsVBO);
    glDeleteBuffers(1, &EBO);
    isLoaded = false;
}

std::unique_ptr<Mesh> createCube()
{
    return std::make_unique<Mesh>(
        std::vector<float>({ -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f,

            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }),
        36);
}
