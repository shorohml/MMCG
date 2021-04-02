#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in mat4 model;

uniform mat4 view;
uniform mat4 projection;

out vec3 normal;
out vec3 fragPos;
out vec2 texCoords;

void main()
{
    mat4 MV = view * model;
    normal = mat3(transpose(inverse(MV))) * aNormal;
    texCoords = aTexCoords;
    vec4 frag = MV * vec4(aPos, 1.0);
    gl_Position = projection * frag;
    fragPos = vec3(frag);
}