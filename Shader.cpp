#include "Shader.h"

Shader::Shader(const char* vertexPath, const char* fragmnetPath)
{
	std::string vertexCode = ReadFile(vertexPath);
	std::string fragmentCode = ReadFile(fragmnetPath);

	unsigned int vertex = LoadShader(
		vertexCode.c_str(), GL_VERTEX_SHADER, "vertex"
	);
	unsigned int fragment = LoadShader(
		fragmentCode.c_str(), GL_FRAGMENT_SHADER, "fragment"
	);

	CreateShaderProgram(&vertex, &fragment);
}

const std::string Shader::ReadFile(const char* filePath) const
{
	std::string line;
	std::stringstream streamCode;
	std::ifstream myfile(filePath);

	if (myfile.is_open())
	{
		while (getline(myfile, line))
		{
			streamCode << line << "\n";
		}
		myfile.close();
	}
	else {
		std::cout << "Unable to open file: " << filePath << std::endl;
	}

	return streamCode.str();
}

unsigned int Shader::LoadShader(const char* code, GLenum shaderType, std::string typeName)
{
	unsigned int shader;
	int success;
	char infoLog[512];

	shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &code, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cout
			<< "ERROR::SHADER::" << typeName << "::COMPILATION_FAILED\n"
			<< infoLog
			<< std::endl;
	}
	return shader;
}

void Shader::CreateShaderProgram(unsigned int* vertex, unsigned int* fragment)
{
	int success;
	char infoLog[512];

	ID = glCreateProgram();
	glAttachShader(ID, *vertex);
	glAttachShader(ID, *fragment);
	glLinkProgram(ID);

	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cout
			<< "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n"
			<< infoLog
			<< std::endl;
	}

	glDeleteShader(*vertex);
	glDeleteShader(*fragment);
}

void Shader::Use()
{
	glUseProgram(ID);
}

void Shader::SetValue(const std::string& name, bool value) const
{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::SetValue(const std::string& name, int value) const
{
	glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::SetValue(const std::string& name, float value) const
{
	glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::CleanUp()
{
	glDeleteProgram(ID);
}
