//
// Button3D_Fader_Housed.cpp
//
// 3D fader control: a small 3D button on a vertical track housed within a 3D frame.
// The housing is rendered with a 3D style similar to the button walls but remains fixed.
// When you press and drag the button, it moves up and down along the track.
//
// Uses GLFW/GLM and immediate-mode OpenGL. Requires stb_easy_font.h in the same folder.
// Compile with Visual Studio (link against glfw3.lib, opengl32.lib, user32.lib, gdi32.lib).

#include <windows.h>
#include <iostream>
#include <string>
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
// UI Button Structure (the fader knob)
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

Button g_faderButton;

// Global variable for tracking vertical drag offset.
float g_dragOffsetY = 0.0f;

// Timing constant for press animation
constexpr double PRESS_FEEDBACK_DURATION = 0.15; // seconds

// For animation updates
double g_lastFrameTime = 0.0;

// -------------------------
// Track (Fader) definitions
// -------------------------
struct Track {
    float centerX;    // x-coordinate of track center
    float topY;       // top of track
    float bottomY;    // bottom of track
    float width;      // track width
};

Track g_track;

// -------------------------
// Housing definitions (the frame around the fader)
// -------------------------
struct Housing {
    float x;       // top-left x of front face
    float y;       // top-left y of front face
    float width;   // width of front face
    float height;  // height of front face
    float depth;   // thickness of the housing walls
};

Housing g_housing;

// -------------------------
// 3D Button Drawing with Press Animation
// -------------------------
void drawButton3D(float bx, float by, float bw, float bh, float depth, float pressAnim, bool darkTheme = false)
{
    // pressAnim in [0, 0.5]: 0.5 means fully pressed.
    float shiftLeft = 10.0f * pressAnim;
    float pressOffsetZ = depth * pressAnim;
    float newDepth = depth * (1.0f - 0.5f * pressAnim);
    float x = bx - shiftLeft;
    float y = by;

    if (darkTheme) {
        float frontColor = 0.3f - 0.1f * (pressAnim * 2.0f);
        glColor3f(frontColor, frontColor, frontColor);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glEnd();

        glColor3f(0.4f, 0.4f, 0.4f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        glColor3f(0.25f, 0.25f, 0.25f);
        glBegin(GL_QUADS);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        glColor3f(0.35f, 0.35f, 0.35f);
        glBegin(GL_QUADS);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        glColor3f(0.28f, 0.28f, 0.28f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();
    }
    else {
        float frontColor = 0.8f - 0.2f * (pressAnim * 2.0f);
        glColor3f(frontColor, frontColor, frontColor);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glEnd();

        glColor3f(0.9f, 0.9f, 0.9f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_QUADS);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

        glColor3f(0.7f, 0.7f, 0.7f);
        glBegin(GL_QUADS);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
        glEnd();

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

    glDisable(GL_DEPTH_TEST);
    if (darkTheme)
        glColor3f(0.9f, 0.9f, 0.9f);
    else
        glColor3f(0.0f, 0.0f, 0.0f);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_DEPTH_TEST);
}

// -------------------------
// Hit Testing for the button
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
// Draw the housing (3D frame) for the fader
// -------------------------
void drawHousing()
{
    // Mimic the light theme style used for the button.
    float x = g_housing.x;
    float y = g_housing.y;
    float w = g_housing.width;
    float h = g_housing.height;
    float depth = g_housing.depth;  // wall thickness

    // Front face (z = 0)
    glColor3f(0.8f, 0.8f, 0.8f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x, y + h, 0);
    glEnd();

    // Top face
    glColor3f(0.9f, 0.9f, 0.9f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w - depth, y - depth, -depth);
    glVertex3f(x - depth, y - depth, -depth);
    glEnd();

    // Right face
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_QUADS);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x + w - depth, y + h - depth, -depth);
    glVertex3f(x + w - depth, y - depth, -depth);
    glEnd();

    // Bottom face
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + h, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x + w - depth, y + h - depth, -depth);
    glVertex3f(x - depth, y + h - depth, -depth);
    glEnd();

    // Left face
    glColor3f(0.65f, 0.65f, 0.65f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x, y + h, 0);
    glVertex3f(x - depth, y + h - depth, -depth);
    glVertex3f(x - depth, y - depth, -depth);
    glEnd();
}

// -------------------------
// Initialize UI: set up track, housing, and button positions
// -------------------------
void initUI(int screenWidth, int screenHeight)
{
    // Define the fader track as a vertical strip in the center.
    g_track.centerX = screenWidth * 0.5f;
    g_track.width = 20.0f;
    // Let the track be 300 pixels tall and centered vertically.
    g_track.topY = (screenHeight - 300.0f) * 0.5f;
    g_track.bottomY = g_track.topY + 300.0f;

    // Define the housing to surround the track.
    // Add a border of 10 pixels on each side.
    g_housing.x = g_track.centerX - (g_track.width + 20.0f) * 0.5f;
    g_housing.y = g_track.topY - 10.0f;
    g_housing.width = g_track.width + 20.0f;
    g_housing.height = 300.0f + 20.0f;
    g_housing.depth = 3.0f;  // wall thickness

    // Scale down the button to 1/6 of its original size.
    float halfSize = 75.0f / 6.0f;  // ≈12.5
    // Position the button on the track (horizontally centered) and initially centered vertically.
    float buttonY = (g_track.topY + g_track.bottomY) * 0.5f;
    g_faderButton.pos = glm::vec2(g_track.centerX, buttonY);
    g_faderButton.size = glm::vec2(halfSize, halfSize);
    // Set the label to a single backslash.
    g_faderButton.label = "\\";
}

// -------------------------
// Mouse Button Callback
// -------------------------
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            if (isInside(g_faderButton, (float)mx, (float)my)) {
                g_faderButton.isPressed = true;
                g_faderButton.pressTime = glfwGetTime();
                // Record vertical offset between mouse and button center.
                g_dragOffsetY = g_faderButton.pos.y - (float)my;
            }
        }
        else if (action == GLFW_RELEASE) {
            g_faderButton.isPressed = false;
        }
    }
}

// -------------------------
// Cursor Position Callback (for dragging)
// -------------------------
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (g_faderButton.isPressed) {
        float newY = (float)ypos + g_dragOffsetY;
        // Clamp within the track boundaries.
        if (newY < g_track.topY + g_faderButton.size.y)
            newY = g_track.topY + g_faderButton.size.y;
        if (newY > g_track.bottomY - g_faderButton.size.y)
            newY = g_track.bottomY - g_faderButton.size.y;
        g_faderButton.pos.y = newY;
        // Keep x fixed on the track.
        g_faderButton.pos.x = g_track.centerX;
    }
}

// -------------------------
// Update Press Animation for the button
// -------------------------
void updateButtonAnimation(Button& btn, float deltaTime)
{
    float animSpeed = (float)(0.5 / PRESS_FEEDBACK_DURATION);
    float target = btn.isPressed ? 0.5f : 0.0f;
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
// Main
// -------------------------
int main()
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Fullscreen mode
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int fullWidth = mode->width;
    int fullHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(fullWidth, fullHeight, "3D Fader with Housing", monitor, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create fullscreen window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, fullWidth, fullHeight, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Initialize UI components
    initUI(fullWidth, fullHeight);
    g_lastFrameTime = glfwGetTime();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - g_lastFrameTime);
        g_lastFrameTime = currentTime;

        updateButtonAnimation(g_faderButton, deltaTime);

        glClearColor(0.933f, 0.933f, 0.933f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Draw the housing (frame)
        drawHousing();

        // Draw the fader track inside the housing.
        glColor3f(0.8f, 0.8f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(g_track.centerX - g_track.width * 0.5f, g_track.topY);
        glVertex2f(g_track.centerX + g_track.width * 0.5f, g_track.topY);
        glVertex2f(g_track.centerX + g_track.width * 0.5f, g_track.bottomY);
        glVertex2f(g_track.centerX - g_track.width * 0.5f, g_track.bottomY);
        glEnd();

        // Disable depth test so the button draws on top.
        glDisable(GL_DEPTH_TEST);
        {
            float bx = g_faderButton.pos.x - g_faderButton.size.x;
            float by = g_faderButton.pos.y - g_faderButton.size.y;
            float bw = g_faderButton.size.x * 2.0f;
            float bh = g_faderButton.size.y * 2.0f;
            float depth = 15.0f / 6.0f;  // scaled depth for button
            drawButton3D(bx, by, bw, bh, depth, g_faderButton.pressAnim, false);

            // Center the label (offsets chosen experimentally)
            float textX = bx + (bw * 0.5f) - 5.0f;
            float textY = by + (bh * 0.5f) - 3.0f;
            renderText(textX, textY, g_faderButton.label.c_str(), false);
        }
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
