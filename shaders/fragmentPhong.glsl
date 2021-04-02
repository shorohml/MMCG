#version 330 core
in vec3 normal;
in vec3 fragPos;
in vec2 texCoords;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position; //in View space

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant; //parameters for attenuation
    float linear;
    float quadratic;
};

struct SpotLight {
    PointLight pointLight;

    vec3 direction; // direction the spotlight is aiming at
    float cutOff; //cutoff angle specifing the spotlight's radius
    float outerCutOff; //cutoff angle specifing outer radius to smooth the spotlight
};

//light sources 
#define NR_POINT_LIGHTS 4
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform DirLight dirLight;
uniform SpotLight spotLight;
uniform bool spotlightOn;

//material
uniform Material material;

out vec4 FragColor;

vec3 calcDiffuse(
    vec3 lightDir, //direction from fragment to the light source (normalized)
    vec3 norm, //fragment normal
    vec3 materialDiffuse) //material.diffuse, but already sampled for optimization
{
    float diff = max(dot(norm, lightDir), 0.0);
    return diff * materialDiffuse;
}

vec3 calcSpecular(
    vec3 lightDir, //direction from fragment to the light source (normalized)
    vec3 norm, //fragment normal
    vec3 viewDir, //direction from fragment to viewer (normalized)
    vec3 materialSpecular, //material.specular, but already sampled for optimization
    float shininess) //shininess of material
{
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    return materialSpecular * spec;
}

vec3 calcDirLight(
    DirLight light, //light
    Material material, //material
    vec3 materialDiffuse, //material.diffuse, but already sampled for optimization
    vec3 materialSpecular, //material.specular, but already sampled for optimization
    vec3 norm, //normal
    vec3 viewDir) //direction from fragment to viewer (normalized)
{
    vec3 lightDir = normalize(-light.direction);

    //ambient
    vec3 ambient = light.ambient * materialDiffuse;

    //diffuse
    vec3 diffuse = light.diffuse * calcDiffuse(lightDir, norm, materialDiffuse);

    //specular
    vec3 specular = light.specular * calcSpecular(lightDir, norm, viewDir, materialSpecular, material.shininess);

    return ambient + diffuse + specular;
}

float calcAttenuation(
    PointLight light, //light
    vec3 fragPos) //fragment postion in View space
{
    float dist = length(light.position - fragPos);
    return 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
}

vec3 calcPointLight(
    PointLight light, //light
    Material material, //material
    vec3 materialDiffuse, //material.diffuse, but already sampled for optimization
    vec3 materialSpecular, //material.specular, but already sampled for optimization
    vec3 norm, //normal
    vec3 fragPos, //fragment postion in View space
    vec3 viewDir) //direction from fragment to viewer (normalized)
{
    vec3 lightDir = normalize(light.position - fragPos);

    //attenuation
    float attenuation = calcAttenuation(
        light,
        fragPos);

    //ambient
    vec3 ambient = light.ambient * materialDiffuse;

    //diffuse
    vec3 diffuse = light.diffuse * calcDiffuse(lightDir, norm, materialDiffuse);

    //specular
    vec3 specular = light.specular * calcSpecular(lightDir, norm, viewDir, materialSpecular, material.shininess);

    return (ambient + diffuse + specular) * attenuation;
}

vec3 calcSpotLight(
    SpotLight light, //light
    Material material, //material
    vec3 materialDiffuse, //material.diffuse, but already sampled for optimization
    vec3 materialSpecular, //material.specular, but already sampled for optimization
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
    vec3 ambient = light.pointLight.ambient * materialDiffuse;

    //diffuse
    vec3 diffuse = light.pointLight.diffuse * calcDiffuse(lightDir, norm, materialDiffuse);

    //specular
    vec3 specular = light.pointLight.specular * calcSpecular(lightDir, norm, viewDir, materialSpecular, material.shininess);

    diffuse *= intensity;
    specular *= intensity;

    return (ambient + diffuse + specular) * attenuation;
}

void main()
{
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(-fragPos);
    vec3 materialDiffuse = texture(material.diffuse, texCoords).rgb;
    vec3 materialSpecular = texture(material.specular, texCoords).rgb;

    //directional light
    vec3 color = calcDirLight(
        dirLight,
        material,
        materialDiffuse,
        materialSpecular,
        norm,
        viewDir);

    //point light
    for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
        color += calcPointLight(
            pointLights[i],
            material,
            materialDiffuse,
            materialSpecular,
            norm,
            fragPos,
            viewDir);
    }

    if (spotlightOn) {
        //spotlight
        color += calcSpotLight(
            spotLight,
            material,
            materialDiffuse,
            materialSpecular,
            norm,
            fragPos,
            viewDir);
    }

    FragColor = vec4(color, 1.0);
}