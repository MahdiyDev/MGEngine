#define GLFW_INCLUDE_NONE
#include "mge.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Shader.h"

#define ENABLE_DUAL_GPU

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

int main(int argc, char* argv[])
{
    Init_Window(WIDTH, HEIGHT, "MGEngine v1.0");

    float vertices[] = {
        -0.5f, -0.5f, -0.5f, +0.0f, +0.0f,
        +0.5f, -0.5f, -0.5f, +1.0f, +0.0f,
        +0.5f, +0.5f, -0.5f, +1.0f, +1.0f,
        +0.5f, +0.5f, -0.5f, +1.0f, +1.0f,
        -0.5f, +0.5f, -0.5f, +0.0f, +1.0f,
        -0.5f, -0.5f, -0.5f, +0.0f, +0.0f,

        -0.5f, -0.5f, +0.5f, +0.0f, +0.0f,
        +0.5f, -0.5f, +0.5f, +1.0f, +0.0f,
        +0.5f, +0.5f, +0.5f, +1.0f, +1.0f,
        +0.5f, +0.5f, +0.5f, +1.0f, +1.0f,
        -0.5f, +0.5f, +0.5f, +0.0f, +1.0f,
        -0.5f, -0.5f, +0.5f, +0.0f, +0.0f,

        -0.5f, +0.5f, +0.5f, +1.0f, +0.0f,
        -0.5f, +0.5f, -0.5f, +1.0f, +1.0f,
        -0.5f, -0.5f, -0.5f, +0.0f, +1.0f,
        -0.5f, -0.5f, -0.5f, +0.0f, +1.0f,
        -0.5f, -0.5f, +0.5f, +0.0f, +0.0f,
        -0.5f, +0.5f, +0.5f, +1.0f, +0.0f,

        +0.5f, +0.5f, +0.5f, +1.0f, +0.0f,
        +0.5f, +0.5f, -0.5f, +1.0f, +1.0f,
        +0.5f, -0.5f, -0.5f, +0.0f, +1.0f,
        +0.5f, -0.5f, -0.5f, +0.0f, +1.0f,
        +0.5f, -0.5f, +0.5f, +0.0f, +0.0f,
        +0.5f, +0.5f, +0.5f, +1.0f, +0.0f,

        -0.5f, -0.5f, -0.5f, +0.0f, +1.0f,
        +0.5f, -0.5f, -0.5f, +1.0f, +1.0f,
        +0.5f, -0.5f, +0.5f, +1.0f, +0.0f,
        +0.5f, -0.5f, +0.5f, +1.0f, +0.0f,
        -0.5f, -0.5f, +0.5f, +0.0f, +0.0f,
        -0.5f, -0.5f, -0.5f, +0.0f, +1.0f,

        -0.5f, +0.5f, -0.5f, +0.0f, +1.0f,
        +0.5f, +0.5f, -0.5f, +1.0f, +1.0f,
        +0.5f, +0.5f, +0.5f, +1.0f, +0.0f,
        +0.5f, +0.5f, +0.5f, +1.0f, +0.0f,
        -0.5f, +0.5f, +0.5f, +0.0f, +0.0f,
        -0.5f, +0.5f, -0.5f, +0.0f, +1.0f
    };

    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3, // second triangle
    };

    int width, height, rnChannels;
    unsigned char* data = stbi_load("assests/wall.jpg", &width, &height, &rnChannels, 0);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    Shader newShader(
        "shaders/shader.vert",
        "shaders/shader.frag"
	);

    GLuint VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    Clear_Background(Color {51, 76, 76, 255});

    while (!Window_Should_Close()) {
        Begin_Drawing();

		Clear_Background(Color {51, 76, 76, 255});

        glDrawArrays(GL_TRIANGLES, 0, 36);

        newShader.Use();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, (float)glfwGetTime() * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));

        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(
            glm::radians(45.0f),
            (float)WIDTH / (float)HEIGHT,
            0.1f, 100.0f
		);

        newShader.Set_Mat4("model", model);
        newShader.Set_Mat4("view", view);
        newShader.Set_Mat4("projection", projection);

        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        End_Drawing();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    newShader.CleanUp();

    Close_Window();

    return 0;
}
