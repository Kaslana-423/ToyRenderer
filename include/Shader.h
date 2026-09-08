#ifndef MYRENDERER_SHADER_H
#define MYRENDERER_SHADER_H

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class Shader
{
public:
    unsigned int ID = 0;

    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
    {
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;

        std::ifstream vertexShaderFile;
        std::ifstream fragmentShaderFile;
        std::ifstream geometryShaderFile;

        vertexShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fragmentShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        geometryShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            vertexShaderFile.open(vertexPath);
            fragmentShaderFile.open(fragmentPath);

            std::stringstream vertexShaderStream;
            std::stringstream fragmentShaderStream;
            vertexShaderStream << vertexShaderFile.rdbuf();
            fragmentShaderStream << fragmentShaderFile.rdbuf();

            vertexShaderFile.close();
            fragmentShaderFile.close();

            vertexCode = vertexShaderStream.str();
            fragmentCode = fragmentShaderStream.str();

            if (geometryPath != nullptr)
            {
                geometryShaderFile.open(geometryPath);
                std::stringstream geometryShaderStream;
                geometryShaderStream << geometryShaderFile.rdbuf();
                geometryShaderFile.close();
                geometryCode = geometryShaderStream.str();
            }
        }
        catch (const std::ifstream::failure& exception)
        {
            std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: "
                      << exception.what() << '\n';
        }

        const char* vertexShaderCode = vertexCode.c_str();
        const char* fragmentShaderCode = fragmentCode.c_str();

        const unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexShaderCode, nullptr);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        const unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentShaderCode, nullptr);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        unsigned int geometry = 0;
        if (geometryPath != nullptr)
        {
            const char* geometryShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &geometryShaderCode, nullptr);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if (geometryPath != nullptr)
        {
            glAttachShader(ID, geometry);
        }
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (geometryPath != nullptr)
        {
            glDeleteShader(geometry);
        }
    }

    void use() const
    {
        glUseProgram(ID);
    }

    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), static_cast<int>(value));
    }

    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec2(const std::string& name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }

    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec4(const std::string& name, float x, float y, float z, float w) const
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }

    void setMat2(const std::string& name, const glm::mat2& matrix) const
    {
        glUniformMatrix2fv(
            glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &matrix[0][0]);
    }

    void setMat3(const std::string& name, const glm::mat3& matrix) const
    {
        glUniformMatrix3fv(
            glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &matrix[0][0]);
    }

    void setMat4(const std::string& name, const glm::mat4& matrix) const
    {
        glUniformMatrix4fv(
            glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &matrix[0][0]);
    }

private:
    static void checkCompileErrors(unsigned int shader, const std::string& type)
    {
        int success = 0;
        char infoLog[1024] = {};

        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (success == 0)
            {
                glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
                std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type
                          << '\n' << infoLog
                          << "\n-- --------------------------------------------------- --\n";
            }
            return;
        }

        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (success == 0)
        {
            glGetProgramInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type
                      << '\n' << infoLog
                      << "\n-- --------------------------------------------------- --\n";
        }
    }
};

#endif
