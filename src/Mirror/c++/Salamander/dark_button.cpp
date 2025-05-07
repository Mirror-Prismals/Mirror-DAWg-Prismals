// Button3D_Dark.cpp
//
// A demonstration of a 3D dark-mode button using GLFW/GLM and immediate-mode OpenGL.
// This single button toggles between a pressed and released state and uses 3D quads
// to simulate depth, giving it a tangible, three-dimensional appearance.
// 
// Key Features:
// 1) Full 3D sides drawn as quads (front, top, right, bottom, left).
// 2) A dark theme with colors suited for a dark UI.
// 3) Press animation that sinks in, shifts left, and compresses depth.
// 4) Toggle behavior: clicking the button toggles its state.
// 5) A label is rendered each frame.
//
// Requires stb_easy_font.h in the same folder.
// Compile with Visual Studio (link against glfw3.lib, opengl32.lib, user32.lib, gdi32.lib).

#include <windows.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "stb_easy_font.h"

#ifndef PI
constexpr double PI = 3.14159265358979323846;
#endif

// -------------------------
// UI Button Structure
// -------------------------
struct Button {
    glm::vec2 pos;         // Center position in window coordinates
    glm::vec2 size;        // Half-width and half-height
    std::string label;

    bool isPressed = false;     // True while mouse is down on the button
    bool isSelected = false;    // True if toggled "on"
    double pressTime = 0.0;     // Timestamp of last mouse press
    float pressAnim = 0.0f;     // Animation state: 0.0 = not pressed, 0.5 = fully pressed
};

// Single dark-mode button
Button g_darkButton;

// Timing constants
constexpr double PRESS_FEEDBACK_DURATION = 0.15; // Duration for press animation
double g_lastFrameTime = 0.0;

// -------------------------
// 3D Button Drawing Function
// -------------------------
// This function creates a 3D illusion by drawing each side of the button with quads.
// The press animation causes the front face to shift and sink, and compresses the button's depth.
void drawButton3D(float bx, float by, float bw, float bh, float depth, float pressAnim, bool darkTheme = true)
{
    // Calculate leftward shift for the press effect (max 10 px shift)
    float shiftLeft = 10.0f * pressAnim;
    // Calculate how far the front face sinks (increases with pressAnim)
    float pressOffsetZ = depth * pressAnim;
    // Compress the depth (reduce thickness up to 50% when fully pressed)
    float newDepth = depth * (1.0f - 0.5f * pressAnim);
    // Adjusted starting coordinates for the button
    float x = bx - shiftLeft;
    float y = by;

    if (darkTheme) {
        // DARK THEME COLORS

        // FRONT FACE: dark gray, darkening with press animation
        float frontColor = 0.3f - 0.1f * (pressAnim * 2.0f);
        glColor3f(frontColor, frontColor, frontColor);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glEnd();

        // TOP FACE: gives the illusion of thickness by sloping away from the viewer
        glColor3f(0.4f, 0.4f, 0.4f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        // RIGHT FACE: side view, darker to enhance the 3D look
        glColor3f(0.25f, 0.25f, 0.25f);
        glBegin(GL_QUADS);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        // BOTTOM FACE: shadowed to create depth
        glColor3f(0.35f, 0.35f, 0.35f);
        glBegin(GL_QUADS);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        // LEFT FACE: highlights the depth on the opposite side
        glColor3f(0.28f, 0.28f, 0.28f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();
    }
}

// -------------------------
// Text Rendering Function
// -------------------------
// Renders text using stb_easy_font. Depth test is disabled temporarily
// so that the label is not hidden by the 3D elements.
void renderText(float x, float y, const char* text, bool darkTheme = true)
{
    char buffer[99999];
    int num_quads = stb_easy_font_print(x, y, const_cast<char*>(text), NULL, buffer, sizeof(buffer));

    glDisable(GL_DEPTH_TEST);
    if (darkTheme) {
        glColor3f(0.9f, 0.9f, 0.9f);
    }
    else {
        glColor3f(0.0f, 0.0f, 0.0f);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_DEPTH_TEST);
}

// -------------------------
// Hit Testing
// -------------------------
// Checks if a given (x,y) coordinate is inside the button bounds.
bool isInside(const Button& btn, float x, float y)
{
    float left = btn.pos.x - btn.size.x;
    float right = btn.pos.x + btn.size.x;
    float top = btn.pos.y - btn.size.y;
    float bottom = btn.pos.y + btn.size.y;
    return (x >= left && x <= right && y >= top && y <= bottom);
}

// -------------------------
// UI Initialization
// -------------------------
// Positions and sizes the single dark-mode button.
void initUI(int screenWidth, int screenHeight)
{
    // Button is 150 px wide and 40 px tall (half-sizes: 75, 20)
    g_darkButton.pos = glm::vec2(screenWidth * 0.5f, screenHeight * 0.5f);
    g_darkButton.size = glm::vec2(75.0f, 20.0f);
    g_darkButton.label = "Dark Mode Button";
}

// -------------------------
// Mouse Button Callback
// -------------------------
// Implements toggling behavior for the button.
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    double currentTime = glfwGetTime();
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (isInside(g_darkButton, (float)mx, (float)my)) {
                g_darkButton.isPressed = true;
                g_darkButton.pressTime = currentTime;
            }
        }
        else if (action == GLFW_RELEASE) {
            if (g_darkButton.isPressed && isInside(g_darkButton, (float)mx, (float)my)) {
                g_darkButton.isSelected = !g_darkButton.isSelected;
            }
            g_darkButton.isPressed = false;
        }
    }
}

// -------------------------
// Update Press Animation
// -------------------------
// Smoothly animates the press state by moving the animation value toward
// the target (0.5 when pressed or selected, 0.0 otherwise).
void updateButtonAnimation(Button& btn, float deltaTime)
{
    float animSpeed = (float)(0.5 / PRESS_FEEDBACK_DURATION);
    bool shouldPress = (btn.isPressed || btn.isSelected);
    float target = shouldPress ? 0.5f : 0.0f;

    if (btn.pressAnim < target) {
        btn.pressAnim += animSpeed * deltaTime;
        if (btn.pressAnim > target) btn.pressAnim = target;
    }
    else if (btn.pressAnim > target) {
        btn.pressAnim -= animSpeed * deltaTime;
        if (btn.pressAnim < target) btn.pressAnim = target;
    }
}

// -------------------------
// Main Function
// -------------------------
int main()
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Fullscreen window
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int fullWidth = mode->width;
    int fullHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(fullWidth, fullHeight, "3D Dark Mode Button", monitor, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create fullscreen window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Set up orthographic projection (2D view with depth for 3D effects)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, fullWidth, fullHeight, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    initUI(fullWidth, fullHeight);
    g_lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - g_lastFrameTime);
        g_lastFrameTime = currentTime;

        updateButtonAnimation(g_darkButton, deltaTime);

        // Clear background to a light gray (for contrast with dark mode button)
        glClearColor(0.933f, 0.933f, 0.933f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Calculate button bounds
        float bx = g_darkButton.pos.x - g_darkButton.size.x;
        float by = g_darkButton.pos.y - g_darkButton.size.y;
        float bw = g_darkButton.size.x * 2.0f;
        float bh = g_darkButton.size.y * 2.0f;
        float depth = 10.0f;

        // Draw the dark-mode button and its label
        drawButton3D(bx, by, bw, bh, depth, g_darkButton.pressAnim, true);
        renderText(bx + 15, by + bh / 2 - 5, g_darkButton.label.c_str(), true);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
