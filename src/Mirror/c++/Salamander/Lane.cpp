// CombinedButton_Horizontal_SmallSlot.cpp
//
// This demonstration creates a full–width horizontal 3D dark-mode button ("wall")
// with an integrated negative space cutout effect. Over the entire button,
// an outer negative space frame (with bevels) is drawn. Then, a very narrow
// inner slot is cut out in the center—its fill matches the background (so you see through it)
// and its bottom and left bevels are drawn with a deeper effect.
// 
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
// Button Structure
// -------------------------
struct Button {
    glm::vec2 pos;      // Center position in window coordinates
    glm::vec2 size;     // Half-width and half-height
    std::string label;  // Button label
};

Button g_darkButton;  // Global button instance

// -------------------------
// Outer Negative Space Cutout Function
// -------------------------
// Draws a frame (with bevels) over the given rectangle to create an outer negative space effect.
void drawNegativeSpaceButton(float bx, float by, float bw, float bh,
    float cutoutMargin, float bevelThickness, float depth)
{
    // Colors for the outer cutout.
    const float frameColor[3] = { 0.6f, 0.6f, 0.6f };
    const float bevelBottomColor[3] = { 0.3f, 0.3f, 0.3f };
    const float bevelLeftColor[3] = { 0.35f, 0.35f, 0.35f };

    // Compute the inner rectangle of the outer cutout.
    float cx = bx + cutoutMargin;
    float cy = by + cutoutMargin;
    float cw = bw - 2 * cutoutMargin;
    float ch = bh - 2 * cutoutMargin;

    // Draw the frame borders.
    glColor3f(frameColor[0], frameColor[1], frameColor[2]);
    // Top border.
    glBegin(GL_QUADS);
    glVertex3f(bx, by, -0.1f);
    glVertex3f(bx + bw, by, -0.1f);
    glVertex3f(bx + bw, by + cutoutMargin, -0.1f);
    glVertex3f(bx, by + cutoutMargin, -0.1f);
    glEnd();
    // Bottom border.
    glBegin(GL_QUADS);
    glVertex3f(bx, by + bh - cutoutMargin, -0.1f);
    glVertex3f(bx + bw, by + bh - cutoutMargin, -0.1f);
    glVertex3f(bx + bw, by + bh, -0.1f);
    glVertex3f(bx, by + bh, -0.1f);
    glEnd();
    // Left border.
    glBegin(GL_QUADS);
    glVertex3f(bx, by + cutoutMargin, -0.1f);
    glVertex3f(bx + cutoutMargin, by + cutoutMargin, -0.1f);
    glVertex3f(bx + cutoutMargin, by + bh - cutoutMargin, -0.1f);
    glVertex3f(bx, by + bh - cutoutMargin, -0.1f);
    glEnd();
    // Right border.
    glBegin(GL_QUADS);
    glVertex3f(bx + bw - cutoutMargin, by + cutoutMargin, -0.1f);
    glVertex3f(bx + bw, by + cutoutMargin, -0.1f);
    glVertex3f(bx + bw, by + bh - cutoutMargin, -0.1f);
    glVertex3f(bx + bw - cutoutMargin, by + bh - cutoutMargin, -0.1f);
    glEnd();

    // Draw bevels along the bottom and left edges of the outer cutout.
    // Bottom bevel.
    glColor3f(bevelBottomColor[0], bevelBottomColor[1], bevelBottomColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(cx, cy + ch - bevelThickness, 0);
    glVertex3f(cx + cw, cy + ch - bevelThickness, 0);
    glVertex3f(cx + cw, cy + ch, -depth);
    glVertex3f(cx, cy + ch, -depth);
    glEnd();
    // Left bevel.
    glColor3f(bevelLeftColor[0], bevelLeftColor[1], bevelLeftColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(cx + bevelThickness, cy, 0);
    glVertex3f(cx + bevelThickness, cy + ch, 0);
    glVertex3f(cx, cy + ch, -depth);
    glVertex3f(cx, cy, -depth);
    glEnd();
}

// -------------------------
// Clear Inner Slot Function
// -------------------------
// Draws a small, narrow slot in the center of the outer cutout. The slot is filled with the background color,
// making it appear as a true "cut" (transparent), and its bottom and left bevels are drawn with a deeper effect.
void drawClearInnerSlot(float bx, float by, float bw, float bh,
    float slotWidth, float slotHeight,
    float innerBevelThickness, float innerBevelDepth)
{
    // Background color (matches the clear background).
    const float backgroundColor[3] = { 0.933f, 0.933f, 0.933f };
    // Bevel colors.
    const float bevelBottomColor[3] = { 0.3f, 0.3f, 0.3f };
    const float bevelLeftColor[3] = { 0.35f, 0.35f, 0.35f };

    // Compute the center of the outer cutout rectangle.
    float centerX = bx + bw * 0.5f;
    float centerY = by + bh * 0.5f;
    // Define the small slot rectangle (centered).
    float icx = centerX - slotWidth * 0.5f;
    float icy = centerY - slotHeight * 0.5f;

    // Fill the slot with the background color.
    glColor3f(backgroundColor[0], backgroundColor[1], backgroundColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(icx, icy, 0);
    glVertex3f(icx + slotWidth, icy, 0);
    glVertex3f(icx + slotWidth, icy + slotHeight, 0);
    glVertex3f(icx, icy + slotHeight, 0);
    glEnd();

    // Draw inner bevels with a deeper effect.
    // Bottom bevel.
    glColor3f(bevelBottomColor[0], bevelBottomColor[1], bevelBottomColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(icx, icy + slotHeight - innerBevelThickness, 0);
    glVertex3f(icx + slotWidth, icy + slotHeight - innerBevelThickness, 0);
    glVertex3f(icx + slotWidth, icy + slotHeight, -innerBevelDepth);
    glVertex3f(icx, icy + slotHeight, -innerBevelDepth);
    glEnd();
    // Left bevel.
    glColor3f(bevelLeftColor[0], bevelLeftColor[1], bevelLeftColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(icx + innerBevelThickness, icy, 0);
    glVertex3f(icx + innerBevelThickness, icy + slotHeight, 0);
    glVertex3f(icx, icy + slotHeight, -innerBevelDepth);
    glVertex3f(icx, icy, -innerBevelDepth);
    glEnd();
}

// -------------------------
// 3D Dark Mode Button Function
// -------------------------
// Renders the dark "wall" button with a 3D look and bevels.
void drawButton3D(float bx, float by, float bw, float bh, float depth, bool darkTheme = true)
{
    float pressOffsetZ = 0.0f;
    float bevelThickness = depth * 0.5f;
    float x = bx;
    float y = by;

    if (darkTheme) {
        // Front face (solid dark gray).
        float frontColor = 0.3f;
        glColor3f(frontColor, frontColor, frontColor);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glEnd();

        // Top bevel (highlight).
        glColor3f(0.4f, 0.4f, 0.4f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw - bevelThickness, y - bevelThickness, -(pressOffsetZ + bevelThickness));
        glVertex3f(x - bevelThickness, y - bevelThickness, -(pressOffsetZ + bevelThickness));
        glEnd();

        // Left bevel (highlight).
        glColor3f(0.42f, 0.42f, 0.42f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x - bevelThickness, y + bh - bevelThickness, -(pressOffsetZ + bevelThickness));
        glVertex3f(x - bevelThickness, y - bevelThickness, -(pressOffsetZ + bevelThickness));
        glEnd();

        // Right bevel (shadow).
        glColor3f(0.25f, 0.25f, 0.25f);
        glBegin(GL_QUADS);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw + bevelThickness, y + bh + bevelThickness, -(pressOffsetZ + bevelThickness));
        glVertex3f(x + bw + bevelThickness, y + bevelThickness, -(pressOffsetZ + bevelThickness));
        glEnd();

        // Bottom bevel (shadow).
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
// Render Text Function
// -------------------------
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
// UI Initialization (Horizontal, Full–Width)
// -------------------------
void initUI(int screenWidth, int screenHeight)
{
    // Make the button span the full horizontal width.
    g_darkButton.pos = glm::vec2(screenWidth * 0.5f, screenHeight * 0.5f);
    // Set half-width to screenWidth/2 (so full width = screenWidth) and half-height fixed (e.g., 50 pixels).
    g_darkButton.size = glm::vec2(screenWidth * 0.5f, 50.0f);
    g_darkButton.label = "Fixed Unpressable Horizontal Button";
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

    GLFWwindow* window = glfwCreateWindow(fullWidth, fullHeight, "Horizontal Button with Small Deep Slot", monitor, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create fullscreen window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set up an orthographic projection.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, fullWidth, fullHeight, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    initUI(fullWidth, fullHeight);

    // Parameters for the outer negative space.
    float cutoutMargin = 10.0f;  // Margin from button edge for the outer cutout
    float nsBevelThickness = 5.0f;   // Bevel thickness for outer cutout
    float nsDepth = 10.0f;  // Depth for outer bevels

    // Parameters for the clear inner slot.
    float slotWidth = 20.0f;  // Narrow slot width
    float slotHeight = 40.0f;  // Slot height (can be adjusted)
    float innerBevelThickness = 5.0f;  // Thickness for the inner bevel
    float innerBevelDepth = nsDepth + 20.0f; // Deeper inner bevel (e.g., 10+20 = 30)

    // Depth for the dark mode button.
    float buttonDepth = 20.0f;

    // Main render loop.
    while (!glfwWindowShouldClose(window)) {
        // Clear background (light gray).
        glClearColor(0.933f, 0.933f, 0.933f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Compute button bounds.
        float bx = g_darkButton.pos.x - g_darkButton.size.x;
        float by = g_darkButton.pos.y - g_darkButton.size.y;
        float bw = g_darkButton.size.x * 2.0f;
        float bh = g_darkButton.size.y * 2.0f;

        // Draw the dark 3D button.
        drawButton3D(bx, by, bw, bh, buttonDepth, true);

        // Draw the outer negative space cutout.
        glDisable(GL_DEPTH_TEST);
        drawNegativeSpaceButton(bx, by, bw, bh, cutoutMargin, nsBevelThickness, nsDepth);

        // Draw the clear inner slot (the "actual cut") within the outer cutout.
        // The outer cutout area is defined by: (bx+cutoutMargin, by+cutoutMargin, bw-2*cutoutMargin, bh-2*cutoutMargin)
        drawClearInnerSlot(bx + cutoutMargin, by + cutoutMargin, bw - 2 * cutoutMargin, bh - 2 * cutoutMargin,
            slotWidth, slotHeight, innerBevelThickness, innerBevelDepth);
        glEnable(GL_DEPTH_TEST);

        // Render the button label.
        renderText(bx + 15, by + bh / 2 - 5, g_darkButton.label.c_str(), true);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
