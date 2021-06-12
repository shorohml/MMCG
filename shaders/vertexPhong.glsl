#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    int twosided;
    float opacity;

    bool hasDiffuseMap;
    sampler2D diffuseMap;
    bool hasSpecularMap;
    sampler2D specularMap;
    bool hasNormalMap;
    sampler2D normalMap;
};

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform mat4 lightSpaceMatrix;
uniform Material material;

out VS_OUT {
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace;
    vec4 fragPosWorldSpace;
    vec2 texCoords;
    mat3 TBN;
} vsOut;

void main()
{
    mat4 MV = view * model;
    vec4 frag = MV * vec4(aPos, 1.0);
    gl_Position = projection * frag;
 
    //calculate TBN
    mat3 TBN = mat3(1.0f);
    if (material.hasNormalMap) {
        vec3 T = normalMatrix * normalize(aTangent);
        vec3 B = normalMatrix * normalize(aBitangent);
        vec3 N = normalMatrix * normalize(aNormal);
        TBN = mat3(T, B, N);
    }

    vsOut.normal = normalMatrix * aNormal;
    vsOut.fragPos = vec3(frag);
    vsOut.fragPosLightSpace = lightSpaceMatrix * model * vec4(aPos, 1.0);
    vsOut.fragPosWorldSpace = model * vec4(aPos, 1.0);
    vsOut.texCoords = aTexCoords;
    vsOut.TBN = TBN;
}