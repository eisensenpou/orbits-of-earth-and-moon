/*************************
 * File: orbit_viewer.cpp
 * Author: Sinan Demir
 * Date: 11/27/2025
 * Purpose: OpenGL 3D viewer for Earth–Moon–Sun orbits.
 *
 * Notes:
 *  - Uses option C lighting (ambient-boosted Lambert + rim)
 *  - Sun, Earth, Moon drawn as UV spheres (SphereMesh)
 *  - Moon orbit is exaggerated in CSV loader for visibility
 *  - HUD legend (Sun / Earth / Moon) in top-left
 *  - NEW: click legend squares to change camera center
 *        (Sun / Earth / Moon). Scroll = zoom, RMB drag = orbit.
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

// Orbit camera spherical coords
static float g_yaw   = glm::radians(45.0f);
static float g_pitch = glm::radians(20.0f);
static float g_radius = 250.0f;   // distance from target

static bool   g_mouseRotating = false;
static double g_lastMouseX = 0.0;
static double g_lastMouseY = 0.0;

static size_t g_frameIndex = 0;
static std::vector<Frame> g_frames;

// Camera target selection
enum class CameraTarget {
    Barycenter = 0,
    Sun,
    Earth,
    Moon
};

static CameraTarget g_cameraTarget = CameraTarget::Barycenter;

// Legend rendering objects (2D)
static GLuint g_legendShader = 0;
static GLuint g_legendVAO    = 0;
static GLuint g_legendVBO    = 0;
static GLint  g_legLocOffset = -1;
static GLint  g_legLocScale  = -1;
static GLint  g_legLocColor  = -1;

// Legend layout (in pixels)
static const float LEGEND_BASE_X   = 28.0f;  // from left
static const float LEGEND_BASE_Y   = 40.0f;  // from top
static const float LEGEND_SPACING  = 24.0f;  // between rows
static const float LEGEND_SIZE_PX  = 14.0f;  // base square size

// Forward declaration so mouse callback can call this
static void handleLegendClick(double mouseX, double mouseY);

// --------------------------------------------------
// Callbacks
// --------------------------------------------------
/**
 * @brief Handles window resize events by updating the viewport.
 */
static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    g_windowWidth  = w;
    g_windowHeight = h;
    glViewport(0, 0, w, h);
}

/**
 * @brief Handles mouse button events for:
 *  - RMB press/release → start/stop camera rotation
 *  - LMB press on legend → change camera target
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

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double mx, my;
        glfwGetCursorPos(win, &mx, &my);
        handleLegendClick(mx, my);
    }
}

/**
 * @brief Handles cursor position changes for mouse rotation.
 */
static void cursor_pos_callback(GLFWwindow*, double xpos, double ypos) {
    if (!g_mouseRotating) return;
    double dx = xpos - g_lastMouseX;
    double dy = ypos - g_lastMouseY;

    g_lastMouseX = xpos;
    g_lastMouseY = ypos;

    g_yaw   += static_cast<float>(dx) * 0.005f;
    g_pitch -= static_cast<float>(dy) * 0.005f;

    g_pitch = glm::clamp(g_pitch, glm::radians(-89.0f), glm::radians(89.0f));
}

/**
 * @brief Mouse wheel zoom: scroll up = zoom in, scroll down = zoom out.
 */
static void scroll_callback(GLFWwindow*, double /*xoff*/, double yoff) {
    g_radius -= static_cast<float>(yoff) * 12.0f;
    g_radius = glm::clamp(g_radius, 50.0f, 5000.0f);
}


// --------------------------------------------------
// Shader helpers
// --------------------------------------------------
/**
 * @brief Compiles a shader of given type from source code.
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
 * @brief Creates an OpenGL program from a vertex + fragment shader pair.
 */
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
 * @brief Initializes the legend renderer: shader + unit quad VAO/VBO.
 *
 * Draws small colored squares in NDC that we then place in pixel space.
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
 * @brief Draws a small colored box at a specified pixel position.
 *
 * @param centerPx  X center in window pixels.
 * @param centerPy  Y center in window pixels.
 * @param sizePx    Square size in pixels.
 * @param color     RGB color.
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

/**
 * @brief Handle a left-click; if it lands on a legend box,
 *        update camera target (Sun/Earth/Moon).
 */
static void handleLegendClick(double mouseX, double mouseY) {
    // Legend row centers in pixels
    float centersY[3] = {
        LEGEND_BASE_Y,
        LEGEND_BASE_Y + LEGEND_SPACING,
        LEGEND_BASE_Y + 2.0f * LEGEND_SPACING
    };

    float halfSize = LEGEND_SIZE_PX * 0.5f;
    float x0 = LEGEND_BASE_X - halfSize;
    float x1 = LEGEND_BASE_X + halfSize;

    for (int i = 0; i < 3; ++i) {
        float yCenter = centersY[i];
        float y0 = yCenter - halfSize;
        float y1 = yCenter + halfSize;

        if (mouseX >= x0 && mouseX <= x1 &&
            mouseY >= y0 && mouseY <= y1) {

            switch (i) {
                case 0: g_cameraTarget = CameraTarget::Sun;   break;
                case 1: g_cameraTarget = CameraTarget::Earth; break;
                case 2: g_cameraTarget = CameraTarget::Moon;  break;
            }

            std::cout << "📌 Camera target set to "
                      << (i == 0 ? "Sun" : (i == 1 ? "Earth" : "Moon"))
                      << "\n";
            return;
        }
    }
}


// --------------------------------------------------
// MAIN
// --------------------------------------------------
/**
 * @brief Entry point for the Orbit Viewer application.
 */
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win =
        glfwCreateWindow(g_windowWidth, g_windowHeight,
                         "Orbit Viewer (3D Spheres)", nullptr, nullptr);

    if (!win) {
        std::cerr << "❌ Failed to create window\n";
        return -1;
    }

    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetMouseButtonCallback(win, mouse_button_callback);
    glfwSetCursorPosCallback(win, cursor_pos_callback);
    glfwSetScrollCallback(win, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "❌ Failed to init GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // ----------------------------------------------------
    // Load CSV orbit frames (already scaled in CSVLoader)
    // ----------------------------------------------------
    CSVLoader loader;
    g_frames = loader.loadOrbitCSV("../build/orbit_three_body.csv");

    if (g_frames.empty()) {
        std::cerr << "⚠️ No frames loaded.\n";
    }

    // ----------------------------------------------------
    // Create 3D mesh spheres (visualization radii)
    // ----------------------------------------------------
    SphereMesh sunMesh;   sunMesh.build(5.0f, 48, 48);
    SphereMesh earthMesh; earthMesh.build(1.0f, 32, 32);
    SphereMesh moonMesh;  moonMesh.build(0.27f, 24, 24);

    // ----------------------------------------------------
    // Create main 3D shader (Option C lighting)
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

            // Lambert + Blinn–Phong
            float diff = max(dot(N, L), 0.0);
            float spec = pow(max(dot(N, H), 0.0), 32.0);
            float ambient = 0.18;

            vec3 base = uColor * (ambient + diff)
                      + vec3(0.4) * spec;

            // Rim light for cinematic look
            float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0);
            vec3 rimColor = vec3(0.3, 0.4, 0.9) * rim * 0.5;

            vec3 color = base + rimColor;

            // Gamma
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

            // Camera offset in local spherical coords
            glm::vec3 camOffset(
                g_radius * cos(g_pitch) * sin(g_yaw),
                g_radius * sin(g_pitch),
                g_radius * cos(g_pitch) * cos(g_yaw)
            );

            // Choose camera target (what we orbit around)
            glm::vec3 target(0.0f);
            switch (g_cameraTarget) {
                case CameraTarget::Sun:   target = f.sun;   break;
                case CameraTarget::Earth: target = f.earth; break;
                case CameraTarget::Moon:  target = f.moon;  break;
                case CameraTarget::Barycenter:
                default:
                    target = glm::vec3(0.0f);
                    break;
            }

            glm::vec3 camPos = target + camOffset;

            float aspect = float(g_windowWidth) / float(g_windowHeight);
            glm::mat4 proj = glm::perspective(glm::radians(45.0f),
                                              aspect, 0.1f, 50000.0f);
            glm::mat4 view = glm::lookAt(camPos, target, glm::vec3(0, 1, 0));

            glUseProgram(shader);

            // view position for specular
            glUniform3fv(locViewPos, 1, glm::value_ptr(camPos));
            // light at Sun position
            glUniform3fv(locLight, 1, glm::value_ptr(f.sun));

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

        float size      = LEGEND_SIZE_PX;
        float sizeSel   = LEGEND_SIZE_PX * 1.4f; // highlight selected

        float y0 = LEGEND_BASE_Y;
        float y1 = LEGEND_BASE_Y + LEGEND_SPACING;
        float y2 = LEGEND_BASE_Y + 2.0f * LEGEND_SPACING;

        // Sun icon (top)
        drawLegendBox(
            LEGEND_BASE_X, y0,
            (g_cameraTarget == CameraTarget::Sun ? sizeSel : size),
            glm::vec3(1.4f, 1.1f, 0.3f)
        );

        // Earth icon (middle)
        drawLegendBox(
            LEGEND_BASE_X, y1,
            (g_cameraTarget == CameraTarget::Earth ? sizeSel : size),
            glm::vec3(0.2f, 0.8f, 1.2f)
        );

        // Moon icon (bottom)
        drawLegendBox(
            LEGEND_BASE_X, y2,
            (g_cameraTarget == CameraTarget::Moon ? sizeSel : size),
            glm::vec3(0.85f, 0.85f, 0.92f)
        );

        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);

    // cleanup legend objects
    glDeleteBuffers(1, &g_legendVBO);
    glDeleteVertexArrays(1, &g_legendVAO);
    glDeleteProgram(g_legendShader);

    glfwTerminate();
    return 0;
}
