/*************************
 * File: orbit_viewer.cpp
 * Author: Sinan Demir
 * Date: 11/27/2025
 * Purpose: OpenGL 3D viewer for Earth–Moon–Sun orbits.
 *
 * Notes:
 *  - Uses option C lighting (ambient-boosted Lambert + rim)
 *  - Sun, Earth, Moon drawn as UV spheres (SphereMesh)
 *  - Orbital scaling / exaggeration is handled in CSVLoader
 *  - Right mouse drag = orbit camera, scroll = zoom
 *  - Simple HUD legend (Sun/Earth/Moon) in top-left
 *************************/

#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "viewer/csv_loader.h"
#include "viewer/sphere_mesh.h"

// ---------------------------
// Global Viewer State
// ---------------------------
static int   g_windowWidth  = 1280;
static int   g_windowHeight = 720;

// Orbit camera state
static float g_yaw   = glm::radians(45.0f);   // around Y axis
static float g_pitch = glm::radians(20.0f);   // up/down
static float g_radius = 400.0f;               // distance from origin

static bool   g_mouseRotating = false;
static double g_lastMouseX = 0.0;
static double g_lastMouseY = 0.0;

// Simulation frames
static size_t g_frameIndex = 0;
static std::vector<Frame> g_frames;

// Legend rendering objects (2D)
static GLuint g_legendShader = 0;
static GLuint g_legendVAO    = 0;
static GLuint g_legendVBO    = 0;
static GLint  g_legLocOffset = -1;
static GLint  g_legLocScale  = -1;
static GLint  g_legLocColor  = -1;


// --------------------------------------------------
// Callbacks
// --------------------------------------------------
/**
 * @brief Handles window resize events by updating the viewport.
 *
 * @param The GLFW window.
 * @param w The new width of the window.
 * @param h The new height of the window.
 */
static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    g_windowWidth  = w;
    g_windowHeight = h;
    glViewport(0, 0, w, h);
}

/**
 * @brief Handles mouse button events for initiating or stopping rotation.
 *
 * Right mouse button: hold & drag to orbit.
 *
 * @param win   The GLFW window.
 * @param button The mouse button that was pressed or released.
 * @param action The action (press or release).
 * @param mods   Modifier keys (not used).
 */
static void mouse_button_callback(GLFWwindow* win, int button, int action, int /*mods*/) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            g_mouseRotating = true;
            glfwGetCursorPos(win, &g_lastMouseX, &g_lastMouseY);
        } else if (action == GLFW_RELEASE) {
            g_mouseRotating = false;
        }
    }
}

/**
 * @brief Handles cursor position changes for mouse-based rotation.
 *
 * @param xpos The new x-coordinate of the cursor.
 * @param ypos The new y-coordinate of the cursor.
 */
static void cursor_pos_callback(GLFWwindow*, double xpos, double ypos) {
    if (!g_mouseRotating) return;

    double dx = xpos - g_lastMouseX;
    double dy = ypos - g_lastMouseY;

    g_lastMouseX = xpos;
    g_lastMouseY = ypos;

    const float sensitivity = 0.005f;
    g_yaw   += static_cast<float>(dx) * sensitivity;
    g_pitch -= static_cast<float>(dy) * sensitivity;

    // Clamp pitch to avoid flipping over the poles
    const float pitchLimit = glm::radians(89.0f);
    g_pitch = glm::clamp(g_pitch, -pitchLimit, pitchLimit);
}

/**
 * @brief Handles scroll wheel input for zooming the camera.
 *
 * @param yoff Scroll amount in Y (positive = scroll up).
 */
static void scroll_callback(GLFWwindow*, double /*xoff*/, double yoff) {
    g_radius -= static_cast<float>(yoff) * 10.0f;
    g_radius  = glm::clamp(g_radius, 50.0f, 2000.0f);
}


// --------------------------------------------------
// Shader helpers
// --------------------------------------------------
/**
 * @brief Compiles a shader of given type from source code.
 *
 * @param type The type of shader (e.g., GL_VERTEX_SHADER).
 * @param src  The source code of the shader.
 * @return GLuint The compiled shader object.
 */
static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::cerr << "❌ Shader error:\n" << log << "\n";
    }
    return s;
}

/**
 * @brief Creates an OpenGL program object from vertex and fragment shader sources.
 *
 * @param vsSrc Vertex shader source code.
 * @param fsSrc Fragment shader source code.
 * @return GLuint The created OpenGL program object.
 */
static GLuint createProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER,   vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::cerr << "❌ Link error:\n" << log << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}


// --------------------------------------------------
// Legend helpers (2D quads in NDC)
// --------------------------------------------------

/**
 * @brief Initializes the legend renderer (tiny colored quads in NDC).
 *
 * Builds a single unit square mesh we can reuse for each legend box.
 */
static void initLegendRenderer() {
    const char* legendVs = R"GLSL(
        #version 330 core
        layout(location = 0) in vec2 aPos;

        uniform vec2 uOffset;  // NDC center
        uniform vec2 uScale;   // NDC scale

        void main() {
            vec2 pos = aPos * uScale + uOffset;
            gl_Position = vec4(pos, 0.0, 1.0);
        }
    )GLSL";

    const char* legendFs = R"GLSL(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 uColor;

        void main() {
            FragColor = vec4(uColor, 1.0);
        }
    )GLSL";

    g_legendShader = createProgram(legendVs, legendFs);

    g_legLocOffset = glGetUniformLocation(g_legendShader, "uOffset");
    g_legLocScale  = glGetUniformLocation(g_legendShader, "uScale");
    g_legLocColor  = glGetUniformLocation(g_legendShader, "uColor");

    // Unit square centered at origin, two triangles
    float quadVerts[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,

        -0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f
    };

    glGenVertexArrays(1, &g_legendVAO);
    glGenBuffers(1, &g_legendVBO);

    glBindVertexArray(g_legendVAO);
    glBindBuffer(GL_ARRAY_BUFFER, g_legendVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0
    );

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
 * @brief Draws a small colored box at a specified pixel position on the screen.
 *
 * @param centerPx X coordinate of the box center in pixels (from left).
 * @param centerPy Y coordinate of the box center in pixels (from top).
 * @param sizePx   Box size in pixels (width = height).
 * @param color    Box color as an RGB vector.
 */
static void drawLegendBox(float centerPx, float centerPy,
                          float sizePx,
                          const glm::vec3& color) {
    if (g_windowWidth <= 0 || g_windowHeight <= 0) return;

    // Convert center in pixels → NDC
    float x_ndc =  2.0f * centerPx / static_cast<float>(g_windowWidth) - 1.0f;
    float y_ndc =  1.0f - 2.0f * centerPy / static_cast<float>(g_windowHeight);

    // Convert size in pixels → NDC scale (quad is [-0.5..0.5])
    float sx = sizePx / static_cast<float>(g_windowWidth)  * 2.0f;
    float sy = sizePx / static_cast<float>(g_windowHeight) * 2.0f;

    glUseProgram(g_legendShader);
    glUniform2f(g_legLocOffset, x_ndc, y_ndc);
    glUniform2f(g_legLocScale,  sx, sy);
    glUniform3fv(g_legLocColor, 1, glm::value_ptr(color));

    glBindVertexArray(g_legendVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}


// --------------------------------------------------
// MAIN
// --------------------------------------------------
/**
 * @brief Entry point for the Orbit Viewer application.
 *
 * Creates an OpenGL window, loads CSV data, builds sphere meshes,
 * and renders the Sun–Earth–Moon system with interactive camera
 * controls and a small HUD legend.
 */
int main() {
    // --- Initialize GLFW & window ---
    if (!glfwInit()) {
        std::cerr << "❌ Failed to init GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win =
        glfwCreateWindow(g_windowWidth, g_windowHeight,
                         "Orbit Viewer (3D Spheres)", nullptr, nullptr);

    if (!win) {
        std::cerr << "❌ Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetMouseButtonCallback(win, mouse_button_callback);
    glfwSetCursorPosCallback(win, cursor_pos_callback);
    glfwSetScrollCallback(win, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "❌ Failed to init GLAD\n";
        glfwDestroyWindow(win);
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // ----------------------------------------------------
    // Load CSV orbit frames (scaled / exaggerated in CSVLoader)
    // ----------------------------------------------------
    CSVLoader loader;
    g_frames = loader.loadOrbitCSV("../build/orbit_three_body.csv");

    if (g_frames.empty()) {
        std::cerr << "⚠️ No frames loaded.\n";
    }

    // ----------------------------------------------------
    // Create 3D mesh spheres
    // Radii are visualization-scale, not physical.
    // ----------------------------------------------------
    SphereMesh sunMesh;   sunMesh.build(5.0f,   48, 48);
    SphereMesh earthMesh; earthMesh.build(1.0f, 32, 32);
    SphereMesh moonMesh;  moonMesh.build(0.27f, 24, 24);


    // ----------------------------------------------------
    // Create main 3D shader (Option C lighting + rim)
    // ----------------------------------------------------
    const char* vsSrc = R"GLSL(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;

        uniform mat4 uMVP;
        uniform mat4 uModel;

        out vec3 vNormal;
        out vec3 vWorldPos;

        void main() {
            mat3 normalMat = mat3(transpose(inverse(uModel)));
            vNormal   = normalMat * aNormal;
            vWorldPos = vec3(uModel * vec4(aPos, 1.0));
            gl_Position = uMVP * vec4(aPos, 1.0);
        }
    )GLSL";

    const char* fsSrc = R"GLSL(
        #version 330 core

        in vec3 vNormal;
        in vec3 vWorldPos;

        out vec4 FragColor;

        uniform vec3 uColor;
        uniform vec3 uLightPos;
        uniform vec3 uViewPos;

        void main() {
            vec3 N = normalize(vNormal);
            vec3 L = normalize(uLightPos - vWorldPos);
            vec3 V = normalize(uViewPos - vWorldPos);
            vec3 H = normalize(L + V);

            // Lambert + Blinn–Phong specular
            float diff    = max(dot(N, L), 0.0);
            float spec    = pow(max(dot(N, H), 0.0), 32.0);
            float ambient = 0.18;

            vec3 base = uColor * (ambient + diff)
                      + vec3(0.4) * spec;

            // Rim light for cinematic look
            float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0);
            vec3 rimColor = vec3(0.3, 0.4, 0.9) * rim * 0.5;

            vec3 color = base + rimColor;

            // Simple gamma correction
            color = pow(color, vec3(1.0 / 2.2));

            FragColor = vec4(color, 1.0);
        }
    )GLSL";

    GLuint shader = createProgram(vsSrc, fsSrc);
    GLint locMVP     = glGetUniformLocation(shader, "uMVP");
    GLint locModel   = glGetUniformLocation(shader, "uModel");
    GLint locColor   = glGetUniformLocation(shader, "uColor");
    GLint locLight   = glGetUniformLocation(shader, "uLightPos");
    GLint locViewPos = glGetUniformLocation(shader, "uViewPos");

    // ----------------------------------------------------
    // Init legend renderer (2D colored boxes in NDC)
    // ----------------------------------------------------
    initLegendRenderer();

    // ----------------------------------------------------
    // Main render loop
    // ----------------------------------------------------
    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        if (!g_frames.empty())
            g_frameIndex = (g_frameIndex + 1) % g_frames.size();

        glClearColor(0.02f, 0.02f, 0.05f, 1.0f); // deep navy space
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (!g_frames.empty()) {
            const Frame& f = g_frames[g_frameIndex];

            // ---------------- Camera ----------------
            float aspect = float(g_windowWidth) / float(g_windowHeight);

            glm::vec3 camPos;
            camPos.x = g_radius * std::cos(g_pitch) * std::sin(g_yaw);
            camPos.y = g_radius * std::sin(g_pitch);
            camPos.z = g_radius * std::cos(g_pitch) * std::cos(g_yaw);

            glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                              aspect, 0.1f, 50000.0f);
            glm::mat4 view = glm::lookAt(camPos,
                                         glm::vec3(0.0f),
                                         glm::vec3(0, 1, 0));

            glUseProgram(shader);

            // View position for specular
            glUniform3fv(locViewPos, 1, glm::value_ptr(camPos));
            // Light at Sun position
            glUniform3fv(locLight,   1, glm::value_ptr(f.sun));

            // ---------------- Sun ----------------
            glm::mat4 model = glm::translate(glm::mat4(1.0f), f.sun);
            glm::mat4 mvp   = proj * view * model;
            glUniformMatrix4fv(locMVP,   1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(locColor, 1.4f, 1.1f, 0.3f);  // hot gold
            sunMesh.draw();

            // ---------------- Earth --------------
            model = glm::translate(glm::mat4(1.0f), f.earth);
            mvp   = proj * view * model;
            glUniformMatrix4fv(locMVP,   1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(locColor, 0.2f, 0.8f, 1.2f);  // teal NASA blue
            earthMesh.draw();

            // ---------------- Moon ---------------
            model = glm::translate(glm::mat4(1.0f), f.moon);
            mvp   = proj * view * model;
            glUniformMatrix4fv(locMVP,   1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(locColor, 0.85f, 0.85f, 0.92f); // pale white
            moonMesh.draw();
        }

        // ------------------------------------------------
        // HUD legend (Sun / Earth / Moon) – top-left
        // ------------------------------------------------
        glDisable(GL_DEPTH_TEST);

        float baseX   = 40.0f;  // pixels from left
        float baseY   = 40.0f;  // pixels from top
        float spacing = 22.0f;  // vertical spacing
        float sizePx  = 12.0f;  // box size

        // Sun icon (matches Sun color)
        drawLegendBox(baseX,             baseY,              sizePx,
                      glm::vec3(1.4f, 1.1f, 0.3f));

        // Earth icon
        drawLegendBox(baseX,             baseY + spacing,    sizePx,
                      glm::vec3(0.2f, 0.8f, 1.2f));

        // Moon icon
        drawLegendBox(baseX,             baseY + 2.0f*spacing,  sizePx,
                      glm::vec3(0.85f, 0.85f, 0.92f));

        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(win);
    }

    // ------------------------------------------------
    // Cleanup
    // ------------------------------------------------
    glfwDestroyWindow(win);

    glDeleteProgram(shader);
    glDeleteBuffers(1, &g_legendVBO);
    glDeleteVertexArrays(1, &g_legendVAO);
    glDeleteProgram(g_legendShader);

    glfwTerminate();
    return 0;
}