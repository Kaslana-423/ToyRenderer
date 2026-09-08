#ifndef MYRENDERER_RENDERER_H
#define MYRENDERER_RENDERER_H

#include "Camera.h"
#include "Scene.h"
#include "Shader.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

struct RendererSettings
{
    float exposure = 1.0F;
    float emissiveMapIntensity = 1.5F;
    float shadowBiasMin = 0.12F;
    float shadowBiasSlope = 0.60F;
    float shadowPcfRadius = 0.75F;
    float ssaoRadius = 3.0F;
    float ssaoBias = 0.12F;
    float ssaoPower = 1.5F;
    float ssaoStrength = 1.0F;
    int shadowMapResolution = 1024;
    int shadowPcfSamples = 24;
    int ssaoKernelSize = 32;
    int msaaSamples = 4;
    glm::vec3 clearColor{0.015F, 0.025F, 0.05F};
    bool shadowsEnabled = true;
    bool ssaoEnabled = true;
    bool msaaEnabled = true;
    bool wireframe = false;
};

class Renderer
{
public:
    Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void Render(
        const Scene& scene,
        const Camera& camera,
        int framebufferWidth,
        int framebufferHeight);
    void Destroy();

    RendererSettings settings;

private:
    Shader lightingShader;
    Shader shadowDepthShader;
    Shader geometryShader;
    Shader ssaoShader;
    Shader ssaoBlurShader;
    Shader hdrShader;

    static constexpr int kShadowTextureUnit = 15;
    static constexpr int kSsaoTextureUnit = 14;

    GLuint shadowFramebuffer = 0;
    GLuint shadowDepthCubeMap = 0;
    GLuint geometryFramebuffer = 0;
    GLuint geometryPositionTexture = 0;
    GLuint geometryNormalTexture = 0;
    GLuint geometryDepthRenderbuffer = 0;
    GLuint ssaoFramebuffer = 0;
    GLuint ssaoTexture = 0;
    GLuint ssaoBlurFramebuffer = 0;
    GLuint ssaoBlurTexture = 0;
    GLuint ssaoNoiseTexture = 0;
    GLuint msaaFramebuffer = 0;
    GLuint msaaColorTexture = 0;
    GLuint msaaDepthRenderbuffer = 0;
    GLuint hdrFramebuffer = 0;
    GLuint hdrColorTexture = 0;
    GLuint hdrDepthRenderbuffer = 0;
    GLuint screenVertexArray = 0;
    GLuint screenVertexBuffer = 0;
    int allocatedShadowMapResolution = 0;
    int allocatedMsaaSamples = 0;
    int msaaBufferWidth = 0;
    int msaaBufferHeight = 0;
    int bufferWidth = 0;
    int bufferHeight = 0;
    bool isDestroyed = false;

    void SetupScreenQuad();
    void SetupShadowMap();
    void SetupSsaoResources();
    void SetupMsaaResources();
    void ResizeShadowMap(int resolution);
    void ResizeMsaaBuffers(int width, int height, int samples);
    void ResizeBuffers(int width, int height);
    void ShadowPass(const Scene& scene);
    void GeometryPass(
        const Scene& scene,
        const Camera& camera,
        int width,
        int height);
    void ResolveMsaa(int width, int height);
    void SsaoPass(const Camera& camera, int width, int height);
    void SsaoBlurPass(int width, int height);
    void ForwardPass(
        const Scene& scene,
        const Camera& camera,
        int width,
        int height);
    void FinalPass(int width, int height);
};

#endif
