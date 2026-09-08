#version 330 core

in vec2 textureCoordinate;
out vec4 fragmentColor;

uniform sampler2D hdrBuffer;
uniform float exposure;

void main()
{
    vec3 hdrColor = texture(hdrBuffer, textureCoordinate).rgb;

    // Exponential tone mapping preserves bright emissive detail without
    // clipping the floating-point scene directly to white.
    vec3 mappedColor = vec3(1.0) - exp(-hdrColor * exposure);
    mappedColor = pow(mappedColor, vec3(1.0 / 2.2));

    fragmentColor = vec4(mappedColor, 1.0);
}
