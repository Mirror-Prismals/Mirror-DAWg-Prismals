// RoundedButton3D.cpp
//
// A high-quality 3D button with rounded corners (front and back)
// that shortens front-to-back when pressed.
// 
// Key Features:
// 1) Full 3D sides with rounded corners (front and back faces)
// 2) Press animation sinks in, shifts left, and compresses depth
// 3) Toggling behavior
// 
// Uses GLFW/GLM and immediate-mode OpenGL. Requires stb_easy_font.h.

#include <windows.h>
#include <iostream>
#include <string>
#include <cmath>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "stb_easy_font.h"

#ifndef PI
constexpr double PI = 3.14159265358979323846;
#endif

// Button structure
struct Button {
    glm::vec2 pos;         // Center position
    glm::vec2 size;        // Half-width and half-height
    std::string label;
    float cornerRadius;    // Radius for rounded corners

    bool isPressed = false;      // True while mouse is down
    bool isSelected = false;     // True if toggled "on"
    float pressAnim = 0.0f;      // 0.0 -> not pressed, 0.5 -> fully pressed
};

// Global button and timing variables
Button g_roundedButton;
double g_lastFrameTime = 0.0;
constexpr double PRESS_FEEDBACK_DURATION = 0.15; // time to animate to pressed

// Draw a rounded corner using triangle fan
void drawRoundedCorner(float centerX, float centerY, float centerZ, float radius,
    float startAngle, float endAngle, int segments,
    float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(centerX, centerY, centerZ); // Center point
    for (int i = 0; i <= segments; i++) {
        float angle = startAngle + (endAngle - startAngle) * static_cast<float>(i) / segments;
        glVertex3f(centerX + radius * cos(angle), centerY + radius * sin(angle), centerZ);
    }
    glEnd();
}

// 3D Rounded Button Drawing with Press Animation
void drawRoundedButton3D(float bx, float by, float bw, float bh, float radius, float depth, float pressAnim)
{
    // Animation calculations
    float shiftLeft = 10.0f * pressAnim;
    float pressOffsetZ = depth * pressAnim;
    float newDepth = depth * (1.0f - 0.5f * pressAnim);

    float x = bx - shiftLeft;
    float y = by;
    float z = -pressOffsetZ;
    float backZ = -(pressOffsetZ + newDepth);

    // Front face color (changes with press animation)
    float frontColor = 0.8f - 0.2f * (pressAnim * 2.0f);

    // Corner rendering segments and center positions
    int cornerSegments = 8;
    float topLeftX = x + radius;
    float topLeftY = y + radius;
    float topRightX = x + bw - radius;
    float topRightY = y + radius;
    float bottomLeftX = x + radius;
    float bottomLeftY = y + bh - radius;
    float bottomRightX = x + bw - radius;
    float bottomRightY = y + bh - radius;

    // Back face corner centers
    float backTopLeftX = x - newDepth + radius;
    float backTopLeftY = y - newDepth + radius;
    float backTopRightX = x + bw - newDepth - radius;
    float backTopRightY = y - newDepth + radius;
    float backBottomLeftX = x - newDepth + radius;
    float backBottomLeftY = y + bh - newDepth - radius;
    float backBottomRightX = x + bw - newDepth - radius;
    float backBottomRightY = y + bh - newDepth - radius;

    // FRONT FACE - CENTER RECTANGLE
    glColor3f(frontColor, frontColor, frontColor);
    glBegin(GL_QUADS);
    glVertex3f(x + radius, y, z);
    glVertex3f(x + bw - radius, y, z);
    glVertex3f(x + bw - radius, y + bh, z);
    glVertex3f(x + radius, y + bh, z);
    glEnd();

    // FRONT FACE - LEFT, RIGHT, TOP, BOTTOM RECTANGLES
    glBegin(GL_QUADS);
    // Left
    glVertex3f(x, y + radius, z);
    glVertex3f(x + radius, y + radius, z);
    glVertex3f(x + radius, y + bh - radius, z);
    glVertex3f(x, y + bh - radius, z);
    // Right
    glVertex3f(x + bw - radius, y + radius, z);
    glVertex3f(x + bw, y + radius, z);
    glVertex3f(x + bw, y + bh - radius, z);
    glVertex3f(x + bw - radius, y + bh - radius, z);
    // Top
    glVertex3f(x + radius, y, z);
    glVertex3f(x + bw - radius, y, z);
    glVertex3f(x + bw - radius, y + radius, z);
    glVertex3f(x + radius, y + radius, z);
    // Bottom
    glVertex3f(x + radius, y + bh - radius, z);
    glVertex3f(x + bw - radius, y + bh - radius, z);
    glVertex3f(x + bw - radius, y + bh, z);
    glVertex3f(x + radius, y + bh, z);
    glEnd();

    // FRONT FACE - ROUNDED CORNERS
    drawRoundedCorner(topLeftX, topLeftY, z, radius, PI, 1.5f * PI, cornerSegments,
        frontColor, frontColor, frontColor);
    drawRoundedCorner(topRightX, topRightY, z, radius, 1.5f * PI, 2.0f * PI, cornerSegments,
        frontColor, frontColor, frontColor);
    drawRoundedCorner(bottomLeftX, bottomLeftY, z, radius, 0.5f * PI, PI, cornerSegments,
        frontColor, frontColor, frontColor);
    drawRoundedCorner(bottomRightX, bottomRightY, z, radius, 0, 0.5f * PI, cornerSegments,
        frontColor, frontColor, frontColor);

    // BACK FACE COLOR
    float backColor = 0.5f - 0.1f * (pressAnim * 2.0f);

    // BACK FACE - CENTER RECTANGLE
    glColor3f(backColor, backColor, backColor);
    glBegin(GL_QUADS);
    glVertex3f(x - newDepth + radius, y - newDepth, backZ);
    glVertex3f(x + bw - newDepth - radius, y - newDepth, backZ);
    glVertex3f(x + bw - newDepth - radius, y + bh - newDepth, backZ);
    glVertex3f(x - newDepth + radius, y + bh - newDepth, backZ);
    glEnd();

    // BACK FACE - LEFT, RIGHT, TOP, BOTTOM RECTANGLES
    glBegin(GL_QUADS);
    // Left
    glVertex3f(x - newDepth, y - newDepth + radius, backZ);
    glVertex3f(x - newDepth + radius, y - newDepth + radius, backZ);
    glVertex3f(x - newDepth + radius, y + bh - newDepth - radius, backZ);
    glVertex3f(x - newDepth, y + bh - newDepth - radius, backZ);
    // Right
    glVertex3f(x + bw - newDepth - radius, y - newDepth + radius, backZ);
    glVertex3f(x + bw - newDepth, y - newDepth + radius, backZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth - radius, backZ);
    glVertex3f(x + bw - newDepth - radius, y + bh - newDepth - radius, backZ);
    // Top
    glVertex3f(x - newDepth + radius, y - newDepth, backZ);
    glVertex3f(x + bw - newDepth - radius, y - newDepth, backZ);
    glVertex3f(x + bw - newDepth - radius, y - newDepth + radius, backZ);
    glVertex3f(x - newDepth + radius, y - newDepth + radius, backZ);
    // Bottom
    glVertex3f(x - newDepth + radius, y + bh - newDepth - radius, backZ);
    glVertex3f(x + bw - newDepth - radius, y + bh - newDepth - radius, backZ);
    glVertex3f(x + bw - newDepth - radius, y + bh - newDepth, backZ);
    glVertex3f(x - newDepth + radius, y + bh - newDepth, backZ);
    glEnd();

    // BACK FACE - ROUNDED CORNERS
    drawRoundedCorner(backTopLeftX, backTopLeftY, backZ, radius, PI, 1.5f * PI, cornerSegments,
        backColor, backColor, backColor);
    drawRoundedCorner(backTopRightX, backTopRightY, backZ, radius, 1.5f * PI, 2.0f * PI, cornerSegments,
        backColor, backColor, backColor);
    drawRoundedCorner(backBottomLeftX, backBottomLeftY, backZ, radius, 0.5f * PI, PI, cornerSegments,
        backColor, backColor, backColor);
    drawRoundedCorner(backBottomRightX, backBottomRightY, backZ, radius, 0, 0.5f * PI, cornerSegments,
        backColor, backColor, backColor);

    // SIDE FACES COLORS
    float topColor = 0.9f;
    float rightColor = 0.6f;
    float bottomColor = 0.7f;
    float leftColor = 0.65f;

    // CONNECT FRONT TO BACK - TOP SIDE
    glColor3f(topColor, topColor, topColor);
    // Center section
    glBegin(GL_QUADS);
    glVertex3f(x + radius, y, z);
    glVertex3f(x + bw - radius, y, z);
    glVertex3f(x + bw - newDepth - radius, y - newDepth, backZ);
    glVertex3f(x - newDepth + radius, y - newDepth, backZ);
    glEnd();

    // TOP CORNERS CONNECTING SECTIONS
    // Top-left corner connecting section
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= cornerSegments; i++) {
        float angle = PI + (PI / 2) * static_cast<float>(i) / cornerSegments;
        float frontX = topLeftX + radius * cos(angle);
        float frontY = topLeftY + radius * sin(angle);
        float backX = backTopLeftX + radius * cos(angle);
        float backY = backTopLeftY + radius * sin(angle);
        glVertex3f(frontX, frontY, z);
        glVertex3f(backX, backY, backZ);
    }
    glEnd();

    // Top-right corner connecting section
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= cornerSegments; i++) {
        float angle = 1.5f * PI + (PI / 2) * static_cast<float>(i) / cornerSegments;
        float frontX = topRightX + radius * cos(angle);
        float frontY = topRightY + radius * sin(angle);
        float backX = backTopRightX + radius * cos(angle);
        float backY = backTopRightY + radius * sin(angle);
        glVertex3f(frontX, frontY, z);
        glVertex3f(backX, backY, backZ);
    }
    glEnd();

    // CONNECT FRONT TO BACK - RIGHT SIDE
    glColor3f(rightColor, rightColor, rightColor);
    // Center section
    glBegin(GL_QUADS);
    glVertex3f(x + bw, y + radius, z);
    glVertex3f(x + bw, y + bh - radius, z);
    glVertex3f(x + bw - newDepth, y + bh - newDepth - radius, backZ);
    glVertex3f(x + bw - newDepth, y - newDepth + radius, backZ);
    glEnd();

    // RIGHT CORNERS CONNECTING SECTIONS
    // Top-right corner connecting section
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= cornerSegments; i++) {
        float angle = 1.5f * PI + (PI / 2) * static_cast<float>(i) / cornerSegments;
        float frontX = topRightX + radius * cos(angle);
        float frontY = topRightY + radius * sin(angle);
        float backX = backTopRightX + radius * cos(angle);
        float backY = backTopRightY + radius * sin(angle);
        glVertex3f(frontX, frontY, z);
        glVertex3f(backX, backY, backZ);
    }
    glEnd();

    // Bottom-right corner connecting section
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= cornerSegments; i++) {
        float angle = 0 + (PI / 2) * static_cast<float>(i) / cornerSegments;
        float frontX = bottomRightX + radius * cos(angle);
        float frontY = bottomRightY + radius * sin(angle);
        float backX = backBottomRightX + radius * cos(angle);
        float backY = backBottomRightY + radius * sin(angle);
        glVertex3f(frontX, frontY, z);
        glVertex3f(backX, backY, backZ);
    }
    glEnd();

    // CONNECT FRONT TO BACK - BOTTOM SIDE
    glColor3f(bottomColor, bottomColor, bottomColor);
    // Center section
    glBegin(GL_QUADS);
    glVertex3f(x + radius, y + bh, z);
    glVertex3f(x + bw - radius, y + bh, z);
    glVertex3f(x + bw - newDepth - radius, y + bh - newDepth, backZ);
    glVertex3f(x - newDepth + radius, y + bh - newDepth, backZ);
    glEnd();

    // BOTTOM CORNERS CONNECTING SECTIONS
    // Bottom-left corner connecting section
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= cornerSegments; i++) {
        float angle = 0.5f * PI + (PI / 2) * static_cast<float>(i) / cornerSegments;
        float frontX = bottomLeftX + radius * cos(angle);
        float frontY = bottomLeftY + radius * sin(angle);
        float backX = backBottomLeftX + radius * cos(angle);
        float backY = backBottomLeftY + radius * sin(angle);
        glVertex3f(frontX, frontY, z);
        glVertex3f(backX, backY, backZ);
    }
    glEnd();

    // Bottom-right corner connecting section
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= cornerSegments; i++) {
        float angle = 0 + (PI / 2) * static_cast<float>(i) / cornerSegments;
        float frontX = bottomRightX + radius * cos(angle);
        float frontY = bottomRightY + radius * sin(angle);
        float backX = backBottomRightX + radius * cos(angle);
        float backY = backBottomRightY + radius * sin(angle);
        glVertex3f(frontX, frontY, z);
        glVertex3f(backX, backY, backZ);
    }
    glEnd();

    // CONNECT FRONT TO BACK - LEFT SIDE
    glColor3f(leftColor, leftColor, leftColor);
    // Center section
    glBegin(GL_QUADS);
    glVertex3f(x, y + radius, z);
    glVertex3f(x, y + bh - radius, z);
    glVertex3f(x - newDepth, y + bh - newDepth - radius, backZ);
    glVertex3f(x - newDepth, y - newDepth + radius, backZ);
    glEnd();

    // LEFT CORNERS CONNECTING SECTIONS
    // Top-left corner connecting section
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= cornerSegments; i++) {
        float angle = PI + (PI / 2) * static_cast<float>(i) / cornerSegments;
        float frontX = topLeftX + radius * cos(angle);
        float frontY = topLeftY + radius * sin(angle);
        float backX = backTopLeftX + radius * cos(angle);
        float backY = backTopLeftY + radius * sin(angle);
        glVertex3f(frontX, frontY, z);
        glVertex3f(backX, backY, backZ);
    }
    glEnd();

    // Bottom-left corner connecting section
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= cornerSegments; i++) {
        float angle = 0.5f * PI + (PI / 2) * static_cast<float>(i) / cornerSegments;
        float frontX = bottomLeftX + radius * cos(angle);
        float frontY = bottomLeftY + radius * sin(angle);
        float backX = backBottomLeftX + radius * cos(angle);
        float backY = backBottomLeftY + radius * sin(angle);
        glVertex3f(frontX, frontY, z);
        glVertex3f(backX, backY, backZ);
    }
    glEnd();
}

// Text Rendering
void renderText(float x, float y, const char* text)
{
    char buffer[99999];
    int num_quads = stb_easy_font_print(x, y, const_cast<char*>(text), NULL, buffer, sizeof(buffer));
    glDisable(GL_DEPTH_TEST);
    glColor3f(0.0f, 0.0f, 0.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_DEPTH_TEST);
}

// Hit Testing
bool isInside(const Button& btn, float x, float y)
{
    float left = btn.pos.x - btn.size.x;
    float right = btn.pos.x + btn.size.x;
    float top = btn.pos.y - btn.size.y;
    float bottom = btn.pos.y + btn.size.y;

    if (x < left || x > right || y < top || y > bottom) return false;

    // Check rounded corners
    float r = btn.cornerRadius;
    if (r <= 0) return true;

    // Top-left corner
    if (x < left + r && y < top + r) {
        if ((x - (left + r)) * (x - (left + r)) + (y - (top + r)) * (y - (top + r)) > r * r)
            return false;
    }

    // Top-right corner
    if (x > right - r && y < top + r) {
        if ((x - (right - r)) * (x - (right - r)) + (y - (top + r)) * (y - (top + r)) > r * r)
            return false;
    }

    // Bottom-left corner
    if (x < left + r && y > bottom - r) {
        if ((x - (left + r)) * (x - (left + r)) + (y - (bottom - r)) * (y - (bottom - r)) > r * r)
            return false;
    }

    // Bottom-right corner
    if (x > right - r && y > bottom - r) {
        if ((x - (right - r)) * (x - (right - r)) + (y - (bottom - r)) * (y - (bottom - r)) > r * r)
            return false;
    }

    return true;
}

// Update Animation
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

// Mouse Button Callback
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    if (action == GLFW_PRESS) {
        if (isInside(g_roundedButton, (float)mx, (float)my)) {
            g_roundedButton.isPressed = true;
        }
    }
    else if (action == GLFW_RELEASE) {
        if (g_roundedButton.isPressed && isInside(g_roundedButton, (float)mx, (float)my)) {
            g_roundedButton.isSelected = !g_roundedButton.isSelected;
        }
        g_roundedButton.isPressed = false;
    }
}

int main()
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    int windowWidth = 500;
    int windowHeight = 300;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight,
        "Rounded 3D Button", nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create window\n";
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Set up projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Enable anti-aliasing
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Initialize button
    g_roundedButton.pos = glm::vec2(windowWidth * 0.5f, windowHeight * 0.5f);
    g_roundedButton.size = glm::vec2(60.0f, 20.0f);
    g_roundedButton.label = "Rounded Button";
    g_roundedButton.cornerRadius = 6.0f;

    g_lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - g_lastFrameTime);
        g_lastFrameTime = currentTime;

        updateButtonAnimation(g_roundedButton, deltaTime);

        glClearColor(0.133f, 0.133f, 0.133f, 1.0f);  // #EEEEEE
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Draw button
        float bx = g_roundedButton.pos.x - g_roundedButton.size.x;
        float by = g_roundedButton.pos.y - g_roundedButton.size.y;
        float bw = g_roundedButton.size.x * 2.0f;
        float bh = g_roundedButton.size.y * 2.0f;
        float depth = 10.0f;

        drawRoundedButton3D(bx, by, bw, bh, g_roundedButton.cornerRadius, depth,
            g_roundedButton.pressAnim);

        // Center text in button
        float textWidth = g_roundedButton.label.length() * 8.0f;
        float textX = bx + (bw - textWidth) / 2;
        float textY = by + bh / 2 - 5;
        renderText(textX, textY, g_roundedButton.label.c_str());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
