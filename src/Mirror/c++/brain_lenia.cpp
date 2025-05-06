#define _CRT_SECURE_NO_WARNINGS
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <random>
#include <algorithm>
#include <chrono>

// ---------------------------------------------------------
// A color stop structure
// ---------------------------------------------------------
struct ColorStop {
    float position;    // in [0,1]
    unsigned char r, g, b;
};

// ---------------------------------------------------------
// Your 18 custom colors
// ---------------------------------------------------------
static std::vector<ColorStop> gStops = {
    {0.0000f, 0xFF, 0x00, 0x00}, // Red        #FF0000
    {0.0588f, 0xFF, 0x00, 0x80}, // Pink       #FF0080
    {0.1176f, 0xFF, 0x80, 0x80}, // Infrared   #FF8080
    {0.1764f, 0xFF, 0x80, 0x00}, // Orange     #FF8000
    {0.2352f, 0x00, 0xFF, 0x80}, // Thalo      #00FF80
    {0.2941f, 0x80, 0xFF, 0x80}, // Camo       #80FF80
    {0.3529f, 0x00, 0xFF, 0x00}, // Lime       #00FF00
    {0.4117f, 0x00, 0xFF, 0xFF}, // Cyan       #00FFFF
    {0.4705f, 0x80, 0x00, 0xFF}, // Violet     #8000FF
    {0.5294f, 0xFF, 0x80, 0xFF}, // Ultraviolet #FF80FF
    {0.5882f, 0x00, 0x00, 0x00}, // Blue       #0000FF
    {0.6470f, 0x00, 0x80, 0xFF}, // Cerulean   #0080FF
    {0.7058f, 0x80, 0x80, 0xFF}, // Indigo     #8080FF
    {0.7647f, 0xFF, 0x00, 0xFF}, // Magenta    #FF00FF
    {0.8235f, 0x80, 0xFF, 0x00}, // Chartreuse #80FF00
    {0.8823f, 0x80, 0xFF, 0xFF}, // Aqua       #80FFFF
    {0.9411f, 0x00, 0x00, 0x00}, // Yellow     #FFFF00
    {1.0000f, 0x00, 0x00, 0x00}  // Mustard    #FFFF80
};

// ---------------------------------------------------------
// Interpolate color from stops
// ---------------------------------------------------------
static void colorFromStops(float pos, unsigned char& r, unsigned char& g, unsigned char& b) {
    if (pos <= gStops.front().position) {
        r = gStops.front().r;
        g = gStops.front().g;
        b = gStops.front().b;
        return;
    }
    if (pos >= gStops.back().position) {
        r = gStops.back().r;
        g = gStops.back().g;
        b = gStops.back().b;
        return;
    }
    for (int i = 1; i < (int)gStops.size(); i++) {
        if (pos <= gStops[i].position) {
            auto& left = gStops[i - 1];
            auto& right = gStops[i];
            float t = (pos - left.position) / (right.position - left.position);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            r = (unsigned char)(left.r + t * (right.r - left.r));
            g = (unsigned char)(left.g + t * (right.g - left.g));
            b = (unsigned char)(left.b + t * (right.b - left.b));
            return;
        }
    }
    // fallback
    r = gStops.back().r;
    g = gStops.back().g;
    b = gStops.back().b;
}

// ---------------------------------------------------------
// Create an OpenGL texture with linear filtering
// ---------------------------------------------------------
static GLuint createTexture(int width, int height) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    // Allocate empty texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
        0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    // Linear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

// ---------------------------------------------------------
// Rotate the state array by a small angle (nearest-neighbor)
// (Not used if we remove random rotation)
// ---------------------------------------------------------
static std::vector<float> rotateState(const std::vector<float>& state,
    int width, int height, float angle) {
    std::vector<float> rotated(state.size(), 0.0f);
    float cosA = std::cos(angle);
    float sinA = std::sin(angle);
    float cx = width / 2.0f;
    float cy = height / 2.0f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float rx = x - cx;
            float ry = y - cy;
            float srcX = rx * cosA + ry * sinA + cx;
            float srcY = -rx * sinA + ry * cosA + cy;
            int sx = (int)std::round(srcX);
            int sy = (int)std::round(srcY);
            // Wrap
            sx = (sx % width + width) % width;
            sy = (sy % height + height) % height;
            rotated[y * width + x] = state[sy * width + sx];
        }
    }
    return rotated;
}

// ---------------------------------------------------------
// The main simulator class
// ---------------------------------------------------------
class LeniaSim {
public:
    int width, height;
    float dt;
    std::vector<float> state, newState;
    std::vector<float> kernel;
    int kernelSize;
    int Rint;
    std::mt19937 rng;

    float m;  // growth center
    float s;  // growth width
    float initialRadius; // radius of initial circle
    float R;  // kernel radius
    int stepCount;

    LeniaSim(int w, int h)
        : width(w), height(h), dt(0.08f), m(0.14f), s(0.016f), initialRadius(20.0f), R(14.0f), stepCount(0)
    {
        rng.seed((unsigned)std::time(nullptr));

        state.resize(width * height, 0.0f);
        newState.resize(width * height, 0.0f);

        initializeSystem();
    }

    void initializeSystem() {
        // Clear previous state
        std::fill(state.begin(), state.end(), 0.0f);
        std::fill(newState.begin(), newState.end(), 0.0f);

        // Place a circle in the center with radius = initialRadius
        float cx = width / 2.0f;
        float cy = height / 2.0f;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float dx = x - cx;
                float dy = y - cy;
                float r = std::sqrt(dx * dx + dy * dy);
                if (r < initialRadius) {
                    // fill w/ moderate activation
                    state[y * width + x] = 0.8f;
                }
            }
        }

        // Build polynomial bump kernel
        Rint = (int)std::floor(R);
        kernelSize = 2 * Rint + 1;
        kernel.resize(kernelSize * kernelSize, 0.0f);

        double sum = 0.0;
        for (int dy = -Rint; dy <= Rint; dy++) {
            for (int dx = -Rint; dx <= Rint; dx++) {
                float rr = std::sqrt(float(dx * dx + dy * dy)) / R;
                float val = 0.0f;
                if (rr <= 1.0f) {
                    float xVal = 4.0f * rr * (1.0f - rr);
                    if (xVal < 0.0f) xVal = 0.0f;
                    val = std::pow(xVal, 4.0f);
                }
                kernel[(dy + Rint) * kernelSize + (dx + Rint)] = val;
                sum += val;
            }
        }
        // Normalize
        if (sum > 0.0) {
            for (auto& k : kernel) {
                k = float(k / sum);
            }
        }
    }

    void randomizeParameters() {
        // Randomize parameters within reasonable ranges
        std::uniform_real_distribution<float> distR(10.0f, 18.0f);
        std::uniform_real_distribution<float> distM(0.10f, 0.20f);
        std::uniform_real_distribution<float> distS(0.010f, 0.025f);
        std::uniform_real_distribution<float> distDt(0.05f, 0.10f);
        std::uniform_real_distribution<float> distRad(15.0f, 25.0f);

        R = distR(rng);
        m = distM(rng);
        s = distS(rng);
        dt = distDt(rng);
        initialRadius = distRad(rng);

        // Reinitialize the system with new parameters
        initializeSystem();
        stepCount = 0;
    }

    float growth(float n) const {
        // Typical polynomial growth function
        float diff = n - m;
        float frac = (diff * diff) / (9.0f * s * s);
        float inner = 1.0f - frac;
        if (inner < 0.0f) inner = 0.0f;
        float val = 2.0f * std::pow(inner, 4.0f) - 1.0f;
        return val;
    }

    void step() {
        // Convolution + growth
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float n = 0.0f;
                for (int ky = -Rint; ky <= Rint; ky++) {
                    int yy = (y + ky + height) % height;
                    for (int kx = -Rint; kx <= Rint; kx++) {
                        int xx = (x + kx + width) % width;
                        float w = kernel[(ky + Rint) * kernelSize + (kx + Rint)];
                        n += state[yy * width + xx] * w;
                    }
                }
                float gVal = growth(n);
                float val = state[y * width + x] + dt * gVal;
                if (val < 0.0f) val = 0.0f;
                // Limit the maximum value to 0.95 as requested
                if (val > 0.95f) val = 0.95f;
                newState[y * width + x] = val;
            }
        }
        state.swap(newState);
        stepCount++;
    }
};

// ---------------------------------------------------------
// Fill the *current frame* into a pixel array
// We'll later blend it with a "trail" to get motion blur
// ---------------------------------------------------------
static void fillCurrentFrame(std::vector<unsigned char>& pix, const LeniaSim& sim) {
    pix.resize(sim.width * sim.height * 3);
    for (int i = 0; i < (int)sim.state.size(); i++) {
        float v = sim.state[i];
        unsigned char rr, gg, bb;

        if (v <= 0.0f) {
            // Force black background for "off" cells
            rr = gg = bb = 0;
        }
        else {
            // Use the existing 18-color gradient
            colorFromStops(v, rr, gg, bb);
        }

        pix[i * 3 + 0] = rr;
        pix[i * 3 + 1] = gg;
        pix[i * 3 + 2] = bb;
    }
}

// ---------------------------------------------------------
// Draw a textured quad while preserving the simulation's
// aspect ratio (64:36) => 16:9. 
// ---------------------------------------------------------
static void drawQuadMaintainAspect(GLuint tex, int winW, int winH,
    int simW, int simH)
{
    float simAspect = float(simW) / float(simH);
    float winAspect = float(winW) / float(winH);

    int offsetX = 0, offsetY = 0;
    int viewW = winW, viewH = winH;

    if (winAspect > simAspect) {
        // window is too wide => match height
        viewH = winH;
        viewW = int(winH * simAspect);
        offsetX = (winW - viewW) / 2;
        offsetY = 0;
    }
    else {
        // window is too tall => match width
        viewW = winW;
        viewH = int(winW / simAspect);
        offsetX = 0;
        offsetY = (winH - viewH) / 2;
    }

    glViewport(offsetX, offsetY, viewW, viewH);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1.f, -1.f);
    glTexCoord2f(1, 0); glVertex2f(1.f, -1.f);
    glTexCoord2f(1, 1); glVertex2f(1.f, 1.f);
    glTexCoord2f(0, 1); glVertex2f(-1.f, 1.f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

// ---------------------------------------------------------
static void myKeyCallback(GLFWwindow* window, int key, int scancode,
    int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }

    // Create a full-screen window
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode) {
        std::cerr << "Failed to get video mode\n";
        glfwTerminate();
        return -1;
    }
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height,
        "Lenia 64x36 - Fullscreen, No Stretch",
        monitor, nullptr);
    if (!window) {
        std::cerr << "Failed to create fullscreen window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Optionally disable vsync for max FPS
    // glfwSwapInterval(0);

    glfwSetKeyCallback(window, myKeyCallback);

    int simW = 113;
    int simH = 64;
    LeniaSim sim(simW, simH);

    // Create a texture for that sim
    GLuint tex = createTexture(simW, simH);

    // This extra "trail" buffer will accumulate colors over time
    static std::vector<unsigned char> trail(simW * simH * 3, 0);

    // FPS tracking
    double lastTime = glfwGetTime();
    int frameCount = 0;

    // Add timer for parameter randomization
    auto startTime = std::chrono::steady_clock::now();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Check if 20 seconds have passed
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsedSeconds = currentTime - startTime;

        if (elapsedSeconds.count() >= 90.0) {
            // Randomize parameters and reset timer
            sim.randomizeParameters();
            startTime = std::chrono::steady_clock::now();

            // Clear the trail buffer when parameters change
            std::fill(trail.begin(), trail.end(), 0);

            std::cout << "Parameters randomized: R=" << sim.R
                << ", m=" << sim.m
                << ", s=" << sim.s
                << ", dt=" << sim.dt
                << ", rad=" << sim.initialRadius << std::endl;
        }

        // Step the simulation
        sim.step();

        // Current frame (just the new state)
        std::vector<unsigned char> current;
        fillCurrentFrame(current, sim);

        // Blend current frame into 'trail' with alpha-blending
        // e.g. 90% of old color + 10% new color => ghostly trails
        for (int i = 0; i < simW * simH * 3; i++) {
            float oldVal = (float)trail[i];
            float newVal = (float)current[i];
            float blended = 0.90f * oldVal + 0.10f * newVal;
            if (blended > 255.f) blended = 255.f;
            trail[i] = (unsigned char)blended;
        }

        // Update the texture using the "trail" data
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            simW, simH,
            GL_RGB, GL_UNSIGNED_BYTE,
            trail.data());

        // Draw with aspect ratio maintained
        int winW, winH;
        glfwGetFramebufferSize(window, &winW, &winH);
        drawQuadMaintainAspect(tex, winW, winH, simW, simH);

        // FPS in title
        frameCount++;
        double now = glfwGetTime();
        if (now - lastTime >= 1.0) {
            double fps = double(frameCount) / (now - lastTime);
            lastTime = now;
            frameCount = 0;
            std::ostringstream oss;
            oss << "Lenia + Smooth Trails - " << fps << " FPS";
            glfwSetWindowTitle(window, oss.str().c_str());
        }

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteTextures(1, &tex);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
