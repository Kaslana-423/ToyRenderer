#ifndef MYRENDERER_SCENE_H
#define MYRENDERER_SCENE_H

#include "Model.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

struct SceneLight
{
    glm::vec3 position{45.0F, 95.0F, 80.0F};
    glm::vec3 color{1.0F, 0.96F, 0.86F};
    float ambientStrength = 0.18F;
    float diffuseStrength = 0.72F;
    float specularStrength = 0.70F;
    float shadowNearPlane = 1.0F;
    float shadowFarPlane = 500.0F;
};

struct SceneTransform
{
    glm::vec3 position{0.0F};
    glm::vec3 rotationDegrees{0.0F};
    glm::vec3 scale{1.0F};

    [[nodiscard]] glm::mat4 Matrix() const;
};

class SceneObject
{
public:
    std::string name;
    SceneTransform transform;
    bool visible = true;
    bool castsShadow = true;

    void ResetTransform();

private:
    friend class Scene;

    std::unique_ptr<Model> model;
    SceneTransform initialTransform;
};

class Scene
{
public:
    SceneLight light;

    SceneObject& AddModel(
        const std::string& name,
        const std::string& path,
        const SceneTransform& transform = {},
        bool gammaCorrection = true,
        std::vector<std::string> ignoredMeshes = {});

    void Draw(Shader& shader) const;
    void DrawShadow(Shader& shader) const;
    [[nodiscard]] std::size_t ObjectCount() const;
    SceneObject& GetObject(std::size_t index);
    [[nodiscard]] const SceneObject& GetObject(std::size_t index) const;
    void Destroy();

private:
    std::vector<SceneObject> objects;
};

#endif
