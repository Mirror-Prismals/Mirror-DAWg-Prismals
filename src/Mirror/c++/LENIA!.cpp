#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>

// Window dimensions
const int WIDTH = 800, HEIGHT = 800;

// Simulation parameters
const int SIM_SIZE = 200;
const float dt = 0.1f;
bool paused = true;
int frameCounter = 0;
float zoom = 1.0f;
glm::vec2 panOffset(0.0f, 0.0f);
float speed = 1.0f;

// Lenia parameters
const int R = 20;                   // Kernel radius
const float T = 10.0f;              // Time scale
float mu = 0.15f;                   // Growth center
float sigma = 0.015f;               // Growth width
float kn = 1.0f;                    // Kernel norm

// Arrays
std::vector<float> A(SIM_SIZE* SIM_SIZE, 0.0f);  // World state
std::vector<float> K(2 * R + 1, 0.0f);                // Kernel (we use a radial kernel)
std::vector<float> colorMap(256 * 3);             // Color map for visualization

// Function prototypes
void initLenia();
void processInput(GLFWwindow* window);
void updateSimulation();
void renderSimulation(GLFWwindow* window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void createColorMap();
float growth(float x);
float gaussianKernel(float dist, float r);
void addPattern(int centerX, int centerY);
void randomizeWorld();

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Lenia - Advanced Cellular Automaton", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Set callback functions
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Initialize Lenia
    initLenia();
    createColorMap();

    // Main loop
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Process input
        processInput(window);

        // Update simulation
        if (!paused && frameCounter % std::max(1, static_cast<int>(6 - 5 * speed)) == 0) {
            updateSimulation();
        }
        frameCounter++;

        // Render
        glClear(GL_COLOR_BUFFER_BIT);
        renderSimulation(window);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void initLenia() {
    // Create kernel (Gaussian core function)
    for (int i = 0; i <= 2 * R; i++) {
        float d = static_cast<float>(i) / R;  // normalized distance [0,2]
        K[i] = gaussianKernel(d, 1.0f);
    }

    // Normalize kernel
    float sum = 0.0f;
    for (int i = 0; i <= 2 * R; i++) {
        sum += K[i];
    }
    for (int i = 0; i <= 2 * R; i++) {
        K[i] /= sum * kn;
    }

    // Add an initial pattern
    addPattern(SIM_SIZE / 2, SIM_SIZE / 2);
}

void createColorMap() {
    // Create a viridis-like color map
    for (int i = 0; i < 256; i++) {
        float t = i / 255.0f;

        // Smoothstep for better gradients
        float r = std::max(0.0f, std::min(1.0f, -0.5f + 1.0f * std::pow(t, 0.5f)));
        float g = std::max(0.0f, std::min(1.0f, 0.4f + 0.6f * std::sin(3.1415926f * (t - 0.2f))));
        float b = std::max(0.0f, std::min(1.0f, 0.8f * (1.0f - t) + 0.2f));

        colorMap[i * 3] = r;
        colorMap[i * 3 + 1] = g;
        colorMap[i * 3 + 2] = b;
    }
}

float growth(float x) {
    // Gaussian-like growth function
    return 2.0f * std::exp(-std::pow((x - mu) / sigma, 2.0f)) - 1.0f;
}

float gaussianKernel(float dist, float r) {
    // Gaussian kernel with given radius
    if (dist > r * 2.0f) return 0.0f;
    return std::exp(-std::pow(dist - r, 2.0f) / (2.0f * 0.1f * 0.1f));
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Pan with arrow keys
    float panSpeed = 5.0f / zoom;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        panOffset.x += panSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        panOffset.x -= panSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        panOffset.y += panSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        panOffset.y -= panSpeed;
}

void updateSimulation() {
    // Create a new state for update
    std::vector<float> newA(SIM_SIZE * SIM_SIZE, 0.0f);

    // For each cell in the grid
    for (int y = 0; y < SIM_SIZE; y++) {
        for (int x = 0; x < SIM_SIZE; x++) {
            // Current cell value
            float a = A[y * SIM_SIZE + x];

            // Calculate neighborhood potential using convolution
            float U = 0.0f;
            for (int dy = -R; dy <= R; dy++) {
                for (int dx = -R; dx <= R; dx++) {
                    // Toroidal wrapping
                    int nx = (x + dx + SIM_SIZE) % SIM_SIZE;
                    int ny = (y + dy + SIM_SIZE) % SIM_SIZE;

                    // Calculate distance for kernel lookup
                    float dist = std::sqrt(dx * dx + dy * dy);
                    int kernelIdx = static_cast<int>(dist);
                    if (kernelIdx <= 2 * R) {
                        U += K[kernelIdx] * A[ny * SIM_SIZE + nx];
                    }
                }
            }

            // Calculate growth
            float G = growth(U);

            // Update state
            float a_new = a + dt / T * G;
            a_new = std::max(0.0f, std::min(1.0f, a_new));

            // Store new state
            newA[y * SIM_SIZE + x] = a_new;
        }
    }

    // Update world state
    A = newA;
}

void renderSimulation(GLFWwindow* window) {
    // Set up orthographic projection
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Set background color (dark)
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);

    // Calculate cell size for rendering
    float cellSize = std::min(width, height) / static_cast<float>(SIM_SIZE) * zoom;
    float offsetX = (width - SIM_SIZE * cellSize) / 2.0f + panOffset.x * cellSize;
    float offsetY = (height - SIM_SIZE * cellSize) / 2.0f + panOffset.y * cellSize;

    // Draw cells
    glBegin(GL_QUADS);
    for (int y = 0; y < SIM_SIZE; y++) {
        for (int x = 0; x < SIM_SIZE; x++) {
            float val = A[y * SIM_SIZE + x];
            if (val > 0.01f) {  // Only draw visible cells
                // Map value to color
                int colorIdx = static_cast<int>(val * 255.0f);
                colorIdx = std::max(0, std::min(255, colorIdx));

                glColor3f(colorMap[colorIdx * 3], colorMap[colorIdx * 3 + 1], colorMap[colorIdx * 3 + 2]);

                float px = offsetX + x * cellSize;
                float py = offsetY + y * cellSize;

                // Only draw if in view
                if (px + cellSize >= 0 && px < width && py + cellSize >= 0 && py < height) {
                    glVertex2f(px, py);
                    glVertex2f(px + cellSize, py);
                    glVertex2f(px + cellSize, py + cellSize);
                    glVertex2f(px, py + cellSize);
                }
            }
        }
    }
    glEnd();

    // Display status info
    if (paused) {
        // Draw "PAUSED" text (if you have text rendering implemented)
        // For this basic example, we just output to console
        if (frameCounter % 60 == 0) {
            std::cout << "PAUSED - Press SPACE to resume, R to reset, X to randomize" << std::endl;
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Get cursor position
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Convert to grid coordinates
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        float cellSize = std::min(width, height) / static_cast<float>(SIM_SIZE) * zoom;
        float offsetX = (width - SIM_SIZE * cellSize) / 2.0f + panOffset.x * cellSize;
        float offsetY = (height - SIM_SIZE * cellSize) / 2.0f + panOffset.y * cellSize;

        int gridX = static_cast<int>((xpos - offsetX) / cellSize);
        int gridY = static_cast<int>((ypos - offsetY) / cellSize);

        // Add pattern if within grid bounds
        if (gridX >= 0 && gridX < SIM_SIZE && gridY >= 0 && gridY < SIM_SIZE) {
            addPattern(gridX, gridY);
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_SPACE:
            // Toggle pause
            paused = !paused;
            break;
        case GLFW_KEY_R:
            // Reset
            std::fill(A.begin(), A.end(), 0.0f);
            addPattern(SIM_SIZE / 2, SIM_SIZE / 2);
            break;
        case GLFW_KEY_X:
            // Randomize
            randomizeWorld();
            break;
        case GLFW_KEY_N:
            // Single step
            if (paused) {
                updateSimulation();
            }
            break;
        case GLFW_KEY_1:
            // Decrease growth center
            mu = std::max(0.0f, mu - 0.01f);
            std::cout << "Growth center: " << mu << std::endl;
            break;
        case GLFW_KEY_2:
            // Increase growth center
            mu = std::min(0.5f, mu + 0.01f);
            std::cout << "Growth center: " << mu << std::endl;
            break;
        case GLFW_KEY_3:
            // Decrease growth width
            sigma = std::max(0.001f, sigma - 0.001f);
            std::cout << "Growth width: " << sigma << std::endl;
            break;
        case GLFW_KEY_4:
            // Increase growth width
            sigma = std::min(0.1f, sigma + 0.001f);
            std::cout << "Growth width: " << sigma << std::endl;
            break;
        case GLFW_KEY_5:
            // Decrease speed
            speed = std::max(0.1f, speed - 0.1f);
            std::cout << "Speed: " << speed << std::endl;
            break;
        case GLFW_KEY_6:
            // Increase speed
            speed = std::min(1.0f, speed + 0.1f);
            std::cout << "Speed: " << speed << std::endl;
            break;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Zoom with mouse wheel
    zoom *= (1.0f + 0.1f * yoffset);
    zoom = std::max(0.1f, std::min(10.0f, zoom));
}

void addPattern(int centerX, int centerY) {
    // Create a Orbium-like pattern (from original Lenia examples)
    const int patternSize = 20;
    std::vector<std::vector<float>> pattern(patternSize, std::vector<float>(patternSize, 0.0f));

    // Soft circular pattern
    for (int y = 0; y < patternSize; y++) {
        for (int x = 0; x < patternSize; x++) {
            float dx = x - patternSize / 2.0f + 0.5f;
            float dy = y - patternSize / 2.0f + 0.5f;
            float dist = std::sqrt(dx * dx + dy * dy) / (patternSize / 2.0f);

            if (dist < 0.9f) {
                pattern[y][x] = (0.9f - dist) / 0.9f;

                // Add some variation for more interesting patterns
                pattern[y][x] *= (1.0f - 0.3f * std::sin(dx * 0.5f) * std::sin(dy * 0.5f));
            }
        }
    }

    // Add pattern to world
    for (int y = 0; y < patternSize; y++) {
        for (int x = 0; x < patternSize; x++) {
            int worldX = (centerX - patternSize / 2 + x + SIM_SIZE) % SIM_SIZE;
            int worldY = (centerY - patternSize / 2 + y + SIM_SIZE) % SIM_SIZE;

            A[worldY * SIM_SIZE + worldX] = pattern[y][x];
        }
    }
}

void randomizeWorld() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    // Create random "bubbles" of activity
    std::fill(A.begin(), A.end(), 0.0f);

    const int numBubbles = 5 + dis(gen) * 10;

    for (int i = 0; i < numBubbles; i++) {
        int x = dis(gen) * SIM_SIZE;
        int y = dis(gen) * SIM_SIZE;
        float radius = 5.0f + dis(gen) * 15.0f;

        for (int dy = -static_cast<int>(radius); dy <= static_cast<int>(radius); dy++) {
            for (int dx = -static_cast<int>(radius); dx <= static_cast<int>(radius); dx++) {
                float d = std::sqrt(dx * dx + dy * dy);
                if (d <= radius) {
                    int nx = (x + dx + SIM_SIZE) % SIM_SIZE;
                    int ny = (y + dy + SIM_SIZE) % SIM_SIZE;

                    A[ny * SIM_SIZE + nx] = std::max(A[ny * SIM_SIZE + nx],
                        (1.0f - d / radius) * (0.5f + 0.5f * dis(gen)));
                }
            }
        }
    }
}
