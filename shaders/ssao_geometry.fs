#version 330 core

struct Material
{
    bool usePbr;
};

in vec3 viewPosition;
in vec3 viewNormal;
in vec3 viewTangent;
in vec3 viewBitangent;
in vec2 textureCoordinate;

layout (location = 0) out vec4 geometryPosition;
layout (location = 1) out vec4 geometryNormal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform bool hasDiffuseMap;
uniform bool hasNormalMap;
uniform Material material;

void main()
{
    if (hasDiffuseMap &&
        texture(texture_diffuse1, textureCoordinate).a < 0.05)
    {
        discard;
    }

    vec3 unitNormal = normalize(viewNormal);
    if (material.usePbr && hasNormalMap)
    {
        vec3 tangentNormal =
            texture(texture_normal1, textureCoordinate).rgb * 2.0 - 1.0;
        mat3 tangentToView = mat3(
            normalize(viewTangent),
            normalize(viewBitangent),
            unitNormal);
        unitNormal = normalize(tangentToView * tangentNormal);
    }

    geometryPosition = vec4(viewPosition, 1.0);
    geometryNormal = vec4(unitNormal, 1.0);
}
