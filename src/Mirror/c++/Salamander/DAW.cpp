// main.cpp
#define _CRT_SECURE_NO_WARNINGS

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <string>

// ----------------------------------------------------
// Global Definitions & Helper Structures

struct Color {
    float r, g, b, a;
};

// "Nice Dark" Theme Colors:
const Color bgColor = { 0.12f, 0.12f, 0.12f, 1.0f };

// Panel base colors (for fully visible panels):
const Color leftCol = { 0.18f, 0.18f, 0.18f, 1.0f };
const Color rightCol = { 0.17f, 0.17f, 0.17f, 1.0f };
const Color topCol = { 0.16f, 0.16f, 0.16f, 1.0f };
const Color bottomCol = { 0.15f, 0.15f, 0.15f, 1.0f };

// Translucent versions:
const Color leftColTrans = { leftCol.r, leftCol.g, leftCol.b, 0.5f };
const Color rightColTrans = { rightCol.r, rightCol.g, rightCol.b, 0.85f };
const Color topColTrans = { topCol.r, topCol.g, topCol.b, 0.85f };
const Color bottomColTrans = { bottomCol.r, bottomCol.g, bottomCol.b, 0.85f };

// ----------------------------------------------------
// Global Animation and Window State

// Animation factors (0 = fully visible, 1 = hidden)
float leftAnim = 0.0f, rightAnim = 0.0f, topAnim = 0.0f, bottomAnim = 0.0f;
// Hidden state toggles
bool leftHidden = false, rightHidden = false, topHidden = false, bottomHidden = false;
const float animSpeed = 4.0f;

int winWidth = 1024, winHeight = 768;
GLFWmonitor* primaryMonitor = nullptr;

// Panel size fractions relative to window dimensions
const float L_FULL = 0.2f; // Left panel width fraction
const float R_FULL = 0.2f; // Right panel width fraction
const float T_FULL = 0.2f; // Top panel height fraction
const float B_FULL = 0.3f; // Bottom panel height fraction

// ----------------------------------------------------
// Utility Drawing Function (Panels)
// drawPanel3D draws a beveled quad.
void drawPanel3D(float bx, float by, float bw, float bh, float depth,
    const Color& baseColor, float pressAnim, bool darkTheme)
{
    float shift = 10.0f * pressAnim;
    float pressOffsetZ = depth * pressAnim;
    float newDepth = depth * (1.0f - 0.5f * pressAnim);
    float x = bx - shift;
    float y = by;

    // Front face
    glColor4f(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glEnd();

    // Top-left bevel
    glColor4f(baseColor.r * 1.1f, baseColor.g * 1.1f, baseColor.b * 1.1f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    // Right bevel
    glColor4f(baseColor.r * 0.9f, baseColor.g * 0.9f, baseColor.b * 0.9f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    // Bottom bevel
    glColor4f(baseColor.r * 1.05f, baseColor.g * 1.05f, baseColor.b * 1.05f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    // Left bevel
    glColor4f(baseColor.r * 0.95f, baseColor.g * 0.95f, baseColor.b * 0.95f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();
}

// ----------------------------------------------------
// Render Panels
// Panels are drawn at fixed positions relative to the window and may overlap.
void renderPanels() {
    // Full panel dimensions:
    float L_full = winWidth * L_FULL;
    float R_full = winWidth * R_FULL;
    float T_full = winHeight * T_FULL;
    float B_full = winHeight * B_FULL;

    // Left panel: slides horizontally from x = 0 (visible) to x = -L_full (hidden)
    float leftX = 0 - (L_full * leftAnim);
    float leftY = 0;
    float leftW = L_full;
    float leftH = winHeight;

    // Right panel: slides from x = winWidth - R_full (visible) to winWidth (hidden)
    float rightX = winWidth - R_full + (R_full * rightAnim);
    float rightY = 0;
    float rightW = R_full;
    float rightH = winHeight;

    // Top panel: slides vertically from y = 0 (visible) to y = -T_full (hidden)
    float topX = 0;
    float topY = 0 - (T_full * topAnim);
    float topW = winWidth;
    float topH = T_full;

    // Bottom panel: slides from y = winHeight - B_full (visible) to winHeight (hidden)
    float bottomX = 0;
    float bottomY = winHeight - B_full + (B_full * bottomAnim);
    float bottomW = winWidth;
    float bottomH = B_full;

    float panelDepth = 15.0f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw each panel using its translucent color.
    drawPanel3D(leftX, leftY, leftW, leftH, panelDepth, leftColTrans, 0.0f, true);
    drawPanel3D(rightX, rightY, rightW, rightH, panelDepth, rightColTrans, 0.0f, true);
    drawPanel3D(topX, topY, topW, topH, panelDepth, topColTrans, 0.0f, true);
    drawPanel3D(bottomX, bottomY, bottomW, bottomH, panelDepth, bottomColTrans, 0.0f, true);

    glDisable(GL_BLEND);
}

// ----------------------------------------------------
// Animation Update Function

void updateAnimations(double dt) {
    // Update each panel's animation factor (0 = visible, 1 = hidden)
    float targetLeft = leftHidden ? 1.0f : 0.0f;
    float targetRight = rightHidden ? 1.0f : 0.0f;
    float targetTop = topHidden ? 1.0f : 0.0f;
    float targetBottom = bottomHidden ? 1.0f : 0.0f;

    leftAnim += (targetLeft - leftAnim) * dt * animSpeed;
    rightAnim += (targetRight - rightAnim) * dt * animSpeed;
    topAnim += (targetTop - topAnim) * dt * animSpeed;
    bottomAnim += (targetBottom - bottomAnim) * dt * animSpeed;
}

// ----------------------------------------------------
// GLFW Callback Functions

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // No mouse interaction in this example.
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    // No mouse interaction in this example.
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        // Use arrow keys to toggle panels:
        if (key == GLFW_KEY_LEFT) {    // Toggle left panel
            leftHidden = !leftHidden;
        }
        else if (key == GLFW_KEY_RIGHT) { // Toggle right panel
            rightHidden = !rightHidden;
        }
        else if (key == GLFW_KEY_UP) {    // Toggle top panel
            topHidden = !topHidden;
        }
        else if (key == GLFW_KEY_DOWN) {  // Toggle bottom panel
            bottomHidden = !bottomHidden;
        }
        else if (key == GLFW_KEY_F) {     // Toggle fullscreen
            static bool isFullscreen = false;
            static int windowedX, windowedY, windowedWidth, windowedHeight;
            GLFWwindow* curr = glfwGetCurrentContext();
            if (!isFullscreen) {
                glfwGetWindowPos(curr, &windowedX, &windowedY);
                glfwGetWindowSize(curr, &windowedWidth, &windowedHeight);
                primaryMonitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
                winWidth = mode->width;
                winHeight = mode->height;
                glfwSetWindowMonitor(curr, primaryMonitor, 0, 0, winWidth, winHeight, mode->refreshRate);
            }
            else {
                winWidth = windowedWidth;
                winHeight = windowedHeight;
                glfwSetWindowMonitor(curr, nullptr, windowedX, windowedY, winWidth, winHeight, 0);
            }
            isFullscreen = !isFullscreen;
            glViewport(0, 0, winWidth, winHeight);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, winWidth, winHeight, 0, -100, 100);
        }
    }
}

// ----------------------------------------------------
// Main Entry Point

int main(void) {
    srand((unsigned int)time(nullptr));
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    winWidth = mode->width * 0.8;
    winHeight = mode->height * 0.8;

    GLFWwindow* window = glfwCreateWindow(winWidth, winHeight, "Four-Panel Dark Theme (Overlapping)", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, winWidth, winHeight, 0, -100, 100);
    glViewport(0, 0, winWidth, winHeight);

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double dt = currentTime - lastTime;
        lastTime = currentTime;
        updateAnimations(dt);

        glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render all four panels (which may overlap).
        renderPanels();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
