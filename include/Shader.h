#pragma once

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
private:
	const std::string ReadFile(const char* filePath) const;
	unsigned int LoadShader(const char* code, GLenum shaderType, std::string typeName);
	void CreateShaderProgram(unsigned int* vertex, unsigned int* fragment);

public:
	Shader(const char* vertexPath, const char* fragmnetPath);

	void Use();
	void SetValue(const std::string& name, bool value) const;
	void SetValue(const std::string& name, int value) const;
	void SetValue(const std::string& name, float value) const;
	void CleanUp();

	unsigned int ID;
};
