#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTextureCoordinate;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec3 viewPosition;
out vec3 viewNormal;
out vec3 viewTangent;
out vec3 viewBitangent;
out vec2 textureCoordinate;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    mat4 modelView = view * model;
    vec4 position = modelView * vec4(aPosition, 1.0);
    mat3 normalMatrix = mat3(transpose(inverse(modelView)));

    viewPosition = position.xyz;
    viewNormal = normalMatrix * aNormal;
    viewTangent = normalMatrix * aTangent;
    viewBitangent = normalMatrix * aBitangent;
    textureCoordinate = aTextureCoordinate;
    gl_Position = projection * position;
}
