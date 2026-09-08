#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <random>
#include <stdexcept>
#include <string>

Renderer::Renderer()
    : lightingShader("shaders/lighting.vs", "shaders/lighting.fs"),
      shadowDepthShader(
          "shaders/shadow_depth.vs", "shaders/shadow_depth.fs"),
      geometryShader(
          "shaders/ssao_geometry.vs", "shaders/ssao_geometry.fs"),
      ssaoShader("shaders/hdr.vs", "shaders/ssao.fs"),
      ssaoBlurShader("shaders/hdr.vs", "shaders/ssao_blur.fs"),
      hdrShader("shaders/hdr.vs", "shaders/hdr.fs")
{
    SetupScreenQuad();
    SetupShadowMap();
    SetupSsaoResources();
    SetupMsaaResources();

    lightingShader.use();
    lightingShader.setInt("shadowMap", kShadowTextureUnit);
    lightingShader.setInt("ssaoTexture", kSsaoTextureUnit);

    ssaoShader.use();
    ssaoShader.setInt("geometryPosition", 0);
    ssaoShader.setInt("geometryNormal", 1);
    ssaoShader.setInt("noiseTexture", 2);

    ssaoBlurShader.use();
    ssaoBlurShader.setInt("ssaoInput", 0);

    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);
}

void Renderer::Render(
    const Scene& scene,
    const Camera& camera,
    int framebufferWidth,
    int framebufferHeight)
{
    if (framebufferWidth <= 0 || framebufferHeight <= 0)
    {
        return;
    }

    ResizeBuffers(framebufferWidth, framebufferHeight);

    if (settings.shadowsEnabled)
    {
        ResizeShadowMap(settings.shadowMapResolution);
        ShadowPass(scene);
    }
    if (settings.ssaoEnabled)
    {
        GeometryPass(
            scene, camera, framebufferWidth, framebufferHeight);
        SsaoPass(camera, framebufferWidth, framebufferHeight);
        SsaoBlurPass(framebufferWidth, framebufferHeight);
    }
    if (settings.msaaEnabled)
    {
        ResizeMsaaBuffers(
            framebufferWidth,
            framebufferHeight,
            settings.msaaSamples);
    }
    ForwardPass(scene, camera, framebufferWidth, framebufferHeight);
    FinalPass(framebufferWidth, framebufferHeight);
}

void Renderer::SetupShadowMap()
{
    glGenFramebuffers(1, &shadowFramebuffer);
    glGenTextures(1, &shadowDepthCubeMap);

    glBindTexture(GL_TEXTURE_CUBE_MAP, shadowDepthCubeMap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_COMPARE_MODE,
        GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_COMPARE_FUNC,
        GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    ResizeShadowMap(settings.shadowMapResolution);
}

void Renderer::SetupSsaoResources()
{
    glGenFramebuffers(1, &geometryFramebuffer);
    glGenTextures(1, &geometryPositionTexture);
    glGenTextures(1, &geometryNormalTexture);
    glGenRenderbuffers(1, &geometryDepthRenderbuffer);
    glGenFramebuffers(1, &ssaoFramebuffer);
    glGenTextures(1, &ssaoTexture);
    glGenFramebuffers(1, &ssaoBlurFramebuffer);
    glGenTextures(1, &ssaoBlurTexture);
    glGenTextures(1, &ssaoNoiseTexture);

    std::mt19937 randomGenerator(1337U);
    std::uniform_real_distribution<float> randomFloat(0.0F, 1.0F);

    ssaoShader.use();
    for (int sampleIndex = 0; sampleIndex < 64; ++sampleIndex)
    {
        glm::vec3 sample(
            randomFloat(randomGenerator) * 2.0F - 1.0F,
            randomFloat(randomGenerator) * 2.0F - 1.0F,
            randomFloat(randomGenerator));
        sample = glm::normalize(sample);
        sample *= randomFloat(randomGenerator);

        float scale = static_cast<float>(sampleIndex) / 64.0F;
        scale = 0.1F + 0.9F * scale * scale;
        sample *= scale;
        ssaoShader.setVec3(
            "samples[" + std::to_string(sampleIndex) + "]", sample);
    }

    std::array<glm::vec3, 16> noiseVectors{};
    for (glm::vec3& noise : noiseVectors)
    {
        noise = glm::vec3(
            randomFloat(randomGenerator) * 2.0F - 1.0F,
            randomFloat(randomGenerator) * 2.0F - 1.0F,
            0.0F);
    }

    glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB16F,
        4,
        4,
        0,
        GL_RGB,
        GL_FLOAT,
        noiseVectors.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void Renderer::SetupMsaaResources()
{
    glGenFramebuffers(1, &msaaFramebuffer);
    glGenTextures(1, &msaaColorTexture);
    glGenRenderbuffers(1, &msaaDepthRenderbuffer);
}

void Renderer::ResizeMsaaBuffers(int width, int height, int samples)
{
    GLint maximumSamples = 1;
    glGetIntegerv(GL_MAX_SAMPLES, &maximumSamples);
    samples = std::clamp(samples, 1, maximumSamples);
    settings.msaaSamples = samples;

    if (width == msaaBufferWidth &&
        height == msaaBufferHeight &&
        samples == allocatedMsaaSamples)
    {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, msaaFramebuffer);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaaColorTexture);
    glTexImage2DMultisample(
        GL_TEXTURE_2D_MULTISAMPLE,
        samples,
        GL_RGBA16F,
        width,
        height,
        GL_TRUE);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D_MULTISAMPLE,
        msaaColorTexture,
        0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glBindRenderbuffer(GL_RENDERBUFFER, msaaDepthRenderbuffer);
    glRenderbufferStorageMultisample(
        GL_RENDERBUFFER,
        samples,
        GL_DEPTH_COMPONENT24,
        width,
        height);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER,
        msaaDepthRenderbuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        throw std::runtime_error("HDR MSAA framebuffer is incomplete.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    allocatedMsaaSamples = samples;
    msaaBufferWidth = width;
    msaaBufferHeight = height;
}

void Renderer::ResizeShadowMap(int resolution)
{
    resolution = std::clamp(resolution, 256, 4096);
    settings.shadowMapResolution = resolution;
    if (resolution == allocatedShadowMapResolution)
    {
        return;
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, shadowDepthCubeMap);
    for (unsigned int face = 0; face < 6; ++face)
    {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            0,
            GL_DEPTH_COMPONENT24,
            resolution,
            resolution,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        shadowDepthCubeMap,
        0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        throw std::runtime_error("Point-shadow framebuffer is incomplete.");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    allocatedShadowMapResolution = resolution;
}

void Renderer::SetupScreenQuad()
{
    constexpr float screenVertices[] = {
        // position    // texture coordinate
        -1.0F, -1.0F, 0.0F, 0.0F,
         1.0F, -1.0F, 1.0F, 0.0F,
         1.0F,  1.0F, 1.0F, 1.0F,
        -1.0F, -1.0F, 0.0F, 0.0F,
         1.0F,  1.0F, 1.0F, 1.0F,
        -1.0F,  1.0F, 0.0F, 1.0F,
    };

    glGenVertexArrays(1, &screenVertexArray);
    glGenBuffers(1, &screenVertexBuffer);
    glBindVertexArray(screenVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, screenVertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);
}

void Renderer::ResizeBuffers(int width, int height)
{
    if (width == bufferWidth && height == bufferHeight)
    {
        return;
    }

    if (hdrFramebuffer == 0)
    {
        glGenFramebuffers(1, &hdrFramebuffer);
        glGenTextures(1, &hdrColorTexture);
        glGenRenderbuffers(1, &hdrDepthRenderbuffer);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFramebuffer);
    glBindTexture(GL_TEXTURE_2D, hdrColorTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA16F,
        width,
        height,
        0,
        GL_RGBA,
        GL_FLOAT,
        nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        hdrColorTexture,
        0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER,
        hdrDepthRenderbuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        throw std::runtime_error("HDR framebuffer is incomplete.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, geometryFramebuffer);
    const GLuint geometryTextures[] = {
        geometryPositionTexture,
        geometryNormalTexture,
    };
    for (unsigned int index = 0; index < 2; ++index)
    {
        glBindTexture(GL_TEXTURE_2D, geometryTextures[index]);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA16F,
            width,
            height,
            0,
            GL_RGBA,
            GL_FLOAT,
            nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0 + index,
            GL_TEXTURE_2D,
            geometryTextures[index],
            0);
    }
    constexpr GLenum geometryColorAttachments[] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
    };
    glDrawBuffers(2, geometryColorAttachments);

    glBindRenderbuffer(GL_RENDERBUFFER, geometryDepthRenderbuffer);
    glRenderbufferStorage(
        GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER,
        geometryDepthRenderbuffer);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        throw std::runtime_error("SSAO geometry framebuffer is incomplete.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFramebuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R16F,
        width,
        height,
        0,
        GL_RED,
        GL_FLOAT,
        nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        ssaoTexture,
        0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        throw std::runtime_error("SSAO framebuffer is incomplete.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFramebuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R16F,
        width,
        height,
        0,
        GL_RED,
        GL_FLOAT,
        nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        ssaoBlurTexture,
        0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        throw std::runtime_error("SSAO blur framebuffer is incomplete.");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    bufferWidth = width;
    bufferHeight = height;
}

void Renderer::ShadowPass(const Scene& scene)
{
    const float nearPlane = std::max(scene.light.shadowNearPlane, 0.01F);
    const float farPlane = std::max(
        scene.light.shadowFarPlane, nearPlane + 1.0F);
    const glm::mat4 shadowProjection = glm::perspective(
        glm::radians(90.0F), 1.0F, nearPlane, farPlane);
    const glm::vec3& lightPosition = scene.light.position;

    const std::array<glm::mat4, 6> shadowMatrices = {
        shadowProjection * glm::lookAt(
            lightPosition,
            lightPosition + glm::vec3(1.0F, 0.0F, 0.0F),
            glm::vec3(0.0F, -1.0F, 0.0F)),
        shadowProjection * glm::lookAt(
            lightPosition,
            lightPosition + glm::vec3(-1.0F, 0.0F, 0.0F),
            glm::vec3(0.0F, -1.0F, 0.0F)),
        shadowProjection * glm::lookAt(
            lightPosition,
            lightPosition + glm::vec3(0.0F, 1.0F, 0.0F),
            glm::vec3(0.0F, 0.0F, 1.0F)),
        shadowProjection * glm::lookAt(
            lightPosition,
            lightPosition + glm::vec3(0.0F, -1.0F, 0.0F),
            glm::vec3(0.0F, 0.0F, -1.0F)),
        shadowProjection * glm::lookAt(
            lightPosition,
            lightPosition + glm::vec3(0.0F, 0.0F, 1.0F),
            glm::vec3(0.0F, -1.0F, 0.0F)),
        shadowProjection * glm::lookAt(
            lightPosition,
            lightPosition + glm::vec3(0.0F, 0.0F, -1.0F),
            glm::vec3(0.0F, -1.0F, 0.0F)),
    };

    glViewport(
        0,
        0,
        allocatedShadowMapResolution,
        allocatedShadowMapResolution);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    shadowDepthShader.use();
    shadowDepthShader.setVec3("lightPosition", lightPosition);
    shadowDepthShader.setFloat("farPlane", farPlane);
    for (unsigned int face = 0; face < shadowMatrices.size(); ++face)
    {
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            shadowDepthCubeMap,
            0);
        glClear(GL_DEPTH_BUFFER_BIT);
        shadowDepthShader.setMat4("shadowMatrix", shadowMatrices[face]);
        scene.DrawShadow(shadowDepthShader);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::GeometryPass(
    const Scene& scene,
    const Camera& camera,
    int width,
    int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, geometryFramebuffer);
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    constexpr GLfloat emptyGeometry[] = {0.0F, 0.0F, 0.0F, 0.0F};
    constexpr GLfloat clearDepth = 1.0F;
    glClearBufferfv(GL_COLOR, 0, emptyGeometry);
    glClearBufferfv(GL_COLOR, 1, emptyGeometry);
    glClearBufferfv(GL_DEPTH, 0, &clearDepth);

    const float aspectRatio = static_cast<float>(width) /
        static_cast<float>(height);
    const glm::mat4 projection = glm::perspective(
        glm::radians(camera.Zoom), aspectRatio, 0.1F, 600.0F);

    geometryShader.use();
    geometryShader.setMat4("view", camera.GetViewMatrix());
    geometryShader.setMat4("projection", projection);
    scene.Draw(geometryShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SsaoPass(const Camera& camera, int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFramebuffer);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    constexpr GLfloat clearOcclusion[] = {1.0F, 0.0F, 0.0F, 1.0F};
    glClearBufferfv(GL_COLOR, 0, clearOcclusion);

    const float aspectRatio = static_cast<float>(width) /
        static_cast<float>(height);
    const glm::mat4 projection = glm::perspective(
        glm::radians(camera.Zoom), aspectRatio, 0.1F, 600.0F);

    ssaoShader.use();
    ssaoShader.setMat4("projection", projection);
    ssaoShader.setVec2(
        "noiseScale",
        static_cast<float>(width) / 4.0F,
        static_cast<float>(height) / 4.0F);
    ssaoShader.setInt(
        "kernelSize", std::clamp(settings.ssaoKernelSize, 1, 64));
    ssaoShader.setFloat("radius", std::max(settings.ssaoRadius, 0.01F));
    ssaoShader.setFloat("bias", std::max(settings.ssaoBias, 0.0F));
    ssaoShader.setFloat("power", std::max(settings.ssaoPower, 0.01F));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, geometryPositionTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, geometryNormalTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);
    glBindVertexArray(screenVertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::SsaoBlurPass(int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFramebuffer);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    ssaoBlurShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoTexture);
    glBindVertexArray(screenVertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::ForwardPass(
    const Scene& scene,
    const Camera& camera,
    int width,
    int height)
{
    glBindFramebuffer(
        GL_FRAMEBUFFER,
        settings.msaaEnabled ? msaaFramebuffer : hdrFramebuffer);
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    if (settings.msaaEnabled)
    {
        glEnable(GL_MULTISAMPLE);
    }
    else
    {
        glDisable(GL_MULTISAMPLE);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const GLfloat sceneClearColor[] = {
        settings.clearColor.r,
        settings.clearColor.g,
        settings.clearColor.b,
        1.0F,
    };
    constexpr GLfloat clearDepth = 1.0F;
    glClearBufferfv(GL_COLOR, 0, sceneClearColor);
    glClearBufferfv(GL_DEPTH, 0, &clearDepth);
    glPolygonMode(
        GL_FRONT_AND_BACK, settings.wireframe ? GL_LINE : GL_FILL);

    const float aspectRatio = static_cast<float>(width) /
        static_cast<float>(height);
    const glm::mat4 projection = glm::perspective(
        glm::radians(camera.Zoom), aspectRatio, 0.1F, 600.0F);

    lightingShader.use();
    lightingShader.setMat4("view", camera.GetViewMatrix());
    lightingShader.setMat4("projection", projection);
    lightingShader.setVec3("viewPosition", camera.Position);
    lightingShader.setVec3("light.position", scene.light.position);
    lightingShader.setVec3(
        "light.ambient", scene.light.color * scene.light.ambientStrength);
    lightingShader.setVec3(
        "light.diffuse", scene.light.color * scene.light.diffuseStrength);
    lightingShader.setVec3(
        "light.specular", scene.light.color * scene.light.specularStrength);
    lightingShader.setFloat(
        "emissiveMapIntensity", settings.emissiveMapIntensity);
    lightingShader.setBool("shadowsEnabled", settings.shadowsEnabled);
    lightingShader.setFloat(
        "shadowFarPlane",
        std::max(
            scene.light.shadowFarPlane,
            std::max(scene.light.shadowNearPlane, 0.01F) + 1.0F));
    lightingShader.setFloat("shadowBiasMin", settings.shadowBiasMin);
    lightingShader.setFloat("shadowBiasSlope", settings.shadowBiasSlope);
    lightingShader.setFloat("shadowPcfRadius", settings.shadowPcfRadius);
    lightingShader.setInt(
        "shadowPcfSamples", std::clamp(settings.shadowPcfSamples, 1, 48));
    lightingShader.setBool("ssaoEnabled", settings.ssaoEnabled);
    lightingShader.setFloat(
        "ssaoStrength", std::max(settings.ssaoStrength, 0.0F));
    lightingShader.setVec2(
        "viewportSize",
        static_cast<float>(width),
        static_cast<float>(height));
    glActiveTexture(GL_TEXTURE0 + kSsaoTextureUnit);
    glBindTexture(GL_TEXTURE_2D, ssaoBlurTexture);
    glActiveTexture(GL_TEXTURE0 + kShadowTextureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, shadowDepthCubeMap);
    scene.Draw(lightingShader);

    if (settings.msaaEnabled)
    {
        ResolveMsaa(width, height);
    }
}

void Renderer::ResolveMsaa(int width, int height)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hdrFramebuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(
        0,
        0,
        width,
        height,
        0,
        0,
        width,
        height,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::FinalPass(int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClear(GL_COLOR_BUFFER_BIT);

    hdrShader.use();
    hdrShader.setFloat("exposure", settings.exposure);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrColorTexture);
    glBindVertexArray(screenVertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::Destroy()
{
    if (isDestroyed)
    {
        return;
    }

    glDeleteRenderbuffers(1, &hdrDepthRenderbuffer);
    glDeleteRenderbuffers(1, &geometryDepthRenderbuffer);
    glDeleteRenderbuffers(1, &msaaDepthRenderbuffer);
    glDeleteTextures(1, &shadowDepthCubeMap);
    glDeleteFramebuffers(1, &shadowFramebuffer);
    glDeleteTextures(1, &geometryPositionTexture);
    glDeleteTextures(1, &geometryNormalTexture);
    glDeleteFramebuffers(1, &geometryFramebuffer);
    glDeleteTextures(1, &ssaoTexture);
    glDeleteFramebuffers(1, &ssaoFramebuffer);
    glDeleteTextures(1, &ssaoBlurTexture);
    glDeleteFramebuffers(1, &ssaoBlurFramebuffer);
    glDeleteTextures(1, &ssaoNoiseTexture);
    glDeleteTextures(1, &msaaColorTexture);
    glDeleteFramebuffers(1, &msaaFramebuffer);
    glDeleteTextures(1, &hdrColorTexture);
    glDeleteFramebuffers(1, &hdrFramebuffer);
    glDeleteBuffers(1, &screenVertexBuffer);
    glDeleteVertexArrays(1, &screenVertexArray);
    glDeleteProgram(ssaoBlurShader.ID);
    glDeleteProgram(ssaoShader.ID);
    glDeleteProgram(geometryShader.ID);
    glDeleteProgram(shadowDepthShader.ID);
    glDeleteProgram(hdrShader.ID);
    glDeleteProgram(lightingShader.ID);
    isDestroyed = true;
}
