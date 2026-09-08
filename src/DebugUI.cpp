#include "DebugUI.h"

#include "Camera.h"
#include "Renderer.h"
#include "Scene.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace {

void DrawRendererSettings(RendererSettings& settings)
{
    if (!ImGui::CollapsingHeader(
            "渲染 / HDR", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::ColorEdit3("背景颜色", &settings.clearColor.x);
    ImGui::SliderFloat("曝光", &settings.exposure, 0.05F, 5.0F, "%.2f");
    ImGui::SliderFloat(
        "自发光强度", &settings.emissiveMapIntensity, 0.0F, 8.0F, "%.2f");
    ImGui::Checkbox("线框模式", &settings.wireframe);
}

void DrawMsaaSettings(RendererSettings& settings)
{
    if (!ImGui::CollapsingHeader(
            "MSAA 多重采样", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("启用 MSAA", &settings.msaaEnabled);
    ImGui::BeginDisabled(!settings.msaaEnabled);
    const int sampleCounts[] = {2, 4, 8};
    const char* sampleLabels[] = {"2x", "4x", "8x"};
    int sampleIndex = settings.msaaSamples <= 2
        ? 0
        : (settings.msaaSamples <= 4 ? 1 : 2);
    if (ImGui::Combo(
            "采样数量##MSAA", &sampleIndex, sampleLabels, 3))
    {
        settings.msaaSamples = sampleCounts[sampleIndex];
    }
    ImGui::TextWrapped(
        "MSAA 处理场景几何边缘；HDR 色调映射和 ImGui 保持单采样后处理。");
    ImGui::EndDisabled();
}

void DrawLightSettings(SceneLight& light)
{
    if (!ImGui::CollapsingHeader("灯光", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::DragFloat3("位置##Light", &light.position.x, 0.25F);
    ImGui::ColorEdit3("颜色##Light", &light.color.x);
    ImGui::SliderFloat(
        "环境光", &light.ambientStrength, 0.0F, 2.0F, "%.3f");
    ImGui::SliderFloat(
        "漫反射", &light.diffuseStrength, 0.0F, 3.0F, "%.3f");
    ImGui::SliderFloat(
        "高光", &light.specularStrength, 0.0F, 3.0F, "%.3f");
}

void DrawShadowSettings(RendererSettings& settings, SceneLight& light)
{
    if (!ImGui::CollapsingHeader(
            "阴影", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("启用阴影", &settings.shadowsEnabled);
    ImGui::BeginDisabled(!settings.shadowsEnabled);
    const int resolutions[] = {512, 1024, 2048};
    const char* resolutionLabels[] = {"低 (512)", "中 (1024)", "高 (2048)"};
    int resolutionIndex = settings.shadowMapResolution <= 512
        ? 0
        : (settings.shadowMapResolution <= 1024 ? 1 : 2);
    if (ImGui::Combo(
            "阴影质量", &resolutionIndex, resolutionLabels, 3))
    {
        settings.shadowMapResolution = resolutions[resolutionIndex];
    }
    ImGui::Text(
        "深度立方体贴图：%d x %d x 6",
        settings.shadowMapResolution,
        settings.shadowMapResolution);
    ImGui::DragFloat(
        "近裁剪面", &light.shadowNearPlane, 0.05F, 0.01F, 50.0F, "%.2f");
    ImGui::DragFloat(
        "远裁剪面", &light.shadowFarPlane, 1.0F, 10.0F, 2000.0F, "%.1f");
    light.shadowNearPlane = std::max(light.shadowNearPlane, 0.01F);
    light.shadowFarPlane = std::max(
        light.shadowFarPlane, light.shadowNearPlane + 1.0F);
    ImGui::DragFloat(
        "最小偏移", &settings.shadowBiasMin, 0.005F, 0.0F, 3.0F, "%.3f");
    ImGui::DragFloat(
        "斜率偏移", &settings.shadowBiasSlope, 0.01F, 0.0F, 5.0F, "%.3f");
    ImGui::DragFloat(
        "PCF 柔和范围", &settings.shadowPcfRadius, 0.02F, 0.0F, 5.0F, "%.2f");
    ImGui::SliderInt(
        "PCF 采样数量", &settings.shadowPcfSamples, 8, 48);
    ImGui::EndDisabled();
}

void DrawSsaoSettings(RendererSettings& settings)
{
    if (!ImGui::CollapsingHeader(
            "SSAO 环境光遮蔽", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::Checkbox("启用 SSAO", &settings.ssaoEnabled);
    ImGui::BeginDisabled(!settings.ssaoEnabled);
    ImGui::SliderInt(
        "采样数量", &settings.ssaoKernelSize, 8, 64);
    ImGui::DragFloat(
        "采样半径", &settings.ssaoRadius, 0.05F, 0.1F, 15.0F, "%.2f");
    ImGui::DragFloat(
        "深度偏移", &settings.ssaoBias, 0.005F, 0.0F, 2.0F, "%.3f");
    ImGui::SliderFloat(
        "对比度", &settings.ssaoPower, 0.25F, 4.0F, "%.2f");
    ImGui::SliderFloat(
        "混合强度", &settings.ssaoStrength, 0.0F, 3.0F, "%.2f");
    ImGui::TextWrapped(
        "SSAO 只影响环境光；直接光、阴影和自发光保持独立。");
    ImGui::EndDisabled();
}

void DrawCameraSettings(
    Camera& camera,
    const glm::vec3& defaultPosition,
    float defaultYaw,
    float defaultPitch,
    float defaultZoom,
    float defaultSpeed,
    float defaultSensitivity)
{
    if (!ImGui::CollapsingHeader("相机"))
    {
        return;
    }

    ImGui::DragFloat3("位置##Camera", &camera.Position.x, 0.1F);
    float orientation[] = {camera.Yaw, camera.Pitch};
    if (ImGui::DragFloat2("Yaw / Pitch", orientation, 0.2F))
    {
        camera.SetOrientation(orientation[0], orientation[1]);
    }
    ImGui::SliderFloat("视野角", &camera.Zoom, 1.0F, 90.0F, "%.1f°");
    ImGui::DragFloat(
        "移动速度", &camera.MovementSpeed, 0.1F, 0.1F, 100.0F, "%.1f");
    ImGui::DragFloat(
        "鼠标灵敏度",
        &camera.MouseSensitivity,
        0.005F,
        0.01F,
        2.0F,
        "%.3f");

    if (ImGui::Button("重置相机"))
    {
        camera.Position = defaultPosition;
        camera.SetOrientation(defaultYaw, defaultPitch);
        camera.Zoom = defaultZoom;
        camera.MovementSpeed = defaultSpeed;
        camera.MouseSensitivity = defaultSensitivity;
    }
}

void DrawSceneObjects(Scene& scene)
{
    if (!ImGui::CollapsingHeader(
            "场景对象", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    for (std::size_t index = 0; index < scene.ObjectCount(); ++index)
    {
        SceneObject& object = scene.GetObject(index);
        ImGui::PushID(static_cast<int>(index));
        if (ImGui::TreeNodeEx(
                object.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("显示", &object.visible);
            ImGui::SameLine();
            ImGui::Checkbox("投射阴影", &object.castsShadow);
            ImGui::DragFloat3(
                "位置##Object", &object.transform.position.x, 0.1F);
            ImGui::DragFloat3(
                "旋转##Object",
                &object.transform.rotationDegrees.x,
                0.25F,
                -360.0F,
                360.0F,
                "%.1f°");
            ImGui::DragFloat3(
                "缩放##Object",
                &object.transform.scale.x,
                0.01F,
                0.001F,
                100.0F,
                "%.3f");
            object.transform.scale = glm::max(
                object.transform.scale, glm::vec3(0.001F));
            if (ImGui::Button("重置 Transform"))
            {
                object.ResetTransform();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

} // namespace

void DebugUI::Initialize(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0F;
    style.FrameRounding = 4.0F;
    style.GrabRounding = 4.0F;

    const std::filesystem::path chineseFont =
        "C:/Windows/Fonts/msyh.ttc";
    if (std::filesystem::exists(chineseFont))
    {
        io.Fonts->AddFontFromFileTTF(
            chineseFont.string().c_str(),
            18.0F,
            nullptr,
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    }

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
    {
        ImGui::DestroyContext();
        throw std::runtime_error("Failed to initialize ImGui GLFW backend.");
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330 core"))
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        throw std::runtime_error("Failed to initialize ImGui OpenGL backend.");
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    initialized = true;
}

void DebugUI::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void DebugUI::Draw(Scene& scene, Camera& camera, Renderer& renderer)
{
    if (!cameraDefaultsCaptured)
    {
        defaultCameraPosition = camera.Position;
        defaultCameraYaw = camera.Yaw;
        defaultCameraPitch = camera.Pitch;
        defaultCameraZoom = camera.Zoom;
        defaultCameraSpeed = camera.MovementSpeed;
        defaultMouseSensitivity = camera.MouseSensitivity;
        cameraDefaultsCaptured = true;
    }

    ImGui::SetNextWindowPos(ImVec2(16.0F, 16.0F), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430.0F, 680.0F), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("场景调试面板"))
    {
        const ImGuiIO& io = ImGui::GetIO();
        ImGui::Text(
            "%.1f FPS  |  %.2f ms",
            io.Framerate,
            io.Framerate > 0.0F ? 1000.0F / io.Framerate : 0.0F);
        ImGui::TextColored(
            cameraControlEnabled
                ? ImVec4(0.35F, 0.85F, 0.45F, 1.0F)
                : ImVec4(0.35F, 0.70F, 1.0F, 1.0F),
            cameraControlEnabled
                ? "相机控制模式（按 Tab 编辑参数）"
                : "参数编辑模式（按 Tab 控制相机）");
        ImGui::Separator();

        DrawRendererSettings(renderer.settings);
        DrawMsaaSettings(renderer.settings);
        DrawSsaoSettings(renderer.settings);
        DrawLightSettings(scene.light);
        DrawShadowSettings(renderer.settings, scene.light);
        DrawCameraSettings(
            camera,
            defaultCameraPosition,
            defaultCameraYaw,
            defaultCameraPitch,
            defaultCameraZoom,
            defaultCameraSpeed,
            defaultMouseSensitivity);
        DrawSceneObjects(scene);
    }
    ImGui::End();
}

void DebugUI::Render()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DebugUI::Shutdown()
{
    if (!initialized)
    {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    initialized = false;
}

bool DebugUI::UpdateInputMode(GLFWwindow* window)
{
    const bool tabIsPressed =
        glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
    const bool didToggle = tabIsPressed && !tabWasPressed;
    tabWasPressed = tabIsPressed;
    if (!didToggle)
    {
        return false;
    }

    cameraControlEnabled = !cameraControlEnabled;
    glfwSetInputMode(
        window,
        GLFW_CURSOR,
        cameraControlEnabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    return true;
}

bool DebugUI::CameraControlEnabled() const
{
    return cameraControlEnabled;
}
