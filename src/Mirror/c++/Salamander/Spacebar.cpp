#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include "stb_easy_font.h"

// Constants for spacebar dimensions
constexpr float SPACEBAR_WIDTH = 360.0f;  // 6x normal key width
constexpr float SPACEBAR_HEIGHT = 60.0f;
constexpr float SPACEBAR_DEPTH = 18.0f;
constexpr double PRESS_FEEDBACK_DURATION = 0.15; // seconds for press animation

// Spacebar state
struct Spacebar {
    float x, y;           // Position
    float pressAnim = 0.0f; // 0.0 (up) to 0.5 (fully pressed)
    bool isPressed = false;
    bool keycapRemoved = false; // Track if keycap is removed
};

Spacebar spacebar;
double g_lastFrameTime = 0.0;
bool g_leftMouseDown = false;

// Embedded ChucK Code for click sound
const char* embeddedChucKCode = R"(
// Ultra-Crisp Mechanical Keyboard Click in ChucK
Noise clickNoise => HPF noiseHPF => ADSR noiseEnv => dac;
SinOsc clickSine => ADSR sineEnv => dac;

// Noise component: ultra-short burst for the raw click edge
1.0 => clickNoise.gain;
5000 => noiseHPF.freq;      // High-pass filter to cut out lower frequencies
noiseEnv.set(0, 1, 0.0003, 0.02); // Blisteringly fast attack and decay

// Sine component: a piercing transient to accentuate the click
10000 => clickSine.freq;    // Extremely high frequency for extra snap
1.0 => clickSine.gain;
sineEnv.set(0, 1, 0.0001, 0.015);  // Even shorter envelope for a razor-thin burst

// Fire both components simultaneously for maximum impact
noiseEnv.keyOn();
sineEnv.keyOn();
1::ms => now;   // A brief moment for the click to be audible
noiseEnv.keyOff();
sineEnv.keyOff();
10::ms => now;  // Allow the tails to decay naturally
)";
const char* TEMP_CHUCK_FILENAME = "temp_chuck.ck";

// Draw the spacebar keycap in 3D
void drawSpacebarKeycap(const Spacebar& bar) {
    float pressAnim = bar.pressAnim;
    float bx = bar.x, by = bar.y;
    float bw = SPACEBAR_WIDTH, bh = SPACEBAR_HEIGHT;
    float shiftLeft = 10.0f * pressAnim;
    float shiftUp = 10.0f * pressAnim;
    float pressOffsetZ = SPACEBAR_DEPTH * pressAnim;
    float newDepth = SPACEBAR_DEPTH * (1.0f - 0.5f * pressAnim);
    float x = bx - shiftLeft;
    float y = by - shiftUp;

    float baseR = 0.93f, baseG = 0.93f, baseB = 0.88f;

    // FRONT face
    glColor3f(baseR, baseG, baseB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glEnd();

    // TOP face
    glColor3f(baseR + 0.07f, baseG + 0.07f, baseB + 0.07f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    // RIGHT face
    glColor3f(baseR - 0.05f, baseG - 0.05f, baseB - 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    // BOTTOM face
    glColor3f(baseR - 0.02f, baseG - 0.02f, baseB - 0.02f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    // LEFT face
    glColor3f(baseR - 0.03f, baseG - 0.03f, baseB - 0.03f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();
}

// Draw a beveled box in 3D
void drawBeveledBox3D(float x, float y, float w, float h,
    float depth,
    float baseR, float baseG, float baseB)
{
    float bevel = depth * 0.5f;
    // FRONT face
    glColor3f(baseR, baseG, baseB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x, y + h, 0);
    glEnd();

    // TOP face
    glColor3f(baseR + 0.07f, baseG + 0.07f, baseB + 0.07f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w - bevel, y - bevel, -depth);
    glVertex3f(x - bevel, y - bevel, -depth);
    glEnd();

    // RIGHT face
    glColor3f(baseR - 0.05f, baseG - 0.05f, baseB - 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x + w - bevel, y + h - bevel, -depth);
    glVertex3f(x + w - bevel, y - bevel, -depth);
    glEnd();

    // BOTTOM face
    glColor3f(baseR - 0.02f, baseG - 0.02f, baseB - 0.02f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + h, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x + w - bevel, y + h - bevel, -depth);
    glVertex3f(x - bevel, y + h - bevel, -depth);
    glEnd();

    // LEFT face
    glColor3f(baseR - 0.03f, baseG - 0.03f, baseB - 0.03f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x, y + h, 0);
    glVertex3f(x - bevel, y + h - bevel, -depth);
    glVertex3f(x - bevel, y - bevel, -depth);
    glEnd();
}

// Draw three-faced cube (for stem)
void drawThreeFacedCube(float x, float y, float w, float h,
    float depth,
    float baseR, float baseG, float baseB)
{
    // FRONT face
    glColor3f(baseR, baseG, baseB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x, y + h, 0);
    glEnd();

    float bevel = depth * 0.5f;
    // TOP face
    glColor3f(baseR + 0.07f, baseG + 0.07f, baseB + 0.07f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w - bevel, y - bevel, -depth);
    glVertex3f(x - bevel, y - bevel, -depth);
    glEnd();

    // LEFT face
    glColor3f(baseR - 0.03f, baseG - 0.03f, baseB - 0.03f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x, y + h, 0);
    glVertex3f(x - bevel, y + h - bevel, -depth);
    glVertex3f(x - bevel, y - bevel, -depth);
    glEnd();
}

// Draw mechanical switch when keycap is removed
void drawMechanicalSwitch(const Spacebar& bar) {
    float pressAnim = bar.pressAnim;
    float shiftLeft = 10.0f * pressAnim;
    float shiftUp = 10.0f * pressAnim;
    float pressOffsetZ = SPACEBAR_DEPTH * pressAnim;

    // Outer "switch housing"
    float bx = bar.x + SPACEBAR_WIDTH * 0.3f;
    float by = bar.y + SPACEBAR_HEIGHT * 0.3f;
    float bw = SPACEBAR_WIDTH * 0.4f;
    float bh = SPACEBAR_HEIGHT * 0.4f;
    float outerDepth = 16.0f * (SPACEBAR_DEPTH / 15.0f); // scaled accordingly
    drawBeveledBox3D(bx, by, bw, bh, outerDepth, 0.5f, 0.5f, 0.5f);

    // Inner "stem"
    float animDepth = outerDepth - 6.0f;
    {
        float cloneScale = 0.7f * 0.8f; // 0.56
        float cloneW = bw * cloneScale;
        float cloneH = bh * cloneScale;
        float greenCubeDepth = animDepth * cloneScale * 0.7143f; // ~4.0
        float normalizedPress = pressAnim / 0.5f;
        float restingZ = -(greenCubeDepth / 2.0f);
        float pressedZ = -(greenCubeDepth - 1.0f);
        float zTranslation = restingZ + normalizedPress * (pressedZ - restingZ);

        float cloneX = bx + (bw - cloneW) / 2.0f + 2.0f;
        float cloneY = by + (bh - cloneH) / 2.0f + 2.0f;

        glPushMatrix();
        glTranslatef(-0.5f * shiftLeft, -0.5f * shiftUp, -pressOffsetZ);
        glTranslatef(0, 0, zTranslation);
        glPushAttrib(GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_ALWAYS);
        drawThreeFacedCube(cloneX, cloneY, cloneW, cloneH, greenCubeDepth, 0.1f, 0.4f, 0.1f);
        glPopAttrib();
        glPopMatrix();
    }
}

// Draw the spacebar (either keycap or mechanical switch)
void drawSpacebar(const Spacebar& bar) {
    if (bar.keycapRemoved) {
        drawMechanicalSwitch(bar);
    }
    else {
        drawSpacebarKeycap(bar);
    }
}

// Update spacebar animation based on press state
void updateSpacebarAnimation(Spacebar& bar, float deltaTime) {
    float animSpeed = 0.5f / static_cast<float>(PRESS_FEEDBACK_DURATION);
    float target = bar.isPressed ? 0.5f : 0.0f;
    if (bar.pressAnim < target) {
        bar.pressAnim += animSpeed * deltaTime;
        if (bar.pressAnim > target) bar.pressAnim = target;
    }
    else if (bar.pressAnim > target) {
        bar.pressAnim -= animSpeed * deltaTime;
        if (bar.pressAnim < target) bar.pressAnim = target;
    }
}

// GLFW Callbacks
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE) {
        if (action == GLFW_PRESS) {
            spacebar.isPressed = true;
            std::thread([]() {
                std::string command = std::string("chuck ") + TEMP_CHUCK_FILENAME;
                system(command.c_str());
                }).detach();
        }
        else if (action == GLFW_RELEASE) {
            spacebar.isPressed = false;
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_leftMouseDown = true;
            // Check if mouse is over spacebar
            if (xpos >= spacebar.x && xpos <= spacebar.x + SPACEBAR_WIDTH &&
                ypos >= spacebar.y && ypos <= spacebar.y + SPACEBAR_HEIGHT) {
                spacebar.isPressed = true;
                std::thread([]() {
                    std::string command = std::string("chuck ") + TEMP_CHUCK_FILENAME;
                    system(command.c_str());
                    }).detach();
            }
        }
        else if (action == GLFW_RELEASE) {
            g_leftMouseDown = false;
            spacebar.isPressed = false;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            // Right click toggles keycap removal
            if (xpos >= spacebar.x && xpos <= spacebar.x + SPACEBAR_WIDTH &&
                ypos >= spacebar.y && ypos <= spacebar.y + SPACEBAR_HEIGHT) {
                spacebar.keycapRemoved = !spacebar.keycapRemoved;
            }
        }
    }
}

// Render text using stb_easy_font
void renderText(float x, float y, const char* text) {
    char buffer[99999];
    int num_quads = stb_easy_font_print(
        x,
        y,
        const_cast<char*>(text),
        nullptr,
        buffer,
        sizeof(buffer)
    );

    glDisable(GL_DEPTH_TEST);
    glColor3f(0.0f, 0.0f, 0.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_DEPTH_TEST);
}

// Initialize ChucK for sound
void initChucK() {
    std::ofstream out(TEMP_CHUCK_FILENAME);
    if (!out) {
        std::cerr << "Error: Could not create temporary ChucK file.\n";
        return;
    }
    out << embeddedChucKCode;
    out.close();
}

// Main function
int main() {
    if (!glfwInit()) {
        std::cerr << "Error: Failed to initialize GLFW\n";
        return -1;
    }

    // Create a windowed mode window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Spacebar Simulator", nullptr, nullptr);
    if (!window) {
        std::cerr << "Error: Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set up callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 800, 600, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Initialize spacebar position (centered)
    spacebar.x = (800 - SPACEBAR_WIDTH) / 2.0f;
    spacebar.y = (600 - SPACEBAR_HEIGHT) / 2.0f;

    g_lastFrameTime = glfwGetTime();
    initChucK();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - g_lastFrameTime);
        g_lastFrameTime = currentTime;

        // Background color: teal (matching original)
        glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Update & draw spacebar
        updateSpacebarAnimation(spacebar, deltaTime);
        drawSpacebar(spacebar);

        // Render "Space" text (only when keycap is not removed)
        if (!spacebar.keycapRemoved) {
            float shiftLeft = 10.0f * spacebar.pressAnim;
            float shiftUp = 10.0f * spacebar.pressAnim;
            float labelX = spacebar.x + (SPACEBAR_WIDTH * 0.5f) - 20.0f - shiftLeft;
            float labelY = spacebar.y + (SPACEBAR_HEIGHT * 0.5f) - 8.0f - shiftUp;
            renderText(labelX, labelY, "Space");
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
