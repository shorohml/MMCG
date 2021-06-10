#include "Models/Material.h"

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
    program.SetUniform("material.opacity", opacity);
    GL_CHECK_ERRORS;
    program.SetUniform("material.twosided", twosided);
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