#version 330 core
in vec3 normal;
in vec3 fragPos;
in vec2 texCoords;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;
    float shininess;
};
struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Light light;
uniform Material material;

out vec4 FragColor;

void main()
{
    //ambient
    vec3 ambient = texture(material.diffuse, texCoords).rbg * light.ambient;

    //diffuse
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * texture(material.diffuse, texCoords).rgb * light.diffuse;

    //specular
    vec3 viewDir = normalize(vec3(0.) - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = spec * texture(material.specular, texCoords).rgb * light.specular;

    //emission
    vec3 emission = texture(material.emission, texCoords).rgb;

    FragColor = vec4(ambient + diffuse + specular + emission, 1.0);
}