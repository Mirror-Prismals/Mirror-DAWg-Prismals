// NegativeSpace_FourColors_Truss_Slots_Spacebar_Corrected_V3.cpp
//
// Combines the four-color negative-space effect with a dark truss,
// six refined deep slots, and a mechanical switch (spacebar).
// Uses the CORRECTED slot drawing, with TRUE negative space and NO top/right lines.
//
// Compile with Visual Studio (link against glfw3.lib, opengl32.lib, user32.lib, gdi32.lib).

#include <windows.h>
#include <iostream>
#include <string>
#include <cmath>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#ifndef PI
constexpr double PI = 3.14159265358979323846;
#endif

// -------------------------
// Global Constants
// -------------------------

// 1) Dark Truss (vertical button)
static const float TRUSS_TOTAL_WIDTH = 200.0f;
static const float TRUSS_DEPTH = 20.0f;

// 2) Four–Color Negative Space
static const float CUTOUT_MARGIN = 28.0f;  // Original cutout margin
static const float BEVEL_THICKNESS = 5.0f;
static const float NEGSPACE_DEPTH = 10.0f;

// 3) Refined Slots
static const int   NUM_SLOTS = 6;
static const float SLOT_WIDTH = 20.0f;
static const float SLOT_HEIGHT = 50.0f;
static const float SLOT_BEVEL_THICK = 5.0f;
static const float SLOT_BEVEL_DEPTH = 80.0f;
static const float HOLE_TOP_MARGIN = 30.0f;
static const float HOLE_BOTTOM_MARGIN = 30.0f;

// 4) Mechanical Switch / Spacebar
static const float ORIGINAL_SPACEBAR_WIDTH = 360.0f;
static const float SPACEBAR_HEIGHT = 60.0f;
static const float SPACEBAR_DEPTH = 18.0f;
static const double PRESS_FEEDBACK_DURATION = 0.15;
static const float SPACEBAR_Z_OFFSET = 10.0f;

// -------------------------
// Structures
// -------------------------

// The dark “truss” button
struct Button {
    glm::vec2 pos;   // Center
    glm::vec2 size;  // Half–width, half–height
};
Button g_truss;

// Mechanical switch
struct Spacebar {
    float x, y;       // Keycap spans full width
    float pressAnim = 0.0f;
    bool  isPressed = false;
    bool  keycapRemoved = false;
};
Spacebar g_spacebar;

// -------------------------
// Negative Space (4-Color) Function -  OUTER FRAME
// -------------------------
void drawNegativeSpaceButton(float bx, float by, float bw, float bh,
    float cutoutMargin, float bevelThickness, float depth)
{
    // Colors
    const float frontFaceColor[3] = { 0.6f, 0.6f, 0.6f };
    const float bevelBottomColor[3] = { 0.3f, 0.3f, 0.3f };
    const float bevelLeftColor[3] = { 0.35f, 0.35f, 0.35f };

    float cx = bx + cutoutMargin;
    float cy = by + cutoutMargin;
    float cw = bw - 2 * cutoutMargin;
    float ch = bh - 2 * cutoutMargin;

    // Top border
    glColor3f(frontFaceColor[0], frontFaceColor[1], frontFaceColor[2]);
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

    // Bottom bevel (drawn last with top vertices at z=0)
    glColor3f(bevelBottomColor[0], bevelBottomColor[1], bevelBottomColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(cx, cy + ch - bevelThickness, 0);
    glVertex3f(cx + cw, cy + ch - bevelThickness, 0);
    glVertex3f(cx + cw, cy + ch, -depth * 0.5f);
    glVertex3f(cx, cy + ch, -depth * 0.5f);

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
// Draw the Dark Truss (vertical button)
// -------------------------
void drawDarkTruss(float bx, float by, float bw, float bh, float depth)
{
    float frontColor = 0.15f;
    float bevel = depth * 0.5f;

    // Front face
    glColor3f(frontColor, frontColor, frontColor);
    glBegin(GL_QUADS);
    glVertex3f(bx, by, 0);
    glVertex3f(bx + bw, by, 0);
    glVertex3f(bx + bw, by + bh, 0);
    glVertex3f(bx, by + bh, 0);
    glEnd();

    // Top bevel
    glColor3f(frontColor + 0.05f, frontColor + 0.05f, frontColor + 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(bx, by, 0);
    glVertex3f(bx + bw, by, 0);
    glVertex3f(bx + bw - bevel, by - bevel, -depth);
    glVertex3f(bx - bevel, by - bevel, -depth);
    glEnd();
    // Right bevel
    glColor3f(frontColor - 0.05f, frontColor - 0.05f, frontColor - 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(bx + bw, by, 0);
    glVertex3f(bx + bw, by + bh, 0);
    glVertex3f(bx + bw + bevel, by + bh + bevel, -depth);
    glVertex3f(bx + bw + bevel, by - bevel, -depth);
    glEnd();
    // Bottom bevel
    glColor3f(frontColor - 0.07f, frontColor - 0.07f, frontColor - 0.07f);
    glBegin(GL_QUADS);
    glVertex3f(bx, by + bh, 0);
    glVertex3f(bx + bw, by + bh, 0);
    glVertex3f(bx + bw + bevel, by + bh + bevel, -depth);
    glVertex3f(bx - bevel, by + bh + bevel, -depth);
    glEnd();
    // Left bevel
    glColor3f(frontColor - 0.02f, frontColor - 0.02f, frontColor - 0.02f);
    glBegin(GL_QUADS);
    glVertex3f(bx, by, 0);
    glVertex3f(bx, by + bh, 0);
    glVertex3f(bx - bevel, by + bh + bevel, -depth);
    glVertex3f(bx - bevel, by - bevel, -depth);
    glEnd();
}
// ----------------------------------------------
//  Refined Slot -  TRUE NEGATIVE SPACE VERSION
// ----------------------------------------------
void drawRefinedSlot(float centerX, float centerY,
    float slotWidth, float slotHeight,
    float bevelThickness, float bevelDepth)
{
    // --- Negative Space Colors (same as outer frame) ---
    const float backgroundColor[3] = { 0.933f, 0.933f, 0.933f }; // MATCHES BACKGROUND
    const float bevelBottomColor[3] = { 0.2f, 0.2f, 0.2f };
    const float bevelLeftColor[3] = { 0.35f, 0.35f, 0.35f };

    // Calculate the outer rectangle of the slot
    float bx = centerX - slotWidth * 0.5f;
    float by = centerY - slotHeight * 0.5f;

    // Adjusted inner front face size (Make it smaller and move up-right)
    float shrinkFactor = 0.8f;  // Adjust this to control shrinkage
    float offsetX = 20.0f;       // Move right
    float offsetY = -2.0f;      // Move up

    float innerWidth = slotWidth * shrinkFactor;
    float innerHeight = slotHeight * shrinkFactor;
    float innerX = bx + (slotWidth - innerWidth) * 0.5f + offsetX;
    float innerY = by + (slotHeight - innerHeight) * 0.5f + offsetY;

    // --- Draw the Bevels (Bottom and Left) ---

    // Bottom bevel (DOUBLE HEIGHT)
    glColor3f(bevelBottomColor[0], bevelBottomColor[1], bevelBottomColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(bx, by + slotHeight - 2.0f * bevelThickness, 0.1f); // Doubled thickness
    glVertex3f(bx + slotWidth, by + slotHeight - 2.0f * bevelThickness, 0.1f);
    glVertex3f(bx + slotWidth, by + slotHeight, -bevelDepth);
    glVertex3f(bx, by + slotHeight, -bevelDepth);
    glEnd();

    // Left bevel (DOUBLE WIDTH)
    glColor3f(bevelLeftColor[0], bevelLeftColor[1], bevelLeftColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(bx + 2.0f * bevelThickness, by, 0.1f); // Doubled thickness
    glVertex3f(bx + 2.0f * bevelThickness, by + slotHeight, 0.1f);
    glVertex3f(bx, by + slotHeight, -bevelDepth);
    glVertex3f(bx, by, -bevelDepth);
    glEnd();

    // --- Blend the Bottom-Left Corner (Two Triangles) ---

    // Blended corner color
    float cornerColor[3] = {
        0.5f * (bevelBottomColor[0] + bevelLeftColor[0]),
        0.5f * (bevelBottomColor[1] + bevelLeftColor[1]),
        0.5f * (bevelBottomColor[2] + bevelLeftColor[2])
    };

    float Ax = bx;
    float Ay = by + slotHeight - 2.0f * bevelThickness; // Match doubled thickness
    float Bx = bx + 2.0f * bevelThickness; // Match doubled thickness
    float By = by + slotHeight - 2.0f * bevelThickness;
    float Cx = bx + 2.0f * bevelThickness;
    float Cy = by + slotHeight;
    float Dx = bx;
    float Dy = by + slotHeight;

    // Triangle 1: A,B,D
    glBegin(GL_TRIANGLES);
    glColor3f(bevelLeftColor[0], bevelLeftColor[1], bevelLeftColor[2]); // A
    glVertex3f(Ax, Ay, 0.1f);
    glColor3fv(cornerColor); // B
    glVertex3f(Bx, By, 0.1f);
    glColor3f(bevelLeftColor[0], bevelLeftColor[1], bevelLeftColor[2]); // D
    glVertex3f(Dx, Dy, -bevelDepth);
    glEnd();

    // Triangle 2: B,C,D
    glBegin(GL_TRIANGLES);
    glColor3fv(cornerColor); // B
    glVertex3f(Bx, By, 0.1f);
    glColor3f(bevelBottomColor[0], bevelBottomColor[1], bevelBottomColor[2]); // C
    glVertex3f(Cx, Cy, -bevelDepth);
    glColor3f(bevelLeftColor[0], bevelLeftColor[1], bevelLeftColor[2]); // D
    glVertex3f(Dx, Dy, -bevelDepth);
    glEnd();

    // --- Fill the smaller cutout with the background color ---
    glColor3f(backgroundColor[0], backgroundColor[1], backgroundColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(innerX, innerY, 0.05f);
    glVertex3f(innerX + innerWidth, innerY, 0.05f);
    glVertex3f(innerX + innerWidth, innerY + innerHeight, 0.05f);
    glVertex3f(innerX, innerY + innerHeight, 0.05f);
    glEnd();
}

// -------------------------
// Mechanical Switch / Spacebar Functions
// -------------------------
void drawBeveledBox3D(float x, float y, float w, float h, float depth,
    float baseR, float baseG, float baseB)
{
    float bevel = depth * 0.5f;
    // Top face
    glColor3f(baseR, baseG, baseB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x, y + h, 0);
    glEnd();
    // Top bevel
    glColor3f(baseR + 0.07f, baseG + 0.07f, baseB + 0.07f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w - bevel, y - bevel, -depth);
    glVertex3f(x - bevel, y - bevel, -depth);
    glEnd();
    // Right bevel
    glColor3f(baseR - 0.05f, baseG - 0.05f, baseB - 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x + w - bevel, y + h - bevel, -depth);
    glVertex3f(x + w - bevel, y - bevel, -depth);
    glEnd();
    // Bottom bevel
    glColor3f(baseR - 0.02f, baseG - 0.02f, baseB - 0.02f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + h, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x + w - bevel, y + h - bevel, -depth);
    glVertex3f(x - bevel, y + h - bevel, -depth);
    glEnd();
    // Left bevel
    glColor3f(baseR - 0.03f, baseG - 0.03f, baseB - 0.03f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x, y + h, 0);
    glVertex3f(x - bevel, y + h - bevel, -depth);
    glVertex3f(x - bevel, y - bevel, -depth);
    glEnd();
}

void drawThreeFacedCube(float x, float y, float w, float h, float depth,
    float baseR, float baseG, float baseB)
{
    glColor3f(baseR, baseG, baseB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x, y + h, 0);
    glEnd();
    float bevel = depth * 0.5f;
    glColor3f(baseR + 0.07f, baseG + 0.07f, baseB + 0.07f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w - bevel, y - bevel, -depth);
    glVertex3f(x - bevel, y - bevel, -depth);
    glEnd();
    glColor3f(baseR - 0.03f, baseG - 0.03f, baseB - 0.03f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x, y + h, 0);
    glVertex3f(x - bevel, y + h - bevel, -depth);
    glVertex3f(x - bevel, y - bevel, -depth);
    glEnd();
}

void drawMechanicalSwitch(const Spacebar& bar, float windowWidth)
{
    float pressAnim = bar.pressAnim;
    float shiftLeft = 10.0f * pressAnim;
    float shiftUp = 10.0f * pressAnim;
    float pressOffsetZ = SPACEBAR_DEPTH * pressAnim;
    float bw = ORIGINAL_SPACEBAR_WIDTH * 0.4f;
    float bh = SPACEBAR_HEIGHT * 0.4f;
    float outerDepth = 16.0f;
    float bx = (windowWidth - bw) * 0.5f;
    float by = bar.y + SPACEBAR_HEIGHT * 0.3f;
    drawBeveledBox3D(bx, by, bw, bh, outerDepth, 0.5f, 0.5f, 0.5f);

    float animDepth = outerDepth - 6.0f;
    float cloneScale = 0.56f;
    float cloneW = bw * cloneScale;
    float cloneH = bh * cloneScale;
    float greenCubeDepth = animDepth * cloneScale * 0.7143f;
    float normalizedPress = pressAnim / 0.5f;
    float restingZ = -(greenCubeDepth * 0.5f);
    float pressedZ = -(greenCubeDepth - 1.0f);
    float zTranslation = restingZ + normalizedPress * (pressedZ - restingZ);
    float cloneX = bx + (bw - cloneW) * 0.5f + 2.0f;
    float cloneY = by + (bh - cloneH) * 0.5f + 2.0f;
    glPushMatrix();
    glTranslatef(-0.5f * shiftLeft, -0.5f * shiftUp, -pressOffsetZ);
    glTranslatef(0, 0, zTranslation);
    glPushAttrib(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_ALWAYS);
    drawThreeFacedCube(cloneX, cloneY, cloneW, cloneH, greenCubeDepth, 0.1f, 0.4f, 0.1f);
    glPopAttrib();
    glPopMatrix();
}

void drawSpacebarKeycap(const Spacebar& bar, float windowWidth)
{
    float pressAnim = bar.pressAnim;
    float bx = bar.x;
    float by = bar.y;
    float bw = windowWidth;
    float bh = SPACEBAR_HEIGHT;
    float shiftLeft = 10.0f * pressAnim;
    float shiftUp = 10.0f * pressAnim;
    float pressOffsetZ = SPACEBAR_DEPTH * pressAnim;
    float newDepth = SPACEBAR_DEPTH * (1.0f - 0.5f * pressAnim);
    float x = bx - shiftLeft;
    float y = by - shiftUp;
    float baseR = 0.93f, baseG = 0.93f, baseB = 0.88f;

    // Front face
    glColor3f(baseR, baseG, baseB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glEnd();
    // Top face
    glColor3f(baseR + 0.07f, baseG + 0.07f, baseB + 0.07f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();
    // Right face
    glColor3f(baseR - 0.05f, baseG - 0.05f, baseB - 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();
    // Bottom face
    glColor3f(baseR - 0.02f, baseG - 0.02f, baseB - 0.02f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glEnd();
    // Left face
    glColor3f(baseR - 0.03f, baseG - 0.03f, baseB - 0.03f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();
}

void drawSpacebar(const Spacebar& bar, float windowWidth)
{
    if (bar.keycapRemoved)
        drawMechanicalSwitch(bar, windowWidth);
    else
        drawSpacebarKeycap(bar, windowWidth);
}

void updateSpacebarAnimation(Spacebar& bar, float deltaTime)
{
    float animSpeed = 0.5f / float(PRESS_FEEDBACK_DURATION);
    float target = bar.isPressed ? 0.5f : 0.0f;
    if (bar.pressAnim < target) {
        bar.pressAnim += animSpeed * deltaTime;
        if (bar.pressAnim > target)
            bar.pressAnim = target;
    }
    else if (bar.pressAnim > target) {
        bar.pressAnim -= animSpeed * deltaTime;
        if (bar.pressAnim < target)
            bar.pressAnim = target;
    }
}

// -------------------------
// Callbacks
// -------------------------
void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        g_spacebar.isPressed = true;
    else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
        g_spacebar.isPressed = false;
}

void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS)
            g_spacebar.isPressed = true;
        else if (action == GLFW_RELEASE)
            g_spacebar.isPressed = false;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        g_spacebar.keycapRemoved = !g_spacebar.keycapRemoved;
    }
}

void framebufferSizeCallback(GLFWwindow* w, int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    g_truss.pos = glm::vec2(width * 0.5f, height * 0.5f);
    g_truss.size = glm::vec2(100.0f, height * 0.5f);

    g_spacebar.x = 0.0f;
    g_spacebar.y = (height - SPACEBAR_HEIGHT) * 0.5f;
}

// -------------------------
// Main
// -------------------------
int main()
{
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int fullW = mode->width;
    int fullH = mode->height;

    GLFWwindow* window = glfwCreateWindow(fullW, fullH,
        "NegativeSpace + Dark Truss + Slots + Spacebar", monitor, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create fullscreen window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glViewport(0, 0, fullW, fullH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, fullW, fullH, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    g_truss.pos = glm::vec2(fullW * 0.5f, fullH * 0.5f);
    g_truss.size = glm::vec2(100.0f, fullH * 0.5f);
    g_spacebar.x = 0.0f;
    g_spacebar.y = (fullH - SPACEBAR_HEIGHT) * 0.5f;

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = float(now - lastTime);
        lastTime = now;

        glClearColor(0.93f, 0.93f, 0.93f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Draw dark truss
        float bx = g_truss.pos.x - g_truss.size.x;
        float by = g_truss.pos.y - g_truss.size.y;
        float bw = g_truss.size.x * 2.0f;
        float bh = g_truss.size.y * 2.0f;
        drawDarkTruss(bx, by, bw, bh, TRUSS_DEPTH);

        // Draw negative space overlay (disable depth test temporarily)
        glDisable(GL_DEPTH_TEST);
        drawNegativeSpaceButton(bx, by, bw, bh, CUTOUT_MARGIN, BEVEL_THICKNESS, NEGSPACE_DEPTH);
        glEnable(GL_DEPTH_TEST);

        // Compute inner region for slots
        float outerX = bx + CUTOUT_MARGIN;
        float outerY = by + CUTOUT_MARGIN;
        float outerW = bw - 2 * CUTOUT_MARGIN;
        float outerH = bh - 2 * CUTOUT_MARGIN;
        float usedSpace = outerH - (HOLE_TOP_MARGIN + HOLE_BOTTOM_MARGIN);
        float totalSlotH = NUM_SLOTS * SLOT_HEIGHT;
        int gaps = NUM_SLOTS - 1;
        float spacing = (gaps > 0 && usedSpace > totalSlotH) ?
            ((usedSpace - totalSlotH) / gaps) : 10.0f;

        // Draw each refined slot - Using CORRECTED Negative Space function
        float currentY = outerY + HOLE_TOP_MARGIN;
        for (int i = 0; i < NUM_SLOTS; i++) {
            float centerY = currentY + SLOT_HEIGHT * 0.5f;
            float centerX = outerX + outerW * 0.5f;
            drawRefinedSlot(centerX, centerY, SLOT_WIDTH, SLOT_HEIGHT, SLOT_BEVEL_THICK, SLOT_BEVEL_DEPTH);
            currentY += SLOT_HEIGHT + spacing;
        }

        // Draw mechanical switch / spacebar
        updateSpacebarAnimation(g_spacebar, dt);
        glPushMatrix();
        glTranslatef(0, 0, SPACEBAR_Z_OFFSET);
        drawSpacebar(g_spacebar, float(fullW));
        glPopMatrix();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
