// Button3D_Primitives.cpp
//
// High-quality 3D buttons that toggle and shorten front-to-back
// when pressed, rather than shrinking them left-to-right.
// 
// Key Features:
// 1) Full 3D sides (front, top, right, bottom, left).
// 2) Two button sizes: "Medium" and "Extra Medium"
// 3) Press animation sinks in, shifts left, and compresses depth.
// 4) Toggling behavior for buttons.
// 5) Labels drawn every frame (always visible).
// 
// Uses GLFW/GLM and immediate-mode OpenGL. Requires stb_easy_font.h in the same folder.
// Compile with Visual Studio (link against glfw3.lib, opengl32.lib, user32.lib, gdi32.lib).

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// Use your local stb_easy_font.h (make sure it's in your source folder)
#include "stb_easy_font.h"

#ifndef PI
constexpr double PI = 3.14159265358979323846;
#endif

// -------------------------
// UI Button Structure
// -------------------------
struct Button {
    glm::vec2 pos;         // Center position (window coordinates)
    glm::vec2 size;        // Half-width and half-height
    std::string label;

    bool isPressed = false;     // True while mouse is down on this button
    bool isSelected = false;    // True if toggled "on" (pressed in)
    double pressTime = 0.0;     // Timestamp of last mouse press
    float pressAnim = 0.0f;     // 0.0 -> not pressed, 0.5 -> fully pressed
};

// One medium button
Button g_mediumButton;

// One extra medium button
Button g_extraMediumButton;

// Timing constants
constexpr double PRESS_FEEDBACK_DURATION = 0.15; // time to animate to pressed

// For animation updates
double g_lastFrameTime = 0.0;

// -------------------------
// 3D Button Drawing with Press Animation
// Shortens the "depth" dimension rather than the button width,
// and shifts the button left slightly while keeping width the same.
// -------------------------
void drawButton3D(float bx, float by, float bw, float bh, float depth, float pressAnim, bool darkTheme = false)
{
    // pressAnim in [0, 0.5]. We interpret 0.5 as "fully pressed."
    // SHIFT: move the button left by up to 10 px
    float shiftLeft = 10.0f * pressAnim;

    // SINK: the front face is offset deeper into the screen by pressOffsetZ
    float pressOffsetZ = depth * pressAnim;

    // COMPRESS: reduce the "depth" dimension from the front face to the back face
    // We'll shrink the overall thickness by up to 50% at full press
    float newDepth = depth * (1.0f - 0.5f * pressAnim);

    // The front face's corners are at (bx, by, -pressOffsetZ)
    float x = bx - shiftLeft;
    float y = by;

    if (darkTheme) {
        // DARK THEME COLORS

        // FRONT FACE: Dark gray that gets darker when pressed
        // 0.3 -> 0.2 as pressAnim goes 0->0.5
        float frontColor = 0.3f - 0.1f * (pressAnim * 2.0f);
        glColor3f(frontColor, frontColor, frontColor);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glEnd();

        // TOP FACE
        glColor3f(0.4f, 0.4f, 0.4f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        // RIGHT FACE
        glColor3f(0.25f, 0.25f, 0.25f);
        glBegin(GL_QUADS);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        // BOTTOM FACE
        glColor3f(0.35f, 0.35f, 0.35f);
        glBegin(GL_QUADS);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        // LEFT FACE
        glColor3f(0.28f, 0.28f, 0.28f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();
    }
    else {
        // LIGHT THEME COLORS (ORIGINAL)

        // FRONT FACE color: 0.8 -> 0.6 as pressAnim goes 0->0.5
        float frontColor = 0.8f - 0.2f * (pressAnim * 2.0f);
        glColor3f(frontColor, frontColor, frontColor);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glEnd();

        // TOP FACE
        glColor3f(0.9f, 0.9f, 0.9f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        // RIGHT FACE
        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_QUADS);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        // BOTTOM FACE
        glColor3f(0.7f, 0.7f, 0.7f);
        glBegin(GL_QUADS);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        // LEFT FACE
        glColor3f(0.65f, 0.65f, 0.65f);
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
void renderText(float x, float y, const char* text, bool darkTheme = false)
{
    char buffer[99999];
    int num_quads = stb_easy_font_print(x, y, const_cast<char*>(text), NULL, buffer, sizeof(buffer));

    // Disable depth test so text isn't occluded
    glDisable(GL_DEPTH_TEST);

    if (darkTheme) {
        glColor3f(0.9f, 0.9f, 0.9f); // white/light gray text for dark theme
    }
    else {
        glColor3f(0.0f, 0.0f, 0.0f); // black text for light theme
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
// Initialize UI
// We'll place 1 medium button and 1 extra medium button
// -------------------------
void initUI(int screenWidth, int screenHeight)
{
    // Medium button is 120 wide, 40 tall in total (half-sizes: 60, 20).
    g_mediumButton.pos = glm::vec2(screenWidth * 0.33f, screenHeight * 0.5f);
    g_mediumButton.size = glm::vec2(60.0f, 20.0f);
    g_mediumButton.label = "Medium Button";

    // Extra medium button (dark theme)
    g_extraMediumButton.pos = glm::vec2(screenWidth * 0.66f, screenHeight * 0.5f);
    g_extraMediumButton.size = glm::vec2(75.0f, 20.0f);
    g_extraMediumButton.label = "Dark Theme";
}

// -------------------------
// Mouse Button Callback
// - Toggling behavior for buttons
// -------------------------
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    double currentTime = glfwGetTime();
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Mark button as pressed if inside
            if (isInside(g_mediumButton, (float)mx, (float)my)) {
                g_mediumButton.isPressed = true;
                g_mediumButton.pressTime = currentTime;
            }
            if (isInside(g_extraMediumButton, (float)mx, (float)my)) {
                g_extraMediumButton.isPressed = true;
                g_extraMediumButton.pressTime = currentTime;
            }
        }
        else if (action == GLFW_RELEASE) {
            // Medium button: toggle selection
            if (g_mediumButton.isPressed && isInside(g_mediumButton, (float)mx, (float)my)) {
                g_mediumButton.isSelected = !g_mediumButton.isSelected;
            }
            g_mediumButton.isPressed = false;

            // Extra medium button: toggle on/off
            if (g_extraMediumButton.isPressed && isInside(g_extraMediumButton, (float)mx, (float)my)) {
                g_extraMediumButton.isSelected = !g_extraMediumButton.isSelected;
            }
            g_extraMediumButton.isPressed = false;
        }
    }
}

// -------------------------
// Update Press Animations
// For both toggled state and actual press
// - If button is pressed or selected => animate to 0.5
// - Otherwise => animate to 0.0
// -------------------------
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
// Main
// -------------------------
int main()
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Fullscreen
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int fullWidth = mode->width;
    int fullHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(fullWidth, fullHeight, "3D Button Primitives", monitor, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create fullscreen window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Set up orthographic projection
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

        // Update press animations
        updateButtonAnimation(g_mediumButton, deltaTime);
        updateButtonAnimation(g_extraMediumButton, deltaTime);

        // Clear background to #EEEEEE (light gray/white)
        glClearColor(0.933f, 0.933f, 0.933f, 1.0f);  // #EEEEEE converted to 0-1 range
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Draw medium button (light theme)
        {
            float bx = g_mediumButton.pos.x - g_mediumButton.size.x;
            float by = g_mediumButton.pos.y - g_mediumButton.size.y;
            float bw = g_mediumButton.size.x * 2.0f;
            float bh = g_mediumButton.size.y * 2.0f;
            float depth = 10.0f;

            drawButton3D(bx, by, bw, bh, depth, g_mediumButton.pressAnim, false);
            // Render label (center it a bit)
            renderText(bx + 10, by + bh / 2 - 5, g_mediumButton.label.c_str(), false);
        }

        // Draw extra medium button (dark theme)
        {
            float cx = g_extraMediumButton.pos.x - g_extraMediumButton.size.x;
            float cy = g_extraMediumButton.pos.y - g_extraMediumButton.size.y;
            float cw = g_extraMediumButton.size.x * 2.0f;
            float ch = g_extraMediumButton.size.y * 2.0f;
            float depth = 10.0f;

            drawButton3D(cx, cy, cw, ch, depth, g_extraMediumButton.pressAnim, true);
            renderText(cx + 15, cy + ch / 2 - 5, g_extraMediumButton.label.c_str(), true);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
