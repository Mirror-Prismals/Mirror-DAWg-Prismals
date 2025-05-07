// CombinedButton_Vertical_6Slots_RefinedCorners.cpp
//
// This code creates a full–height, double–wide (200 px) vertical 3D dark–mode button
// centered on the screen. An outer negative–space frame is drawn over it. 
// Inside that frame, exactly 6 slots are drawn, evenly spaced, but leaving extra 
// top/bottom margins so they don't go all the way to the edge. Each slot is rendered 
// with a very deep bottom/left bevel. The bottom–left corner is subdivided into two 
// triangles to avoid overlapping quads and to blend colors smoothly.
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
    glm::vec2 size;     // Half–width and half–height
    std::string label;  // Button label
};

Button g_darkButton;  // Global button instance

// -------------------------
// Outer Negative Space Cutout Function
// -------------------------
void drawNegativeSpaceButton(float bx, float by, float bw, float bh,
    float cutoutMargin, float bevelThickness, float depth)
{
    const float frameColor[3] = { 0.6f, 0.6f, 0.6f };
    const float bevelBottomColor[3] = { 0.3f, 0.3f, 0.3f };
    const float bevelLeftColor[3] = { 0.35f, 0.35f, 0.35f };

    float cx = bx + cutoutMargin;
    float cy = by + cutoutMargin;
    float cw = bw - 2 * cutoutMargin;
    float ch = bh - 2 * cutoutMargin;

    // Frame borders
    glColor3f(frameColor[0], frameColor[1], frameColor[2]);
    // Top border
    glBegin(GL_QUADS);
    glVertex3f(bx, by, -0.1f);
    glVertex3f(bx + bw, by, -0.1f);
    glVertex3f(bx + bw, by + cutoutMargin, -0.1f);
    glVertex3f(bx, by + cutoutMargin, -0.1f);
    glEnd();
    // Bottom border
    glBegin(GL_QUADS);
    glVertex3f(bx, by + bh - cutoutMargin, -0.1f);
    glVertex3f(bx + bw, by + bh - cutoutMargin, -0.1f);
    glVertex3f(bx + bw, by + bh, -0.1f);
    glVertex3f(bx, by + bh, -0.1f);
    glEnd();
    // Left border
    glBegin(GL_QUADS);
    glVertex3f(bx, by + cutoutMargin, -0.1f);
    glVertex3f(bx + cutoutMargin, by + cutoutMargin, -0.1f);
    glVertex3f(bx + cutoutMargin, by + bh - cutoutMargin, -0.1f);
    glVertex3f(bx, by + bh - cutoutMargin, -0.1f);
    glEnd();
    // Right border
    glBegin(GL_QUADS);
    glVertex3f(bx + bw - cutoutMargin, by + cutoutMargin, -0.1f);
    glVertex3f(bx + bw, by + cutoutMargin, -0.1f);
    glVertex3f(bx + bw, by + bh - cutoutMargin, -0.1f);
    glVertex3f(bx + bw - cutoutMargin, by + bh - cutoutMargin, -0.1f);
    glEnd();

    // Bottom bevel
    glColor3f(bevelBottomColor[0], bevelBottomColor[1], bevelBottomColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(cx, cy + ch - bevelThickness, 0);
    glVertex3f(cx + cw, cy + ch - bevelThickness, 0);
    glVertex3f(cx + cw, cy + ch, -depth);
    glVertex3f(cx, cy + ch, -depth);
    glEnd();
    // Left bevel
    glColor3f(bevelLeftColor[0], bevelLeftColor[1], bevelLeftColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(cx + bevelThickness, cy, 0);
    glVertex3f(cx + bevelThickness, cy + ch, 0);
    glVertex3f(cx, cy + ch, -depth);
    glVertex3f(cx, cy, -depth);
    glEnd();
}

// -------------------------
// 3D Dark Mode Button Function
// -------------------------
void drawButton3D(float bx, float by, float bw, float bh, float depth, bool darkTheme = true)
{
    float pressOffsetZ = 0.0f;
    float bevelThickness = depth * 0.5f;
    float x = bx;
    float y = by;

    if (darkTheme) {
        // Front face
        float frontColor = 0.3f;
        glColor3f(frontColor, frontColor, frontColor);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glEnd();

        // Top bevel
        glColor3f(0.4f, 0.4f, 0.4f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw - bevelThickness, y - bevelThickness, -(pressOffsetZ + bevelThickness));
        glVertex3f(x - bevelThickness, y - bevelThickness, -(pressOffsetZ + bevelThickness));
        glEnd();

        // Left bevel
        glColor3f(0.42f, 0.42f, 0.42f);
        glBegin(GL_QUADS);
        glVertex3f(x, y, -pressOffsetZ);
        glVertex3f(x, y + bh, -pressOffsetZ);
        glVertex3f(x - bevelThickness, y + bh - bevelThickness, -(pressOffsetZ + bevelThickness));
        glVertex3f(x - bevelThickness, y - bevelThickness, -(pressOffsetZ + bevelThickness));
        glEnd();

        // Right bevel
        glColor3f(0.25f, 0.25f, 0.25f);
        glBegin(GL_QUADS);
        glVertex3f(x + bw, y, -pressOffsetZ);
        glVertex3f(x + bw, y + bh, -pressOffsetZ);
        glVertex3f(x + bw + bevelThickness, y + bh + bevelThickness, -(pressOffsetZ + bevelThickness));
        glVertex3f(x + bw + bevelThickness, y + bevelThickness, -(pressOffsetZ + bevelThickness));
        glEnd();

        // Bottom bevel
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
// Refined Slot Drawing
// -------------------------
// Draws one narrow slot with a very deep bottom/left bevel. The bottom-left corner
// is subdivided into two triangles so that we don’t overlap quads. 
void drawRefinedSlot(float centerX, float centerY,
    float slotWidth, float slotHeight,
    float bevelThickness, float bevelDepth)
{
    // Background color
    const float backgroundColor[3] = { 0.933f, 0.933f, 0.933f };

    // Bottom and left bevel colors
    const float bottomBevelColor[3] = { 0.2f, 0.2f, 0.2f };
    const float leftBevelColor[3] = { 0.35f, 0.35f, 0.35f };

    // Blended corner color
    const float cornerColor[3] = {
        0.5f * (bottomBevelColor[0] + leftBevelColor[0]),
        0.5f * (bottomBevelColor[1] + leftBevelColor[1]),
        0.5f * (bottomBevelColor[2] + leftBevelColor[2])
    };

    float icx = centerX - slotWidth * 0.5f;
    float icy = centerY - slotHeight * 0.5f;

    // 1) Fill slot with background
    glColor3f(backgroundColor[0], backgroundColor[1], backgroundColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(icx, icy, 0);
    glVertex3f(icx + slotWidth, icy, 0);
    glVertex3f(icx + slotWidth, icy + slotHeight, 0);
    glVertex3f(icx, icy + slotHeight, 0);
    glEnd();

    // 2) Bottom bevel (excluding bottom-left corner)
    // We'll start this quad at x = icx + bevelThickness
    // so that the corner region is left out.
    glColor3f(bottomBevelColor[0], bottomBevelColor[1], bottomBevelColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(icx + bevelThickness, icy + slotHeight - bevelThickness, 0);
    glVertex3f(icx + slotWidth, icy + slotHeight - bevelThickness, 0);
    glVertex3f(icx + slotWidth, icy + slotHeight, -bevelDepth);
    glVertex3f(icx + bevelThickness, icy + slotHeight, -bevelDepth);
    glEnd();

    // 3) Left bevel (excluding bottom-left corner)
    glColor3f(leftBevelColor[0], leftBevelColor[1], leftBevelColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(icx + bevelThickness, icy, 0);
    glVertex3f(icx + bevelThickness, icy + slotHeight - bevelThickness, 0);
    glVertex3f(icx, icy + slotHeight - bevelThickness, -bevelDepth);
    glVertex3f(icx, icy, -bevelDepth);
    glEnd();

    // 4) Bottom-left corner (two small triangles to blend colors)
    // Define these 4 corner points:
    // A = (icx,                     icy + slotHeight - bevelThickness)
    // B = (icx + bevelThickness,    icy + slotHeight - bevelThickness)
    // C = (icx + bevelThickness,    icy + slotHeight)
    // D = (icx,                     icy + slotHeight)
    //
    // We'll do two triangles: (A,B,D) and (B,C,D)
    // with carefully assigned vertex colors.

    float Ax = icx;
    float Ay = icy + slotHeight - bevelThickness;
    float Bx = icx + bevelThickness;
    float By = icy + slotHeight - bevelThickness;
    float Cx = icx + bevelThickness;
    float Cy = icy + slotHeight;
    float Dx = icx;
    float Dy = icy + slotHeight;

    // Triangle 1: A,B,D
    glBegin(GL_TRIANGLES);
    // A -> left bevel color
    glColor3f(leftBevelColor[0], leftBevelColor[1], leftBevelColor[2]);
    glVertex3f(Ax, Ay, 0);

    // B -> corner color
    glColor3f(cornerColor[0], cornerColor[1], cornerColor[2]);
    glVertex3f(Bx, By, 0);

    // D -> bottom-left corner extends into the "deep" left side
    glColor3f(leftBevelColor[0], leftBevelColor[1], leftBevelColor[2]);
    glVertex3f(Dx, Dy, -bevelDepth);
    glEnd();

    // Triangle 2: B,C,D
    glBegin(GL_TRIANGLES);
    // B -> corner color
    glColor3f(cornerColor[0], cornerColor[1], cornerColor[2]);
    glVertex3f(Bx, By, 0);

    // C -> bottom bevel color
    glColor3f(bottomBevelColor[0], bottomBevelColor[1], bottomBevelColor[2]);
    glVertex3f(Cx, Cy, -bevelDepth);

    // D -> left bevel color
    glColor3f(leftBevelColor[0], leftBevelColor[1], leftBevelColor[2]);
    glVertex3f(Dx, Dy, -bevelDepth);
    glEnd();
}

// -------------------------
// UI Initialization
// -------------------------
void initUI(int screenWidth, int screenHeight)
{
    // Center the vertical button in the screen.
    // Full height equals the window's height (half-height = screenHeight/2)
    // Double–wide: half-width = 100 => full width = 200
    g_darkButton.pos = glm::vec2(screenWidth * 0.5f, screenHeight * 0.5f);
    g_darkButton.size = glm::vec2(100.0f, screenHeight * 0.5f);
    g_darkButton.label = "Fixed Unpressable Vertical Button";
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

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int fullWidth = mode->width;
    int fullHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(fullWidth, fullHeight, "Vertical Button with 6 Refined Deep Slots", monitor, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create fullscreen window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 2D orthographic projection with some depth range.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, fullWidth, fullHeight, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    initUI(fullWidth, fullHeight);

    // Outer negative space parameters
    float cutoutMargin = 10.0f;
    float nsBevelThickness = 5.0f;
    float nsDepth = 10.0f;

    // "Dark button" 3D effect depth
    float buttonDepth = 20.0f;

    // We want exactly 6 slots, spaced evenly, leaving some top/bottom margin 
    // so they don't go all the way to the edge.
    int   numSlots = 6;
    float slotWidth = 20.0f;
    float slotHeight = 50.0f; // each slot is 50 px tall
    float slotBevelThickness = 5.0f;
    float slotBevelDepth = 80.0f; // very deep

    // We'll define an extra margin at the top/bottom inside the outer cutout area 
    // so the topmost and bottommost slots are not flush against the edge.
    float holeTopMargin = 30.0f;
    float holeBottomMargin = 30.0f;

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.933f, 0.933f, 0.933f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Compute the button's bounding box
        float bx = g_darkButton.pos.x - g_darkButton.size.x;
        float by = g_darkButton.pos.y - g_darkButton.size.y;
        float bw = g_darkButton.size.x * 2.0f;
        float bh = g_darkButton.size.y * 2.0f;

        // 1) Draw the main 3D dark button
        drawButton3D(bx, by, bw, bh, buttonDepth, true);

        // 2) Draw the outer negative space cutout
        glDisable(GL_DEPTH_TEST);
        drawNegativeSpaceButton(bx, by, bw, bh, cutoutMargin, nsBevelThickness, nsDepth);

        // 3) Compute the inner region (outer cutout bounds)
        float outerX = bx + cutoutMargin;
        float outerY = by + cutoutMargin;
        float outerW = bw - 2 * cutoutMargin;
        float outerH = bh - 2 * cutoutMargin;

        // We have 6 slots to place, each slotHeight tall, plus 5 gaps between them.
        // We also want a top/bottom margin for the slot area itself:
        float usedSpace = outerH - (holeTopMargin + holeBottomMargin);
        float totalSlotsHeight = numSlots * slotHeight;
        int   gaps = numSlots - 1;

        // Solve for the spacing between slots
        float spacing = 0.0f;
        if (gaps > 0 && usedSpace > totalSlotsHeight) {
            spacing = (usedSpace - totalSlotsHeight) / gaps;
        }
        else {
            // fallback if there's not enough space or if numSlots=1
            spacing = 10.0f;
        }

        // Now place each slot
        float currentY = outerY + holeTopMargin; // bottom of the first slot
        for (int i = 0; i < numSlots; i++) {
            // The center of this slot is currentY + slotHeight/2
            float centerY = currentY + slotHeight * 0.5f;
            // The center X is the horizontal center of the cutout
            float centerX = outerX + outerW * 0.5f;

            // Draw the refined slot with deeper bevel
            drawRefinedSlot(centerX, centerY, slotWidth, slotHeight,
                slotBevelThickness, slotBevelDepth);

            // Move up for next slot
            currentY += slotHeight + spacing;
        }

        glEnable(GL_DEPTH_TEST);

        // 4) Render the label
        renderText(bx + 15, by + bh / 2 - 5, g_darkButton.label.c_str(), true);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
