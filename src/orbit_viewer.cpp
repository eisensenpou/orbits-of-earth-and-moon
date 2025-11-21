/*************************
 * File: orbit_viewer.cpp
 * Author: Sinan Demir
 * Date: 11/20/2025
 * Purpose: Basic OpenGL application to visualize orbits.
 *************************/

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

static void framebuffer_size_callback(GLFWwindow* win, int w, int h) {
    glViewport(0, 0, w, h);
}

int main() {
    // --- Initialize GLFW ---
    if (!glfwInit()) {
        std::cerr << "❌ Failed to init GLFW\n";
        return -1;
    }

    // OpenGL version 3.3 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // --- Create window ---
    GLFWwindow* window = glfwCreateWindow(800, 600, "Orbit Viewer", nullptr, nullptr);
    if (!window) {
        std::cerr << "❌ Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // --- Load OpenGL functions via GLAD ---
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "❌ Failed to initialize GLAD\n";
        return -1;
    }

    glViewport(0, 0, 800, 600);

    // --- Main loop ---
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO: Render orbits here

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
