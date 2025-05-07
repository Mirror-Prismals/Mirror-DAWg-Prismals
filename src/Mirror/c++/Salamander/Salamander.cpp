// main.cpp
#define _CRT_SECURE_NO_WARNINGS

#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <string>
#include <algorithm>

// ----------------------------------------------------
// Global Structures & Helper Constants
struct Color {
    float r, g, b, a;
};

// Translucent Ruby theme colors (alpha less than 1 for translucency)
const Color bgColor = { 0.10f, 0.05f, 0.05f, 0.9f };    // Dark translucent ruby background
const Color leftCol = { 0.70f, 0.10f, 0.10f, 0.8f };    // Rich ruby red for left panel
const Color rightCol = { 0.75f, 0.20f, 0.20f, 0.8f };    // Brighter red for right panel
const Color topCol = { 0.80f, 0.15f, 0.15f, 0.8f };    // Gem-like red for top panel
const Color bottomCol = { 0.65f, 0.10f, 0.10f, 0.8f };    // Deep ruby for bottom panel

// Drawing colors (use same alpha as panels)
const Color leftDrawCol = { leftCol.r, leftCol.g, leftCol.b, leftCol.a };
const Color rightDrawCol = { rightCol.r, rightCol.g, rightCol.b, rightCol.a };
const Color topDrawCol = { topCol.r, topCol.g, topCol.b, topCol.a };
const Color bottomDrawCol = { bottomCol.r, bottomCol.g, bottomCol.b, bottomCol.a };

// ----------------------------------------------------
// Outer Panel Animated State Variables
float topState = 1.0f, bottomState = 1.0f, leftState = 1.0f, rightState = 1.0f;
float topTarget = 1.0f, bottomTarget = 1.0f, leftTarget = 1.0f, rightTarget = 1.0f;

// Outer panel composite states (state 0 = closed, 1 = normal, 2 = fullscreen)
int verticalTopState = 1;
int verticalBottomState = 1;
int horizontalLeftState = 1;
int horizontalRightState = 1;

// Hold-to-Toggle Variables for arrow keys
bool upHoldActive = false, downHoldActive = false, leftHoldActive = false, rightHoldActive = false;
float upHoldTimer = 0.0f, downHoldTimer = 0.0f, leftHoldTimer = 0.0f, rightHoldTimer = 0.0f;
const float holdThreshold = 0.5f, extraHoldThreshold = 0.5f;

// Outer panel commit flags
bool upArrowBottomCommitted = false, upArrowTopCommitted = false;
bool downArrowTopCommitted = false, downArrowBottomCommitted = false;
bool leftArrowRightCommitted = false, leftArrowLeftCommitted = false;
bool rightArrowLeftCommitted = false, rightArrowRightCommitted = false;

// ----------------------------------------------------
// Inner Panel Animated State Variables (Controlled via WASD)
float innerTopState = 1.0f, innerBottomState = 1.0f, innerLeftState = 1.0f, innerRightState = 1.0f;
float innerTopTarget = 1.0f, innerBottomTarget = 1.0f, innerLeftTarget = 1.0f, innerRightTarget = 1.0f;

// Inner panel composite states
int innerVerticalTopState = 1, innerVerticalBottomState = 1;
int innerHorizontalLeftState = 1, innerHorizontalRightState = 1;

// Hold-to-Toggle Variables for WASD
bool wHoldActive = false, sHoldActive = false, aHoldActive = false, dHoldActive = false;
float wHoldTimer = 0.0f, sHoldTimer = 0.0f, aHoldTimer = 0.0f, dHoldTimer = 0.0f;

// Inner panel commit flags
bool wArrowBottomCommitted = false, wArrowTopCommitted = false;
bool sArrowTopCommitted = false, sArrowBottomCommitted = false;
bool aArrowRightCommitted = false, aArrowLeftCommitted = false;
bool dArrowLeftCommitted = false, dArrowRightCommitted = false;

// ----------------------------------------------------
// Animation Parameter
const float stateSpeed = 4.0f;

// ----------------------------------------------------
// Window & Layout Constants
int winWidth = 1024, winHeight = 768;
GLFWmonitor* primaryMonitor = nullptr;
// Fractions for outer panels when not fullscreen (these are still used in animations)
const float L_FULL = 0.2f, R_FULL = 0.2f, T_FULL = 0.2f, B_FULL = 0.3f;

// ----------------------------------------------------
// Geometry Functions for Outer Panels
// Outer Top: covers full width and a fraction of the height.
void topGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {
        x = 0; y = -winHeight * T_FULL; w = winWidth; h = winHeight * T_FULL;
    }
    else if (state == 1) {
        x = 0; y = 0; w = winWidth; h = winHeight * T_FULL;
    }
    else { // fullscreen
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

// Outer Bottom: full width, a fraction of the height.
void bottomGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {
        x = 0; y = winHeight; w = winWidth; h = winHeight * B_FULL;
    }
    else if (state == 1) {
        x = 0; y = winHeight - winHeight * B_FULL; w = winWidth; h = winHeight * B_FULL;
    }
    else { // fullscreen
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

// Outer Left: occupies a fraction of the width.
void leftGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {
        x = -winWidth * L_FULL; y = 0; w = winWidth * L_FULL; h = winHeight;
    }
    else if (state == 1) {
        x = 0; y = 0; w = winWidth * L_FULL; h = winHeight;
    }
    else { // fullscreen
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

// Outer Right: occupies a fraction of the width.
void rightGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {
        x = winWidth; y = 0; w = winWidth * R_FULL; h = winHeight;
    }
    else if (state == 1) {
        x = winWidth - winWidth * R_FULL; y = 0; w = winWidth * R_FULL; h = winHeight;
    }
    else { // fullscreen
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

// ----------------------------------------------------
// Geometry Functions for Inner Panels
// Inner panels use their own fractions.
void innerTopGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {
        x = 0; y = -winHeight * 0.3f; w = winWidth; h = winHeight * 0.3f;
    }
    else if (state == 1) {
        x = 0; y = 0; w = winWidth; h = winHeight * 0.3f;
    }
    else {
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

void innerBottomGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {
        x = 0; y = winHeight; w = winWidth; h = winHeight * 0.4f;
    }
    else if (state == 1) {
        x = 0; y = winHeight - winHeight * 0.4f; w = winWidth; h = winHeight * 0.4f;
    }
    else {
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

void innerLeftGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {
        x = -winWidth * 0.35f; y = 0; w = winWidth * 0.35f; h = winHeight;
    }
    else if (state == 1) {
        x = 0; y = 0; w = winWidth * 0.35f; h = winHeight;
    }
    else {
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

void innerRightGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {
        x = winWidth; y = 0; w = winWidth * 0.35f; h = winHeight;
    }
    else if (state == 1) {
        x = winWidth - winWidth * 0.35f; y = 0; w = winWidth * 0.35f; h = winHeight;
    }
    else {
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

// ----------------------------------------------------
// Interpolate between discrete states.
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
    if (t >= threshold) return amplitude;
    float half = threshold * 0.5f;
    return (t <= half) ? amplitude * (t / half) : amplitude * ((threshold - t) / half);
}

// ----------------------------------------------------
// Render Inner Panels (WASD controlled)
void renderInnerPanels() {
    float effectiveInnerTop = innerTopState;
    float effectiveInnerBottom = innerBottomState;
    float effectiveInnerLeft = innerLeftState;
    float effectiveInnerRight = innerRightState;
    const float wiggleAmp = 0.1f;

    if (wHoldActive && !wArrowBottomCommitted && wHoldTimer < holdThreshold) {
        float wiggle = computeWiggle(wHoldTimer, holdThreshold, wiggleAmp);
        effectiveInnerBottom = innerBottomState * (1 - wiggle);
    }
    if (sHoldActive && !sArrowTopCommitted && sHoldTimer < holdThreshold) {
        float wiggle = computeWiggle(sHoldTimer, holdThreshold, wiggleAmp);
        effectiveInnerTop = innerTopState * (1 - wiggle);
    }
    if (aHoldActive && !aArrowRightCommitted && aHoldTimer < holdThreshold) {
        float wiggle = computeWiggle(aHoldTimer, holdThreshold, wiggleAmp);
        effectiveInnerRight = innerRightState * (1 - wiggle);
    }
    if (dHoldActive && !dArrowLeftCommitted && dHoldTimer < holdThreshold) {
        float wiggle = computeWiggle(dHoldTimer, holdThreshold, wiggleAmp);
        effectiveInnerLeft = innerLeftState * (1 - wiggle);
    }

    float lx, ly, lw, lh;
    float rx, ry, rw, rh;
    float tx, ty, tw, th;
    float bx, by, bw, bh;

    computePanelRect(effectiveInnerLeft, innerLeftGeometry, lx, ly, lw, lh);
    computePanelRect(effectiveInnerRight, innerRightGeometry, rx, ry, rw, rh);
    computePanelRect(effectiveInnerTop, innerTopGeometry, tx, ty, tw, th);
    computePanelRect(effectiveInnerBottom, innerBottomGeometry, bx, by, bw, bh);

    float innerPanelDepth = 30.0f;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawPanel3D(lx, ly, lw, lh, innerPanelDepth, leftDrawCol, 0.0f, true);
    drawPanel3D(rx, ry, rw, rh, innerPanelDepth, rightDrawCol, 0.0f, true);
    drawPanel3D(tx, ty, tw, th, innerPanelDepth, topDrawCol, 0.0f, true);
    drawPanel3D(bx, by, bw, bh, innerPanelDepth, bottomDrawCol, 0.0f, true);

    glDisable(GL_BLEND);
}

// ----------------------------------------------------
// Render Outer Panels (arrow keys controlled)
void renderPanels() {
    float effectiveTop = topState, effectiveBottom = bottomState;
    const float wiggleAmp = 0.1f;
    if (upHoldActive && !upArrowBottomCommitted && upHoldTimer < holdThreshold) {
        float wiggle = computeWiggle(upHoldTimer, holdThreshold, wiggleAmp);
        effectiveBottom = topState * (1 - wiggle);
    }
    if (downHoldActive && !downArrowTopCommitted && downHoldTimer < holdThreshold) {
        float wiggle = computeWiggle(downHoldTimer, holdThreshold, wiggleAmp);
        effectiveTop = topState * (1 - wiggle);
    }
    float effectiveLeft = leftState, effectiveRight = rightState;
    if (leftHoldActive && !leftArrowRightCommitted && leftHoldTimer < holdThreshold) {
        float wiggle = computeWiggle(leftHoldTimer, holdThreshold, wiggleAmp);
        effectiveRight = rightState * (1 - wiggle);
    }
    if (rightHoldActive && !rightArrowLeftCommitted && rightHoldTimer < holdThreshold) {
        float wiggle = computeWiggle(rightHoldTimer, holdThreshold, wiggleAmp);
        effectiveLeft = leftState * (1 - wiggle);
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
// Update Outer Panel Targets and Animations
void updateTargetsFromComposite() {
    topTarget = (float)verticalTopState;
    bottomTarget = (float)verticalBottomState;
    leftTarget = (float)horizontalLeftState;
    rightTarget = (float)horizontalRightState;
}

void updateAnimations(double dt) {
    if (upHoldActive) {
        upHoldTimer += dt;
        if (upHoldTimer >= holdThreshold && !upArrowBottomCommitted) { verticalBottomState = 0; upArrowBottomCommitted = true; }
        if (upHoldTimer >= holdThreshold + extraHoldThreshold && !upArrowTopCommitted) { verticalTopState = 2; upArrowTopCommitted = true; }
    }
    if (downHoldActive) {
        downHoldTimer += dt;
        if (downHoldTimer >= holdThreshold && !downArrowTopCommitted) { verticalTopState = 0; downArrowTopCommitted = true; }
        if (downHoldTimer >= holdThreshold + extraHoldThreshold && !downArrowBottomCommitted) { verticalBottomState = 2; downArrowBottomCommitted = true; }
    }
    if (leftHoldActive) {
        leftHoldTimer += dt;
        if (leftHoldTimer >= holdThreshold && !leftArrowRightCommitted) { horizontalRightState = 0; leftArrowRightCommitted = true; }
        if (leftHoldTimer >= holdThreshold + extraHoldThreshold && !leftArrowLeftCommitted) { horizontalLeftState = 2; leftArrowLeftCommitted = true; }
    }
    if (rightHoldActive) {
        rightHoldTimer += dt;
        if (rightHoldTimer >= holdThreshold && !rightArrowLeftCommitted) { horizontalLeftState = 0; rightArrowLeftCommitted = true; }
        if (rightHoldTimer >= holdThreshold + extraHoldThreshold && !rightArrowRightCommitted) { horizontalRightState = 2; rightArrowRightCommitted = true; }
    }
    updateTargetsFromComposite();
    leftState += (leftTarget - leftState) * dt * stateSpeed;
    rightState += (rightTarget - rightState) * dt * stateSpeed;
    topState += (topTarget - topState) * dt * stateSpeed;
    bottomState += (bottomTarget - bottomState) * dt * stateSpeed;
}

// ----------------------------------------------------
// Update Inner Panel Animations (WASD)
void updateInnerAnimations(double dt) {
    if (wHoldActive) {
        wHoldTimer += dt;
        if (wHoldTimer >= holdThreshold && !wArrowBottomCommitted) { innerVerticalBottomState = 0; wArrowBottomCommitted = true; }
        if (wHoldTimer >= holdThreshold + extraHoldThreshold && !wArrowTopCommitted) { innerVerticalTopState = 2; wArrowTopCommitted = true; }
    }
    if (sHoldActive) {
        sHoldTimer += dt;
        if (sHoldTimer >= holdThreshold && !sArrowTopCommitted) { innerVerticalTopState = 0; sArrowTopCommitted = true; }
        if (sHoldTimer >= holdThreshold + extraHoldThreshold && !sArrowBottomCommitted) { innerVerticalBottomState = 2; sArrowBottomCommitted = true; }
    }
    if (aHoldActive) {
        aHoldTimer += dt;
        if (aHoldTimer >= holdThreshold && !aArrowRightCommitted) { innerHorizontalRightState = 0; aArrowRightCommitted = true; }
        if (aHoldTimer >= holdThreshold + extraHoldThreshold && !aArrowLeftCommitted) { innerHorizontalLeftState = 2; aArrowLeftCommitted = true; }
    }
    if (dHoldActive) {
        dHoldTimer += dt;
        if (dHoldTimer >= holdThreshold && !dArrowLeftCommitted) { innerHorizontalLeftState = 0; dArrowLeftCommitted = true; }
        if (dHoldTimer >= holdThreshold + extraHoldThreshold && !dArrowRightCommitted) { innerHorizontalRightState = 2; dArrowRightCommitted = true; }
    }
    innerTopTarget = (float)innerVerticalTopState;
    innerBottomTarget = (float)innerVerticalBottomState;
    innerLeftTarget = (float)innerHorizontalLeftState;
    innerRightTarget = (float)innerHorizontalRightState;
    innerLeftState += (innerLeftTarget - innerLeftState) * dt * stateSpeed;
    innerRightState += (innerRightTarget - innerRightState) * dt * stateSpeed;
    innerTopState += (innerTopTarget - innerTopState) * dt * stateSpeed;
    innerBottomState += (innerBottomTarget - innerBottomState) * dt * stateSpeed;
}

// ----------------------------------------------------
// GLFW Callback Functions
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {}
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {}
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_UP) { upHoldActive = true; upHoldTimer = 0.0f; upArrowBottomCommitted = upArrowTopCommitted = false; }
        else if (key == GLFW_KEY_DOWN) { downHoldActive = true; downHoldTimer = 0.0f; downArrowTopCommitted = downArrowBottomCommitted = false; }
        else if (key == GLFW_KEY_LEFT) { leftHoldActive = true; leftHoldTimer = 0.0f; leftArrowRightCommitted = leftArrowLeftCommitted = false; }
        else if (key == GLFW_KEY_RIGHT) { rightHoldActive = true; rightHoldTimer = 0.0f; rightArrowLeftCommitted = rightArrowRightCommitted = false; }
        else if (key == GLFW_KEY_W) { wHoldActive = true; wHoldTimer = 0.0f; wArrowBottomCommitted = wArrowTopCommitted = false; }
        else if (key == GLFW_KEY_S) { sHoldActive = true; sHoldTimer = 0.0f; sArrowTopCommitted = sArrowBottomCommitted = false; }
        else if (key == GLFW_KEY_A) { aHoldActive = true; aHoldTimer = 0.0f; aArrowRightCommitted = aArrowLeftCommitted = false; }
        else if (key == GLFW_KEY_D) { dHoldActive = true; dHoldTimer = 0.0f; dArrowLeftCommitted = dArrowRightCommitted = false; }
    }
    else if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_UP) { upHoldActive = false; upHoldTimer = 0.0f; if (!upArrowBottomCommitted && !upArrowTopCommitted) { verticalTopState = verticalBottomState = 1; } upArrowBottomCommitted = upArrowTopCommitted = false; }
        if (key == GLFW_KEY_DOWN) { downHoldActive = false; downHoldTimer = 0.0f; if (!downArrowTopCommitted && !downArrowBottomCommitted) { verticalTopState = verticalBottomState = 1; } downArrowTopCommitted = downArrowBottomCommitted = false; }
        if (key == GLFW_KEY_LEFT) { leftHoldActive = false; leftHoldTimer = 0.0f; if (!leftArrowRightCommitted && !leftArrowLeftCommitted) { horizontalLeftState = horizontalRightState = 1; } leftArrowRightCommitted = leftArrowLeftCommitted = false; }
        if (key == GLFW_KEY_RIGHT) { rightHoldActive = false; rightHoldTimer = 0.0f; if (!rightArrowLeftCommitted && !rightArrowRightCommitted) { horizontalLeftState = horizontalRightState = 1; } rightArrowLeftCommitted = rightArrowRightCommitted = false; }
        if (key == GLFW_KEY_W) { wHoldActive = false; wHoldTimer = 0.0f; if (!wArrowBottomCommitted && !wArrowTopCommitted) { innerVerticalTopState = innerVerticalBottomState = 1; } wArrowBottomCommitted = wArrowTopCommitted = false; }
        if (key == GLFW_KEY_S) { sHoldActive = false; sHoldTimer = 0.0f; if (!sArrowTopCommitted && !sArrowBottomCommitted) { innerVerticalTopState = innerVerticalBottomState = 1; } sArrowTopCommitted = sArrowBottomCommitted = false; }
        if (key == GLFW_KEY_A) { aHoldActive = false; aHoldTimer = 0.0f; if (!aArrowRightCommitted && !aArrowLeftCommitted) { innerHorizontalLeftState = innerHorizontalRightState = 1; } aArrowRightCommitted = aArrowLeftCommitted = false; }
        if (key == GLFW_KEY_D) { dHoldActive = false; dHoldTimer = 0.0f; if (!dArrowLeftCommitted && !dArrowRightCommitted) { innerHorizontalLeftState = innerHorizontalRightState = 1; } dArrowLeftCommitted = dArrowRightCommitted = false; }
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
    // Set full resolution for fullscreen
    winWidth = mode->width;
    winHeight = mode->height;

    // Create a fullscreen window on the primary monitor
    GLFWwindow* window = glfwCreateWindow(winWidth, winHeight, "Tunnel UI Demo (Translucent Ruby Fullscreen)", primaryMonitor, nullptr);
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
    // Set orthographic projection for the full window size
    glOrtho(0, winWidth, winHeight, 0, -100, 100);
    glViewport(0, 0, winWidth, winHeight);

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double dt = currentTime - lastTime;
        lastTime = currentTime;

        updateAnimations(dt);
        updateInnerAnimations(dt);

        glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw inner panels (the tunnel layer)
        renderInnerPanels();
        // Draw outer panels on top
        renderPanels();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
