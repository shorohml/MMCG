//Texture class (loads image from disk and creates OpenGl texture)
#pragma once

#include "common.h"
#include <memory>
#include <stb_image.h>
#include <string>

class Texture {
private:
    using pointerType = std::shared_ptr<stbi_uc>;

    pointerType data;
    int width, height, nrChannels;
    GLuint textureID;

    pointerType loadImg(const std::string& path);

public:
    Texture(const std::string& path)
        : data(loadImg(path))
        , textureID(0)
    {
    }

    Texture(const Texture&) = delete;

    Texture& operator=(const Texture& other) = delete;

    void GLBind() const;

    void GLLoad();

    void Release();

    GLuint GetTextureID() const
    {
        return textureID;
    }
};
