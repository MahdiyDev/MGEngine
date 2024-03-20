#pragma once

#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
private:
    GLuint VAO, VBO;
	const std::string ReadFile(const char* filePath) const;
	unsigned int LoadShader(const char* code, GLenum shaderType, std::string typeName);
	void CreateShaderProgram(unsigned int* vertex, unsigned int* fragment);

public:
	Shader(const char* vertexPath, const char* fragmnetPath);

	void Set_Vertices(void* vertices, size_t each_size, size_t actual_each_size);
	void DrawArrays(void);
	void Use();
	void SetValue(const std::string& name, bool value) const;
	void SetValue(const std::string& name, int value) const;
	void SetValue(const std::string& name, float value) const;
	void SetValue(const std::string& name, glm::mat4 value) const;
	void SetValue(const std::string& name, glm::vec3 value) const;
	void CleanUp();

	unsigned int ID;
};
