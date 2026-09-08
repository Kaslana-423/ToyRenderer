#version 330 core

in vec2 textureCoordinate;
out vec4 fragmentColor;

uniform sampler2D cubeTexture;

void main()
{
    fragmentColor = texture(cubeTexture, textureCoordinate);
}
