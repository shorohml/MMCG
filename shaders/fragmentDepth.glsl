#version 330 core
out vec4 fragColor;

void main()
{
    fragColor = vec4(gl_FragCoord.z, gl_FragCoord.z * gl_FragCoord.z, 0.0, 1.0);
}