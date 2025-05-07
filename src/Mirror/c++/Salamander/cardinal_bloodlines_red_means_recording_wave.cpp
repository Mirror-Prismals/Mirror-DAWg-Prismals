/*****************************************************
 * northbeam_3d_fullscreen_spaced.cpp
 *
 * A creative edit to the original northbeam animation.
 * Instead of drawing flat squares (points) along Bezier curves,
 * this version computes each point on the CPU and draws a
 * small 3D cube at that position using immediate-mode OpenGL.
 *
 * This version creates a fullscreen window, accounts for
 * the screen aspect ratio (so squares stay squares),
 * and uses opaque colors (no transparency).
 *
 * Additionally, each node (cube) now independently transitions
 * its color. Inner nodes (center copies) transition from black to white,
 * while outer nodes (left/right copies) transition from white to black.
 *
 * Inner node transitions occur with a random delay and duration
 * between 1 and 5 seconds. Outer nodes use delays and durations
 * between 6 and 10 seconds.
 *
 * NEW: Two new global constants (CUBE_SCALE_BOTTOM and CUBE_SCALE_TOP)
 * control the scaling of cubes so that they get smaller as they get
 * higher on the screen.
 *
 * NEW: Two new global constants (BASE_COLOR_BLACK and BASE_COLOR_WHITE)
 * define the base colors used for interpolation.
 *
 * NEW: On startup the script randomly picks one inner node and one
 * outer node to track. Their color transitions are used to create a
 * gradient background that updates over time.
 *
 * NEW: The speed of travel depends on discrete grouping:
 * lines are grouped by their sorted order so that center group(s)
 * get an extra boost while outer groups are slowed down.
 *
 * NEW: Pressing the spacebar now triggers a keyframed smooth stop.
 * A single press (you do not need to hold the key) starts a one‑second
 * animation that smoothly eases the effective acceleration down to 0.
 *
 * After the stop phase, the animation enters a slow restart phase where
 * effective acceleration is eased from 0 up to a slow target (a fraction
 * defined by SLOW_RESTART_FACTOR of the standard target) over RESTART_PHASE_DURATION seconds.
 *
 * Then a boost (third stage) is triggered where, over THIRD_STAGE_DURATION seconds,
 * the effective acceleration is eased from the slow target up to a boosted target
 * (the pre‑stop acceleration multiplied by BOOST_FACTOR).
 *
 * Once the boost is complete, the effective acceleration is reset to 0 so that
 * the standard accumulation (oscillatory "whirr") restarts from the beginning.
 *
 * At startup the third stage is immediately triggered so that the animation
 * quickly ramps from a low acceleration to the boosted target and then resets.
 *
 * The effective multiplier is computed as:
 *
 *    multiplier = 1.0 + ACCELERATION_FACTOR * effectiveAccelTime
 *
 * Compile on Windows with appropriate libraries (GLFW, OpenGL).
 *****************************************************/

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

 // ------------------------------------------------------
 // Adjustable parameters
 // ------------------------------------------------------
const float CENTER_CUBE_SIZE = 0.02f;
const float OUTER_CUBE_SIZE = 0.02f;
const float FLOW_SPEED = 0.025f;  // Default speed
const float CURVE_RATE = 0.5f;
const float CURVE_PARAM_A = 0.3f;
const float CURVE_PARAM_B = 0.7f;
const float PIXEL_COPY_SPACING = 0.07f;
float T_INCREMENT = 0.01f;

// ------------------------------------------------------
// Acceleration parameters
// ------------------------------------------------------
const float CENTER_SPEED_INTENSITY = 1.0f;
const float OUTER_SLOW_INTENSITY = 0.5f;
const float ACCELERATION_FACTOR = 0.1f;
const float NORMAL_ACCEL = 0.6f; // Normal continuous acceleration

// ------------------------------------------------------
// Stop animation parameters (keyframed)
// ------------------------------------------------------
const float STOP_ANIM_DURATION = 1.0f; // Duration (in seconds) of the smooth stop

// ------------------------------------------------------
// Restart phase parameters (slow restart stage)
// ------------------------------------------------------
const float RESTART_PHASE_DURATION = 6.0f; // Duration (in seconds) for easing from 0 to slow target
const float SLOW_RESTART_FACTOR = 0.2f;    // Fraction of regular acceleration target for slow restart

// ------------------------------------------------------
// Third stage parameters (boost stage)
// ------------------------------------------------------
const float THIRD_STAGE_DURATION = 3.0f;    // Duration (in seconds) to ease from slow target to boosted target
const float BOOST_FACTOR = 1.5f;            // Boost multiplier (boosted target = pre‑stop acceleration * BOOST_FACTOR)

// ------------------------------------------------------
// Configuration for lines and screen
// ------------------------------------------------------
const int   CENTRAL_LINES = 8;
const int   EXTRA_LEFT = 26;
const int   EXTRA_RIGHT = 26;
const float bottomY = -1.0f;
const float topY = 1.0f;
const float TOP_SPREAD = 0.2f;
const float CUBE_SCALE_BOTTOM = 2.0f;
const float CUBE_SCALE_TOP = 0.5f;
const float BASE_COLOR_BLACK[3] = { 0.5f, 0.0f, 0.0f };
const float BASE_COLOR_WHITE[3] = { 0.25f, 0.0f, 0.0f };

// ------------------------------------------------------
// Global variable for net effective acceleration
// ------------------------------------------------------
float effectiveAccelTime = 0.0f;

// Variables for the stop animation.
bool stopAnimActive = false;
float stopAnimTimer = 0.0f;
float stopAnimInitialAccel = 0.0f;  // Stores effectiveAccelTime when space is pressed (or used at startup)

// Variables for the restart phase.
bool restartPhaseActive = false;
float restartPhaseTimer = 0.0f;

// Variables for the third stage.
bool thirdStageActive = false;
float thirdStageTimer = 0.0f;
float regularTarget = 0.0f; // Pre‑stop acceleration level

// ------------------------------------------------------
// Vertex data structure
// ------------------------------------------------------
struct VertexData {
    float startX;
    int stepIndex;
    float copyIndicator; // -1: left, 0: center, +1: right.
    float currentInterp;
    float nextTransitionTime;
    float transitionDuration;
    bool  transitioning;
    bool  targetIsFlash;
    float transitionStartTime;
    float startInterp;
};

// ------------------------------------------------------
// Utility: Generate random float between min and max
// ------------------------------------------------------
float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

// ------------------------------------------------------
// Generate fan data
// ------------------------------------------------------
std::vector<VertexData> generateFanData() {
    std::vector<VertexData> data;
    float centralSpacing = 2.0f / (CENTRAL_LINES - 1);
    std::vector<float> mainLineXs;
    for (int i = EXTRA_LEFT; i >= 1; --i)
        mainLineXs.push_back(-1.0f - i * centralSpacing);
    for (int i = 0; i < CENTRAL_LINES; ++i) {
        float fraction = static_cast<float>(i) / (CENTRAL_LINES - 1);
        mainLineXs.push_back(-1.0f + fraction * 2.0f);
    }
    for (int i = 1; i <= EXTRA_RIGHT; ++i)
        mainLineXs.push_back(1.0f + i * centralSpacing);

    int numSteps = static_cast<int>(ceil(1.0f / T_INCREMENT));
    for (float startX : mainLineXs) {
        for (int i = 0; i < numSteps; i++) {
            // Left copy.
            {
                VertexData node;
                node.startX = startX;
                node.stepIndex = i;
                node.copyIndicator = -1.0f;
                node.transitioning = false;
                node.currentInterp = 0.0f;
                node.nextTransitionTime = randomFloat(6.0f, 10.0f);
                node.transitionDuration = randomFloat(6.0f, 10.0f);
                node.targetIsFlash = true;
                node.transitionStartTime = 0.0f;
                node.startInterp = 0.0f;
                data.push_back(node);
            }
            // Center copy.
            {
                VertexData node;
                node.startX = startX;
                node.stepIndex = i;
                node.copyIndicator = 0.0f;
                node.transitioning = false;
                node.currentInterp = 0.0f;
                node.nextTransitionTime = randomFloat(1.0f, 5.0f);
                node.transitionDuration = randomFloat(1.0f, 5.0f);
                node.targetIsFlash = true;
                node.transitionStartTime = 0.0f;
                node.startInterp = 0.0f;
                data.push_back(node);
            }
            // Right copy.
            {
                VertexData node;
                node.startX = startX;
                node.stepIndex = i;
                node.copyIndicator = 1.0f;
                node.transitioning = false;
                node.currentInterp = 0.0f;
                node.nextTransitionTime = randomFloat(6.0f, 10.0f);
                node.transitionDuration = randomFloat(6.0f, 10.0f);
                node.targetIsFlash = true;
                node.transitionStartTime = 0.0f;
                node.startInterp = 0.0f;
                data.push_back(node);
            }
        }
    }
    return data;
}

// ------------------------------------------------------
// Compute cubic Bezier position for a vertex
// ------------------------------------------------------
void computeBezier(const VertexData& v, float timeOffset, float timeValue, float& outX, float& outY) {
    float centralSpacing = 2.0f / (CENTRAL_LINES - 1);
    int totalMainLines = EXTRA_LEFT + CENTRAL_LINES + EXTRA_RIGHT;
    float x_min = -1.0f - EXTRA_LEFT * centralSpacing;
    float centerIndex = (totalMainLines - 1) / 2.0f;
    int index = static_cast<int>(round((v.startX - x_min) / centralSpacing));
    float group = fabs(index - centerIndex);
    int numGroups = (totalMainLines + 1) / 2;
    float discreteSpeedFactor = 1.0f +
        CENTER_SPEED_INTENSITY * ((numGroups - group) / float(numGroups)) -
        OUTER_SLOW_INTENSITY * (group / float(numGroups));

    float multiplier = 1.0f + ACCELERATION_FACTOR * effectiveAccelTime * discreteSpeedFactor;
    float effectiveTimeOffset = timeOffset * multiplier;
    float t = fmod(v.stepIndex * T_INCREMENT + effectiveTimeOffset, 1.0f);
    if (t < 0)
        t += 1.0f;
    float one_minus_t = 1.0f - t;

    float P0x = v.startX, P0y = bottomY;
    float P1x = CURVE_RATE * v.startX, P1y = bottomY + (topY - bottomY) * CURVE_PARAM_A;
    float P2x = CURVE_RATE * v.startX, P2y = bottomY + (topY - bottomY) * CURVE_PARAM_B;
    float P3x = v.startX * TOP_SPREAD, P3y = topY;

    outX = one_minus_t * one_minus_t * one_minus_t * P0x +
        3.0f * one_minus_t * one_minus_t * t * P1x +
        3.0f * one_minus_t * t * t * P2x +
        t * t * t * P3x;
    outY = one_minus_t * one_minus_t * one_minus_t * P0y +
        3.0f * one_minus_t * one_minus_t * t * P1y +
        3.0f * one_minus_t * t * t * P2y +
        t * t * t * P3y;
    outX += v.copyIndicator * PIXEL_COPY_SPACING;
}

// ------------------------------------------------------
// Draw a 3D cube with shaded faces
// ------------------------------------------------------
void drawCube(float cx, float cy, float cz, float size, float depth, const float baseColor[3]) {
    glColor3f(baseColor[0], baseColor[1], baseColor[2]);
    glBegin(GL_QUADS);
    glVertex3f(cx - size, cy - size, cz);
    glVertex3f(cx + size, cy - size, cz);
    glVertex3f(cx + size, cy + size, cz);
    glVertex3f(cx - size, cy + size, cz);
    glEnd();

    glColor3f(baseColor[0] + 0.15f, baseColor[1] + 0.15f, baseColor[2] + 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(cx - size, cy - size, cz);
    glVertex3f(cx + size, cy - size, cz);
    glVertex3f(cx + size - depth, cy - size - depth, cz - depth);
    glVertex3f(cx - size - depth, cy - size - depth, cz - depth);
    glEnd();

    glColor3f(baseColor[0] - 0.1f, baseColor[1] - 0.1f, baseColor[2] - 0.1f);
    glBegin(GL_QUADS);
    glVertex3f(cx + size, cy - size, cz);
    glVertex3f(cx + size, cy + size, cz);
    glVertex3f(cx + size - depth, cy + size - depth, cz - depth);
    glVertex3f(cx + size - depth, cy - size - depth, cz - depth);
    glEnd();

    glColor3f(baseColor[0] + 0.05f, baseColor[1] + 0.05f, baseColor[2] + 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(cx - size, cy + size, cz);
    glVertex3f(cx + size, cy + size, cz);
    glVertex3f(cx + size - depth, cy + size - depth, cz - depth);
    glVertex3f(cx - size - depth, cy + size - depth, cz - depth);
    glEnd();

    glColor3f(baseColor[0] - 0.05f, baseColor[1] - 0.05f, baseColor[2] - 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(cx - size, cy - size, cz);
    glVertex3f(cx - size, cy + size, cz);
    glVertex3f(cx - size - depth, cy + size - depth, cz - depth);
    glVertex3f(cx - size - depth, cy - size - depth, cz - depth);
    glEnd();
}

// ------------------------------------------------------
// Main function
// ------------------------------------------------------
int main() {
    srand(static_cast<unsigned int>(time(nullptr)));
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int screenWidth = mode->width;
    int screenHeight = mode->height;
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight,
        "Northbeam 3D - Fullscreen Rollercoaster Reset (No Reverse)",
        monitor, nullptr);
    if (!window) {
        std::cerr << "Failed to create fullscreen window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, screenWidth, screenHeight);
    float aspect = static_cast<float>(screenWidth) / screenHeight;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float left, right, bottom, top;
    if (aspect >= 1.0f) {
        left = -aspect;
        right = aspect;
        bottom = -1.0f;
        top = 1.0f;
        glOrtho(left, right, bottom, top, -10.0, 10.0);
    }
    else {
        left = -1.0f;
        right = 1.0f;
        bottom = -1.0f / aspect;
        top = 1.0f / aspect;
        glOrtho(left, right, bottom, top, -10.0, 10.0);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    effectiveAccelTime = 0.0f;
    stopAnimActive = false;
    stopAnimTimer = 0.0f;
    restartPhaseActive = false;
    restartPhaseTimer = 0.0f;
    thirdStageActive = false;
    thirdStageTimer = 0.0f;

    // At startup, immediately trigger the third stage boost.
    regularTarget = NORMAL_ACCEL * RESTART_PHASE_DURATION;
    stopAnimInitialAccel = regularTarget;
    thirdStageActive = true;
    thirdStageTimer = 0.0f;

    std::vector<VertexData> fanData = generateFanData();
    int innerIndex = -1, outerIndex = -1;
    std::vector<int> innerIndices, outerIndices;
    for (size_t i = 0; i < fanData.size(); i++) {
        if (fanData[i].copyIndicator == 0.0f)
            innerIndices.push_back(i);
        else
            outerIndices.push_back(i);
    }
    if (!innerIndices.empty())
        innerIndex = innerIndices[rand() % innerIndices.size()];
    if (!outerIndices.empty())
        outerIndex = outerIndices[rand() % outerIndices.size()];

    float previousTime = glfwGetTime();
    bool spacePressed = false; // State from previous frame

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float dt = currentTime - previousTime;
        previousTime = currentTime;

        float offset = currentTime * FLOW_SPEED;
        bool curSpace = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);

        // If spacebar is pressed, start the stop phase.
        if (!stopAnimActive && !spacePressed && curSpace) {
            stopAnimActive = true;
            stopAnimTimer = 0.0f;
            stopAnimInitialAccel = effectiveAccelTime; // store pre-stop value for boost target
            // Cancel any ongoing restart or third stage.
            restartPhaseActive = false;
            restartPhaseTimer = 0.0f;
            thirdStageActive = false;
            thirdStageTimer = 0.0f;
        }

        if (stopAnimActive) {
            stopAnimTimer += dt;
            float t = stopAnimTimer / STOP_ANIM_DURATION;
            if (t > 1.0f) t = 1.0f;
            float eased = t * t * (3 - 2 * t); // smoothstep easing
            effectiveAccelTime = stopAnimInitialAccel * (1.0f - eased);
            if (stopAnimTimer >= STOP_ANIM_DURATION) {
                stopAnimActive = false;
                effectiveAccelTime = 0.0f;
                restartPhaseActive = true;
                restartPhaseTimer = 0.0f;
            }
        }
        else if (restartPhaseActive) {
            restartPhaseTimer += dt;
            float fraction = restartPhaseTimer / RESTART_PHASE_DURATION;
            if (fraction > 1.0f) fraction = 1.0f;
            float easeVal = fraction * fraction * (3 - 2 * fraction);
            float slowTarget = NORMAL_ACCEL * RESTART_PHASE_DURATION * SLOW_RESTART_FACTOR;
            effectiveAccelTime = easeVal * slowTarget;
            if (restartPhaseTimer >= RESTART_PHASE_DURATION) {
                restartPhaseActive = false;
                thirdStageActive = true;
                thirdStageTimer = 0.0f;
            }
        }
        else if (thirdStageActive) {
            thirdStageTimer += dt;
            float fraction = thirdStageTimer / THIRD_STAGE_DURATION;
            if (fraction > 1.0f) fraction = 1.0f;
            float easeVal = fraction * fraction * (3 - 2 * fraction);
            float slowTarget = NORMAL_ACCEL * RESTART_PHASE_DURATION * SLOW_RESTART_FACTOR;
            // Boost target is the pre-stop acceleration multiplied by BOOST_FACTOR.
            float boostTarget = stopAnimInitialAccel * BOOST_FACTOR;
            effectiveAccelTime = slowTarget + easeVal * (boostTarget - slowTarget);
            if (thirdStageTimer >= THIRD_STAGE_DURATION) {
                thirdStageActive = false;
                // Reset the accumulation instead of resuming from the boosted level.
                effectiveAccelTime = 0.0f;
            }
        }
        else {
            // Standard continuous acceleration (oscillatory "whirr").
            effectiveAccelTime += NORMAL_ACCEL * dt;
        }
        spacePressed = curSpace;

        float multiplier = 1.0f + ACCELERATION_FACTOR * effectiveAccelTime;

        glClear(GL_DEPTH_BUFFER_BIT);

        // Background gradient.
        float innerColor[3] = { 0.0f, 0.0f, 0.0f };
        float outerColor[3] = { 0.0f, 0.0f, 0.0f };
        if (innerIndex >= 0) {
            VertexData& innerNode = fanData[innerIndex];
            innerColor[0] = BASE_COLOR_BLACK[0] + innerNode.currentInterp * (BASE_COLOR_WHITE[0] - BASE_COLOR_BLACK[0]);
            innerColor[1] = BASE_COLOR_BLACK[1] + innerNode.currentInterp * (BASE_COLOR_WHITE[1] - BASE_COLOR_BLACK[1]);
            innerColor[2] = BASE_COLOR_BLACK[2] + innerNode.currentInterp * (BASE_COLOR_WHITE[2] - BASE_COLOR_BLACK[2]);
        }
        if (outerIndex >= 0) {
            VertexData& outerNode = fanData[outerIndex];
            outerColor[0] = BASE_COLOR_WHITE[0] - outerNode.currentInterp * (BASE_COLOR_WHITE[0] - BASE_COLOR_BLACK[0]);
            outerColor[1] = BASE_COLOR_WHITE[1] - outerNode.currentInterp * (BASE_COLOR_WHITE[1] - BASE_COLOR_BLACK[1]);
            outerColor[2] = BASE_COLOR_WHITE[2] - outerNode.currentInterp * (BASE_COLOR_WHITE[2] - BASE_COLOR_BLACK[2]);
        }
        glDisable(GL_DEPTH_TEST);
        glBegin(GL_QUADS);
        glColor3f(innerColor[0], innerColor[1], innerColor[2]);
        glVertex2f(left, bottom);
        glVertex2f(right, bottom);
        glColor3f(outerColor[0], outerColor[1], outerColor[2]);
        glVertex2f(right, top);
        glVertex2f(left, top);
        glEnd();
        glEnable(GL_DEPTH_TEST);

        // Update and draw each cube.
        for (VertexData& v : fanData) {
            if (currentTime >= v.nextTransitionTime && !v.transitioning) {
                v.transitioning = true;
                v.transitionStartTime = currentTime;
                v.startInterp = v.currentInterp;
                v.targetIsFlash = (v.currentInterp < 0.5f);
                if (v.copyIndicator == 0.0f) {
                    v.transitionDuration = randomFloat(1.0f, 5.0f);
                    v.nextTransitionTime = currentTime + v.transitionDuration + randomFloat(1.0f, 5.0f);
                }
                else {
                    v.transitionDuration = randomFloat(6.0f, 10.0f);
                    v.nextTransitionTime = currentTime + v.transitionDuration + randomFloat(6.0f, 10.0f);
                }
            }
            if (v.transitioning) {
                float progress = (currentTime - v.transitionStartTime) / v.transitionDuration;
                if (progress >= 1.0f) { progress = 1.0f; v.transitioning = false; }
                float target = v.targetIsFlash ? 1.0f : 0.0f;
                v.currentInterp = v.startInterp + (target - v.startInterp) * progress;
            }
            float posX, posY;
            computeBezier(v, offset * multiplier, currentTime, posX, posY);
            float normalizedY = (posY - bottomY) / (topY - bottomY);
            float scale = CUBE_SCALE_BOTTOM + (CUBE_SCALE_TOP - CUBE_SCALE_BOTTOM) * normalizedY;
            float cubeSize = (v.copyIndicator == 0.0f) ? CENTER_CUBE_SIZE : OUTER_CUBE_SIZE;
            cubeSize *= scale;
            float cubeDepth = cubeSize * 0.5f;
            float baseColor[3];
            if (v.copyIndicator == 0.0f) {
                baseColor[0] = BASE_COLOR_BLACK[0] + v.currentInterp * (BASE_COLOR_WHITE[0] - BASE_COLOR_BLACK[0]);
                baseColor[1] = BASE_COLOR_BLACK[1] + v.currentInterp * (BASE_COLOR_WHITE[1] - BASE_COLOR_BLACK[1]);
                baseColor[2] = BASE_COLOR_BLACK[2] + v.currentInterp * (BASE_COLOR_WHITE[2] - BASE_COLOR_BLACK[2]);
            }
            else {
                baseColor[0] = BASE_COLOR_WHITE[0] - v.currentInterp * (BASE_COLOR_WHITE[0] - BASE_COLOR_BLACK[0]);
                baseColor[1] = BASE_COLOR_WHITE[1] - v.currentInterp * (BASE_COLOR_WHITE[1] - BASE_COLOR_BLACK[1]);
                baseColor[2] = BASE_COLOR_WHITE[2] - v.currentInterp * (BASE_COLOR_WHITE[2] - BASE_COLOR_BLACK[2]);
            }
            drawCube(posX, posY, 0.0f, cubeSize, cubeDepth, baseColor);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
