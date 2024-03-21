#pragma once

#include <glad/glad.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

class Shader
{
private:

public:
	Shader(const char* vertexPath, const char* fragmnetPath);

	const std::string ReadFile(const char* filePath) const;
	void CreateShaderProgram(unsigned int vertex, unsigned int fragment);
	unsigned int LoadShader(const char* code, GLenum shaderType, std::string typeName);
	void Set_Vertices(std::vector<float>&, size_t each_size, size_t actual_each_size);
	void DrawArrays(void);
	void Use();
	void SetValue(const std::string& name, bool value) const;
	void SetValue(const std::string& name, int value) const;
	void SetValue(const std::string& name, float value) const;
	void SetMat4(const std::string& name, glm::mat4& value) const;
	void SetVec3(const std::string& name, glm::vec3& value) const;
	void CleanUp();

    GLuint VAO, VBO;
	unsigned int ID;
};
