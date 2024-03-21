#include "Shader.h"
#include "mge_utils.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

Shader::Shader(const char* vertexPath, const char* fragmnetPath)
{
    std::string vertexCode = ReadFile(vertexPath);
    std::string fragmentCode = ReadFile(fragmnetPath);

    unsigned int vertex = LoadShader(
        vertexCode.c_str(), GL_VERTEX_SHADER, "vertex");
    unsigned int fragment = LoadShader(
        fragmentCode.c_str(), GL_FRAGMENT_SHADER, "fragment");

    CreateShaderProgram(vertex, fragment);
}

const std::string Shader::ReadFile(const char* filePath) const
{
    std::string line;
    std::stringstream streamCode;
    std::ifstream myfile(filePath);

    if (myfile.is_open()) {
        TRACE_LOG(LOG_INFO, "Shader: open file: %s", filePath);

        while (getline(myfile, line)) {
            streamCode << line << "\n";
        }
        myfile.close();
    } else {
        TRACE_LOG(LOG_ERROR, "Shader: Unable to open file: %s", filePath);
    }

    return streamCode.str();
}

unsigned int Shader::LoadShader(const char* code, GLenum shaderType, std::string typeName)
{
    unsigned int shader;
    int success;
    char infoLog[512];

    shader = glCreateShader(shaderType);
	TRACE_LOG(LOG_INFO, "Shader: %s shader created", typeName.c_str());
    if (shader == 0) {
        TRACE_LOG(LOG_ERROR, "Shader: Unable to create shader: %s", typeName.c_str());
        return EXIT_FAILURE;
    }
    glShaderSource(shader, 1, &code, NULL);
    glCompileShader(shader);
	TRACE_LOG(LOG_INFO, "Shader: %s shader compiled", typeName.c_str());

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        TRACE_LOG(LOG_ERROR, 
            "Shader: %s shader not compiled Error: %s", 
            typeName.c_str(), infoLog
        );
    }
    return shader;
}

void Shader::CreateShaderProgram(unsigned int vertex, unsigned int fragment)
{
    int success;
    char infoLog[512];

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        TRACE_LOG(LOG_ERROR, 
            "Shader: program not created\n%s", infoLog
        );
    }
	TRACE_LOG(LOG_INFO, "Shader: program created");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::Set_Vertices(std::vector<float>& vertices, size_t each_size, size_t line_size)
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);	
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);	
	glVertexAttribPointer(0, each_size, GL_FLOAT, GL_FALSE, line_size * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	glBindVertexArray(0); 
}

void Shader::DrawArrays(void)
{
	glBindVertexArray(VAO);
	glDrawArrays(GL_LINES, 0, 2);
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

void Shader::SetMat4(const std::string& name, glm::mat4& value) const
{
	glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void Shader::SetVec3(const std::string& name, glm::vec3& value) const
{
	glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::CleanUp()
{
    glDeleteProgram(ID);
}
