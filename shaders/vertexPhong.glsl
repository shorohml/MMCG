#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

//we'll have to transform light sources to tangent space for normal mapping
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

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    sampler2D shadowMap;
};

struct PointLight {
    vec3 position; //in View space
    vec3 positionWorldSpace;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant; //parameters for attenuation
    float linear;
    float quadratic;

    samplerCube pointShadowMap;
};

struct SpotLight {
    PointLight pointLight;

    vec3 direction; // direction the spotlight is aiming at
    float cutOff; //cutoff angle specifing the spotlight's radius
    float outerCutOff; //cutoff angle specifing outer radius to smooth the spotlight
};

//light sources
#define NR_POINT_LIGHTS 2
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform DirLight dirLight;
uniform SpotLight spotLight;

uniform Material material;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform mat4 lightSpaceMatrix;

out VS_OUT {
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace;
    vec4 fragPosWorldSpace;
    vec2 texCoords;
    vec3 dirLightDirection;
    vec3 pointLightPositions[NR_POINT_LIGHTS];
    vec3 spotlightPosition;
    vec3 spotlightDirection;
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
        vec3 N = normalMatrix * normalize(aNormal);
        vec3 B = normalMatrix * normalize(aBitangent);
        TBN = transpose(mat3(T, B, N));
    }

    vsOut.normal = normalMatrix * aNormal;
    vsOut.texCoords = aTexCoords;
    vsOut.fragPosLightSpace = lightSpaceMatrix * model * vec4(aPos, 1.0);
    vsOut.fragPosWorldSpace = model * vec4(aPos, 1.0);

    //transform all this stuff to tangent space
    vsOut.fragPos = TBN * vec3(frag);
    vsOut.dirLightDirection = TBN * dirLight.direction;
    for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
        vsOut.pointLightPositions[i] = TBN * pointLights[i].position;
    }
    vsOut.spotlightDirection = TBN * spotLight.direction;
    vsOut.spotlightPosition = TBN * spotLight.pointLight.position;
    vsOut.dirLightDirection = TBN * dirLight.direction;
}