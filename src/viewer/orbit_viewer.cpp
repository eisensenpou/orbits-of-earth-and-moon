/*************************
 * File: orbit_viewer.cpp
 * Author: Sinan Demir
 * Date: 11/20/2025
 * Purpose: Basic OpenGL application to visualize orbits.
 *************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// glm is header-only and already installed on your system
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ----------------------
// Types & Globals
// ----------------------
/************
 * Frame
 * Represents a single frame of simulation data
 ************/
struct Frame {
    glm::vec3 sun;
    glm::vec3 earth;
    glm::vec3 moon;
};

static int g_windowWidth  = 800;
static int g_windowHeight = 600;
static std::vector<Frame> g_frames;
// Orbit camera state
static float g_yaw   = glm::radians(45.0f);  // around Y axis
static float g_pitch = glm::radians(20.0f);  // up/down
static float g_radius = 400.0f;              // distance from origin

static bool   g_mouseRotating = false;
static double g_lastMouseX    = 0.0;
static double g_lastMouseY    = 0.0;

// Advance 1 frame per render for now
static size_t g_frameIndex = 0;

// ----------------------
// Callbacks
// ----------------------
/*****************
 * framebuffer_size_callback
 * @param win GLFW window pointer
 * @param w New width
 * @param h New height
 * @note Updates the viewport on window resize
 ******************/
static void framebuffer_size_callback(GLFWwindow* win, int w, int h) {
    g_windowWidth  = w;
    g_windowHeight = h;
    glViewport(0, 0, w, h);
}

/*****************
 * mouse_button_callback
 * @param window GLFW window pointer
 * @param button Mouse button
 * @param action Press/release
 * @param mods Modifier keys
 * @note Starts/stops camera rotation on right mouse button
 ******************/
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            g_mouseRotating = true;
            glfwGetCursorPos(window, &g_lastMouseX, &g_lastMouseY);
        } else if (action == GLFW_RELEASE) {
            g_mouseRotating = false;
        }
    }
}

/*****************
 * cursor_pos_callback
 * @param window GLFW window pointer
 * @param xpos New cursor X position
 * @param ypos New cursor Y position
 * @note Updates camera yaw/pitch when rotating
 ******************/
static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!g_mouseRotating) return;

    double dx = xpos - g_lastMouseX;
    double dy = ypos - g_lastMouseY;
    g_lastMouseX = xpos;
    g_lastMouseY = ypos;

    // sensitivity scaling
    float sensitivity = 0.005f;
    g_yaw   += static_cast<float>(dx) * sensitivity;
    g_pitch -= static_cast<float>(dy) * sensitivity; // invert so dragging up looks down

    // clamp pitch to avoid flipping over the top
    const float pitchLimit = glm::radians(89.0f);
    if (g_pitch > pitchLimit)  g_pitch = pitchLimit;
    if (g_pitch < -pitchLimit) g_pitch = -pitchLimit;
}

/***************
 * scroll_callback
 * @param window GLFW window pointer
 * @param xoffset Scroll in X (unused)
 * @param yoffset Scroll in Y
 * @note Zooms the camera in/out
 ****************/
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // zoom in/out
    g_radius -= static_cast<float>(yoffset) * 10.0f;
    if (g_radius < 50.0f)   g_radius = 50.0f;
    if (g_radius > 2000.0f) g_radius = 2000.0f;
}


/*******************
 * loadCSV
 * @param path Path to CSV file
 * @return Vector of Frames loaded from the file
 * @exception Logs errors to stderr if file cannot be opened
 * @note CSV format:
 *  step, x_sun, y_sun, z_sun, x_earth, y_earth, z_earth, x_moon, y_moon, z_moon
 * (all in meters)
 *******************/
std::vector<Frame> loadCSV(const std::string& path) {
    std::vector<Frame> frames;

    std::ifstream file(path);
    if (!file) {
        std::cerr << "❌ Cannot open CSV: " << path << "\n";
        return frames;
    }

    std::string line;
    // Skip header
    if (!std::getline(file, line)) {
        std::cerr << "❌ CSV appears empty: " << path << "\n";
        return frames;
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        Frame f;

        int step;
        char comma;

        // Read: step, x_sun, y_sun, z_sun, x_earth, y_earth, z_earth, x_moon, y_moon, z_moon
        ss >> step >> comma
           >> f.sun.x   >> comma >> f.sun.y   >> comma >> f.sun.z   >> comma
           >> f.earth.x >> comma >> f.earth.y >> comma >> f.earth.z >> comma
           >> f.moon.x  >> comma >> f.moon.y  >> comma >> f.moon.z;

        // Scale meters → something reasonable for OpenGL
        // 1.5e11 m → 150.0 units if scale = 1e-9
        float scale = 1.0f / 1e9f;
        f.sun   *= scale;
        f.earth *= scale;
        f.moon  *= scale;

        frames.push_back(f);
    }

    std::cout << "✅ Loaded " << frames.size()
              << " frames from " << path << "\n";

    return frames;
}

/********************
 * compileShader
 * @param type Shader type (GL_VERTEX_SHADER, etc)
 * @param src Shader source code
 * @return Shader ID
 * @exception Logs errors to stderr if compilation fails
 *******************/
static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        std::cerr << "❌ Shader compile error:\n" << log << "\n";
    }

    return shader;
}

/****************
 * Create Program
 * @param vsSrc Vertex shader source
 * @param fsSrc Fragment shader source
 * @return Program ID
 * @exception Logs errors to stderr if compilation/linking fails
 ****************/
static GLuint createProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetProgramInfoLog(prog, logLen, nullptr, log.data());
        std::cerr << "❌ Program link error:\n" << log << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return prog;
}

/**************
 * Main
 *************/
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
    GLFWwindow* window = glfwCreateWindow(g_windowWidth, g_windowHeight,
                                          "Orbit Viewer", nullptr, nullptr);
    if (!window) {
        std::cerr << "❌ Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // --- Load OpenGL functions via GLAD ---
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "❌ Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, g_windowWidth, g_windowHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE); // allow gl_PointSize in shader

    // --- Load simulation data ---
    // For now, fixed path. We can add CLI args later.
    g_frames = loadCSV("orbit_three_body.csv");
    if (g_frames.empty()) {
        std::cerr << "⚠️ No frames loaded. Showing empty scene.\n";
    }

    // --- Simple shaders (MVP + per-body color) ---

    const char* vsSrc = R"GLSL(
        #version 330 core
        layout(location = 0) in vec3 inPos;

        uniform mat4 uMVP;
        uniform float uPointSize;

        void main() {
            gl_Position = uMVP * vec4(inPos, 1.0);
            gl_PointSize = uPointSize;
        }
    )GLSL";

    const char* fsSrc = R"GLSL(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 uColor;

        void main() {
            FragColor = vec4(uColor, 1.0);
        }
    )GLSL";

    GLuint program = createProgram(vsSrc, fsSrc);
    GLint locMVP       = glGetUniformLocation(program, "uMVP");
    GLint locColor     = glGetUniformLocation(program, "uColor");
    GLint locPointSize = glGetUniformLocation(program, "uPointSize");

    // --- Geometry setup: a dynamic VBO for 3 points ---

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Allocate for 3 vec3 = 9 floats
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);



    // --- Main loop ---
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Advance frame index (simple loop; later we can use time-based)
        if (!g_frames.empty()) {
            g_frameIndex = (g_frameIndex + 1) % g_frames.size();
        }

        // Clear screen
        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Compute matrices
        float aspect = (g_windowWidth > 0 && g_windowHeight > 0)
                       ? (float)g_windowWidth / (float)g_windowHeight
                       : 4.0f / 3.0f;

        // Compute camera position from spherical coords
        glm::vec3 camPos;
        camPos.x = g_radius * std::cos(g_pitch) * std::sin(g_yaw);
        camPos.y = g_radius * std::sin(g_pitch);
        camPos.z = g_radius * std::cos(g_pitch) * std::cos(g_yaw);

        glm::vec3 camTarget = glm::vec3(0.0f);        // look at origin
        glm::vec3 camUp     = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                          aspect,
                                          0.1f,
                                          2000.0f);

        glm::mat4 view = glm::lookAt(camPos, camTarget, camUp);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 mvp = proj * view * model;

        glUseProgram(program);
        glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
        glBindVertexArray(vao);

        if (!g_frames.empty()) {
            const Frame& f = g_frames[g_frameIndex];

            float data[9];

            // --- Draw Sun ---
            data[0] = f.sun.x;
            data[1] = f.sun.y;
            data[2] = f.sun.z;
            // Reuse the same VBO for a single point
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3, data);

            glUniform3f(locColor, 1.0f, 0.9f, 0.2f); // yellowish
            glUniform1f(locPointSize, 12.0f);
            glDrawArrays(GL_POINTS, 0, 1);

            // --- Draw Earth ---
            data[0] = f.earth.x;
            data[1] = f.earth.y;
            data[2] = f.earth.z;
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3, data);

            glUniform3f(locColor, 0.2f, 0.5f, 1.0f); // blue
            glUniform1f(locPointSize, 8.0f);
            glDrawArrays(GL_POINTS, 0, 1);

            // --- Draw Moon ---
            data[0] = f.moon.x;
            data[1] = f.moon.y;
            data[2] = f.moon.z;
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 3, data);

            glUniform3f(locColor, 0.8f, 0.8f, 0.8f); // gray
            glUniform1f(locPointSize, 5.0f);
            glDrawArrays(GL_POINTS, 0, 1);
        }

        glBindVertexArray(0);
        glUseProgram(0);

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwTerminate();
    return 0;
}