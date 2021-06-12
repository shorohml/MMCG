#version 330 core
in vec4 fragPos;

uniform vec3 lightPos;
uniform float farPlane;

void main()
{
    // get distance between fragment and light source
    float lightDistance = length(fragPos.xyz - lightPos);

    // map to [0; 1] range by dividing by far_plane
    lightDistance = clamp(lightDistance / farPlane, 0.0, 1.0);

    // write this as modified depth
    gl_FragDepth = lightDistance;
}