#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTextureCoordinate;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec3 fragmentPosition;
out vec3 normal;
out vec3 tangent;
out vec3 bitangent;
out vec2 textureCoordinate;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPosition = model * vec4(aPosition, 1.0);
    fragmentPosition = worldPosition.xyz;
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    normal = normalMatrix * aNormal;
    tangent = normalMatrix * aTangent;
    bitangent = normalMatrix * aBitangent;
    textureCoordinate = aTextureCoordinate;

    gl_Position = projection * view * worldPosition;
}
