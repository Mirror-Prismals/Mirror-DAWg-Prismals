/*
    fishlenia_stable.cpp

    A minimal, single-file Lenia simulation that produces one stable,
    fishlike blob that “swims” against a blue backdrop. The blob is
    rendered with multiple color layers by smoothly interpolating
    through several stops (brown core, reddish‑orange, yellow, green,
    cyan) to give it an automoto appearance.

    An additional fuel injection mechanism computes the blob's centroid
    and adds a small boost in that region so that the fish doesn't die.

    - Uses a 128×128 grid.
    - The blob is updated via convolution with a polynomial bump kernel
      and a simple polynomial growth function.
    - After each update, a small energy boost is applied near the blob’s centroid.
    - Every 4th step, the grid drifts one pixel to the right to simulate swimming.
    - Color mapping:
         * state < 0.1: blue (#0000FF) (the background)
         * state >= 0.1: normalized u = (state - 0.1)/0.9 is interpolated:
              u = 0.0 : brown (#A52A2A)
              u = 0.25: reddish‑orange (#FF4500)
              u = 0.5 : yellow (#FFFF00)
              u = 0.75: green (#00FF00)
              u = 1.0 : cyan (#00FFFF)
    - Uses only GLFW and standard libraries.
    - ESC quits.

    Build (Linux example):
        g++ fishlenia_stable.cpp -lglfw -lGL -ldl -lpthread -o fishlenia_stable
*/

#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <random>
#include <algorithm>

// Helper: Create an OpenGL texture with nearest-neighbor filtering.
static GLuint createTexture(int width, int height) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return tex;
}

// The simulation class for our stable, fishlike Lenia.
class LeniaSim {
public:
    int width, height;
    float dt; // time step
    std::vector<float> state;
    std::vector<float> newState;
    std::vector<float> kernel;
    int kernelSize;
    int Rint;
    std::mt19937 rng;

    // Growth parameters.
    float m; // growth center
    float s; // growth width

    // Step counter to control drift frequency.
    int stepCount;

    LeniaSim(int w, int h)
        : width(w), height(h), dt(0.05f), m(0.3f), s(0.05f), stepCount(0)
    {
        state.resize(width * height, 0.0f);
        newState.resize(width * height, 0.0f);
        rng.seed((unsigned)std::time(nullptr));

        // Initialize a single blob ("fish") in the center.
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = y * width + x;
                float dx = x - width / 2.0f;
                float dy = y - height / 2.0f;
                float r = std::sqrt(dx * dx + dy * dy);
                // Blob: high state inside a radius of ~10 pixels.
                state[idx] = (r < 10.0f) ? 0.8f : 0.0f;
            }
        }

        // Build the convolution kernel.
        float R = 15.0f; // kernel radius
        Rint = int(std::floor(R));
        kernelSize = 2 * Rint + 1;
        kernel.resize(kernelSize * kernelSize, 0.0f);
        double sum = 0.0;
        for (int dy = -Rint; dy <= Rint; dy++) {
            for (int dx = -Rint; dx <= Rint; dx++) {
                float r = std::sqrt(float(dx * dx + dy * dy)) / R;
                float val = 0.0f;
                if (r <= 1.0f) {
                    float x = 4.0f * r * (1.0f - r);
                    if (x < 0.0f) x = 0.0f;
                    val = std::pow(x, 4.0f);
                }
                int ix = dx + Rint;
                int iy = dy + Rint;
                kernel[iy * kernelSize + ix] = val;
                sum += val;
            }
        }
        if (sum > 0) {
            for (auto& k : kernel) {
                k = float(k / sum);
            }
        }
    }

    // A simple polynomial growth function.
    float growth(float n) const {
        float diff = n - m;
        float frac = (diff * diff) / (9.0f * s * s);
        float inner = 1.0f - frac;
        if (inner < 0.0f) inner = 0.0f;
        float val = 2.0f * std::pow(inner, 4.0f) - 1.0f;
        return val;
    }

    // Perform one simulation step: convolution, fuel injection, then drift.
    void step() {
        // Convolution and growth update.
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float n = 0.0f;
                for (int ky = -Rint; ky <= Rint; ky++) {
                    int yy = (y + ky + height) % height;
                    for (int kx = -Rint; kx <= Rint; kx++) {
                        int xx = (x + kx + width) % width;
                        float kv = kernel[(ky + Rint) * kernelSize + (kx + Rint)];
                        n += state[yy * width + xx] * kv;
                    }
                }
                float g = growth(n);
                float val = state[y * width + x] + dt * g;
                if (val < 0.0f) val = 0.0f;
                if (val > 1.0f) val = 1.0f;
                newState[y * width + x] = val;
            }
        }
        state.swap(newState);

        // --- Fuel Injection ---
        // Compute the weighted centroid of the fish.
        float sumState = 0.0f, sumX = 0.0f, sumY = 0.0f;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = y * width + x;
                float sVal = state[idx];
                sumState += sVal;
                sumX += x * sVal;
                sumY += y * sVal;
            }
        }
        if (sumState > 0) {
            float cx = sumX / sumState;
            float cy = sumY / sumState;
            // For cells within a radius of 10 from the centroid, add a small boost.
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int idx = y * width + x;
                    float dx = x - cx;
                    float dy = y - cy;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist < 10.0f) {
                        state[idx] += 0.02f; // fuel boost
                        if (state[idx] > 1.0f) state[idx] = 1.0f;
                    }
                }
            }
        }

        // Increment step counter.
        stepCount++;

        // Drift: every 4th step, shift the state one pixel to the right.
        if (stepCount % 4 == 0) {
            std::vector<float> temp = state;
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int srcX = (x - 1 + width) % width;
                    state[y * width + x] = temp[y * width + srcX];
                }
            }
        }
    }
};

// Global simulation pointer.
static LeniaSim* gSim = nullptr;

// Color mapping function.
// If state < 0.1, render as blue (#0000FF). Otherwise, normalize u = (state-0.1)/0.9 and
// interpolate through 5 stops:
//   u = 0.0 : brown (#A52A2A)      (RGB: 165, 42, 42)
//   u = 0.25: reddish‑orange (#FF4500) (RGB: 255, 69, 0)
//   u = 0.5 : yellow (#FFFF00)      (RGB: 255, 255, 0)
//   u = 0.75: green (#00FF00)       (RGB: 0, 255, 0)
//   u = 1.0 : cyan (#00FFFF)        (RGB: 0, 255, 255)
static void fillPixels(std::vector<unsigned char>& pixels, const LeniaSim& sim) {
    pixels.resize(sim.width * sim.height * 3);
    for (int i = 0; i < sim.width * sim.height; i++) {
        float v = sim.state[i];
        unsigned char r, g, b;
        if (v < 0.1f) {
            r = 0; g = 0; b = 255; // blue background
        }
        else {
            float u = (v - 0.1f) / 0.9f;
            if (u < 0.25f) {
                float t = u / 0.25f;
                r = static_cast<unsigned char>(165 + t * (255 - 165));
                g = static_cast<unsigned char>(42 + t * (69 - 42));
                b = static_cast<unsigned char>(42 + t * (0 - 42));
            }
            else if (u < 0.5f) {
                float t = (u - 0.25f) / 0.25f;
                r = 255;
                g = static_cast<unsigned char>(69 + t * (255 - 69));
                b = 0;
            }
            else if (u < 0.75f) {
                float t = (u - 0.5f) / 0.25f;
                r = static_cast<unsigned char>(255 * (1 - t));
                g = 255;
                b = 0;
            }
            else {
                float t = (u - 0.75f) / 0.25f;
                r = 0;
                g = 255;
                b = static_cast<unsigned char>(t * 255);
            }
        }
        pixels[i * 3 + 0] = r;
        pixels[i * 3 + 1] = g;
        pixels[i * 3 + 2] = b;
    }
}

// Draw a full-screen textured quad.
static void drawQuad(GLuint tex) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-1, -1);
    glTexCoord2f(1, 0); glVertex2f(1, -1);
    glTexCoord2f(1, 1); glVertex2f(1, 1);
    glTexCoord2f(0, 1); glVertex2f(-1, 1);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

// Simple key callback: ESC quits.
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static double gLastTimeFPS = 0.0;
static int gFrameCount = 0;

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    GLFWwindow* window = glfwCreateWindow(800, 800, "Stable Fishlike Lenia", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    glfwSwapInterval(1);

    int simW = 128, simH = 128;
    LeniaSim sim(simW, simH);
    gSim = &sim;
    GLuint tex = createTexture(simW, simH);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        sim.step();

        std::vector<unsigned char> pixels;
        fillPixels(pixels, sim);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, simW, simH, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

        int winW, winH;
        glfwGetFramebufferSize(window, &winW, &winH);
        glViewport(0, 0, winW, winH);
        glClear(GL_COLOR_BUFFER_BIT);
        drawQuad(tex);

        double now = glfwGetTime();
        gFrameCount++;
        if (now - gLastTimeFPS >= 1.0) {
            double fps = double(gFrameCount) / (now - gLastTimeFPS);
            gFrameCount = 0;
            gLastTimeFPS = now;
            std::ostringstream title;
            title << "Stable Fishlike Lenia - " << fps << " FPS";
            glfwSetWindowTitle(window, title.str().c_str());
        }
        glfwSwapBuffers(window);
    }

    glDeleteTextures(1, &tex);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
