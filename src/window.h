#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <cstdint>

class Window {
private:
    GLFWwindow* window;

    static void ProcessInput(GLFWwindow* window, int key, int scancode, int action, int mods);

public:
    Window();
    void Init();
    void CreateWindow(const uint32_t width, const uint32_t height, const char* title);
    void CloseWindow();
    void SwapBuffers();
    bool WindowShouldClose();
    void EnableVSync();
    void HandleEvents();
};
