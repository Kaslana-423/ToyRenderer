#ifndef MODEL_H
#define MODEL_H

#include <glad/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Mesh.h"
#include "Shader.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

inline unsigned int TextureFromFile(
    const char* path,
    const std::string& directory,
    const aiScene* scene,
    bool gamma = false);

class Model
{
public:
    std::vector<Texture> textures_loaded;
    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection;

    explicit Model(const std::string& path, bool gamma = false)
        : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    Model(
        const std::string& path,
        std::vector<std::string> ignoredMeshes,
        bool gamma = false)
        : gammaCorrection(gamma),
          ignoredMeshNames(std::move(ignoredMeshes))
    {
        loadModel(path);
    }

    void Draw(Shader& shader) const
    {
        for (const Mesh& mesh : meshes)
        {
            mesh.Draw(shader);
        }
    }

    void DrawShadow(Shader& shader) const
    {
        for (const Mesh& mesh : meshes)
        {
            mesh.DrawShadow(shader);
        }
    }

    void Destroy()
    {
        for (Mesh& mesh : meshes)
        {
            mesh.Destroy();
        }

        for (const Texture& texture : textures_loaded)
        {
            glDeleteTextures(1, &texture.id);
        }
        textures_loaded.clear();
    }

private:
    std::vector<std::string> ignoredMeshNames;

    [[nodiscard]] bool shouldIgnore(const char* name) const
    {
        return std::find(
                   ignoredMeshNames.begin(),
                   ignoredMeshNames.end(),
                   name) != ignoredMeshNames.end();
    }

    void loadModel(const std::string& path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate |
                aiProcess_GenSmoothNormals |
                aiProcess_FlipUVs |
                aiProcess_TransformUVCoords |
                aiProcess_CalcTangentSpace);

        if (scene == nullptr ||
            (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0U ||
            scene->mRootNode == nullptr)
        {
            throw std::runtime_error(
                std::string("ERROR::ASSIMP:: ") + importer.GetErrorString());
        }

        const std::size_t separator = path.find_last_of("/\\");
        directory = separator == std::string::npos ? "." : path.substr(0, separator);
        processNode(scene->mRootNode, scene, aiMatrix4x4());
    }

    void processNode(
        aiNode* node,
        const aiScene* scene,
        const aiMatrix4x4& parentTransform)
    {
        const aiMatrix4x4 nodeTransform =
            parentTransform * node->mTransformation;
        for (unsigned int index = 0; index < node->mNumMeshes; ++index)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[index]];
            if (shouldIgnore(node->mName.C_Str()) ||
                shouldIgnore(mesh->mName.C_Str()))
            {
                continue;
            }
            meshes.push_back(processMesh(mesh, scene, nodeTransform));
        }

        for (unsigned int index = 0; index < node->mNumChildren; ++index)
        {
            processNode(node->mChildren[index], scene, nodeTransform);
        }
    }

    Mesh processMesh(
        aiMesh* mesh,
        const aiScene* scene,
        const aiMatrix4x4& nodeTransform)
    {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;
        Material meshMaterial;

        vertices.reserve(mesh->mNumVertices);
        aiMatrix3x3 normalTransform(nodeTransform);
        normalTransform.Inverse().Transpose();
        for (unsigned int index = 0; index < mesh->mNumVertices; ++index)
        {
            Vertex vertex;

            const aiVector3D position =
                nodeTransform * mesh->mVertices[index];
            vertex.Position = glm::vec3(
                position.x, position.y, position.z);

            if (mesh->HasNormals())
            {
                const aiVector3D transformedNormal =
                    (normalTransform * mesh->mNormals[index]).Normalize();
                vertex.Normal = glm::vec3(
                    transformedNormal.x,
                    transformedNormal.y,
                    transformedNormal.z);
            }

            if (mesh->mTextureCoords[0] != nullptr)
            {
                vertex.TexCoords = glm::vec2(
                    mesh->mTextureCoords[0][index].x,
                    mesh->mTextureCoords[0][index].y);
            }

            if (mesh->HasTangentsAndBitangents())
            {
                const aiVector3D transformedTangent =
                    (normalTransform * mesh->mTangents[index]).Normalize();
                const aiVector3D transformedBitangent =
                    (normalTransform * mesh->mBitangents[index]).Normalize();
                vertex.Tangent = glm::vec3(
                    transformedTangent.x,
                    transformedTangent.y,
                    transformedTangent.z);
                vertex.Bitangent = glm::vec3(
                    transformedBitangent.x,
                    transformedBitangent.y,
                    transformedBitangent.z);
            }

            vertices.push_back(vertex);
        }

        for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
        {
            const aiFace& face = mesh->mFaces[faceIndex];
            for (unsigned int index = 0; index < face.mNumIndices; ++index)
            {
                indices.push_back(face.mIndices[index]);
            }
        }

        if (mesh->mMaterialIndex < scene->mNumMaterials)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            aiColor4D baseColor(1.0F, 1.0F, 1.0F, 1.0F);
            if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS)
            {
                meshMaterial.diffuseColor = glm::vec3(
                    baseColor.r, baseColor.g, baseColor.b);
                meshMaterial.usePbr = true;
            }

            aiColor3D diffuseColor(1.0F, 1.0F, 1.0F);
            if (!meshMaterial.usePbr &&
                material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS)
            {
                meshMaterial.diffuseColor = glm::vec3(
                    diffuseColor.r, diffuseColor.g, diffuseColor.b);
            }

            aiColor3D specularColor(0.0F, 0.0F, 0.0F);
            if (material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor) == AI_SUCCESS)
            {
                meshMaterial.specularColor = glm::vec3(
                    specularColor.r, specularColor.g, specularColor.b);
            }

            aiColor3D emissiveColor(0.0F, 0.0F, 0.0F);
            if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS)
            {
                meshMaterial.emissiveColor = glm::vec3(
                    emissiveColor.r, emissiveColor.g, emissiveColor.b);
            }

            float shininess = meshMaterial.shininess;
            if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
            {
                meshMaterial.shininess = glm::max(shininess, 1.0F);
            }

            float metallic = meshMaterial.metallic;
            if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
            {
                meshMaterial.metallic = glm::clamp(metallic, 0.0F, 1.0F);
                meshMaterial.usePbr = true;
            }

            float roughness = meshMaterial.roughness;
            if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
            {
                meshMaterial.roughness = glm::clamp(roughness, 0.04F, 1.0F);
                meshMaterial.usePbr = true;
            }

            float emissiveStrength = meshMaterial.emissiveStrength;
            if (material->Get(
                    AI_MATKEY_EMISSIVE_INTENSITY, emissiveStrength) == AI_SUCCESS)
            {
                meshMaterial.emissiveStrength = glm::max(emissiveStrength, 0.0F);
            }

            auto diffuseMaps = loadMaterialTextures(
                material,
                meshMaterial.usePbr
                    ? aiTextureType_BASE_COLOR
                    : aiTextureType_DIFFUSE,
                "texture_diffuse",
                scene,
                true);
            if (diffuseMaps.empty() && meshMaterial.usePbr)
            {
                diffuseMaps = loadMaterialTextures(
                    material,
                    aiTextureType_DIFFUSE,
                    "texture_diffuse",
                    scene,
                    true);
            }
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            auto emissiveMaps = loadMaterialTextures(
                material,
                aiTextureType_EMISSIVE,
                "texture_emissive",
                scene,
                true);
            textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());

            auto specularMaps = loadMaterialTextures(
                material,
                aiTextureType_SPECULAR,
                "texture_specular",
                scene,
                false);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

            auto normalMaps = loadMaterialTextures(
                material,
                meshMaterial.usePbr
                    ? aiTextureType_NORMALS
                    : aiTextureType_HEIGHT,
                "texture_normal",
                scene,
                false);
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

            if (meshMaterial.usePbr)
            {
                auto metallicRoughnessMaps = loadMaterialTextures(
                    material,
                    aiTextureType_METALNESS,
                    "texture_metallicRoughness",
                    scene,
                    false);
                textures.insert(
                    textures.end(),
                    metallicRoughnessMaps.begin(),
                    metallicRoughnessMaps.end());

                auto ambientOcclusionMaps = loadMaterialTextures(
                    material,
                    aiTextureType_AMBIENT_OCCLUSION,
                    "texture_ao",
                    scene,
                    false);
                textures.insert(
                    textures.end(),
                    ambientOcclusionMaps.begin(),
                    ambientOcclusionMaps.end());
            }
        }

        return Mesh(
            std::move(vertices),
            std::move(indices),
            std::move(textures),
            meshMaterial);
    }

    std::vector<Texture> loadMaterialTextures(
        aiMaterial* material,
        aiTextureType type,
        const std::string& typeName,
        const aiScene* scene,
        bool isSrgb)
    {
        std::vector<Texture> textures;

        for (unsigned int index = 0; index < material->GetTextureCount(type); ++index)
        {
            aiString path;
            material->GetTexture(type, index, &path);

            bool alreadyLoaded = false;
            for (const Texture& loadedTexture : textures_loaded)
            {
                if (std::strcmp(loadedTexture.path.c_str(), path.C_Str()) == 0)
                {
                    if (loadedTexture.isSrgb != isSrgb)
                    {
                        continue;
                    }
                    Texture reusedTexture = loadedTexture;
                    reusedTexture.type = typeName;
                    textures.push_back(std::move(reusedTexture));
                    alreadyLoaded = true;
                    break;
                }
            }

            if (!alreadyLoaded)
            {
                Texture texture;
                texture.id = TextureFromFile(
                    path.C_Str(), directory, scene, gammaCorrection && isSrgb);
                if (texture.id == 0)
                {
                    continue;
                }
                texture.type = typeName;
                texture.path = path.C_Str();
                texture.isSrgb = isSrgb;
                textures.push_back(texture);
                textures_loaded.push_back(std::move(texture));
            }
        }

        return textures;
    }
};

inline unsigned int TextureFromFile(
    const char* path,
    const std::string& directory,
    const aiScene* scene,
    bool gamma)
{
    const std::filesystem::path filename =
        (std::filesystem::u8path(directory) / std::filesystem::u8path(path))
            .lexically_normal();

    // The standalone cube flips its image, while Assimp already flips model UVs.
    stbi_set_flip_vertically_on_load(0);

    const aiTexture* embeddedTexture =
        scene == nullptr ? nullptr : scene->GetEmbeddedTexture(path);
    std::FILE* file = nullptr;
    unsigned char* data = nullptr;
    bool ownsImageData = false;
    int width = 0;
    int height = 0;
    int componentCount = 0;
    GLenum dataFormat = GL_RGB;

    if (embeddedTexture != nullptr && embeddedTexture->mHeight == 0)
    {
        data = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(embeddedTexture->pcData),
            static_cast<int>(embeddedTexture->mWidth),
            &width,
            &height,
            &componentCount,
            0);
        ownsImageData = true;
    }
    else if (embeddedTexture != nullptr)
    {
        width = static_cast<int>(embeddedTexture->mWidth);
        height = static_cast<int>(embeddedTexture->mHeight);
        componentCount = 4;
        data = reinterpret_cast<unsigned char*>(embeddedTexture->pcData);
        dataFormat = GL_BGRA;
    }
    else
    {
#ifdef _WIN32
        _wfopen_s(&file, filename.c_str(), L"rb");
#else
        file = std::fopen(filename.string().c_str(), "rb");
#endif
        if (file == nullptr)
        {
            std::cerr << "Texture file could not be opened: "
                      << filename.u8string() << '\n';
            return 0;
        }
        data = stbi_load_from_file(file, &width, &height, &componentCount, 0);
        std::fclose(file);
        ownsImageData = true;
    }

    if (data == nullptr)
    {
        std::cerr << "Texture failed to load at path: " << filename.u8string() << '\n';
        return 0;
    }

    GLenum internalFormat = GL_RGB;
    if (componentCount == 1)
    {
        dataFormat = GL_RED;
        internalFormat = GL_RED;
    }
    else if (componentCount == 3)
    {
        dataFormat = GL_RGB;
        internalFormat = gamma ? GL_SRGB : GL_RGB;
    }
    else if (componentCount == 4)
    {
        if (embeddedTexture == nullptr || embeddedTexture->mHeight == 0)
        {
            dataFormat = GL_RGBA;
        }
        internalFormat = gamma ? GL_SRGB_ALPHA : GL_RGBA;
    }

    unsigned int textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<int>(internalFormat),
        width,
        height,
        0,
        dataFormat,
        GL_UNSIGNED_BYTE,
        data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (ownsImageData)
    {
        stbi_image_free(data);
    }
    return textureID;
}

#endif
