// main.cpp
#define _CRT_SECURE_NO_WARNINGS

#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <string>
#include <algorithm>

// Forward declarations
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void updateAnimations(double dt);

// ----------------------------------------------------
// Global Structures & Helper Constants

struct Color {
    float r, g, b, a;
};

// Base dark theme colors
const Color bgColor = { 0.12f, 0.12f, 0.12f, 1.0f };
const Color leftCol = { 0.18f, 0.18f, 0.18f, 1.0f };
const Color rightCol = { 0.17f, 0.17f, 0.17f, 1.0f };
const Color topCol = { 0.16f, 0.16f, 0.16f, 1.0f };
const Color bottomCol = { 0.15f, 0.15f, 0.15f, 1.0f };

// Drawing colors (translucent)
const Color leftDrawCol = { leftCol.r, leftCol.g, leftCol.b, 0.85f };
const Color rightDrawCol = { rightCol.r, rightCol.g, rightCol.b, 0.85f };
const Color topDrawCol = { topCol.r, topCol.g, topCol.b, 0.85f };
const Color bottomDrawCol = { bottomCol.r, bottomCol.g, bottomCol.b, 0.85f };

// ----------------------------------------------------
// Animated Panel State Variables
// Each panel’s state is a float in [0,2]:
// 0 = closed, 1 = normal, 2 = fullscreen.
float topState = 1.0f, bottomState = 1.0f, leftState = 1.0f, rightState = 1.0f;
// Their targets are set via composite variables.
float topTarget = 1.0f, bottomTarget = 1.0f, leftTarget = 1.0f, rightTarget = 1.0f;

// ----------------------------------------------------
// Composite State Variables
// For vertical panels we now decouple them:
// verticalTopState: top panel (0 = hidden above, 1 = normal, 2 = fullscreen)
// verticalBottomState: bottom panel (0 = hidden below, 1 = normal, 2 = fullscreen)
int verticalTopState = 1;
int verticalBottomState = 1;

// For horizontal panels we already have decoupled states:
int horizontalLeftState = 1; // left panel (0 = hidden, 1 = normal, 2 = fullscreen)
int horizontalRightState = 1; // right panel

// ----------------------------------------------------
// Hold-to-Toggle Variables (for arrow keys)
bool upHoldActive = false;
bool downHoldActive = false;
bool leftHoldActive = false;
bool rightHoldActive = false;
float upHoldTimer = 0.0f;
float downHoldTimer = 0.0f;
float leftHoldTimer = 0.0f;
float rightHoldTimer = 0.0f;
const float holdThreshold = 0.5f;      // time for stage one commit
const float extraHoldThreshold = 0.5f; // additional time for stage two commit

// Vertical commit flags:
bool upArrowBottomCommitted = false; // up arrow: commit bottom panel to hidden (0)
bool upArrowTopCommitted = false; // up arrow: commit top panel to fullscreen (2)
bool downArrowTopCommitted = false; // down arrow: commit top panel to hidden (0)
bool downArrowBottomCommitted = false; // down arrow: commit bottom panel to fullscreen (2)

// Horizontal commit flags (as before):
bool leftArrowRightCommitted = false; // left arrow: commit right panel to hidden (0)
bool leftArrowLeftCommitted = false; // left arrow: commit left panel to fullscreen (2)
bool rightArrowLeftCommitted = false; // right arrow: commit left panel to hidden (0)
bool rightArrowRightCommitted = false; // right arrow: commit right panel to fullscreen (2)

// ----------------------------------------------------
// Animation Parameter
const float stateSpeed = 4.0f; // speed for state interpolation

// ----------------------------------------------------
// Window & Layout Constants
int winWidth = 1024, winHeight = 768;
GLFWmonitor* primaryMonitor = nullptr;
// Geometry fractions for normal (state 1) layout:
const float L_FULL = 0.2f; // left panel width fraction
const float R_FULL = 0.2f; // right panel width fraction
const float T_FULL = 0.2f; // top panel height fraction
const float B_FULL = 0.3f; // bottom panel height fraction

// ----------------------------------------------------
// Geometry Functions for Panels
// Each function sets (x,y,w,h) for a given discrete state (0, 1, or 2).

void topGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {
        x = 0; y = -winHeight * T_FULL; w = winWidth; h = winHeight * T_FULL;
    }
    else if (state == 1) {
        x = 0; y = 0; w = winWidth; h = winHeight * T_FULL;
    }
    else {
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

void bottomGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {
        x = 0; y = winHeight; w = winWidth; h = winHeight * B_FULL;
    }
    else if (state == 1) {
        x = 0; y = winHeight - winHeight * B_FULL; w = winWidth; h = winHeight * B_FULL;
    }
    else {
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

void leftGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {
        x = -winWidth * L_FULL; y = 0; w = winWidth * L_FULL; h = winHeight;
    }
    else if (state == 1) {
        x = 0; y = 0; w = winWidth * L_FULL; h = winHeight;
    }
    else {
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

void rightGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {
        x = winWidth; y = 0; w = winWidth * R_FULL; h = winHeight;
    }
    else if (state == 1) {
        x = winWidth - winWidth * R_FULL; y = 0; w = winWidth * R_FULL; h = winHeight;
    }
    else {
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

// Interpolate between discrete states.
// For state in [0,1]: interpolate between 0 and 1; for [1,2]: between 1 and 2.
void computePanelRect(float state, void (*geomFunc)(int, float&, float&, float&, float&),
    float& x, float& y, float& w, float& h) {
    if (state <= 1.0f) {
        float x0, y0, w0, h0;
        float x1, y1, w1, h1;
        geomFunc(0, x0, y0, w0, h0);
        geomFunc(1, x1, y1, w1, h1);
        float t = state;
        x = x0 + (x1 - x0) * t;
        y = y0 + (y1 - y0) * t;
        w = w0 + (w1 - w0) * t;
        h = h0 + (h1 - h0) * t;
    }
    else {
        float x1, y1, w1, h1;
        float x2, y2, w2, h2;
        geomFunc(1, x1, y1, w1, h1);
        geomFunc(2, x2, y2, w2, h2);
        float t = state - 1.0f;
        x = x1 + (x2 - x1) * t;
        y = y1 + (y2 - y1) * t;
        w = w1 + (w2 - w1) * t;
        h = h1 + (h2 - h1) * t;
    }
}

// ----------------------------------------------------
// Skeuomorphic Drawing Function
void drawPanel3D(float bx, float by, float bw, float bh, float depth,
    const Color& baseColor, float pressAnim, bool darkTheme) {
    float shift = 10.0f * pressAnim;
    float pressOffsetZ = depth * pressAnim;
    float newDepth = depth * (1.0f - 0.5f * pressAnim);
    float x = bx - shift;
    float y = by;

    // Front face
    glColor4f(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glEnd();

    // Top-left bevel
    glColor4f(baseColor.r * 1.1f, baseColor.g * 1.1f, baseColor.b * 1.1f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    // Right bevel
    glColor4f(baseColor.r * 0.9f, baseColor.g * 0.9f, baseColor.b * 0.9f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    // Bottom bevel
    glColor4f(baseColor.r * 1.05f, baseColor.g * 1.05f, baseColor.b * 1.05f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    // Left bevel
    glColor4f(baseColor.r * 0.95f, baseColor.g * 0.95f, baseColor.b * 0.95f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();
}

// ----------------------------------------------------
// Helper: Compute wiggle offset for a tap (tent curve)
float computeWiggle(float t, float threshold, float amplitude) {
    if (t >= threshold) return amplitude; // should not occur if committed
    float half = threshold * 0.5f;
    if (t <= half)
        return amplitude * (t / half);
    else
        return amplitude * ((threshold - t) / half);
}

// ----------------------------------------------------
// Render Panels
// For vertical, we decouple top and bottom using verticalTopState and verticalBottomState.
// For horizontal, we use horizontalLeftState and horizontalRightState.
// Wiggle previews are applied if a key is held briefly.
void renderPanels() {
    // Vertical preview
    float effectiveTop = topState;
    float effectiveBottom = bottomState;
    const float wiggleAmp = 0.1f; // maximum wiggle amplitude

    // For up arrow: nudge bottom toward hidden (0)
    if (upHoldActive && !upArrowBottomCommitted && upHoldTimer < holdThreshold) {
        float wiggle = computeWiggle(upHoldTimer, holdThreshold, wiggleAmp);
        effectiveBottom = bottomState * (1 - wiggle) + 0.0f * wiggle;
    }
    // For down arrow: nudge top toward hidden (0)
    if (downHoldActive && !downArrowTopCommitted && downHoldTimer < holdThreshold) {
        float wiggle = computeWiggle(downHoldTimer, holdThreshold, wiggleAmp);
        effectiveTop = topState * (1 - wiggle) + 0.0f * wiggle;
    }

    // Horizontal preview (as before)
    float effectiveLeft = leftState;
    float effectiveRight = rightState;
    if (leftHoldActive && !leftArrowRightCommitted && leftHoldTimer < holdThreshold) {
        float wiggle = computeWiggle(leftHoldTimer, holdThreshold, wiggleAmp);
        effectiveRight = rightState * (1 - wiggle) + 0.0f * wiggle;
    }
    if (rightHoldActive && !rightArrowLeftCommitted && rightHoldTimer < holdThreshold) {
        float wiggle = computeWiggle(rightHoldTimer, holdThreshold, wiggleAmp);
        effectiveLeft = leftState * (1 - wiggle) + 0.0f * wiggle;
    }

    float lx, ly, lw, lh;
    float rx, ry, rw, rh;
    float tx, ty, tw, th;
    float bx, by, bw, bh;

    computePanelRect(effectiveLeft, leftGeometry, lx, ly, lw, lh);
    computePanelRect(effectiveRight, rightGeometry, rx, ry, rw, rh);
    computePanelRect(effectiveTop, topGeometry, tx, ty, tw, th);
    computePanelRect(effectiveBottom, bottomGeometry, bx, by, bw, bh);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float panelDepth = 15.0f;
    drawPanel3D(lx, ly, lw, lh, panelDepth, leftDrawCol, 0.0f, true);
    drawPanel3D(rx, ry, rw, rh, panelDepth, rightDrawCol, 0.0f, true);
    drawPanel3D(tx, ty, tw, th, panelDepth, topDrawCol, 0.0f, true);
    drawPanel3D(bx, by, bw, bh, panelDepth, bottomDrawCol, 0.0f, true);

    glDisable(GL_BLEND);
}

// ----------------------------------------------------
// Update Targets from Composite States
// For vertical: topTarget is set by verticalTopState; bottomTarget by verticalBottomState.
// For horizontal: leftTarget and rightTarget come from horizontal states.
void updateTargetsFromComposite() {
    topTarget = (float)verticalTopState;
    bottomTarget = (float)verticalBottomState;
    leftTarget = (float)horizontalLeftState;
    rightTarget = (float)horizontalRightState;
}

// ----------------------------------------------------
// Update Animations and Hold Timers
// Vertical keys: 
//   Up arrow: after holdThreshold, commit bottom to 0; after extraHoldThreshold, commit top to 2.
//   Down arrow: after holdThreshold, commit top to 0; after extraHoldThreshold, commit bottom to 2.
// Horizontal keys are handled as before.
void updateAnimations(double dt) {
    // Vertical: Up arrow
    if (upHoldActive) {
        upHoldTimer += dt;
        if (upHoldTimer >= holdThreshold && !upArrowBottomCommitted) {
            verticalBottomState = 0;
            upArrowBottomCommitted = true;
        }
        if (upHoldTimer >= holdThreshold + extraHoldThreshold && !upArrowTopCommitted) {
            verticalTopState = 2;
            upArrowTopCommitted = true;
        }
    }
    // Vertical: Down arrow
    if (downHoldActive) {
        downHoldTimer += dt;
        if (downHoldTimer >= holdThreshold && !downArrowTopCommitted) {
            verticalTopState = 0;
            downArrowTopCommitted = true;
        }
        if (downHoldTimer >= holdThreshold + extraHoldThreshold && !downArrowBottomCommitted) {
            verticalBottomState = 2;
            downArrowBottomCommitted = true;
        }
    }

    // Horizontal: Left arrow
    if (leftHoldActive) {
        leftHoldTimer += dt;
        if (leftHoldTimer >= holdThreshold && !leftArrowRightCommitted) {
            horizontalRightState = 0;
            leftArrowRightCommitted = true;
        }
        if (leftHoldTimer >= holdThreshold + extraHoldThreshold && !leftArrowLeftCommitted) {
            horizontalLeftState = 2;
            leftArrowLeftCommitted = true;
        }
    }
    // Horizontal: Right arrow
    if (rightHoldActive) {
        rightHoldTimer += dt;
        if (rightHoldTimer >= holdThreshold && !rightArrowLeftCommitted) {
            horizontalLeftState = 0;
            rightArrowLeftCommitted = true;
        }
        if (rightHoldTimer >= holdThreshold + extraHoldThreshold && !rightArrowRightCommitted) {
            horizontalRightState = 2;
            rightArrowRightCommitted = true;
        }
    }

    updateTargetsFromComposite();

    // Smoothly animate panel states toward their targets.
    leftState += (leftTarget - leftState) * dt * stateSpeed;
    rightState += (rightTarget - rightState) * dt * stateSpeed;
    topState += (topTarget - topState) * dt * stateSpeed;
    bottomState += (bottomTarget - bottomState) * dt * stateSpeed;
}

// ----------------------------------------------------
// GLFW Callback Functions

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Not used.
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    // Not used.
}

// Key callback: on press, reset timers and clear commit flags.
// On release, if no commit occurred (tap), revert the composite states to normal (1).
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_UP) {
            upHoldActive = true;
            upHoldTimer = 0.0f;
            upArrowBottomCommitted = false;
            upArrowTopCommitted = false;
        }
        else if (key == GLFW_KEY_DOWN) {
            downHoldActive = true;
            downHoldTimer = 0.0f;
            downArrowTopCommitted = false;
            downArrowBottomCommitted = false;
        }
        else if (key == GLFW_KEY_LEFT) {
            leftHoldActive = true;
            leftHoldTimer = 0.0f;
            leftArrowRightCommitted = false;
            leftArrowLeftCommitted = false;
        }
        else if (key == GLFW_KEY_RIGHT) {
            rightHoldActive = true;
            rightHoldTimer = 0.0f;
            rightArrowLeftCommitted = false;
            rightArrowRightCommitted = false;
        }
    }
    else if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_UP) {
            upHoldActive = false;
            upHoldTimer = 0.0f;
            if (!upArrowBottomCommitted && !upArrowTopCommitted) {
                verticalTopState = 1;
                verticalBottomState = 1;
            }
            upArrowBottomCommitted = false;
            upArrowTopCommitted = false;
        }
        if (key == GLFW_KEY_DOWN) {
            downHoldActive = false;
            downHoldTimer = 0.0f;
            if (!downArrowTopCommitted && !downArrowBottomCommitted) {
                verticalTopState = 1;
                verticalBottomState = 1;
            }
            downArrowTopCommitted = false;
            downArrowBottomCommitted = false;
        }
        if (key == GLFW_KEY_LEFT) {
            leftHoldActive = false;
            leftHoldTimer = 0.0f;
            if (!leftArrowRightCommitted && !leftArrowLeftCommitted) {
                horizontalLeftState = 1;
                horizontalRightState = 1;
            }
            leftArrowRightCommitted = false;
            leftArrowLeftCommitted = false;
        }
        if (key == GLFW_KEY_RIGHT) {
            rightHoldActive = false;
            rightHoldTimer = 0.0f;
            if (!rightArrowLeftCommitted && !rightArrowRightCommitted) {
                horizontalLeftState = 1;
                horizontalRightState = 1;
            }
            rightArrowLeftCommitted = false;
            rightArrowRightCommitted = false;
        }
    }
}

// ----------------------------------------------------
// Main Entry Point
int main(void) {
    srand((unsigned int)time(nullptr));
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    winWidth = mode->width * 0.8;
    winHeight = mode->height * 0.8;

    GLFWwindow* window = glfwCreateWindow(winWidth, winHeight, "Four-Panel Dark Theme (Arrow Keys Only)", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, winWidth, winHeight, 0, -100, 100);
    glViewport(0, 0, winWidth, winHeight);

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double dt = currentTime - lastTime;
        lastTime = currentTime;
        updateAnimations(dt);

        glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderPanels();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
