// Button3D_Flipped.cpp
//
// A single 3D button that is drawn facing the opposite direction (rotated 180° about the Y-axis).
// Uses GLFW/GLM and immediate-mode OpenGL. Requires stb_easy_font.h in the same folder.
// Compile with Visual Studio (link against glfw3.lib, opengl32.lib, user32.lib, gdi32.lib).

#include <windows.h>
#include <iostream>
#include <string>
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
    glm::vec2 pos;         // Center position (window coordinates)
    glm::vec2 size;        // Half-width and half-height
    std::string label;
    float pressAnim = 0.0f; // Animation state (0.0 means not pressed)
};

// Our single button
Button g_button;

// -------------------------
// 3D Button Drawing with Press Animation
// (Same as your original function)
// -------------------------
void drawButton3D(float bx, float by, float bw, float bh, float depth, float pressAnim, bool darkTheme = false)
{
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
// Main
// -------------------------
int main()
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Create a windowed mode window.
    int width = 800, height = 600;
    GLFWwindow* window = glfwCreateWindow(width, height, "3D Flipped Button", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Initialize our single button in the center
    g_button.pos = glm::vec2(width / 2.0f, height / 2.0f);
    g_button.size = glm::vec2(60.0f, 20.0f);
    g_button.label = "Flipped Button";

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.933f, 0.933f, 0.933f, 1.0f);  // light gray background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Compute button's bottom-left corner and dimensions
        float bx = g_button.pos.x - g_button.size.x;
        float by = g_button.pos.y - g_button.size.y;
        float bw = g_button.size.x * 2.0f;
        float bh = g_button.size.y * 2.0f;
        float depth = 10.0f;

        // Draw the button flipped by 180° about the Y-axis.
        glPushMatrix();
        // Translate to the center of the button
        glTranslatef(g_button.pos.x, g_button.pos.y, 0);
        // Rotate 180 degrees around the Y-axis
        glRotatef(180, 0, 1, 0);
        // Translate back so that the button remains in place
        glTranslatef(-g_button.pos.x, -g_button.pos.y, 0);
        // Draw the button (using light theme here)
        drawButton3D(bx, by, bw, bh, depth, g_button.pressAnim, false);
        // Render label (adjust x to center the text roughly)
        renderText(bx + 15, by + bh / 2 - 5, g_button.label.c_str(), false);
        glPopMatrix();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
