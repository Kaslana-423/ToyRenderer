#include "Scene.h"

#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <utility>

glm::mat4 SceneTransform::Matrix() const
{
    glm::mat4 matrix = glm::translate(glm::mat4(1.0F), position);
    matrix = glm::rotate(
        matrix, glm::radians(rotationDegrees.x), glm::vec3(1.0F, 0.0F, 0.0F));
    matrix = glm::rotate(
        matrix, glm::radians(rotationDegrees.y), glm::vec3(0.0F, 1.0F, 0.0F));
    matrix = glm::rotate(
        matrix, glm::radians(rotationDegrees.z), glm::vec3(0.0F, 0.0F, 1.0F));
    return glm::scale(matrix, scale);
}

void SceneObject::ResetTransform()
{
    transform = initialTransform;
}

SceneObject& Scene::AddModel(
    const std::string& name,
    const std::string& path,
    const SceneTransform& transform,
    bool gammaCorrection,
    std::vector<std::string> ignoredMeshes)
{
    auto model = ignoredMeshes.empty()
        ? std::make_unique<Model>(path, gammaCorrection)
        : std::make_unique<Model>(
              path, std::move(ignoredMeshes), gammaCorrection);
    SceneObject object;
    object.name = name;
    object.transform = transform;
    object.initialTransform = transform;
    object.model = std::move(model);
    objects.push_back(std::move(object));
    return objects.back();
}

void Scene::Draw(Shader& shader) const
{
    for (const SceneObject& object : objects)
    {
        if (!object.visible)
        {
            continue;
        }
        shader.setMat4("model", object.transform.Matrix());
        object.model->Draw(shader);
    }
}

void Scene::DrawShadow(Shader& shader) const
{
    for (const SceneObject& object : objects)
    {
        if (!object.visible || !object.castsShadow)
        {
            continue;
        }
        shader.setMat4("model", object.transform.Matrix());
        object.model->DrawShadow(shader);
    }
}

std::size_t Scene::ObjectCount() const
{
    return objects.size();
}

SceneObject& Scene::GetObject(std::size_t index)
{
    if (index >= objects.size())
    {
        throw std::out_of_range("Scene object index is out of range.");
    }
    return objects[index];
}

const SceneObject& Scene::GetObject(std::size_t index) const
{
    if (index >= objects.size())
    {
        throw std::out_of_range("Scene object index is out of range.");
    }
    return objects[index];
}

void Scene::Destroy()
{
    for (SceneObject& object : objects)
    {
        object.model->Destroy();
    }
    objects.clear();
}
