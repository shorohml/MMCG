#version 330 core
out vec4 FragColor;

in VS_OUT
{
    vec2 texCoords;
}
fsIn;
out vec4 fragColor;

uniform sampler2D colorBuffer;

void main()
{
    fragColor = texture(colorBuffer, fsIn.texCoords);
}