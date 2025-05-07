#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <thread>
#include <fstream>
#include "stb_easy_font.h"

// Constants - These define the switch size and MUST NOT CHANGE
const float ORIGINAL_SPACEBAR_WIDTH = 360.0f;
const float SPACEBAR_HEIGHT = 60.0f;
const float SPACEBAR_DEPTH = 18.0f;
const double PRESS_FEEDBACK_DURATION = 0.15;

struct Spacebar {
    float x, y;
    float pressAnim = 0.0f;
    bool isPressed = false;
    bool keycapRemoved = false;
};

Spacebar spacebar;
double g_lastFrameTime = 0.0;
bool g_leftMouseDown = false;

const char* embeddedChucKCode = R"(
Noise clickNoise => HPF noiseHPF => ADSR noiseEnv => dac;
SinOsc clickSine => ADSR sineEnv => dac;
1.0 => clickNoise.gain;
5000 => noiseHPF.freq;
noiseEnv.set(0, 1, 0.0003, 0.02);
10000 => clickSine.freq;
1.0 => clickSine.gain;
sineEnv.set(0, 1, 0.0001, 0.015);
noiseEnv.keyOn();
sineEnv.keyOn();
1::ms => now;
noiseEnv.keyOff();
sineEnv.keyOff();
10::ms => now;
)";
const char* TEMP_CHUCK_FILENAME = "temp_chuck.ck";

// Draw keycap (now full-width)
void drawSpacebarKeycap(const Spacebar& bar, float windowWidth) {
    float pressAnim = bar.pressAnim;
    float bx = bar.x, by = bar.y;
    float bw = windowWidth; // KEYCAP width is the window width
    float bh = SPACEBAR_HEIGHT;
    float shiftLeft = 10.0f * pressAnim;
    float shiftUp = 10.0f * pressAnim;
    float pressOffsetZ = SPACEBAR_DEPTH * pressAnim;
    float newDepth = SPACEBAR_DEPTH * (1.0f - 0.5f * pressAnim);
    float x = bx - shiftLeft;
    float y = by - shiftUp;

    float baseR = 0.93f, baseG = 0.93f, baseB = 0.88f;

    // FRONT, TOP, RIGHT, BOTTOM, LEFT faces (using bw for width)
    glColor3f(baseR, baseG, baseB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glEnd();

    glColor3f(baseR + 0.07f, baseG + 0.07f, baseB + 0.07f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    glColor3f(baseR - 0.05f, baseG - 0.05f, baseB - 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    glColor3f(baseR - 0.02f, baseG - 0.02f, baseB - 0.02f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    glColor3f(baseR - 0.03f, baseG - 0.03f, baseB - 0.03f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();
}

// Draw beveled box (unchanged from original)
void drawBeveledBox3D(float x, float y, float w, float h, float depth, float baseR, float baseG, float baseB) {
    float bevel = depth * 0.5f;
    glColor3f(baseR, baseG, baseB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x, y + h, 0);
    glEnd();

    glColor3f(baseR + 0.07f, baseG + 0.07f, baseB + 0.07f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w - bevel, y - bevel, -depth);
    glVertex3f(x - bevel, y - bevel, -depth);
    glEnd();

    glColor3f(baseR - 0.05f, baseG - 0.05f, baseB - 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x + w - bevel, y + h - bevel, -depth);
    glVertex3f(x + w - bevel, y - bevel, -depth);
    glEnd();

    glColor3f(baseR - 0.02f, baseG - 0.02f, baseB - 0.02f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + h, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x + w - bevel, y + h - bevel, -depth);
    glVertex3f(x - bevel, y + h - bevel, -depth);
    glEnd();

    glColor3f(baseR - 0.03f, baseG - 0.03f, baseB - 0.03f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x, y + h, 0);
    glVertex3f(x - bevel, y + h - bevel, -depth);
    glVertex3f(x - bevel, y - bevel, -depth);
    glEnd();
}

// Draw three-faced cube (unchanged from original)
void drawThreeFacedCube(float x, float y, float w, float h, float depth, float baseR, float baseG, float baseB) {
    glColor3f(baseR, baseG, baseB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x, y + h, 0);
    glEnd();

    float bevel = depth * 0.5f;
    glColor3f(baseR + 0.07f, baseG + 0.07f, baseB + 0.07f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w - bevel, y - bevel, -depth);
    glVertex3f(x - bevel, y - bevel, -depth);
    glEnd();

    glColor3f(baseR - 0.03f, baseG - 0.03f, baseB - 0.03f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x, y + h, 0);
    glVertex3f(x - bevel, y + h - bevel, -depth);
    glVertex3f(x - bevel, y - bevel, -depth);
    glEnd();
}

// Draw switch (CRITICAL: Use ORIGINAL_SPACEBAR_WIDTH for dimensions)
void drawMechanicalSwitch(const Spacebar& bar, float windowWidth) {
    float pressAnim = bar.pressAnim;
    float shiftLeft = 10.0f * pressAnim;
    float shiftUp = 10.0f * pressAnim;
    float pressOffsetZ = SPACEBAR_DEPTH * pressAnim;

    // Calculate centered switch position based on window width
    float bx = (windowWidth - ORIGINAL_SPACEBAR_WIDTH * 0.4f) / 2.0f; // Center the switch
    float by = bar.y + SPACEBAR_HEIGHT * 0.3f;
    float bw = ORIGINAL_SPACEBAR_WIDTH * 0.4f; // Use ORIGINAL width
    float bh = SPACEBAR_HEIGHT * 0.4f;
    float outerDepth = 16.0f * (SPACEBAR_DEPTH / 15.0f);

    drawBeveledBox3D(bx, by, bw, bh, outerDepth, 0.5f, 0.5f, 0.5f);

    float animDepth = outerDepth - 6.0f;
    {
        float cloneScale = 0.7f * 0.8f;
        float cloneW = bw * cloneScale;
        float cloneH = bh * cloneScale;
        float greenCubeDepth = animDepth * cloneScale * 0.7143f;
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

// Draw spacebar (keycap or switch)
void drawSpacebar(const Spacebar& bar, float windowWidth) {
    if (bar.keycapRemoved) {
        drawMechanicalSwitch(bar, windowWidth);
    }
    else {
        drawSpacebarKeycap(bar, windowWidth);
    }
}

// Update animation (unchanged)
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

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        spacebar.isPressed = true;
        std::thread([]() {
            std::string command = std::string("chuck ") + TEMP_CHUCK_FILENAME;
            system(command.c_str());
            }).detach();
    }
    else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
        spacebar.isPressed = false;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_leftMouseDown = true;
            if (xpos >= spacebar.x && xpos <= spacebar.x + windowWidth &&
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
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        if (xpos >= spacebar.x && xpos <= spacebar.x + windowWidth &&
            ypos >= spacebar.y && ypos <= spacebar.y + SPACEBAR_HEIGHT) {
            spacebar.keycapRemoved = !spacebar.keycapRemoved;
        }
    }
}

// Framebuffer size callback (for window resizing)
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    spacebar.x = 0; // Keycap starts at left edge
    spacebar.y = (height - SPACEBAR_HEIGHT) / 2.0f;
}

void renderText(float x, float y, const char* text, float windowWidth) {
    char buffer[99999];
    float textWidth = strlen(text) * 8.0f; // Approximate
    float centeredX = (windowWidth - textWidth) / 2.0f; // Center text
    int num_quads = stb_easy_font_print(centeredX, y, const_cast<char*>(text), nullptr, buffer, sizeof(buffer));

    glDisable(GL_DEPTH_TEST);
    glColor3f(0.0f, 0.0f, 0.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_DEPTH_TEST);
}

void initChucK() {
    std::ofstream out(TEMP_CHUCK_FILENAME);
    if (out) out << embeddedChucKCode;
    out.close();
}

int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(800, 600, "Spacebar Simulator", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    int initialWidth, initialHeight;
    glfwGetWindowSize(window, &initialWidth, &initialHeight);
    framebuffer_size_callback(window, initialWidth, initialHeight);

    g_lastFrameTime = glfwGetTime();
    initChucK();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - g_lastFrameTime);
        g_lastFrameTime = currentTime;

        glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        updateSpacebarAnimation(spacebar, deltaTime);
        drawSpacebar(spacebar, windowWidth); // Pass window width

        if (!spacebar.keycapRemoved) {
            float shiftUp = 10.0f * spacebar.pressAnim;
            float labelY = spacebar.y + (SPACEBAR_HEIGHT * 0.5f) - 8.0f - shiftUp;
            renderText(spacebar.x, labelY, "Space", windowWidth);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
