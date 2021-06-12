#version 330 core
out vec4 FragColor;

in VS_OUT
{
    vec2 texCoords;
}
fsIn;

uniform sampler2D shadowMap;
uniform bool direction;
uniform bool gaussFilter;

void main()
{
    float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);
    float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);

    vec2 depth = texture(shadowMap, fsIn.texCoords).rg;
    if (gaussFilter) {
        depth *= weight[0];
        vec2 size = textureSize(shadowMap, 0);
        for (int i = 1; i < 3; ++i) {
            vec2 coord1 = fsIn.texCoords;
            vec2 coord2 = fsIn.texCoords;
            if (direction) {
                coord1 += vec2(0.0, offset[i]) / size.y;
                coord2 -= vec2(0.0, offset[i]) / size.y;
            } else {
                coord1 += vec2(offset[i], 0.0) / size.x;
                coord2 -= vec2(offset[i], 0.0) / size.x;
            }
            depth += texture(shadowMap, coord1).rg * weight[i];
            depth += texture(shadowMap, coord2).rg * weight[i]; 
        }
    }
    FragColor = vec4(depth.rg, 0.0, 1.0);
}