// Button3D_Primitives.cpp
//
// High-quality 3D jumbo button that toggles and shortens front-to-back
// when pressed, rather than shrinking left-to-right.
// 
// Key Features:
// 1) Full 3D sides (front, top, right, bottom, left).
// 2) Square jumbo button shape
// 3) Press animation sinks in, shifts left, and compresses depth.
// 4) Toggling behavior
// 5) Label drawn every frame (always visible)
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
    double autoUnpressTime = 0.0; // Time when button should automatically unpress
    bool autoUnpress = false;   // Whether button should auto-unpress
};

// Jumbo button (square)
Button g_jumboButton;

// Timing constants
constexpr double PRESS_FEEDBACK_DURATION = 0.02;  // very fast press in (almost instant)
constexpr double UNPRESS_FEEDBACK_DURATION = 0.08; // time to animate back to unpressed (faster snappy return)
constexpr double AUTO_UNPRESS_DURATION = 0.2;     // shorter duration before auto-unpress

// For animation updates
double g_lastFrameTime = 0.0;

// -------------------------
// 3D Button Drawing with Press Animation
// Shortens the "depth" dimension rather than the button width,
// and shifts the button left slightly while keeping width the same.
// -------------------------
void drawButton3D(float bx, float by, float bw, float bh, float depth, float pressAnim, bool darkTheme = false, bool purpleTheme = false)
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

    if (purpleTheme) {
        // PURPLE THEME COLORS

        // FRONT FACE: Purple that gets darker when pressed
        // Base color is medium purple
        float frontR = 0.6f - 0.2f * (pressAnim * 2.0f);
        float frontG = 0.3f - 0.1f * (pressAnim * 2.0f);
        float frontB = 0.8f - 0.2f * (pressAnim * 2.0f);

        glColor3f(frontR, frontG, frontB);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glEnd();

        // TOP FACE - lighter purple
        glColor3f(0.7f, 0.5f, 0.9f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        // RIGHT FACE - darker purple
        glColor3f(0.4f, 0.2f, 0.6f);
        glBegin(GL_QUADS);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        // BOTTOM FACE - medium dark purple
        glColor3f(0.5f, 0.3f, 0.7f);
        glBegin(GL_QUADS);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        // LEFT FACE - medium purple
        glColor3f(0.55f, 0.25f, 0.75f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();
    }
    else if (darkTheme) {
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
void renderText(float x, float y, const char* text, bool darkTheme = false, float pressAnim = 0.0f)
{
    // Apply the same shift that the button gets when pressing
    // The text should move with the button
    float shiftLeft = 10.0f * pressAnim;
    float pressOffsetZ = 15.0f * pressAnim;  // Assuming the standard depth is 15.0f

    // Adjust text position to move with button
    x -= shiftLeft;

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

    glPushMatrix();
    glTranslatef(0, 0, -pressOffsetZ);  // Move text deeper with the button

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();

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
// We'll place a single jumbo square button in the center
// -------------------------
void initUI(int screenWidth, int screenHeight)
{
    // Square jumbo button with dimensions based on extra medium width
    float halfSize = 75.0f; // same as extra medium width

    // Center in screen
    g_jumboButton.pos = glm::vec2(screenWidth * 0.5f, screenHeight * 0.5f);
    g_jumboButton.size = glm::vec2(halfSize, halfSize); // square shape
    g_jumboButton.label = "JUMBO";
}

// -------------------------
// Mouse Button Callback
// - Toggling behavior for button with auto-unpress
// -------------------------
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    double currentTime = glfwGetTime();
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Mark button as pressed if inside
            if (isInside(g_jumboButton, (float)mx, (float)my)) {
                g_jumboButton.isPressed = true;
                g_jumboButton.pressTime = currentTime;
            }
        }
        else if (action == GLFW_RELEASE) {
            // Jumbo button: momentary press instead of toggle
            if (g_jumboButton.isPressed && isInside(g_jumboButton, (float)mx, (float)my)) {
                g_jumboButton.isSelected = true;
                g_jumboButton.autoUnpress = true;
                g_jumboButton.autoUnpressTime = currentTime + AUTO_UNPRESS_DURATION;
            }
            g_jumboButton.isPressed = false;
        }
    }
}

// -------------------------
// Update Press Animations with Nearly Instant Press-In
// - Press in is almost instant
// - Spring back up with a snappy motion
// -------------------------
void updateButtonAnimation(Button& btn, float deltaTime)
{
    float pressAnimSpeed = (float)(0.5 / PRESS_FEEDBACK_DURATION);  // Very fast for press-in
    float unpressAnimSpeed = (float)(0.5 / UNPRESS_FEEDBACK_DURATION);
    bool shouldPress = (btn.isPressed || btn.isSelected);
    float target = shouldPress ? 0.5f : 0.0f;

    if (btn.pressAnim < target) {
        // Pressing down - super fast animation (almost instant)
        // Use an even higher speed multiplier to make it feel instantaneous
        float speedMultiplier = 5.0f;  // boost the speed further
        btn.pressAnim += pressAnimSpeed * speedMultiplier * deltaTime;
        if (btn.pressAnim > target) btn.pressAnim = target;
    }
    else if (btn.pressAnim > target) {
        // Springing back up - ease-in with faster velocity
        // Start slow, then accelerate
        float distanceFromTarget = btn.pressAnim - target;
        float acceleration = 1.5f + (distanceFromTarget * 5.0f); // Higher value = more initial acceleration

        btn.pressAnim -= unpressAnimSpeed * acceleration * deltaTime;
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

    GLFWwindow* window = glfwCreateWindow(fullWidth, fullHeight, "3D Jumbo Button Primitive", monitor, nullptr);
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

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - g_lastFrameTime);
        g_lastFrameTime = currentTime;

        // Update press animations
        updateButtonAnimation(g_jumboButton, deltaTime);

        // Check for auto-unpress
        if (g_jumboButton.autoUnpress && currentTime >= g_jumboButton.autoUnpressTime) {
            g_jumboButton.autoUnpress = false;
            g_jumboButton.isSelected = false;
        }

        // Clear background to dark purple #640168 to match button theme
        float r = 100.0f / 255.0f;  // 0x64
        float g = 1.0f / 255.0f;    // 0x01
        float b = 104.0f / 255.0f;  // 0x68
        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Draw jumbo button
        {
            float bx = g_jumboButton.pos.x - g_jumboButton.size.x;
            float by = g_jumboButton.pos.y - g_jumboButton.size.y;
            float bw = g_jumboButton.size.x * 2.0f;
            float bh = g_jumboButton.size.y * 2.0f;
            float depth = 15.0f; // Slightly deeper for the jumbo button

            // Use purple theme for jumbo button
            drawButton3D(bx, by, bw, bh, depth, g_jumboButton.pressAnim, false, true);

            // Center the text in the jumbo button with white text
            float textX = bx + (bw * 0.5f) - 30.0f; // Approximate centering
            float textY = by + (bh * 0.5f) - 5.0f;
            renderText(textX, textY, g_jumboButton.label.c_str(), true, g_jumboButton.pressAnim); // Pass pressAnim to move text
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
