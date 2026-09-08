#version 330 core

in vec3 fragmentPosition;
in vec2 textureCoordinate;

uniform sampler2D diffuseTexture;
uniform bool hasDiffuseMap;
uniform vec3 lightPosition;
uniform float farPlane;

void main()
{
    if (hasDiffuseMap &&
        texture(diffuseTexture, textureCoordinate).a < 0.05)
    {
        discard;
    }

    float lightDistance = length(fragmentPosition - lightPosition);
    gl_FragDepth = lightDistance / farPlane;
}
