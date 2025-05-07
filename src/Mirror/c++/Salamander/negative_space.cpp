// ButtonNegativeSpace_FourColors_NoText.cpp
//
// A demonstration of a negative space skeuomorphic effect using four distinct colors,
// without any text rendering. The design uses:
// 1) A background color (used for clearing the window).
// 2) A front face (frame) color that is drawn around a central cutout.
// 3) Two bevel colors for the bottom and left edges of the cutout.
// The front face is drawn as a frame (four quads) so that the interior remains transparent (showing the background),
// and then the bevels are drawn last (with higher z values) so they aren’t occluded.
//
// Compile with Visual Studio (link against glfw3.lib, opengl32.lib, user32.lib, gdi32.lib).

#include <windows.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#ifndef PI
constexpr double PI = 3.14159265358979323846;
#endif

// -------------------------
// UI Button Structure
// -------------------------
struct Button {
    glm::vec2 pos;  // Center position in window coordinates
    glm::vec2 size; // Half-width and half-height
    std::string label; // (Not used in rendering)
};

Button g_negativeButton;

// -------------------------
// Draw Negative Space Button Function
// -------------------------
// This function draws the button as a frame (the front face) with a central cutout
// that shows the background. The bevels on the bottom and left edges of the cutout are
// drawn last (with higher z values) so that they appear on top.
void drawNegativeSpaceButton(float bx, float by, float bw, float bh,
    float cutoutMargin, float bevelThickness, float depth)
{
    // Define custom colors:
    const float backgroundColor[3] = { 0.93f, 0.93f, 0.93f }; // Background (and cutout) color
    const float frontFaceColor[3] = { 0.6f, 0.6f, 0.6f };    // Frame (panel) color
    const float bevelBottomColor[3] = { 0.3f, 0.3f, 0.3f };    // Bottom bevel color
    const float bevelLeftColor[3] = { 0.35f, 0.35f, 0.35f }; // Left bevel color

    // Calculate the cutout rectangle (negative space)
    float cx = bx + cutoutMargin;
    float cy = by + cutoutMargin;
    float cw = bw - 2 * cutoutMargin;
    float ch = bh - 2 * cutoutMargin;

    // Draw the frame as a border around the cutout.
    // Draw these quads at z = -0.1 so that later bevels (drawn at z >= 0) appear on top.
    glColor3f(frontFaceColor[0], frontFaceColor[1], frontFaceColor[2]);

    // Top border: from the top edge of the button to the top of the cutout.
    glBegin(GL_QUADS);
    glVertex3f(bx, by, -0.1f);
    glVertex3f(bx + bw, by, -0.1f);
    glVertex3f(bx + bw, by + cutoutMargin, -0.1f);
    glVertex3f(bx, by + cutoutMargin, -0.1f);
    glEnd();

    // Bottom border: from the bottom of the cutout to the bottom edge of the button.
    glBegin(GL_QUADS);
    glVertex3f(bx, by + bh - cutoutMargin, -0.1f);
    glVertex3f(bx + bw, by + bh - cutoutMargin, -0.1f);
    glVertex3f(bx + bw, by + bh, -0.1f);
    glVertex3f(bx, by + bh, -0.1f);
    glEnd();

    // Left border: from the left edge of the button to the left of the cutout (excluding top and bottom).
    glBegin(GL_QUADS);
    glVertex3f(bx, by + cutoutMargin, -0.1f);
    glVertex3f(bx + cutoutMargin, by + cutoutMargin, -0.1f);
    glVertex3f(bx + cutoutMargin, by + bh - cutoutMargin, -0.1f);
    glVertex3f(bx, by + bh - cutoutMargin, -0.1f);
    glEnd();

    // Right border: from the right of the cutout to the right edge of the button.
    glBegin(GL_QUADS);
    glVertex3f(bx + bw - cutoutMargin, by + cutoutMargin, -0.1f);
    glVertex3f(bx + bw, by + cutoutMargin, -0.1f);
    glVertex3f(bx + bw, by + bh - cutoutMargin, -0.1f);
    glVertex3f(bx + bw - cutoutMargin, by + bh - cutoutMargin, -0.1f);
    glEnd();

    // Now draw the bevels along the bottom and left edges of the cutout.
    // These bevels are drawn last with top vertices at z = 0 so they appear on top.

    // Bottom bevel: along the bottom edge of the cutout.
    glColor3f(bevelBottomColor[0], bevelBottomColor[1], bevelBottomColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(cx, cy + ch - bevelThickness, 0);   // top left of bevel
    glVertex3f(cx + cw, cy + ch - bevelThickness, 0);   // top right of bevel
    glVertex3f(cx + cw, cy + ch, -depth); // bottom right (recessed)
    glVertex3f(cx, cy + ch, -depth); // bottom left (recessed)
    glEnd();

    // Left bevel: along the left edge of the cutout.
    glColor3f(bevelLeftColor[0], bevelLeftColor[1], bevelLeftColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(cx + bevelThickness, cy, 0);   // top right of bevel
    glVertex3f(cx + bevelThickness, cy + ch, 0);   // bottom right of bevel
    glVertex3f(cx, cy + ch, -depth); // bottom left (recessed)
    glVertex3f(cx, cy, -depth); // top left (recessed)
    glEnd();
}

// -------------------------
// UI Initialization
// -------------------------
void initUI(int screenWidth, int screenHeight)
{
    // Define a button that is 150 px wide and 40 px tall (half-sizes: 75, 20)
    g_negativeButton.pos = glm::vec2(screenWidth * 0.5f, screenHeight * 0.5f);
    g_negativeButton.size = glm::vec2(75.0f, 20.0f);
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

    GLFWwindow* window = glfwCreateWindow(fullWidth, fullHeight, "Negative Space Button (No Text)", monitor, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create fullscreen window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set up an orthographic projection (2D view with depth for 3D effects).
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, fullWidth, fullHeight, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    initUI(fullWidth, fullHeight);

    // Main loop (static demo)
    while (!glfwWindowShouldClose(window)) {
        // Clear background with the designated background color.
        glClearColor(0.93f, 0.93f, 0.93f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Calculate button bounds based on its center position and half-sizes.
        float bx = g_negativeButton.pos.x - g_negativeButton.size.x;
        float by = g_negativeButton.pos.y - g_negativeButton.size.y;
        float bw = g_negativeButton.size.x * 2.0f;
        float bh = g_negativeButton.size.y * 2.0f;

        // Parameters for the negative space effect.
        float cutoutMargin = 10.0f;    // Margin from the edge for the cutout
        float bevelThickness = 5.0f;   // Thickness of the bevels along the cutout edges
        float depth = 10.0f;           // Depth for the beveled (recessed) effect

        // Draw the negative space button.
        drawNegativeSpaceButton(bx, by, bw, bh, cutoutMargin, bevelThickness, depth);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
