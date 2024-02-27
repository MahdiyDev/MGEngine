#include "window.h"

#include <cstdlib>
#include <glad/glad.h>
#include <iostream>

bool drawLines = false;

void Window::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void Window::CreateWindow(const uint32_t width, const uint32_t height, const char* title)
{
    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window == NULL) {
        std::cout << "glfwCreateWindow error" << std::endl;
        glfwTerminate();
        exit(1);
    }
    glfwMakeContextCurrent(window);
}

void Window::CloseWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::SwapBuffers()
{
    glfwSwapBuffers(window);
}

bool Window::WindowShouldClose()
{
    return glfwWindowShouldClose(window);
}

Window::Window()
{
    glfwSetKeyCallback(window, ProcessInput);
}

void Window::ProcessInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        if (drawLines) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        drawLines = !drawLines;
    }
}

void Window::EnableVSync()
{
    glfwSwapInterval(1);
}

void Window::HandleEvents()
{
    glfwPollEvents();
}
