#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "mge_config.h"
#include "mge_math.h"
#include <glad/glad.h>
#include <cstdint>
#include <stdarg.h>
#include <vector>

#define MGE_VERSION "v0.1"

#if defined(__cplusplus)
#define CLITERAL(type) type
#else
#define CLITERAL(type) (type)
#endif

// Callbacks to hook some internal functions
typedef void (*Trace_Log_Callback)(int logLevel, const char* text, va_list args); // Logging: Redirect trace log messages

typedef enum {
    LOG_ALL = 0, // Display all logs
    LOG_TRACE, // Trace logging, intended for internal use only
    LOG_DEBUG, // Debug logging, used for internal debugging, it should be disabled on release builds
    LOG_INFO, // Info logging, used for program execution info
    LOG_WARNING, // Warning logging, used on recoverable failures
    LOG_ERROR, // Error logging, used on unrecoverable failures
    LOG_FATAL, // Fatal logging, used to abort program: exit(EXIT_FAILURE)
    LOG_NONE // Disable logging
} TraceLogLevel;

typedef struct Color {
    unsigned char r; // Color red value
    unsigned char g; // Color green value
    unsigned char b; // Color blue value
    unsigned char a; // Color alpha value
} Color;

#define RED \
    CLITERAL(Color) { 255, 0, 0, 255 }
#define GREEN \
    CLITERAL(Color) { 0, 255, 0, 255 }
#define BLUE \
    CLITERAL(Color) { 0, 0, 255, 255 }
#define BLACK \
    CLITERAL(Color) { 0, 0, 0, 255 }
#define BLACK \
    CLITERAL(Color) { 0, 0, 0, 255 }
#define LIGHTGRAY \
    CLITERAL(Color) { 200, 200, 200, 255 }
#define GRAY \
    CLITERAL(Color) { 130, 130, 130, 255 }
#define DARKGRAY \
    CLITERAL(Color) { 80, 80, 80, 255 }

void Trace_Log(int logType, const char* text, ...);

void Init_Window(uint32_t width, uint32_t height, const char* title);
bool Window_Should_Close(void);
void Close_Window(void);
float Get_Time(void);

void Clear_Background(Color color);
void Begin_Drawing(void);
void End_Drawing(void);

void Draw_Line(void);
void Load_Shader(Vector3 startPos, Vector3 endPos, Color color);

class Line {
    int shaderProgram;
    unsigned int VBO, VAO;
    std::vector<float> vertices;
    glm::vec3 startPoint;
    glm::vec3 endPoint;
    glm::mat4 MVP;
    glm::vec3 lineColor;
public:
    Line(glm::vec3 start, glm::vec3 end) {

        startPoint = start;
        endPoint = end;
        lineColor = glm::vec3(1,1,1);
        MVP = glm::mat4(1.0f);

        const char *vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "uniform mat4 MVP;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = MVP * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
            "}\0";
        const char *fragmentShaderSource = "#version 330 core\n"
            "out vec4 FragColor;\n"
            "uniform vec3 color;\n"
            "void main()\n"
            "{\n"
            "   FragColor = vec4(color, 1.0f);\n"
            "}\n\0";

        // vertex shader
        int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        // check for shader compile errors

        // fragment shader
        int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        // check for shader compile errors

        // link shaders
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        // check for linking errors

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        vertices = {
             start.x, start.y, start.z,
             end.x, end.y, end.z,

        };
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0); 

    }

    int setMVP(glm::mat4 mvp) {
        MVP = mvp;
        return 1;
    }

    int setColor(glm::vec3 color) {
        lineColor = color;
        return 1;
    }

    int draw() {
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, &MVP[0][0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, &lineColor[0]);

        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, 2);
        return 1;
    }

    ~Line() {

        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }
};
