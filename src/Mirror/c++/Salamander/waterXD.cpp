// WaterSim_MouseVelocity.cpp
//
// A simple water simulation using OpenGL and GLFW.
// The background is teal, and every time you move your mouse by a small amount,
// the program measures your velocity and schedules a ripple event at that location.
// The ripple’s amplitude and its delay (up to 50 ms max) both increase with velocity.
//
// Compile with Visual Studio (link against glfw3.lib, opengl32.lib, user32.lib, gdi32.lib).

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>

// Global window dimensions (set to fullscreen later)
int windowWidth = 800;
int windowHeight = 600;

// Simulation grid parameters
int cellSize = 8;
int gridCols, gridRows;
std::vector<float> wavePrev;
std::vector<float> waveCurr;
std::vector<float> waveNext;
const float DAMPING = 0.995f;

// Mouse motion tracking
double lastMouseX = 0.0, lastMouseY = 0.0;
double lastUpdateTime = 0.0;
const double moveThreshold = 5.0; // minimal movement (pixels) to measure velocity

// Scheduled ripple event
bool rippleScheduled = false;
double scheduledRippleTime = 0.0;  // when to inject the ripple (in seconds)
float scheduledRippleX = 0.0f, scheduledRippleY = 0.0f;
float scheduledAmplitude = 0.0f;

// Parameters for velocity mapping
const double maxVelocity = 1000.0; // pixels per second (velocity at which delay and amplitude hit max)
const double maxDelay = 0.05;      // maximum delay: 50 ms
const float maxAmplitude = 100.0f; // maximum impulse amplitude

// Initialize simulation grid
void initWaveSim(int width, int height) {
    gridCols = width / cellSize + 1;
    gridRows = height / cellSize + 1;
    wavePrev.assign(gridCols * gridRows, 0.0f);
    waveCurr.assign(gridCols * gridRows, 0.0f);
    waveNext.assign(gridCols * gridRows, 0.0f);
}

// Update simulation using a finite-difference method
void updateWaveSim() {
    for (int j = 1; j < gridRows - 1; j++) {
        for (int i = 1; i < gridCols - 1; i++) {
            int idx = j * gridCols + i;
            float sum = waveCurr[idx - 1] + waveCurr[idx + 1] +
                waveCurr[idx - gridCols] + waveCurr[idx + gridCols];
            waveNext[idx] = ((sum / 2.0f) - wavePrev[idx]) * DAMPING;
        }
    }
    wavePrev = waveCurr;
    waveCurr = waveNext;
}

// Render the simulation grid as quads.
// Base teal color is (0, 0.375, 0.375) with the blue channel modulated by wave height.
void renderWaveSim() {
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_QUADS);
    for (int j = 0; j < gridRows - 1; j++) {
        for (int i = 0; i < gridCols - 1; i++) {
            int idx = j * gridCols + i;
            float h = waveCurr[idx];
            float scaled = h / 50.0f;
            float blue = 0.375f + scaled * 0.625f;
            if (blue < 0.0f) blue = 0.0f;
            if (blue > 1.0f) blue = 1.0f;
            glColor3f(0.0f, 0.375f, blue);
            float x = i * cellSize;
            float y = j * cellSize;
            glVertex2f(x, y);
            glVertex2f(x + cellSize, y);
            glVertex2f(x + cellSize, y + cellSize);
            glVertex2f(x, y + cellSize);
        }
    }
    glEnd();
}

// Inject a ripple at (x, y) with the specified impulse amplitude.
void addRipple(float x, float y, float amplitude) {
    int i = static_cast<int>(x / cellSize);
    int j = static_cast<int>(y / cellSize);
    if (i >= 0 && i < gridCols && j >= 0 && j < gridRows) {
        int idx = j * gridCols + i;
        waveCurr[idx] += amplitude;
        wavePrev[idx] += amplitude;
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Create a fullscreen window on the primary monitor.
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    windowWidth = mode->width;
    windowHeight = mode->height;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Water Simulation - Mouse Velocity Ripples", monitor, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set up 2D orthographic projection.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    initWaveSim(windowWidth, windowHeight);

    // Initialize timing and last mouse position.
    lastUpdateTime = glfwGetTime();
    glfwGetCursorPos(window, &lastMouseX, &lastMouseY);

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double dt = currentTime - lastUpdateTime;

        // Poll current mouse position.
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Compute movement distance.
        double dx = mouseX - lastMouseX;
        double dy = mouseY - lastMouseY;
        double distance = std::sqrt(dx * dx + dy * dy);

        // If moved more than a minimal threshold, schedule a ripple.
        if (distance >= moveThreshold && dt > 0.0) {
            // Compute velocity (pixels/second).
            double velocity = distance / dt;
            // Map velocity to a delay (linear up to maxDelay at maxVelocity).
            double delay = (velocity / maxVelocity) * maxDelay;
            if (delay > maxDelay) delay = maxDelay;
            // Map velocity to amplitude (linear mapping up to maxAmplitude).
            float amplitude = static_cast<float>((velocity / maxVelocity) * maxAmplitude);
            if (amplitude > maxAmplitude) amplitude = maxAmplitude;

            // Schedule the ripple event.
            scheduledRippleTime = currentTime + delay;
            scheduledRippleX = static_cast<float>(mouseX);
            scheduledRippleY = static_cast<float>(mouseY);
            scheduledAmplitude = amplitude;
            rippleScheduled = true;

            // Update last position and time.
            lastMouseX = mouseX;
            lastMouseY = mouseY;
            lastUpdateTime = currentTime;
        }

        // Check if a ripple is scheduled and its time has come.
        if (rippleScheduled && currentTime >= scheduledRippleTime) {
            addRipple(scheduledRippleX, scheduledRippleY, scheduledAmplitude);
            rippleScheduled = false;
        }

        updateWaveSim();
        renderWaveSim();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
