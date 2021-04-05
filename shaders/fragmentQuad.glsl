#version 330 core
out vec4 FragColor;

in VS_OUT
{
    vec2 texCoords;
}
fsIn;

uniform sampler2D colorBuffer;
uniform bool edgeDetection;

const float offset = 1.0 / 500.0;

void main()
{
    if (!edgeDetection) {
        FragColor = texture(colorBuffer, fsIn.texCoords);
        return;
    }

    vec2 offsets[9] = vec2[](
        vec2(-offset, offset), // top-left
        vec2(0.0f, offset), // top-center
        vec2(offset, offset), // top-right
        vec2(-offset, 0.0f), // center-left
        vec2(0.0f, 0.0f), // center-center
        vec2(offset, 0.0f), // center-right
        vec2(-offset, -offset), // bottom-left
        vec2(0.0f, -offset), // bottom-center
        vec2(offset, -offset) // bottom-right
    );

    float kernel[9] = float[](
        1, 1, 1,
        1, -8, 1,
        1, 1, 1);

    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(colorBuffer, fsIn.texCoords + offsets[i]));
        float grey = 0.2126 * sampleTex[i].r + 0.7152 * sampleTex[i].g + 0.0072 * sampleTex[i].b;
        sampleTex[i] = vec3(grey);
    }
    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++) {
        col += sampleTex[i] * kernel[i];
    }

    FragColor = vec4(col, 1.0);
}