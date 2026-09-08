#ifndef MYRENDERER_DEBUG_UI_H
#define MYRENDERER_DEBUG_UI_H

#include <glm/glm.hpp>

struct GLFWwindow;
class Camera;
class Renderer;
class Scene;

class DebugUI
{
public:
    void Initialize(GLFWwindow* window);
    void BeginFrame();
    void Draw(Scene& scene, Camera& camera, Renderer& renderer);
    void Render();
    void Shutdown();

    // Tab switches between UI interaction and captured-mouse camera control.
    bool UpdateInputMode(GLFWwindow* window);
    [[nodiscard]] bool CameraControlEnabled() const;

private:
    bool initialized = false;
    bool cameraControlEnabled = false;
    bool tabWasPressed = false;
    bool cameraDefaultsCaptured = false;
    glm::vec3 defaultCameraPosition{0.0F};
    float defaultCameraYaw = 0.0F;
    float defaultCameraPitch = 0.0F;
    float defaultCameraZoom = 45.0F;
    float defaultCameraSpeed = 2.5F;
    float defaultMouseSensitivity = 0.1F;
};

#endif
