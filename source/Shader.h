#pragma once

#include "mge_math.h"

#include <string>
#include <vector>

class Shader
{
private:

public:
	Shader(const char* vertexPath, const char* fragmnetPath);

	const std::string ReadFile(const char* filePath) const;
	void CreateShaderProgram(unsigned int vertex, unsigned int fragment);
	unsigned int LoadShader(const char* code, unsigned int shaderType, std::string typeName);
	void DrawArrays(int mode, int count);
	void Use();
	void Set_Position_Buffer(float* vertices, int size);
	void Set_Texcoord_Buffer(float* vertices, int size);
	void Set_Color_Buffer(float* vertices, int size);
	void Set_Bool(const std::string& name, const bool value) const;
	void Set_Int(const std::string& name, const int value) const;
	void Set_Float(const std::string& name, const float value) const;
	void Set_Mat4(const std::string& name, const Matrix& value) const;
	void Set_Vec3(const std::string& name, const Vector3& value) const;
	void Set_Vec4(const std::string& name, const Vector4& value) const;
	void CleanUp();

    unsigned int VAO, VBO;
	unsigned int ID;
};
