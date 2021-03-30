#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 MV;
uniform mat4 MVP;
uniform mat3 normalMatrix;

out vec3 normal;
out vec3 fragPos;
out vec2 texCoords;

void main()
{
    gl_Position = MVP * vec4(aPos, 1.0);
    fragPos = vec3(MV * vec4(aPos, 1.0));
    normal = normalMatrix * aNormal;
    texCoords = aTexCoords;
}