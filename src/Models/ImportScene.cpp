#include "Models/ImportScene.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <unordered_set>
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

std::string addToDir(const std::string& directory, const std::string& path)
{
    std::string result = directory;
    if (result[result.size() - 1] == '/') {
        result = directory.substr(0, result.size() - 1);
    }
    if (path[0] == '/' || path[0] == '\\') {
        result += "/" + path.substr(1, path.size() - 1);
    } else {
        result += "/" + path;
    }
    return result;
}

void fromAiMesh(
    const aiMesh* assimpMesh,
    const aiScene* assimpScene,
    const glm::mat4& model,
    std::vector<std::shared_ptr<Mesh>>& scene,
    const uint32_t maxMatId)
{
    if (!assimpMesh->HasPositions() || !assimpMesh->HasFaces() || !assimpMesh->HasNormals()) {
        throw std::runtime_error("Invalid mesh");
    }
    std::vector<glm::vec3> positions(assimpMesh->mNumVertices);
    std::vector<glm::vec3> normals(assimpMesh->mNumVertices);
    std::vector<glm::vec2> texCoords(assimpMesh->mNumVertices, glm::vec2(0.0f));
    std::vector<uint32_t> indices(assimpMesh->mNumFaces * 3);
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;
    bool hasTexCoors = assimpMesh->mTextureCoords[0];
    bool hasTangentsBitangents = assimpMesh->HasTangentsAndBitangents();

    //load all geometry data from Assimp aiMesh
    for (std::size_t i = 0; i < assimpMesh->mNumVertices; ++i) {
        positions[i] = ai2glm3D(assimpMesh->mVertices[i]);
        normals[i] = ai2glm3D(assimpMesh->mNormals[i]);
        if (hasTexCoors) {
            texCoords[i] = ai2glm2D(assimpMesh->mTextureCoords[0][i]);
        }
        if (hasTangentsBitangents) {
            tangents.push_back(ai2glm3D(assimpMesh->mTangents[i]));
            bitangents.push_back(ai2glm3D(assimpMesh->mBitangents[i]));
        }
    }
    for (std::size_t i = 0; i < assimpMesh->mNumFaces; i++) {
        for (std::size_t j = 0; j < 3; ++j) {
            indices[3 * i + j] = assimpMesh->mFaces[i].mIndices[j];
        }
    }

    //is there are materials in scene, assign some, else assign default material with Id 0
    uint32_t matId = 0;
    if (assimpScene->HasMaterials()) {
        matId = maxMatId + 1 + assimpMesh->mMaterialIndex;
    }
    if (!hasTangentsBitangents) {
        scene.push_back(std::make_shared<Mesh>(positions, normals, texCoords, indices, matId, model));
    } else {
        scene.push_back(std::make_shared<Mesh>(positions, normals, texCoords, indices, tangents, bitangents, matId, model));
    }
    scene[scene.size() - 1]->name = std::string(assimpMesh->mName.C_Str());
}

void fromAiNode(
    const aiNode* node,
    const aiScene* assimpScene,
    const glm::mat4& parentModel,
    std::vector<std::shared_ptr<Mesh>>& scene,
    const uint32_t maxMatId)
{
    //model is parent model multiplied with transform
    aiMatrix4x4 assimpTransform = node->mTransformation;
    glm::mat4 transform(
        assimpTransform.a1, assimpTransform.a2, assimpTransform.a3, assimpTransform.a4,
        assimpTransform.b1, assimpTransform.b2, assimpTransform.b3, assimpTransform.b4,
        assimpTransform.c1, assimpTransform.c2, assimpTransform.c3, assimpTransform.c4,
        assimpTransform.d1, assimpTransform.d2, assimpTransform.d3, assimpTransform.d4);
    glm::mat4 model = transform * parentModel;
    //process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* assimpMesh = assimpScene->mMeshes[node->mMeshes[i]];
        fromAiMesh(
            assimpMesh,
            assimpScene,
            model,
            scene,
            maxMatId);
    }
    //then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        fromAiNode(
            node->mChildren[i],
            assimpScene,
            model,
            scene,
            maxMatId);
    }
}

}

void importSceneFromFile(
    const std::string& path,
    std::vector<std::shared_ptr<Mesh>>& scene,
    std::unordered_map<uint32_t, Material>& materials,
    std::unordered_map<std::string, std::unique_ptr<Texture>>& textures)
{
    std::cout << "Loading scene from disk..." << std::endl;
    Assimp::Importer importer;
    const aiScene* assimpScene = importer.ReadFile(path,
        aiProcess_JoinIdenticalVertices | aiProcess_GenNormals | aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

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
    ///load materials
    for (uint32_t i = 0; i < assimpScene->mNumMaterials; ++i) {
        Material material;
        material.id = maxMatId + 1 + i;

        aiMaterial* assimpMaterial = assimpScene->mMaterials[i];

        aiColor3D color(0.f, 0.f, 0.f);
        aiString name;
        float shininess = 32.0f;
        int twosided = 0;
        float opacity = 1.0f;

        assimpMaterial->Get(AI_MATKEY_NAME, name);
        material.name = std::string(name.C_Str());

        assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        material.diffuse = glm::vec3(color.r, color.b, color.g);

        assimpMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color);
        material.ambient = glm::vec3(color.r, color.b, color.g);

        assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color);
        material.specular = glm::vec3(color.r, color.b, color.g);

        assimpMaterial->Get(AI_MATKEY_SHININESS, shininess);
        material.shininess = shininess;

        assimpMaterial->Get(AI_MATKEY_TWOSIDED, twosided);
        assimpMaterial->Get(AI_MATKEY_OPACITY, opacity);

        //load maps
        //diffuse
        if (assimpMaterial->GetTextureCount(aiTextureType_DIFFUSE)) {
            aiString str;
            assimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &str);
            std::string path(str.C_Str());
            path = addToDir(directory, path);
            if (textures.count(path) == 0) {
                textures[path] = std::make_unique<Texture>(path);
            }
            material.hasDiffuseMap = true;
            material.diffuseMap = textures[path]->GetTextureID();
            material.diffuseMapPath = path;
        }
        //specular
        if (assimpMaterial->GetTextureCount(aiTextureType_SPECULAR)) {
            aiString str;
            assimpMaterial->GetTexture(aiTextureType_SPECULAR, 0, &str);
            std::string path(str.C_Str());
            path = addToDir(directory, path);
            if (textures.count(path) == 0) {
                textures[path] = std::make_unique<Texture>(path);
            }
            material.hasSpecularMap = true;
            material.specularMap = textures[path]->GetTextureID();
            material.specularMapPath = path;
        }
        //normal
        if (assimpMaterial->GetTextureCount(aiTextureType_HEIGHT)) {
            aiString str;
            assimpMaterial->GetTexture(aiTextureType_HEIGHT, 0, &str);
            std::string path(str.C_Str());
            path = addToDir(directory, path);
            if (textures.count(path) == 0) {
                textures[path] = std::make_unique<Texture>(path);
            }
            material.hasNormalMap = true;
            material.normalMap = textures[path]->GetTextureID();
            material.normalMapPath = path;
        }
        if (assimpMaterial->GetTextureCount(aiTextureType_OPACITY)) {
            twosided = true;
        }
        material.twosided = twosided;
        material.opacity = opacity;
        materials[material.id] = material;
    }
    fromAiNode(
        assimpScene->mRootNode,
        assimpScene,
        glm::mat4(1.0f),
        scene,
        maxMatId);
}

//All dinamic meshes will be moved from old scene!
//For now assume all static meshes have tangents and bitangents
std::vector<std::shared_ptr<Mesh>> unifyStaticMeshes(
    std::vector<std::shared_ptr<Mesh>>& scene,
    const std::unordered_map<uint32_t, Material>& materials)
{
    std::unordered_map<uint32_t, std::vector<uint32_t>> materialToMeshIdx;
    for (const auto& mat : materials) {
        materialToMeshIdx[mat.first] = std::vector<uint32_t>();
    }

    //group meshes in scene by material
    std::unordered_set<std::size_t> skipped;
    for (std::size_t i = 0; i < scene.size(); ++i) {
        if (scene[i]->IsLoaded()) {
            throw std::runtime_error("Can't unify meshes if they are loaded to GPU");
        }
        if (materials.count(scene[i]->matId) == 0 || !scene[i]->isStatic) {
            skipped.insert(i);
            continue;
        }
        materialToMeshIdx[scene[i]->matId].push_back(i);
    }

    std::vector<std::shared_ptr<Mesh>> newScene;
    for (const auto& item : materialToMeshIdx) {
        if (item.second.size() == 0) {
            continue;
        }
        uint32_t numVertices = 0;
        uint32_t numFaces = 0;
        for (uint32_t i : item.second) {
            numVertices += scene[i]->numberOfVertices();
            numFaces += scene[i]->numberOfFaces();
        }
        std::vector<glm::vec3> positions(numVertices);
        std::vector<glm::vec3> normals(numVertices);
        std::vector<glm::vec2> texCoords(numVertices);
        std::vector<glm::vec3> tangents(numVertices);
        std::vector<glm::vec3> bitangents(numVertices);
        std::vector<uint32_t> indices(numFaces * 3);
        std::uint32_t vertexShift = 0;
        std::uint32_t faceShift = 0;
        for (uint32_t i : item.second) {
            for (std::size_t j = 0; j < scene[i]->numberOfVertices(); ++j) {
                positions[vertexShift + j] = scene[i]->model * glm::vec4(scene[i]->positions[j], 1.0f);
                normals[vertexShift + j] = scene[i]->model * glm::vec4(scene[i]->normals[j], 0.0f);
                texCoords[vertexShift + j] = scene[i]->texCoords[j];
                tangents[vertexShift + j] = scene[i]->tangents[j];
                bitangents[vertexShift + j] = scene[i]->bitangents[j];
            }
            for (std::size_t j = 0; j < scene[i]->numberOfFaces() * 3; ++j) {
                indices[faceShift + j] = scene[i]->indices[j] + vertexShift;
            }
            vertexShift += scene[i]->numberOfVertices();
            faceShift += scene[i]->numberOfFaces() * 3;
        }
        newScene.push_back(std::make_shared<Mesh>(
            positions, normals, texCoords, indices, tangents, bitangents, item.first, glm::mat4(1.0f)));
    }
    for (std::size_t i : skipped) {
        newScene.push_back(std::move(scene[i]));
    }
    return newScene;
}