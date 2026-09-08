#version 330 core

in vec2 textureCoordinate;
out float ambientOcclusion;

uniform sampler2D geometryPosition;
uniform sampler2D geometryNormal;
uniform sampler2D noiseTexture;
uniform vec3 samples[64];
uniform mat4 projection;
uniform vec2 noiseScale;
uniform int kernelSize;
uniform float radius;
uniform float bias;
uniform float power;

void main()
{
    vec3 fragmentPosition =
        texture(geometryPosition, textureCoordinate).xyz;
    vec3 normalSample = texture(
        geometryNormal, textureCoordinate).xyz;
    if (length(normalSample) < 0.1)
    {
        ambientOcclusion = 1.0;
        return;
    }
    vec3 unitNormal = normalize(normalSample);
    vec3 randomVector = normalize(
        texture(noiseTexture, textureCoordinate * noiseScale).xyz);

    vec3 tangent = normalize(
        randomVector - unitNormal * dot(randomVector, unitNormal));
    vec3 bitangent = cross(unitNormal, tangent);
    mat3 tangentToView = mat3(tangent, bitangent, unitNormal);

    float occlusion = 0.0;
    for (int sampleIndex = 0; sampleIndex < kernelSize; ++sampleIndex)
    {
        vec3 samplePosition = fragmentPosition +
            tangentToView * samples[sampleIndex] * radius;

        vec4 offset = projection * vec4(samplePosition, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;
        if (offset.x < 0.0 || offset.x > 1.0 ||
            offset.y < 0.0 || offset.y > 1.0)
        {
            continue;
        }

        float sampleDepth = texture(
            geometryPosition, offset.xy).z;
        if (sampleDepth >= -0.0001)
        {
            continue;
        }
        float depthDifference = abs(fragmentPosition.z - sampleDepth);
        float rangeWeight = smoothstep(
            0.0, 1.0, radius / max(depthDifference, 0.0001));
        occlusion += sampleDepth >= samplePosition.z + bias
            ? rangeWeight
            : 0.0;
    }

    float visibility = 1.0 - occlusion / float(kernelSize);
    ambientOcclusion = pow(clamp(visibility, 0.0, 1.0), power);
}
