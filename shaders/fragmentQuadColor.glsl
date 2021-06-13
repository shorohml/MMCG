#version 330 core
out vec4 FragColor;

in VS_OUT
{
    vec2 texCoords;
}
fsIn;

uniform sampler2D colorBuffer;
uniform sampler2D bloomBuffer;
uniform bool direction;
uniform bool gaussFilter;
uniform bool addBloom;
uniform float exposure = 1.5;
uniform float gamma = 0.9;

void main()
{
    float offset[4] = float[](0.0, 1.411764705882353, 3.2941176470588234, 5.176470588235294);
    float weight[4] = float[](0.1964825501511404, 0.2969069646728344, 0.09447039785044732, 0.010381362401148057);

    vec3 color = texture(colorBuffer, fsIn.texCoords).rgb;
    if (gaussFilter) {
        color *= weight[0];
        vec2 size = textureSize(colorBuffer, 0);
        for (int i = 1; i < 4; ++i) {
            vec2 coord1 = fsIn.texCoords;
            vec2 coord2 = fsIn.texCoords;
            if (direction) {
                coord1 += vec2(0.0, offset[i]) / size.y;
                coord2 -= vec2(0.0, offset[i]) / size.y;
            } else {
                coord1 += vec2(offset[i], 0.0) / size.x;
                coord2 -= vec2(offset[i], 0.0) / size.x;
            }
            color += texture(colorBuffer, coord1).rgb * weight[i];
            color += texture(colorBuffer, coord2).rgb * weight[i]; 
        }
    }
    if (addBloom) {
        vec3 bloomColor = texture(bloomBuffer, fsIn.texCoords).rgb;
        color += bloomColor;
        // tone mapping
        color = vec3(1.0) - exp(-color * exposure);
        // gamma correction 
        color = pow(color, vec3(1.0 / gamma));
    }
    FragColor = vec4(color, 1.0);
}