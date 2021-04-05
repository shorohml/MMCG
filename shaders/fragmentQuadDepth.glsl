#version 330 core
out vec4 FragColor;

in VS_OUT
{
    vec2 texCoords;
}
fsIn;

uniform sampler2D depthMap;

void main()
{
    float depth = texture(depthMap, fsIn.texCoords).r;
    FragColor = vec4(vec3(depth), 1.0);
}