#ifndef MYRENDERER_CAMERA_H
#define MYRENDERER_CAMERA_H

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

constexpr float YAW = -90.0F;
constexpr float PITCH = 0.0F;
constexpr float SPEED = 2.5F;
constexpr float SENSITIVITY = 0.1F;
constexpr float ZOOM = 45.0F;

class Camera
{
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    explicit Camera(
        glm::vec3 position = glm::vec3(0.0F, 0.0F, 0.0F),
        glm::vec3 up = glm::vec3(0.0F, 1.0F, 0.0F),
        float yaw = YAW,
        float pitch = PITCH)
        : Position(position),
          Front(0.0F, 0.0F, -1.0F),
          WorldUp(up),
          Yaw(yaw),
          Pitch(pitch),
          MovementSpeed(SPEED),
          MouseSensitivity(SENSITIVITY),
          Zoom(ZOOM)
    {
        updateCameraVectors();
    }

    Camera(
        float posX,
        float posY,
        float posZ,
        float upX,
        float upY,
        float upZ,
        float yaw,
        float pitch)
        : Position(posX, posY, posZ),
          Front(0.0F, 0.0F, -1.0F),
          WorldUp(upX, upY, upZ),
          Yaw(yaw),
          Pitch(pitch),
          MovementSpeed(SPEED),
          MouseSensitivity(SENSITIVITY),
          Zoom(ZOOM)
    {
        updateCameraVectors();
    }

    [[nodiscard]] glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        const float velocity = MovementSpeed * deltaTime;

        if (direction == FORWARD)
        {
            Position += Front * velocity;
        }
        if (direction == BACKWARD)
        {
            Position -= Front * velocity;
        }
        if (direction == LEFT)
        {
            Position -= Right * velocity;
        }
        if (direction == RIGHT)
        {
            Position += Right * velocity;
        }
    }

    void ProcessMouseMovement(
        float xOffset,
        float yOffset,
        GLboolean constrainPitch = GL_TRUE)
    {
        xOffset *= MouseSensitivity;
        yOffset *= MouseSensitivity;

        Yaw += xOffset;
        Pitch += yOffset;

        if (constrainPitch == GL_TRUE)
        {
            if (Pitch > 89.0F)
            {
                Pitch = 89.0F;
            }
            if (Pitch < -89.0F)
            {
                Pitch = -89.0F;
            }
        }

        updateCameraVectors();
    }

    void ProcessMouseScroll(float yOffset)
    {
        Zoom -= yOffset;

        if (Zoom < 1.0F)
        {
            Zoom = 1.0F;
        }
        if (Zoom > 90.0F)
        {
            Zoom = 90.0F;
        }
    }

    void SetOrientation(float yaw, float pitch)
    {
        Yaw = yaw;
        Pitch = std::clamp(pitch, -89.0F, 89.0F);
        updateCameraVectors();
    }

private:
    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = std::cos(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));
        front.y = std::sin(glm::radians(Pitch));
        front.z = std::sin(glm::radians(Yaw)) * std::cos(glm::radians(Pitch));

        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

#endif
