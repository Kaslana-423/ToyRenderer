#version 330 core

struct Light
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material
{
    vec3 diffuseColor;
    vec3 specularColor;
    vec3 emissiveColor;
    float shininess;
    float metallic;
    float roughness;
    float emissiveStrength;
    bool usePbr;
};

in vec3 fragmentPosition;
in vec3 normal;
in vec3 tangent;
in vec3 bitangent;
in vec2 textureCoordinate;

layout (location = 0) out vec4 fragmentColor;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_emissive1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_metallicRoughness1;
uniform sampler2D texture_ao1;
uniform bool hasDiffuseMap;
uniform bool hasSpecularMap;
uniform bool hasEmissiveMap;
uniform bool hasNormalMap;
uniform bool hasMetallicRoughnessMap;
uniform bool hasAmbientOcclusionMap;
uniform float emissiveMapIntensity;
uniform samplerCubeShadow shadowMap;
uniform bool shadowsEnabled;
uniform float shadowFarPlane;
uniform float shadowBiasMin;
uniform float shadowBiasSlope;
uniform float shadowPcfRadius;
uniform int shadowPcfSamples;
uniform sampler2D ssaoTexture;
uniform bool ssaoEnabled;
uniform float ssaoStrength;
uniform vec2 viewportSize;
uniform Material material;
uniform Light light;
uniform vec3 viewPosition;

const float PI = 3.1415926;
const float GOLDEN_ANGLE = 2.39996323;
const int MAX_SHADOW_PCF_SAMPLES = 48;

float shadowRotation(vec3 worldPosition)
{
    float noise = sin(dot(
        worldPosition,
        vec3(12.9898, 78.233, 37.719)));
    return fract(noise * 43758.5453) * 2.0 * PI;
}

float calculatePointShadow(vec3 unitNormal, vec3 lightDirection)
{
    if (!shadowsEnabled)
    {
        return 0.0;
    }

    vec3 fragmentToLight = fragmentPosition - light.position;
    float currentDepth = length(fragmentToLight);
    if (currentDepth >= shadowFarPlane)
    {
        return 0.0;
    }

    float bias = max(
        shadowBiasSlope * (1.0 - max(dot(unitNormal, lightDirection), 0.0)),
        shadowBiasMin);
    float viewDistance = length(viewPosition - fragmentPosition);
    float diskRadius = shadowPcfRadius *
        (1.0 + viewDistance / shadowFarPlane);

    vec3 radialDirection = fragmentToLight / currentDepth;
    vec3 helperAxis = abs(radialDirection.y) < 0.99
        ? vec3(0.0, 1.0, 0.0)
        : vec3(1.0, 0.0, 0.0);
    vec3 diskTangent = normalize(cross(helperAxis, radialDirection));
    vec3 diskBitangent = cross(radialDirection, diskTangent);
    float angularRadius = min(
        diskRadius / max(currentDepth, 0.001), 0.35);
    float referenceDepth = clamp(
        (currentDepth - bias) / shadowFarPlane, 0.0, 1.0);
    float rotation = shadowRotation(fragmentPosition);
    int sampleCount = clamp(
        shadowPcfSamples, 1, MAX_SHADOW_PCF_SAMPLES);

    float visibility = 0.0;
    for (int sampleIndex = 0;
         sampleIndex < MAX_SHADOW_PCF_SAMPLES;
         ++sampleIndex)
    {
        if (sampleIndex >= sampleCount)
        {
            break;
        }

        float sampleRadius = sqrt(
            (float(sampleIndex) + 0.5) / float(sampleCount));
        float sampleAngle =
            float(sampleIndex) * GOLDEN_ANGLE + rotation;
        vec2 diskOffset = sampleRadius *
            vec2(cos(sampleAngle), sin(sampleAngle));
        vec3 sampleDirection = normalize(
            radialDirection +
            (diskTangent * diskOffset.x +
             diskBitangent * diskOffset.y) * angularRadius);
        visibility += texture(
            shadowMap, vec4(sampleDirection, referenceDepth));
    }
    return 1.0 - visibility / float(sampleCount);
}

float distributionGGX(vec3 unitNormal, vec3 halfwayDirection, float roughness)
{
    float roughnessSquared = roughness * roughness;
    float alphaSquared = roughnessSquared * roughnessSquared;
    float normalDotHalfway = max(dot(unitNormal, halfwayDirection), 0.0);
    float denominator = normalDotHalfway * normalDotHalfway *
        (alphaSquared - 1.0) + 1.0;
    return alphaSquared / max(PI * denominator * denominator, 0.0001);
}

float geometrySchlickGGX(float normalDotDirection, float roughness)
{
    float radius = roughness + 1.0;
    float k = radius * radius / 8.0;
    return normalDotDirection /
        max(normalDotDirection * (1.0 - k) + k, 0.0001);
}

float geometrySmith(
    vec3 unitNormal,
    vec3 viewDirection,
    vec3 lightDirection,
    float roughness)
{
    return geometrySchlickGGX(
               max(dot(unitNormal, viewDirection), 0.0), roughness) *
        geometrySchlickGGX(
               max(dot(unitNormal, lightDirection), 0.0), roughness);
}

vec3 fresnelSchlick(float cosine, vec3 reflectance)
{
    return reflectance + (1.0 - reflectance) * pow(1.0 - cosine, 5.0);
}

void main()
{
    vec4 diffuseSample = hasDiffuseMap
        ? texture(texture_diffuse1, textureCoordinate)
        : vec4(1.0);
    float alpha = diffuseSample.a;
    if (alpha < 0.05)
    {
        discard;
    }

    // map_Kd modulates Kd. Without map_Kd, the MTL Kd color is used directly.
    vec3 surfaceColor = diffuseSample.rgb * material.diffuseColor;
    vec3 unitNormal = normalize(normal);
    if (material.usePbr && hasNormalMap)
    {
        vec3 tangentNormal =
            texture(texture_normal1, textureCoordinate).rgb * 2.0 - 1.0;
        mat3 tangentToWorld = mat3(
            normalize(tangent), normalize(bitangent), unitNormal);
        unitNormal = normalize(tangentToWorld * tangentNormal);
    }
    vec3 lightDirection = normalize(light.position - fragmentPosition);
    vec3 viewDirection = normalize(viewPosition - fragmentPosition);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    float shadow = calculatePointShadow(unitNormal, lightDirection);
    float screenSpaceOcclusion = 1.0;
    if (ssaoEnabled)
    {
        vec2 screenCoordinate = gl_FragCoord.xy / viewportSize;
        float sampledOcclusion = texture(
            ssaoTexture, screenCoordinate).r;
        screenSpaceOcclusion = pow(
            max(sampledOcclusion, 0.001), ssaoStrength);
    }
    vec3 litColor;

    if (material.usePbr)
    {
        float metallic = material.metallic;
        float roughness = material.roughness;
        if (hasMetallicRoughnessMap)
        {
            vec3 packedMaterial = texture(
                texture_metallicRoughness1, textureCoordinate).rgb;
            roughness *= packedMaterial.g;
            metallic *= packedMaterial.b;
        }
        roughness = clamp(roughness, 0.04, 1.0);
        metallic = clamp(metallic, 0.0, 1.0);

        float ambientOcclusion = hasAmbientOcclusionMap
            ? texture(texture_ao1, textureCoordinate).r
            : 1.0;
        ambientOcclusion *= screenSpaceOcclusion;
        vec3 baseReflectance = mix(vec3(0.04), surfaceColor, metallic);
        vec3 fresnel = fresnelSchlick(
            max(dot(halfwayDirection, viewDirection), 0.0),
            baseReflectance);
        float distribution =
            distributionGGX(unitNormal, halfwayDirection, roughness);
        float geometry = geometrySmith(
            unitNormal, viewDirection, lightDirection, roughness);
        vec3 specular = distribution * geometry * fresnel /
            max(
                4.0 * max(dot(unitNormal, viewDirection), 0.0) *
                    max(dot(unitNormal, lightDirection), 0.0),
                0.0001);
        vec3 diffuseWeight = (vec3(1.0) - fresnel) * (1.0 - metallic);
        float normalDotLight = max(dot(unitNormal, lightDirection), 0.0);
        vec3 direct =
            (diffuseWeight * surfaceColor / PI + specular) *
            light.diffuse * normalDotLight;
        vec3 ambient = light.ambient * surfaceColor * ambientOcclusion;
        litColor = ambient + (1.0 - shadow) * direct;
    }
    else
    {
        vec3 surfaceSpecular = hasSpecularMap
            ? texture(texture_specular1, textureCoordinate).rgb *
                  material.specularColor
            : material.specularColor;
        vec3 ambient =
            light.ambient * surfaceColor * screenSpaceOcclusion;
        float diffuseStrength = max(dot(unitNormal, lightDirection), 0.0);
        vec3 diffuse = light.diffuse * diffuseStrength * surfaceColor;
        float specularStrength = pow(
            max(dot(unitNormal, halfwayDirection), 0.0),
            material.shininess);
        vec3 specular =
            light.specular * specularStrength * surfaceSpecular;
        litColor = ambient + (1.0 - shadow) * (diffuse + specular);
    }

    // Some Blender OBJ exports provide map_Ke without a non-zero Ke value,
    // so add the emissive texture instead of multiplying it by Ke.
    vec3 emission;
    if (material.usePbr)
    {
        vec3 emissiveSample = hasEmissiveMap
            ? texture(texture_emissive1, textureCoordinate).rgb
            : vec3(1.0);
        emission = emissiveSample * material.emissiveColor *
            material.emissiveStrength;
    }
    else
    {
        emission = material.emissiveColor;
        if (hasEmissiveMap)
        {
            emission += texture(texture_emissive1, textureCoordinate).rgb *
                emissiveMapIntensity;
        }
    }

    vec3 sceneColor = litColor + emission;
    fragmentColor = vec4(sceneColor, alpha);
}
