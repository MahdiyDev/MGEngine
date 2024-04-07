#include "Shader.h"
#include "mge_math.h"
#include "mge_utils.h"
#include <fstream>
#include <sstream>

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
            typeName.c_str(), infoLog);
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
            "Shader: program not created\n%s", infoLog);
    }
    TRACE_LOG(LOG_INFO, "Shader: program created");

	// Deleting shaders
    glDeleteShader(vertex);
    glDeleteShader(fragment);

	// Gen VAO to contain VBO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

	// Gen and fill vertex buffer (VBO)
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
}

void Shader::Set_Position_Buffer(float* vertices, int size)
{
    glBufferData(GL_ARRAY_BUFFER, size*sizeof(float), vertices, GL_STATIC_DRAW);

	// Bind vertex attributes (position)
	glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0); // Positions
}

void Shader::Set_Texcoord_Buffer(float* vertices, int size)
{
}

void Shader::Set_Color_Buffer(unsigned char* vertices, int size)
{
	glBufferData(GL_ARRAY_BUFFER, size*sizeof(unsigned char), vertices, GL_STATIC_DRAW);

	// Bind vertex attributes (color)
	glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0); // Colors
}

void Shader::DrawArrays(unsigned int mode, int offset, int count)
{
    glBindVertexArray(VAO);
    glDrawArrays(mode, offset, count);
	glBindVertexArray(0);
}

void Shader::DrawElements(unsigned int mode, int count, unsigned int type, const void *indices)
{
	glDrawElements(mode, count, type, (GLvoid*)indices);
}

void Shader::Use()
{
    glUseProgram(ID);
}

void Shader::Set_Bool(const std::string& name, const bool value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::Set_Int(const std::string& name, const int value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::Set_Float(const std::string& name, const float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::Set_Mat4(const std::string& name, const Matrix& value) const
{
    float matValue[16] = {
        value.m0, value.m1, value.m2, value.m3,
        value.m4, value.m5, value.m6, value.m7,
        value.m8, value.m9, value.m10, value.m11,
        value.m12, value.m13, value.m14, value.m15
    };
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, matValue);
}

void Shader::Set_Vec3(const std::string& name, const Vector3& value) const
{
    float vectorValue[3] = {
        value.x, value.y, value.z
    };
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, vectorValue);
}

void Shader::Set_Vec4(const std::string& name, const Vector4& value) const
{
    float vectorValue[4] = {
        value.x, value.y, value.z, value.w
    };
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, vectorValue);
}

void Shader::CleanUp()
{
    glDeleteProgram(ID);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}
