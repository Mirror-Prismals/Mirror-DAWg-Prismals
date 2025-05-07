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
// Background: very dark gray.
// Panels: darker grays.
const Color bgColor = { 0.12f, 0.12f, 0.12f, 1.0f };
const Color sampleCol = { 0.18f, 0.18f, 0.18f, 1.0f };  // Sample Manager panel
const Color fxchainCol = { 0.15f, 0.15f, 0.15f, 1.0f };  // FX Chain panel

// (Unused helper, kept for potential future use.)
Color hexToColor(const std::string& hex) {
    unsigned int ri = 255, gi = 255, bi = 255;
    const char* str = hex.c_str();
    if (hex[0] == '#')
        sscanf(str, "#%02x%02x%02x", &ri, &gi, &bi);
    else
        sscanf(str, "%02x%02x%02x", &ri, &gi, &bi);
    return { ri / 255.0f, gi / 255.0f, bi / 255.0f, 1.0f };
}

// ----------------------------------------------------
// Global Animation and Window State

float sampleAnim = 0.0f; // 0 = fully visible, 1 = hidden.
float fxAnim = 0.0f;
bool sampleHidden = false;
bool fxHidden = false;
const float animSpeed = 4.0f;

int winWidth = 1024, winHeight = 768;
GLFWmonitor* primaryMonitor = nullptr;
float sampleWidth = 0; // computed each frame
float fxHeight = 0;    // computed each frame

// ----------------------------------------------------
// Utility Drawing Functions (Panels)

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
// Render Panels (Sample Manager and FX Chain)
// (Dark theme version – no nodes, no text, no gradient background)

void renderPanels() {
    sampleWidth = winWidth * 0.2f * (1.0f - sampleAnim);
    fxHeight = winHeight * 0.3f * (1.0f - fxAnim);
    float panelDepth = 15.0f;

    // Enable blending for a subtle translucent effect.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Make the left panel (Sample Manager) more translucent.
    Color sampleColTrans = { sampleCol.r, sampleCol.g, sampleCol.b, 0.5f };
    // FX Chain panel remains slightly less translucent.
    Color fxchainColTrans = { fxchainCol.r, fxchainCol.g, fxchainCol.b, 0.85f };

    // SAMPLE MANAGER panel with a mirrored effect (left side).
    float samplePanelX = 0 - sampleAnim * (winWidth * 0.2f);
    float samplePanelY = 10;
    float samplePanelW = winWidth * 0.2f;
    float samplePanelH = winHeight - 10;
    glPushMatrix();
    float centerX = samplePanelX + samplePanelW * 0.5f;
    float centerY = samplePanelY + samplePanelH * 0.5f;
    glTranslatef(centerX, centerY, 0);
    glRotatef(180.0f, 0, 1, 0);
    glTranslatef(-centerX, -centerY, 0);
    drawPanel3D(samplePanelX, samplePanelY, samplePanelW, samplePanelH,
        panelDepth, sampleColTrans, 0.0f, true);
    glPopMatrix();

    // FX CHAIN panel (bottom panel): spans the full window width.
    float fxPanelX = 0;
    float fxPanelY = winHeight - fxHeight;
    float fxPanelW = winWidth;
    float fxPanelH = fxHeight;
    drawPanel3D(fxPanelX, fxPanelY, fxPanelW, fxPanelH,
        panelDepth, fxchainColTrans, 0.0f, true);

    glDisable(GL_BLEND);
}

// ----------------------------------------------------
// Animation Update Function

void updateAnimations(double dt) {
    float targetSample = sampleHidden ? 1.0f : 0.0f;
    float targetFx = fxHidden ? 1.0f : 0.0f;
    sampleAnim += (targetSample - sampleAnim) * dt * animSpeed;
    fxAnim += (targetFx - fxAnim) * dt * animSpeed;
}

// ----------------------------------------------------
// GLFW Callback Functions (No Node Interactions)

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // No interactions in this version.
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    // No interactions in this version.
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_S) {
            sampleHidden = !sampleHidden;
        }
        else if (key == GLFW_KEY_X) {
            fxHidden = !fxHidden;
        }
        else if (key == GLFW_KEY_F) {
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

    GLFWwindow* window = glfwCreateWindow(winWidth, winHeight, "Dark Theme - Panels Only", nullptr, nullptr);
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

        // Render the animated panels.
        renderPanels();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
