// Button3D_Dark_AllBevels_Square_Fixed.cpp
//
// A demonstration of a 3D dark-mode button using GLFW/GLM and immediate-mode OpenGL.
// This button toggles between a pressed and released state and uses 3D quads to simulate
// depth. We draw the front face plus bevels on all four edges (top/left highlights,
// bottom/right shadows). The button is square and bigger, with deeper 3D quads.
//
// This version fixes the small artifact on the bottom-left corner by adjusting
// the bottom bevel so it lines up exactly with the left bevel.
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

Button g_darkButton;

// Timing constant for press animation
constexpr double PRESS_FEEDBACK_DURATION = 0.15;
double g_lastFrameTime = 0.0;

// -------------------------
// 3D Button Drawing Function with All Bevels
// -------------------------
void drawButton3D(float bx, float by, float bw, float bh, float depth, float pressAnim, bool darkTheme = true)
{
    // Calculate press parameters
    float shiftLeft = 10.0f * pressAnim;        // Front face shifts left on press
    float pressOffsetZ = depth * pressAnim;     // How much the front face sinks in Z
    float newDepth = depth * (1.0f - 0.5f * pressAnim);

    // Increase bevel thickness by taking a fraction of the new depth
    float bevelThickness = newDepth * 0.5f;

    // Adjusted front face position
    float x = bx - shiftLeft;
    float y = by;

    if (darkTheme) {
        // ----- FRONT FACE -----
        float frontColor = 0.3f - 0.1f * (pressAnim * 2.0f);
        glColor3f(frontColor, frontColor, frontColor);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glEnd();

        // ----- TOP BEVEL (Highlight) -----
        glColor3f(0.4f, 0.4f, 0.4f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw - bevelThickness, y - bevelThickness, -(pressOffsetZ + bevelThickness));
        glVertex3f(x - bevelThickness, y - bevelThickness, -(pressOffsetZ + bevelThickness));
        glEnd();

        // ----- LEFT BEVEL (Highlight) -----
        glColor3f(0.42f, 0.42f, 0.42f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x - bevelThickness, y + bh - bevelThickness, -(pressOffsetZ + bevelThickness));
        glVertex3f(x - bevelThickness, y - bevelThickness, -(pressOffsetZ + bevelThickness));
        glEnd();

        // ----- RIGHT BEVEL (Shadow) -----
        glColor3f(0.25f, 0.25f, 0.25f);
        glBegin(GL_QUADS);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw + bevelThickness, y + bh + bevelThickness, -(pressOffsetZ + bevelThickness));
        glVertex3f(x + bw + bevelThickness, y + bevelThickness, -(pressOffsetZ + bevelThickness));
        glEnd();

        // ----- BOTTOM BEVEL (Shadow) [FIXED] -----
        // Note the final vertex: (x - bevelThickness, y + bh - bevelThickness)
        // so it lines up exactly with the left bevel's bottom corner.
// ----- Draw the bottom bevel (inverted shadow) -----
// ----- Draw the bottom bevel (inverted shadow) -----
        glColor3f(0.23f, 0.23f, 0.23f);
        glBegin(GL_QUADS);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw + bevelThickness, y + bh + bevelThickness, -(pressOffsetZ + bevelThickness));
        // Shift the left vertex from x -> x + bevelThickness:
        glVertex3f(x + bevelThickness, y + bh + bevelThickness, -(pressOffsetZ + bevelThickness));
        glEnd();


    }
}

// -------------------------
// Text Rendering Function
// -------------------------
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
void initUI(int screenWidth, int screenHeight)
{
    // Make the button square and larger: 200px wide and 200px tall (half-sizes: 100, 100)
    g_darkButton.pos = glm::vec2(screenWidth * 0.5f, screenHeight * 0.5f);
    g_darkButton.size = glm::vec2(100.0f, 100.0f);
    g_darkButton.label = "Square Button";
}

// -------------------------
// Mouse Button Callback
// -------------------------
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
void updateButtonAnimation(Button& btn, float deltaTime)
{
    float animSpeed = (float)(0.5 / PRESS_FEEDBACK_DURATION);
    bool shouldPress = (btn.isPressed || btn.isSelected);
    float target = shouldPress ? 0.5f : 0.0f;

    if (btn.pressAnim < target) {
        btn.pressAnim += animSpeed * deltaTime;
        if (btn.pressAnim > target)
            btn.pressAnim = target;
    }
    else if (btn.pressAnim > target) {
        btn.pressAnim -= animSpeed * deltaTime;
        if (btn.pressAnim < target)
            btn.pressAnim = target;
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

    // Create a fullscreen window.
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int fullWidth = mode->width;
    int fullHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(fullWidth, fullHeight, "3D Dark Mode Square Button (Fixed)", monitor, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create fullscreen window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Set up an orthographic projection (2D view with depth for 3D effects)
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

        // Clear the background (light gray for contrast)
        glClearColor(0.933f, 0.933f, 0.933f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Calculate button bounds.
        float bx = g_darkButton.pos.x - g_darkButton.size.x;
        float by = g_darkButton.pos.y - g_darkButton.size.y;
        float bw = g_darkButton.size.x * 2.0f;
        float bh = g_darkButton.size.y * 2.0f;
        // Increased depth for a more pronounced 3D effect.
        float depth = 20.0f;

        // Draw the square button with all bevels (now fixed) and its label.
        drawButton3D(bx, by, bw, bh, depth, g_darkButton.pressAnim, true);
        renderText(bx + 15, by + bh / 2 - 5, g_darkButton.label.c_str(), true);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
