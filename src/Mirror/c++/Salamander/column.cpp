// Button3D_Dark_AllBevels_Fixed_Tutorial.cpp
//
// This demonstration creates a 3D dark-mode button using GLFW/GLM and immediate-mode OpenGL.
// The button is fixed (unpressable) and spans the full height of the screen while retaining its original width.
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
// The button's position is stored as its center.
// The size is defined using half-width and half-height.
struct Button {
    glm::vec2 pos;         // Center position (window coordinates)
    glm::vec2 size;        // Half-width and half-height
    std::string label;     // Button label
};

Button g_darkButton;

// -------------------------
// drawButton3D Function (Modified: Fixed, unpressable button)
// -------------------------
// This function draws a square button with a 3D illusion.
// The press animation parameters have been removed so that the button is static.
void drawButton3D(float bx, float by, float bw, float bh, float depth, bool darkTheme = true)
{
    // No press animation: these values are fixed.
    float shiftLeft = 0.0f;
    float pressOffsetZ = 0.0f;
    float newDepth = depth;

    // Increase bevel thickness based on the new depth.
    float bevelThickness = newDepth * 0.5f;

    // Adjust the starting coordinate of the button (no shift).
    float x = bx;
    float y = by;

    if (darkTheme) {
        // ----- FRONT FACE -----
        // Constant dark gray front face.
        float frontColor = 0.3f;
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

        // ----- BOTTOM BEVEL (Shadow) -----
        glColor3f(0.23f, 0.23f, 0.23f);
        glBegin(GL_QUADS);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw + bevelThickness, y + bh + bevelThickness, -(pressOffsetZ + bevelThickness));
        glVertex3f(x + bevelThickness, y + bh + bevelThickness, -(pressOffsetZ + bevelThickness));
        glEnd();
    }
}

// -------------------------
// renderText Function
// -------------------------
// Uses stb_easy_font to render the button's label.
// Depth testing is temporarily disabled so that the text appears on top of the button.
void renderText(float x, float y, const char* text, bool darkTheme = true)
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
// initUI Function (Modified: Full screen height button)
// -------------------------
// Initializes the button's parameters so that its height equals the screen height while the width remains the same.
void initUI(int screenWidth, int screenHeight)
{
    g_darkButton.pos = glm::vec2(screenWidth * 0.5f, screenHeight * 0.5f);
    // Set half-width as 100 and half-height as half the screen height.
    g_darkButton.size = glm::vec2(100.0f, screenHeight / 2.0f);
    g_darkButton.label = "Fixed Unpressable Button";
}

// -------------------------
// main Function (Modified: Remove mouse callbacks and animation)
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

    GLFWwindow* window = glfwCreateWindow(fullWidth, fullHeight, "3D Dark Mode Fixed Button", monitor, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create fullscreen window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);
    // Mouse callbacks have been removed so the button is unpressable.

    // Set up an orthographic projection for 2D UI with depth effects.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, fullWidth, fullHeight, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    initUI(fullWidth, fullHeight);

    // Main loop: draw the button (no press animation update).
    while (!glfwWindowShouldClose(window)) {
        // Clear the screen with a light gray background.
        glClearColor(0.933f, 0.933f, 0.933f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Calculate the button bounds based on its center position and half-size.
        float bx = g_darkButton.pos.x - g_darkButton.size.x;
        float by = g_darkButton.pos.y - g_darkButton.size.y;
        float bw = g_darkButton.size.x * 2.0f;
        float bh = g_darkButton.size.y * 2.0f;
        // Use an increased depth for a pronounced 3D effect.
        float depth = 20.0f;

        // Draw the button and its label.
        drawButton3D(bx, by, bw, bh, depth, true);
        renderText(bx + 15, by + bh / 2 - 5, g_darkButton.label.c_str(), true);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
