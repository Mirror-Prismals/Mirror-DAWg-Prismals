// main.cpp
#define _CRT_SECURE_NO_WARNINGS

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <string>
#include <algorithm>

// ----------------------------------------------------
// Global Definitions & Helper Structures

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

// Selection tint (blue)
const Color selectTint = { 0.0f, 0.0f, 1.0f, 0.85f };

// ----------------------------------------------------
// Global State for Panels
// Each panel's state is a float in [0,2]: 0 = hidden, 1 = normal, 2 = fullscreen.
// Set initial states and targets to 1 (normal extended) so panels are visible.
float leftState = 1.0f, rightState = 1.0f, topState = 1.0f, bottomState = 1.0f;
float leftTarget = 1.0f, rightTarget = 1.0f, topTarget = 1.0f, bottomTarget = 1.0f;

// Selection flags (only one vertical and one horizontal panel may be selected at a time)
bool leftSelected = false, rightSelected = false, topSelected = false, bottomSelected = false;

// Hold-to-toggle variables for expansion.
// holdDir: +1 for extend, -1 for retract, 0 for none.
int leftHoldDir = 0, rightHoldDir = 0, topHoldDir = 0, bottomHoldDir = 0;
float leftHoldTimer = 0.0f, rightHoldTimer = 0.0f, topHoldTimer = 0.0f, bottomHoldTimer = 0.0f;
const float holdThreshold = 0.5f; // Seconds to hold for toggle

// ----------------------------------------------------
// Animation Parameters

const float stateSpeed = 4.0f; // Speed at which panel state animates

// ----------------------------------------------------
// Window & Layout Constants

int winWidth = 1024, winHeight = 768;
GLFWmonitor* primaryMonitor = nullptr;
// Normal geometry fractions (for state = 1)
const float L_FULL = 0.2f; // Left panel width fraction
const float R_FULL = 0.2f; // Right panel width fraction
const float T_FULL = 0.2f; // Top panel height fraction
const float B_FULL = 0.3f; // Bottom panel height fraction

// ----------------------------------------------------
// Geometry Functions for Panels
// Each function sets x, y, w, h for a given discrete state (0, 1, or 2).

void topGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {           // Hidden: above the window
        x = 0; y = -winHeight * T_FULL; w = winWidth; h = winHeight * T_FULL;
    }
    else if (state == 1) {      // Normal extended: at top edge
        x = 0; y = 0; w = winWidth; h = winHeight * T_FULL;
    }
    else {                    // Fully extended: fullscreen
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

void bottomGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {           // Hidden: below window
        x = 0; y = winHeight; w = winWidth; h = winHeight * B_FULL;
    }
    else if (state == 1) {      // Normal extended: bottom edge
        x = 0; y = winHeight - winHeight * B_FULL; w = winWidth; h = winHeight * B_FULL;
    }
    else {                    // Fully extended: fullscreen
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

void leftGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {           // Hidden: left of window
        x = -winWidth * L_FULL; y = 0; w = winWidth * L_FULL; h = winHeight;
    }
    else if (state == 1) {      // Normal extended: left edge
        x = 0; y = 0; w = winWidth * L_FULL; h = winHeight;
    }
    else {                    // Fully extended: fullscreen
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

void rightGeometry(int state, float& x, float& y, float& w, float& h) {
    if (state == 0) {           // Hidden: right of window
        x = winWidth; y = 0; w = winWidth * R_FULL; h = winHeight;
    }
    else if (state == 1) {      // Normal extended: right edge
        x = winWidth - winWidth * R_FULL; y = 0; w = winWidth * R_FULL; h = winHeight;
    }
    else {                    // Fully extended: fullscreen
        x = 0; y = 0; w = winWidth; h = winHeight;
    }
}

// Given a panel's current state (float between 0 and 2),
// compute its final rectangle by interpolating between states.
void computePanelRect(float state, void (*geomFunc)(int, float&, float&, float&, float&),
    float& x, float& y, float& w, float& h) {
    if (state <= 1.0f) {
        float x0, y0, w0, h0;
        float x1, y1, w1, h1;
        geomFunc(0, x0, y0, w0, h0);
        geomFunc(1, x1, y1, w1, h1);
        float t = state; // t in [0,1]
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
        float t = state - 1.0f; // t in [0,1]
        x = x1 + (x2 - x1) * t;
        y = y1 + (y2 - y1) * t;
        w = w1 + (w2 - w1) * t;
        h = h1 + (h2 - h1) * t;
    }
}

// ----------------------------------------------------
// Simple Drawing Function for a Panel (flat quad)
void drawPanel(float x, float y, float w, float h, const Color& col) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(col.r, col.g, col.b, col.a);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x, y + h, 0);
    glEnd();
    glDisable(GL_BLEND);
}

// ----------------------------------------------------
// Render All Panels
void renderPanels() {
    float lx, ly, lw, lh;
    float rx, ry, rw, rh;
    float tx, ty, tw, th;
    float bx, by, bw, bh;

    computePanelRect(leftState, leftGeometry, lx, ly, lw, lh);
    computePanelRect(rightState, rightGeometry, rx, ry, rw, rh);
    computePanelRect(topState, topGeometry, tx, ty, tw, th);
    computePanelRect(bottomState, bottomGeometry, bx, by, bw, bh);

    // If a panel is selected, use blue tint.
    Color leftDraw = leftSelected ? selectTint : leftDrawCol;
    Color rightDraw = rightSelected ? selectTint : rightDrawCol;
    Color topDraw = topSelected ? selectTint : topDrawCol;
    Color bottomDraw = bottomSelected ? selectTint : bottomDrawCol;

    drawPanel(lx, ly, lw, lh, leftDraw);
    drawPanel(rx, ry, rw, rh, rightDraw);
    drawPanel(tx, ty, tw, th, topDraw);
    drawPanel(bx, by, bw, bh, bottomDraw);
}

// ----------------------------------------------------
// Update State Animations and Hold Timers
void updateAnimations(double dt) {
    // Animate panel states toward their targets.
    leftState += (leftTarget - leftState) * dt * stateSpeed;
    rightState += (rightTarget - rightState) * dt * stateSpeed;
    topState += (topTarget - topState) * dt * stateSpeed;
    bottomState += (bottomTarget - bottomState) * dt * stateSpeed;

    // Update hold timers for expansion if active.
    if (leftHoldDir != 0) {
        leftHoldTimer += dt;
        if (leftHoldDir == +1 && leftHoldTimer >= holdThreshold)
            leftTarget = 2.0f; // fully extended
    }
    if (rightHoldDir != 0) {
        rightHoldTimer += dt;
        if (rightHoldDir == +1 && rightHoldTimer >= holdThreshold)
            rightTarget = 2.0f;
    }
    if (topHoldDir != 0) {
        topHoldTimer += dt;
        if (topHoldDir == +1 && topHoldTimer >= holdThreshold)
            topTarget = 2.0f;
    }
    if (bottomHoldDir != 0) {
        bottomHoldTimer += dt;
        if (bottomHoldDir == +1 && bottomHoldTimer >= holdThreshold)
            bottomTarget = 2.0f;
    }
}

// ----------------------------------------------------
// GLFW Callback Functions

// Mouse button callback for selecting panels.
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        // Compute current panel rectangles (using current state values).
        float lx, ly, lw, lh;
        float rx, ry, rw, rh;
        float tx, ty, tw, th;
        float bx, by, bw, bh;
        computePanelRect(leftState, leftGeometry, lx, ly, lw, lh);
        computePanelRect(rightState, rightGeometry, rx, ry, rw, rh);
        computePanelRect(topState, topGeometry, tx, ty, tw, th);
        computePanelRect(bottomState, bottomGeometry, bx, by, bw, bh);

        // For vertical panels: if within top or bottom, select exclusively.
        if ((mx >= tx && mx <= tx + tw && my >= ty && my <= ty + th) ||
            (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh)) {
            if (mx >= tx && mx <= tx + tw && my >= ty && my <= ty + th) {
                topSelected = true;
                bottomSelected = false;
            }
            else {
                bottomSelected = true;
                topSelected = false;
            }
        }
        // For horizontal panels:
        if ((mx >= lx && mx <= lx + lw && my >= ly && my <= ly + lh) ||
            (mx >= rx && mx <= rx + rw && my >= ry && my <= ry + rh)) {
            if (mx >= lx && mx <= lx + lw && my >= ly && my <= ly + lh) {
                leftSelected = true;
                rightSelected = false;
            }
            else {
                rightSelected = true;
                leftSelected = false;
            }
        }
    }
}

// Cursor position callback (empty)
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    // No additional processing.
}

// Key callback for panel control.
// For selected panels, specific keys extend or retract:
// Bottom: UP extends, DOWN retracts.
// Top: DOWN extends, UP retracts.
// Left: RIGHT extends, LEFT retracts.
// Right: LEFT extends, RIGHT retracts.
// A quick tap moves to state 1, holding past threshold moves to state 2.
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        // For bottom panel:
        if (bottomSelected) {
            if (key == GLFW_KEY_UP) {
                bottomTarget = 1.0f;
                bottomHoldDir = +1;
                bottomHoldTimer = 0.0f;
            }
            else if (key == GLFW_KEY_DOWN) {
                bottomTarget = 0.0f;
                bottomHoldDir = -1;
                bottomHoldTimer = 0.0f;
            }
        }
        // For top panel:
        if (topSelected) {
            if (key == GLFW_KEY_DOWN) {
                topTarget = 1.0f;
                topHoldDir = +1;
                topHoldTimer = 0.0f;
            }
            else if (key == GLFW_KEY_UP) {
                topTarget = 0.0f;
                topHoldDir = -1;
                topHoldTimer = 0.0f;
            }
        }
        // For left panel:
        if (leftSelected) {
            if (key == GLFW_KEY_RIGHT) {
                leftTarget = 1.0f;
                leftHoldDir = +1;
                leftHoldTimer = 0.0f;
            }
            else if (key == GLFW_KEY_LEFT) {
                leftTarget = 0.0f;
                leftHoldDir = -1;
                leftHoldTimer = 0.0f;
            }
        }
        // For right panel:
        if (rightSelected) {
            if (key == GLFW_KEY_LEFT) {
                rightTarget = 1.0f;
                rightHoldDir = +1;
                rightHoldTimer = 0.0f;
            }
            else if (key == GLFW_KEY_RIGHT) {
                rightTarget = 0.0f;
                rightHoldDir = -1;
                rightHoldTimer = 0.0f;
            }
        }
        // Fullscreen toggle (unchanged)
        if (key == GLFW_KEY_F) {
            static bool isFullscreen = false;
            static int windowedX, windowedY, windowedWidth, windowedHeight;
            GLFWwindow* curr = glfwGetCurrentContext();
            if (!isFullscreen) {
                glfwGetWindowPos(curr, &windowedX, &windowedY);
                glfwGetWindowSize(curr, &windowedWidth, &windowedHeight);
                primaryMonitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
                winWidth = mode->width;
                winHeight = mode->height;
                glfwSetWindowMonitor(curr, primaryMonitor, 0, 0, winWidth, winHeight, mode->refreshRate);
            }
            else {
                winWidth = windowedWidth;
                winHeight = windowedHeight;
                glfwSetWindowMonitor(curr, nullptr, windowedX, windowedY, winWidth, winHeight, 0);
            }
            isFullscreen = !isFullscreen;
            glViewport(0, 0, winWidth, winHeight);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, winWidth, winHeight, 0, -100, 100);
        }
    }
    else if (action == GLFW_RELEASE) {
        // On key release, if hold timer did not reach threshold, cancel the hold.
        if (bottomSelected) {
            if (key == GLFW_KEY_UP && bottomHoldDir == +1) {
                if (bottomHoldTimer < holdThreshold)
                    bottomTarget = 1.0f;
                else
                    bottomTarget = 2.0f;
                bottomHoldDir = 0;
                bottomHoldTimer = 0.0f;
            }
            if (key == GLFW_KEY_DOWN && bottomHoldDir == -1) {
                bottomTarget = 0.0f;
                bottomHoldDir = 0;
                bottomHoldTimer = 0.0f;
            }
        }
        if (topSelected) {
            if (key == GLFW_KEY_DOWN && topHoldDir == +1) {
                if (topHoldTimer < holdThreshold)
                    topTarget = 1.0f;
                else
                    topTarget = 2.0f;
                topHoldDir = 0;
                topHoldTimer = 0.0f;
            }
            if (key == GLFW_KEY_UP && topHoldDir == -1) {
                topTarget = 0.0f;
                topHoldDir = 0;
                topHoldTimer = 0.0f;
            }
        }
        if (leftSelected) {
            if (key == GLFW_KEY_RIGHT && leftHoldDir == +1) {
                if (leftHoldTimer < holdThreshold)
                    leftTarget = 1.0f;
                else
                    leftTarget = 2.0f;
                leftHoldDir = 0;
                leftHoldTimer = 0.0f;
            }
            if (key == GLFW_KEY_LEFT && leftHoldDir == -1) {
                leftTarget = 0.0f;
                leftHoldDir = 0;
                leftHoldTimer = 0.0f;
            }
        }
        if (rightSelected) {
            if (key == GLFW_KEY_LEFT && rightHoldDir == +1) {
                if (rightHoldTimer < holdThreshold)
                    rightTarget = 1.0f;
                else
                    rightTarget = 2.0f;
                rightHoldDir = 0;
                rightHoldTimer = 0.0f;
            }
            if (key == GLFW_KEY_RIGHT && rightHoldDir == -1) {
                rightTarget = 0.0f;
                rightHoldDir = 0;
                rightHoldTimer = 0.0f;
            }
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

    GLFWwindow* window = glfwCreateWindow(winWidth, winHeight, "Four-Panel Dark Theme (Selective Extended)", nullptr, nullptr);
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
