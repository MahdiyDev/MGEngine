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
	void DrawArrays(void);
	void Use();
	void Set_Position_Buffer(std::vector<float>& vertices);
	void Set_Texcoord_Buffer(std::vector<float>& vertices);
	void Set_Color_Buffer(std::vector<float>& vertices);
	void Set_Bool(const std::string& name, bool value) const;
	void Set_Int(const std::string& name, int value) const;
	void Set_Float(const std::string& name, float value) const;
	void Set_Mat4(const std::string& name, glm::mat4& value) const;
	void Set_Vec3(const std::string& name, glm::vec3& value) const;
	void CleanUp();

    GLuint VAO, VBO;
	unsigned int ID;
};
