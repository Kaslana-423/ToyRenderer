#ifndef MESH_H
#define MESH_H

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#define MAX_BONE_INFLUENCE 4

struct Vertex
{
    glm::vec3 Position{};
    glm::vec3 Normal{};
    glm::vec2 TexCoords{};
    glm::vec3 Tangent{};
    glm::vec3 Bitangent{};
    int m_BoneIDs[MAX_BONE_INFLUENCE]{-1, -1, -1, -1};
    float m_Weights[MAX_BONE_INFLUENCE]{};
};

struct Texture
{
    unsigned int id = 0;
    std::string type;
    std::string path;
    bool isSrgb = false;
};

struct Material
{
    glm::vec3 diffuseColor{1.0F};
    glm::vec3 specularColor{0.0F};
    glm::vec3 emissiveColor{0.0F};
    float shininess = 32.0F;
    float metallic = 0.0F;
    float roughness = 0.5F;
    float emissiveStrength = 1.0F;
    bool usePbr = false;
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    Material material;
    unsigned int VAO = 0;

    Mesh(
        std::vector<Vertex> meshVertices,
        std::vector<unsigned int> meshIndices,
        std::vector<Texture> meshTextures,
        Material meshMaterial)
        : vertices(std::move(meshVertices)),
          indices(std::move(meshIndices)),
          textures(std::move(meshTextures)),
          material(meshMaterial)
    {
        setupMesh();
    }

    void Draw(Shader& shader) const
    {
        unsigned int diffuseNumber = 1;
        unsigned int specularNumber = 1;
        unsigned int emissiveNumber = 1;
        unsigned int metallicRoughnessNumber = 1;
        unsigned int ambientOcclusionNumber = 1;
        unsigned int normalNumber = 1;
        unsigned int heightNumber = 1;
        bool hasDiffuseMap = false;
        bool hasSpecularMap = false;
        bool hasEmissiveMap = false;
        bool hasNormalMap = false;
        bool hasMetallicRoughnessMap = false;
        bool hasAmbientOcclusionMap = false;

        for (unsigned int index = 0; index < textures.size(); ++index)
        {
            glActiveTexture(GL_TEXTURE0 + index);

            std::string number;
            const std::string& name = textures[index].type;
            if (name == "texture_diffuse")
            {
                number = std::to_string(diffuseNumber++);
                hasDiffuseMap = true;
            }
            else if (name == "texture_specular")
            {
                number = std::to_string(specularNumber++);
                hasSpecularMap = true;
            }
            else if (name == "texture_emissive")
            {
                number = std::to_string(emissiveNumber++);
                hasEmissiveMap = true;
            }
            else if (name == "texture_metallicRoughness")
            {
                number = std::to_string(metallicRoughnessNumber++);
                hasMetallicRoughnessMap = true;
            }
            else if (name == "texture_ao")
            {
                number = std::to_string(ambientOcclusionNumber++);
                hasAmbientOcclusionMap = true;
            }
            else if (name == "texture_normal")
            {
                number = std::to_string(normalNumber++);
                hasNormalMap = true;
            }
            else if (name == "texture_height")
            {
                number = std::to_string(heightNumber++);
            }

            glUniform1i(
                glGetUniformLocation(shader.ID, (name + number).c_str()),
                static_cast<int>(index));
            glBindTexture(GL_TEXTURE_2D, textures[index].id);
        }

        shader.setBool("hasDiffuseMap", hasDiffuseMap);
        shader.setBool("hasSpecularMap", hasSpecularMap);
        shader.setBool("hasEmissiveMap", hasEmissiveMap);
        shader.setBool("hasNormalMap", hasNormalMap);
        shader.setBool(
            "hasMetallicRoughnessMap", hasMetallicRoughnessMap);
        shader.setBool("hasAmbientOcclusionMap", hasAmbientOcclusionMap);
        shader.setVec3("material.diffuseColor", material.diffuseColor);
        shader.setVec3("material.specularColor", material.specularColor);
        shader.setVec3("material.emissiveColor", material.emissiveColor);
        shader.setFloat("material.shininess", material.shininess);
        shader.setFloat("material.metallic", material.metallic);
        shader.setFloat("material.roughness", material.roughness);
        shader.setFloat("material.emissiveStrength", material.emissiveStrength);
        shader.setBool("material.usePbr", material.usePbr);

        glBindVertexArray(VAO);
        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(indices.size()),
            GL_UNSIGNED_INT,
            nullptr);
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }

    // The shadow pass only needs geometry and the diffuse alpha channel.
    // Avoid binding every PBR/material texture six times per frame.
    void DrawShadow(Shader& shader) const
    {
        const Texture* diffuseTexture = nullptr;
        for (const Texture& texture : textures)
        {
            if (texture.type == "texture_diffuse")
            {
                diffuseTexture = &texture;
                break;
            }
        }

        shader.setBool("hasDiffuseMap", diffuseTexture != nullptr);
        shader.setInt("diffuseTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(
            GL_TEXTURE_2D,
            diffuseTexture != nullptr ? diffuseTexture->id : 0);

        glBindVertexArray(VAO);
        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(indices.size()),
            GL_UNSIGNED_INT,
            nullptr);
        glBindVertexArray(0);
    }

    void Destroy()
    {
        glDeleteBuffers(1, &EBO);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
        EBO = 0;
        VBO = 0;
        VAO = 0;
    }

private:
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    void setupMesh()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
            vertices.data(),
            GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
            indices.data(),
            GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, Position)));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, Normal)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(
            2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, TexCoords)));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(
            3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, Tangent)));

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(
            4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, Bitangent)));

        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(
            5, MAX_BONE_INFLUENCE, GL_INT, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, m_BoneIDs)));

        glEnableVertexAttribArray(6);
        glVertexAttribPointer(
            6, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, m_Weights)));

        glBindVertexArray(0);
    }
};

#endif
