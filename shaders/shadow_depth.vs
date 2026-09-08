#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 2) in vec2 aTextureCoordinate;

out vec3 fragmentPosition;
out vec2 textureCoordinate;

uniform mat4 model;
uniform mat4 shadowMatrix;

void main()
{
    vec4 worldPosition = model * vec4(aPosition, 1.0);
    fragmentPosition = worldPosition.xyz;
    textureCoordinate = aTextureCoordinate;
    gl_Position = shadowMatrix * worldPosition;
}
