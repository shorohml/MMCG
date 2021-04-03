#include "Models/ImportScene.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <vector>

namespace {

glm::vec3 ai2glm3D(aiVector3D& vec)
{
    return glm::vec3(vec.x, vec.y, vec.z);
}

glm::vec2 ai2glm2D(aiVector3D& vec)
{
    return glm::vec2(vec.x, vec.y);
}

void fromAiMesh(
    const std::string& directory,
    const aiMesh* assimpMesh,
    const aiScene* assimpScene,
    const glm::mat4& model,
    std::vector<std::unique_ptr<Mesh>>& scene,
    std::unordered_map<uint32_t, Material>& materials,
    std::unordered_map<std::string, std::unique_ptr<Texture>>& textures,
    uint32_t maxMatId)
{
    if (!assimpMesh->HasPositions() || !assimpMesh->HasFaces() || !assimpMesh->HasNormals()) {
        throw std::runtime_error("Invalid mesh");
    }
    std::vector<glm::vec3> positions(assimpMesh->mNumVertices);
    std::vector<glm::vec3> normals(assimpMesh->mNumVertices);
    std::vector<glm::vec2> texCoords(assimpMesh->mNumVertices, glm::vec2(0.0f));
    std::vector<uint32_t> indices(assimpMesh->mNumFaces * 3);
    bool hasTexCoors = assimpMesh->mTextureCoords[0];

    for (std::size_t i = 0; i < assimpMesh->mNumVertices; ++i) {
        positions[i] = ai2glm3D(assimpMesh->mVertices[i]);
        normals[i] = ai2glm3D(assimpMesh->mNormals[i]);
        if (hasTexCoors) {
            texCoords[i] = ai2glm2D(assimpMesh->mTextureCoords[0][i]);
        }
    }
    for (std::size_t i = 0; i < assimpMesh->mNumFaces; i++) {
        for (std::size_t j = 0; j < 3; ++j) {
            indices[3 * i + j] = assimpMesh->mFaces[i].mIndices[j];
        }
    }

    uint32_t matId = 0;
    if (assimpScene->HasMaterials()) {
        matId = maxMatId + 1;
        Material material;
        material.id = matId;

        aiMaterial* assimpMaterial = assimpScene->mMaterials[assimpMesh->mMaterialIndex];

        aiColor3D color(0.f, 0.f, 0.f);
        float shininess = 32.0f;

        assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        material.diffuse = glm::vec3(color.r, color.b, color.g);

        assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color);
        material.ambient = glm::vec3(color.r, color.b, color.g);

        assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color);
        material.specular = glm::vec3(color.r, color.b, color.g);

        assimpMaterial->Get(AI_MATKEY_SHININESS, shininess);
        material.shininess = shininess;

        if (assimpMaterial->GetTextureCount(aiTextureType_DIFFUSE)) {
            aiString str;
            assimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &str);
            std::string path(str.C_Str());
            path = directory + "/" + path;
            if (textures.count(path) == 0) {
                textures[path] = std::make_unique<Texture>(path);
            }
            material.hasDiffuseMap = true;
            material.diffuseMap = textures[path]->GetTextureID();
            material.diffuseMapPath = path;
        }

        if (assimpMaterial->GetTextureCount(aiTextureType_SPECULAR)) {
            aiString str;
            assimpMaterial->GetTexture(aiTextureType_SPECULAR, 0, &str);
            std::string path(str.C_Str());
            path = directory + "/" + path;
            if (textures.count(path) == 0) {
                textures[path] = std::make_unique<Texture>(path);
            }
            material.hasSpecularMap = true;
            material.specularMap = textures[path]->GetTextureID();
            material.specularMapPath = path;
        }
        materials[material.id] = material;
    }
    scene.push_back(std::make_unique<Mesh>(positions, normals, texCoords, indices, matId, model));
}

void fromAiNode(
    const std::string& directory,
    const aiNode* node,
    const aiScene* assimpScene,
    const glm::mat4& parentModel,
    std::vector<std::unique_ptr<Mesh>>& scene,
    std::unordered_map<uint32_t, Material>& materials,
    std::unordered_map<std::string, std::unique_ptr<Texture>>& textures,
    uint32_t& maxMatId)
{
    aiMatrix4x4 assimpTransform = node->mTransformation;
    glm::mat4 transform(
        assimpTransform.a1, assimpTransform.a2, assimpTransform.a3, assimpTransform.a4,
        assimpTransform.b1, assimpTransform.b2, assimpTransform.b3, assimpTransform.b4,
        assimpTransform.c1, assimpTransform.c2, assimpTransform.c3, assimpTransform.c4,
        assimpTransform.d1, assimpTransform.d2, assimpTransform.d3, assimpTransform.d4);
    glm::mat4 model = transform * parentModel;
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* assimpMesh = assimpScene->mMeshes[node->mMeshes[i]];
        fromAiMesh(
            directory,
            assimpMesh,
            assimpScene,
            model,
            scene,
            materials,
            textures,
            maxMatId);
        ++maxMatId;
    }
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        fromAiNode(
            directory,
            node->mChildren[i],
            assimpScene,
            model,
            scene,
            materials,
            textures,
            maxMatId);
    }
}

}

void importSceneFromFile(
    const std::string& path,
    std::vector<std::unique_ptr<Mesh>>& scene,
    std::unordered_map<uint32_t, Material>& materials,
    std::unordered_map<std::string, std::unique_ptr<Texture>>& textures)
{
    Assimp::Importer importer;
    const aiScene* assimpScene = importer.ReadFile(path,
        aiProcess_JoinIdenticalVertices | aiProcess_GenNormals | aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!assimpScene || assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !assimpScene->mRootNode) {
        throw std::runtime_error(importer.GetErrorString());
    }
    std::string directory = path.substr(0, path.find_last_of('/'));

    uint32_t maxMatId = 0;
    for (auto& item : materials) {
        if (item.first > maxMatId) {
            maxMatId = item.first;
        }
    }
    fromAiNode(
        directory,
        assimpScene->mRootNode,
        assimpScene,
        glm::mat4(1.0f),
        scene,
        materials,
        textures,
        maxMatId);
}