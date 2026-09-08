#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "DebugUI.h"
#include "Renderer.h"
#include "Scene.h"

#include <glm/glm.hpp>

#include <exception>
#include <iostream>
#include <memory>

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;

// Start inside Odette Room and look toward its center along -Z.
Camera camera(glm::vec3(0.0F, 18.0F, 85.0F), glm::vec3(0.0F, 1.0F, 0.0F),
              -90.0F, -5.0F);
float lastMouseX = static_cast<float>(kWindowWidth) / 2.0F;
float lastMouseY = static_cast<float>(kWindowHeight) / 2.0F;
bool isFirstMouseEvent = true;
DebugUI* activeDebugUI = nullptr;

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouseCallback(GLFWwindow*, double xPosition, double yPosition) {
    if (activeDebugUI != nullptr &&
        !activeDebugUI->CameraControlEnabled()) {
        return;
    }

    const float currentX = static_cast<float>(xPosition);
    const float currentY = static_cast<float>(yPosition);

    if (isFirstMouseEvent) {
        lastMouseX = currentX;
        lastMouseY = currentY;
        isFirstMouseEvent = false;
    }

    camera.ProcessMouseMovement(currentX - lastMouseX, lastMouseY - currentY);
    lastMouseX = currentX;
    lastMouseY = currentY;
}

void scrollCallback(GLFWwindow*, double, double yOffset) {
    if (activeDebugUI != nullptr &&
        !activeDebugUI->CameraControlEnabled()) {
        return;
    }
    camera.ProcessMouseScroll(static_cast<float>(yOffset));
}

void processInput(
    GLFWwindow* window,
    float deltaTime,
    bool cameraControlEnabled) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (!cameraControlEnabled) {
        return;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

} // namespace

int main() {
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Failed to initialize GLFW.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        kWindowWidth, kWindowHeight, "Odette Room - Lisa", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create a GLFW window.\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSwapInterval(1);

    if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) == 0) {
        std::cerr << "Failed to initialize GLAD.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << '\n';
    std::cout << "Controls: WASD move, mouse look, wheel zoom, Esc quit.\n";

    Scene scene;
    DebugUI debugUI;
    std::unique_ptr<Renderer> renderer;
    int exitCode = 0;

    try {
        debugUI.Initialize(window);
        activeDebugUI = &debugUI;
        renderer = std::make_unique<Renderer>();
        scene.AddModel(
            "PMX House", "res/model/pmxHouse/Odette Room.pmx");

        SceneTransform lisaTransform;
        lisaTransform.position = glm::vec3(0.0F, 0.25F, 0.0F);
        scene.AddModel(
            "Lisa", "res/model/lisa/lisa.obj", lisaTransform);

        camera.MovementSpeed = 12.0F;
        float lastFrameTime = static_cast<float>(glfwGetTime());

        while (glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();

            const float currentFrameTime = static_cast<float>(glfwGetTime());
            const float deltaTime = currentFrameTime - lastFrameTime;
            lastFrameTime = currentFrameTime;

            if (debugUI.UpdateInputMode(window)) {
                isFirstMouseEvent = true;
            }
            debugUI.BeginFrame();
            processInput(
                window, deltaTime, debugUI.CameraControlEnabled());

            int framebufferWidth = 0;
            int framebufferHeight = 0;
            glfwGetFramebufferSize(
                window, &framebufferWidth, &framebufferHeight);
            renderer->Render(
                scene, camera, framebufferWidth, framebufferHeight);
            debugUI.Draw(scene, camera, *renderer);
            debugUI.Render();

            glfwSwapBuffers(window);
        }
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        exitCode = 1;
    }

    activeDebugUI = nullptr;
    debugUI.Shutdown();
    scene.Destroy();
    if (renderer != nullptr) {
        renderer->Destroy();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return exitCode;
}
