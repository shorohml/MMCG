#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

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
#define NR_POINT_LIGHTS 6
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform DirLight dirLight;
uniform SpotLight spotLight;
uniform bool spotlightOn;
uniform float farPlane;

uniform Material material;

uniform bool visualizeNormalsWithColor;

in VS_OUT
{
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace;
    vec4 fragPosWorldSpace;
    vec2 texCoords;
    mat3 TBN;
}
fsIn;

vec3 calcDiffuse(
    vec3 lightDir, //direction from fragment to the light source (normalized)
    vec3 norm, //fragment normal
    vec3 materialDiffuse) //diffuse color
{
    float diff = max(dot(norm, lightDir), 0.0);
    return diff * materialDiffuse;
}

vec3 calcSpecular(
    vec3 lightDir, //direction from fragment to the light source (normalized)
    vec3 norm, //fragment normal
    vec3 viewDir, //direction from fragment to viewer (normalized)
    vec3 materialSpecular, //specular color
    Material material) //shininess of material
{
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    return materialSpecular * spec;
}

float calcDirShadowPCF(DirLight light, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    //transform to [0, 1]
    vec3 projCoords = fragPosLightSpace.xyz * 0.5 + 0.5;
    vec2 texelSize = 1.0 / textureSize(light.shadowMap, 0);
    //bias to remove shadow achne
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.005);
    float currentDepth = projCoords.z;
    float shadow = 0.0;
    //simple PCF
    for (int i = -2; i < 3; ++i) {
        for (int j = -2; j < 3; ++j) {
            vec2 coord = vec2(projCoords.xy + vec2(i, j) * texelSize);
            float pcfDepth = texture(light.shadowMap, coord).r;
            shadow += currentDepth - bias <= pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 25.0;
}

float calcDirShadowVSM(DirLight light, vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    //transform to [0, 1]
    vec3 projCoords = fragPosLightSpace.xyz * 0.5 + 0.5;
    //compute moments
    vec2 moments = texture(light.shadowMap, vec2(projCoords.xy)).rg;
    float sigma2 = moments.g - moments.r * moments.r;
    //compute proba
    // float bias = max(0.01 * (1.0 - dot(normal, lightDir)), 0.01);
    float bias = 0.05;
    float diff = projCoords.z - bias - moments.r;
    float pmax = sigma2 / (sigma2 + diff * diff);
    return projCoords.z - bias <= moments.r ? 1.0 : pmax;
}

vec3 calcDirLight(
    DirLight light, //light
    Material material, //material
    vec3 diffuseMapVal, //sampled from diffuse map
    vec3 specularMapVal, //sampled from specular map
    vec3 norm, //normal
    vec3 viewDir, //direction from fragment to viewer (normalized)
    vec4 fragPosLightSpace) //fragment position in light space
{
    vec3 lightDir = normalize(-light.direction);

    //ambient
    vec3 ambient = light.ambient * material.ambient * diffuseMapVal;

    //diffuse
    vec3 diffuse = light.diffuse * calcDiffuse(lightDir, norm, material.diffuse * diffuseMapVal);

    //specular
    vec3 specular = light.specular * calcSpecular(lightDir, norm, viewDir, specularMapVal * material.specular, material);

    float shadow = calcDirShadowVSM(light, fragPosLightSpace, norm, lightDir);

    return ambient + shadow * (diffuse + specular);
}

float calcAttenuation(
    PointLight light, //light
    vec3 fragPos) //fragment postion in View space
{
    float dist = length(light.position - fragPos);
    return 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
}

float calcPointShadowPCF(PointLight light, vec3 fragPosWorldSpace, vec3 lightPosWorldSpace)
{
    vec3 sampleOffsetDirections[20] = vec3[]
    (
        vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1), 
        vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
        vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
        vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
        vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
    );
    float bias = 10.0;
    float shadow = 0.0;
    float radius = 0.5;
    //vector from fragment to light source
    vec3 fragToLight = fragPosWorldSpace - lightPosWorldSpace; 
    float currentDepth = length(fragToLight);
    for (int i = 0; i < 20; ++i) {
        //distance to closest fragment
        float closestDepth = texture(light.pointShadowMap, fragToLight + sampleOffsetDirections[i] * radius).r;
        //transrotm from [0; 1] to [0, farPlane]
        closestDepth *= farPlane;
        shadow += currentDepth - bias <= closestDepth ? 1.0 : 0.0; 
    }
    return shadow / 20.0;
}

vec3 calcPointLight(
    PointLight light, //light
    Material material, //material
    vec3 diffuseMapVal, //sampled from diffuse map
    vec3 specularMapVal, //sampled from specular map
    vec3 norm, //normal
    vec3 fragPos, //fragment postion in View space
    vec3 fragPosWorldSpace, //fragment postion in world space
    vec3 viewDir) //direction from fragment to viewer (normalized)
{
    vec3 lightDir = normalize(light.position - fragPos);

    //attenuation
    float attenuation = calcAttenuation(
        light,
        fragPos);

    //ambient
    vec3 ambient = light.ambient * material.ambient * diffuseMapVal;

    //diffuse
    vec3 diffuse = light.diffuse * calcDiffuse(lightDir, norm, material.diffuse * diffuseMapVal);

    //specular
    vec3 specular = light.specular * calcSpecular(lightDir, norm, viewDir, specularMapVal * material.specular, material);

    float shadow = calcPointShadowPCF(light, fragPosWorldSpace, light.positionWorldSpace);

    return (ambient + shadow*(diffuse + specular)) * attenuation;
}

vec3 calcSpotLight(
    SpotLight light, //light
    Material material, //material
    vec3 diffuseMapVal, //sampled from diffuse map
    vec3 specularMapVal, //sampled from specular map
    vec3 norm, //normal
    vec3 fragPos, //fragment postion in View space
    vec3 viewDir) //direction from fragment to viewer (normalized)
{
    vec3 lightDir = normalize(light.pointLight.position - fragPos);

    //calculate intensity:
    //0 if fragment is outside of spotlight
    //(0, 1) in outer 'ring' to smooth spotlight
    //1 inside of spotlight
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.outerCutOff - light.cutOff;
    float intensity = clamp((light.outerCutOff - theta) / epsilon, 0.0, 1.0);

    //attenuation
    float attenuation = calcAttenuation(
        light.pointLight,
        fragPos);

    //ambient
    vec3 ambient = light.pointLight.ambient * material.ambient * diffuseMapVal;

    //diffuse
    vec3 diffuse = light.pointLight.diffuse * calcDiffuse(lightDir, norm, material.diffuse * diffuseMapVal);

    //specular
    vec3 specular = light.pointLight.specular * calcSpecular(lightDir, norm, viewDir, specularMapVal * material.specular, material);

    diffuse *= intensity;
    specular *= intensity;

    return (ambient + diffuse + specular) * attenuation;
}

void main()
{
    vec3 normal;
    if (material.hasNormalMap) {
        normal = texture(material.normalMap, fsIn.texCoords).rgb;
        normal = normal * 2.0 - 1.0;
        normal = normalize(fsIn.TBN * normal);
    } else {
        normal = normalize(fsIn.normal);
    }

    if (visualizeNormalsWithColor) {
        FragColor = vec4(normal * 0.5 + 0.5, 1.0f);
        return;
    }

    vec3 viewDir = normalize(-fsIn.fragPos);
    vec3 diffuseMapVal = vec3(1.0);
    if (material.hasDiffuseMap) {
        vec4 color = texture(material.diffuseMap, fsIn.texCoords);
        if (material.twosided != 0 && material.opacity * color.a < 0.5) {
            discard;
        }
        diffuseMapVal = vec3(color);
    }
    vec3 specularMapVal = vec3(1.0);
    if (material.hasSpecularMap) {
        specularMapVal =  texture(material.specularMap, fsIn.texCoords).rgb;
    }

    //directional light
    vec3 color = vec3(0.0);
    color = calcDirLight(
        dirLight,
        material,
        diffuseMapVal,
        specularMapVal,
        normal,
        viewDir,
        fsIn.fragPosLightSpace);

    // point light
    for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
        color += calcPointLight(
            pointLights[i],
            material,
            diffuseMapVal,
            specularMapVal,
            normal,
            fsIn.fragPos,
            fsIn.fragPosWorldSpace.xyz,
            viewDir);
    }

    if (spotlightOn) {
        //spotlight
        color += calcSpotLight(
            spotLight,
            material,
            diffuseMapVal,
            specularMapVal,
            normal,
            fsIn.fragPos,
            viewDir);
    }

    FragColor = vec4(color, 1.0);
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 0.9) {
        BrightColor = vec4(FragColor.rgb, 1.0);
    }
    else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}