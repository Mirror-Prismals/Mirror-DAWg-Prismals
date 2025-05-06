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
    int stepCount;

    LeniaSim(int w, int h)
        : width(w), height(h), dt(0.05f), m(0.3f), s(0.05f), stepCount(0)
    {
        rng.seed((unsigned)std::time(nullptr));

        state.resize(width * height, 0.0f);
        newState.resize(width * height, 0.0f);

        // Place a 32×32 circle in the center => radius=16
        // The grid is 64 wide × 36 tall => center is (32,18)
        float cx = width / 2.0f;
        float cy = height / 2.0f;
        float rad = 16.0f;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float dx = x - cx;
                float dy = y - cy;
                float r = std::sqrt(dx * dx + dy * dy);
                if (r < rad) {
                    // fill w/ moderate activation
                    state[y * width + x] = 0.8f;
                }
            }
        }

        // Build polynomial bump kernel (R=5 or 7 or 15, your choice).
        // Let's keep R=7 so the circle is quite distinct.
        float R = 7.0f;
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
                if (val > 1.0f) val = 1.0f;
                newState[y * width + x] = val;
            }
        }
        state.swap(newState);

        stepCount++;

        // random shift/rotation every 20 steps
        if (stepCount % 20 == 0) {
            std::uniform_int_distribution<int> moveDist(0, 5);
            int move = moveDist(rng);
            auto old = state;

            if (move < 4) {
                // up/down/left/right shift
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        int srcX = x, srcY = y;
                        if (move == 0) srcY = (y + 1) % height;          // shift up
                        else if (move == 1) srcY = (y - 1 + height) % height; // shift down
                        else if (move == 2) srcX = (x + 1) % width;      // shift left
                        else if (move == 3) srcX = (x - 1 + width) % width; // shift right
                        state[y * width + x] = old[srcY * width + srcX];
                    }
                }
            }
            else {
                // small rotation
                float deg = 2.0f;
                if (move == 5) deg = -deg; // clockwise
                float rad = deg * (3.1415926535f / 180.0f);
                state = rotateState(state, width, height, rad);
            }
        }
    }
};

// ---------------------------------------------------------
// Fill the pixel array from the state
//   v=0 => pure blue background
//   otherwise => your 18-color gradient
// ---------------------------------------------------------
static void fillPixels(std::vector<unsigned char>& pix, const LeniaSim& sim) {
    pix.resize(sim.width * sim.height * 3);
    for (int i = 0; i < (int)sim.state.size(); i++) {
        float v = sim.state[i];
        unsigned char rr, gg, bb;

        if (v <= 0.0f) {
            // Force pure blue for background
            rr = 0; gg = 0; bb = 0;
        }
        else {
            colorFromStops(v, rr, gg, bb);
        }

        pix[i * 3 + 0] = rr;
        pix[i * 3 + 1] = gg;
        pix[i * 3 + 2] = bb;
    }
}

// ---------------------------------------------------------
// Draw a textured quad while preserving the simulation's
// aspect ratio (64:36) => 16:9. If the monitor is also 16:9,
// it will fill the screen. Otherwise we get letter/pillar-box.
// ---------------------------------------------------------
static void drawQuadMaintainAspect(GLuint tex, int winW, int winH,
    int simW, int simH)
{
    // We'll compute a viewport that maintains simW:simH aspect
    float simAspect = float(simW) / float(simH); // 64/36 => 1.777...
    float winAspect = float(winW) / float(winH);

    int offsetX = 0, offsetY = 0;
    int viewW = winW, viewH = winH;

    // letterbox/pillarbox if needed
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

    // Our simulation is 64×36. This matches a 16:9 ratio
    int simW = 64;
    int simH = 36;
    LeniaSim sim(simW, simH);

    // Create a texture for that sim
    GLuint tex = createTexture(simW, simH);

    // FPS tracking
    double lastTime = glfwGetTime();
    int frameCount = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Step the simulation
        sim.step();

        // Fill pixel data
        std::vector<unsigned char> pixels;
        fillPixels(pixels, sim);

        // Update the texture
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            simW, simH,
            GL_RGB, GL_UNSIGNED_BYTE,
            pixels.data());

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
            oss << "Lenia 64x36 - " << fps << " FPS";
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
