#include "Models/Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::pointerType Texture::loadImg(const std::string& path)
{
    stbi_uc* img = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (img == nullptr) {
        throw std::runtime_error("Couldn't load image " + path);
    }
    return pointerType(
        img,
        stbi_image_free);
}

void Texture::GLBind() const
{
    if (textureID) {
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
}

void Texture::GLLoad()
{
    if (textureID) {
        Release();
    }
    glGenTextures(1, &textureID);
    GLBind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLint format;
    switch (nrChannels) {
    case 1:
        format = GL_RED;
        break;
    case 2:
        format = GL_RG;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_RGBA;
        break;
    default:
        format = 0;
        break;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data.get());
    glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture::Release()
{
    glDeleteTextures(1, &textureID);
    textureID = 0;
}