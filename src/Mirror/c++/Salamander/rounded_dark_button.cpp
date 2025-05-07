// RoundedButton3D_Primitives.cpp
//
// High-quality 3D rounded buttons that toggle and shorten front-to-back
// when pressed, rather than shrinking them left-to-right.
//
// Features:
// 1) Full 3D extrusion of rounded buttons.
// 2) Two button sizes: "Medium" (light theme) and "Extra Medium" (dark theme).
// 3) Press animation: sinks in, shifts left, and compresses depth.
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

// Include your local stb_easy_font.h
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

// Global buttons
Button g_mediumButton;
Button g_extraMediumButton;

// Timing constant for press animation
constexpr double PRESS_FEEDBACK_DURATION = 0.15; // seconds

// For animation updates
double g_lastFrameTime = 0.0;

// -------------------------
// Helper: Generate Rounded Rectangle Vertices
// -------------------------
// Generates vertices for a rounded rectangle defined by top-left corner (x,y),
// dimensions (width, height), corner radius, and segments per corner.
// The vertices are returned in clockwise order.
std::vector<glm::vec2> getRoundedRectVertices(float x, float y, float width, float height, float radius, int segments) {
    std::vector<glm::vec2> vertices;

    // Clamp the radius to half the width/height
    float r = radius;
    if (r > width / 2.0f) r = width / 2.0f;
    if (r > height / 2.0f) r = height / 2.0f;

    // Define centers for the four corner arcs.
    // Note: our coordinate system has y increasing downward.
    glm::vec2 topLeft(x + r, y + r);
    glm::vec2 topRight(x + width - r, y + r);
    glm::vec2 bottomRight(x + width - r, y + height - r);
    glm::vec2 bottomLeft(x + r, y + height - r);

    // Top-left corner: angles from PI to 1.5PI
    for (int i = 0; i <= segments; i++) {
        float theta = PI + (PI / 2.0f) * (float(i) / segments);
        vertices.push_back(glm::vec2(topLeft.x + cos(theta) * r, topLeft.y + sin(theta) * r));
    }
    // Top edge: top-right arc, angles from 1.5PI to 2PI
    for (int i = 0; i <= segments; i++) {
        float theta = 1.5f * PI + (PI / 2.0f) * (float(i) / segments);
        vertices.push_back(glm::vec2(topRight.x + cos(theta) * r, topRight.y + sin(theta) * r));
    }
    // Right edge: bottom-right arc, angles from 0 to 0.5PI
    for (int i = 0; i <= segments; i++) {
        float theta = 0.0f + (PI / 2.0f) * (float(i) / segments);
        vertices.push_back(glm::vec2(bottomRight.x + cos(theta) * r, bottomRight.y + sin(theta) * r));
    }
    // Bottom edge: bottom-left arc, angles from 0.5PI to PI
    for (int i = 0; i <= segments; i++) {
        float theta = 0.5f * PI + (PI / 2.0f) * (float(i) / segments);
        vertices.push_back(glm::vec2(bottomLeft.x + cos(theta) * r, bottomLeft.y + sin(theta) * r));
    }

    return vertices;
}

// -------------------------
// 3D Rounded Button Drawing with Press Animation
// -------------------------
// Draws a rounded button with front face and extruded side faces.
// The pressAnim parameter [0, 0.5] controls shift, sink, and depth compression.
void drawRoundedButton3D(float bx, float by, float bw, float bh, float cornerRadius, float depth, float pressAnim, bool darkTheme = false)
{
    // Compute animation adjustments
    float shiftLeft = 10.0f * pressAnim;             // shift left up to 10 pixels
    float pressOffsetZ = depth * pressAnim;            // sink effect in z
    float newDepth = depth * (1.0f - 0.5f * pressAnim);  // compress depth

    // Adjusted front face position (apply horizontal shift)
    float x = bx - shiftLeft;
    float y = by;

    // Generate rounded rectangle vertices for the front face.
    // We use 8 segments per corner.
    std::vector<glm::vec2> frontVerts = getRoundedRectVertices(x, y, bw, bh, cornerRadius, 8);

    // Create corresponding back face vertices by offsetting both x and y.
    std::vector<glm::vec2> backVerts;
    for (const auto& v : frontVerts) {
        // For the back face, we offset further (simulate perspective) 
        backVerts.push_back(glm::vec2(v.x - newDepth, v.y - newDepth));
    }

    // Choose colors based on theme and press animation.
    // Front face color (light theme: 0.8->0.6; dark theme: 0.3->0.2)
    float frontColor = darkTheme ? (0.3f - 0.1f * (pressAnim * 2.0f))
        : (0.8f - 0.2f * (pressAnim * 2.0f));
    // Side face color (a bit darker than front face)
    float sideColor = darkTheme ? 0.25f : 0.7f;

    // -------------------------
    // Draw Front Face (rounded polygon)
    // -------------------------
    glColor3f(frontColor, frontColor, frontColor);
    glBegin(GL_POLYGON);
    for (const auto& v : frontVerts) {
        glVertex3f(v.x, v.y, -pressOffsetZ);
    }
    glEnd();

    // -------------------------
    // Draw Side Faces along each edge of the outline
    // -------------------------
    glColor3f(sideColor, sideColor, sideColor);
    int count = frontVerts.size();
    glBegin(GL_QUADS);
    for (int i = 0; i < count; i++) {
        int next = (i + 1) % count;
        // Quad vertices: front current, front next, back next, back current.
        glVertex3f(frontVerts[i].x, frontVerts[i].y, -pressOffsetZ);
        glVertex3f(frontVerts[next].x, frontVerts[next].y, -pressOffsetZ);
        glVertex3f(backVerts[next].x, backVerts[next].y, -(pressOffsetZ + newDepth));
        glVertex3f(backVerts[i].x, backVerts[i].y, -(pressOffsetZ + newDepth));
    }
    glEnd();
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
        glColor3f(0.9f, 0.9f, 0.9f); // light text for dark theme
    }
    else {
        glColor3f(0.0f, 0.0f, 0.0f); // dark text for light theme
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
// Place 1 medium button (light theme) and 1 extra medium button (dark theme)
// -------------------------
void initUI(int screenWidth, int screenHeight)
{
    // Medium button: 120 pixels wide, 40 tall (half sizes: 60, 20)
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
// Toggling behavior for buttons
// -------------------------
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    double currentTime = glfwGetTime();
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
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
            if (g_mediumButton.isPressed && isInside(g_mediumButton, (float)mx, (float)my)) {
                g_mediumButton.isSelected = !g_mediumButton.isSelected;
            }
            g_mediumButton.isPressed = false;

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
// -------------------------
void updateButtonAnimation(Button& btn, float deltaTime)
{
    float animSpeed = static_cast<float>(0.5 / PRESS_FEEDBACK_DURATION);
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

    // Use fullscreen mode
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int fullWidth = mode->width;
    int fullHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(fullWidth, fullHeight, "3D Rounded Button Primitives", monitor, nullptr);
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

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - g_lastFrameTime);
        g_lastFrameTime = currentTime;

        updateButtonAnimation(g_mediumButton, deltaTime);
        updateButtonAnimation(g_extraMediumButton, deltaTime);

        // Clear background to a light gray (#EEEEEE)
        glClearColor(0.933f, 0.933f, 0.933f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Draw medium button (light theme)
        {
            float bx = g_mediumButton.pos.x - g_mediumButton.size.x;
            float by = g_mediumButton.pos.y - g_mediumButton.size.y;
            float bw = g_mediumButton.size.x * 2.0f;
            float bh = g_mediumButton.size.y * 2.0f;
            float depth = 10.0f;
            float cornerRadius = 10.0f; // adjust as needed

            drawRoundedButton3D(bx, by, bw, bh, cornerRadius, depth, g_mediumButton.pressAnim, false);
            // Render label (offset a bit for centering)
            renderText(bx + 10, by + bh / 2 - 5, g_mediumButton.label.c_str(), false);
        }

        // Draw extra medium button (dark theme)
        {
            float bx = g_extraMediumButton.pos.x - g_extraMediumButton.size.x;
            float by = g_extraMediumButton.pos.y - g_extraMediumButton.size.y;
            float bw = g_extraMediumButton.size.x * 2.0f;
            float bh = g_extraMediumButton.size.y * 2.0f;
            float depth = 10.0f;
            float cornerRadius = 10.0f;

            drawRoundedButton3D(bx, by, bw, bh, cornerRadius, depth, g_extraMediumButton.pressAnim, true);
            renderText(bx + 15, by + bh / 2 - 5, g_extraMediumButton.label.c_str(), true);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
