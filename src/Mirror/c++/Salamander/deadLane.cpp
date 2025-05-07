#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include "stb_easy_font.h"

// Constants - These define the switch size and MUST NOT CHANGE
const float SPACEBAR_HEIGHT = 60.0f;
const float SPACEBAR_DEPTH = 18.0f;

struct Spacebar {
    float x, y;  // Position of the spacebar
};

Spacebar spacebar;

// Draw keycap (full-width)
void drawSpacebarKeycap(const Spacebar& bar, float windowWidth) {
    float bx = bar.x, by = bar.y;
    float bw = windowWidth; // KEYCAP width is the window width
    float bh = SPACEBAR_HEIGHT;
    float x = bx;
    float y = by;

    float baseR = 0.93f, baseG = 0.93f, baseB = 0.88f;

    // FRONT, TOP, RIGHT, BOTTOM, LEFT faces (using bw for width)
    glColor3f(baseR, baseG, baseB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + bw, y, 0);
    glVertex3f(x + bw, y + bh, 0);
    glVertex3f(x, y + bh, 0);
    glEnd();

    glColor3f(baseR + 0.07f, baseG + 0.07f, baseB + 0.07f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + bw, y, 0);
    glVertex3f(x + bw - SPACEBAR_DEPTH, y - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glVertex3f(x - SPACEBAR_DEPTH, y - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glEnd();

    glColor3f(baseR - 0.05f, baseG - 0.05f, baseB - 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(x + bw, y, 0);
    glVertex3f(x + bw, y + bh, 0);
    glVertex3f(x + bw - SPACEBAR_DEPTH, y + bh - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glVertex3f(x + bw - SPACEBAR_DEPTH, y - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glEnd();

    glColor3f(baseR - 0.02f, baseG - 0.02f, baseB - 0.02f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + bh, 0);
    glVertex3f(x + bw, y + bh, 0);
    glVertex3f(x + bw - SPACEBAR_DEPTH, y + bh - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glVertex3f(x - SPACEBAR_DEPTH, y + bh - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glEnd();

    glColor3f(baseR - 0.03f, baseG - 0.03f, baseB - 0.03f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x, y + bh, 0);
    glVertex3f(x - SPACEBAR_DEPTH, y + bh - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glVertex3f(x - SPACEBAR_DEPTH, y - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glEnd();
}


// Draw spacebar (keycap only)
void drawSpacebar(const Spacebar& bar, float windowWidth) {
    drawSpacebarKeycap(bar, windowWidth);
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


int main() {
    if (!glfwInit()) return -1;

    // --- Fullscreen Setup ---
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Spacebar Simulator", primaryMonitor, nullptr);
    // --- End Fullscreen Setup ---

    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    int initialWidth, initialHeight;
    glfwGetWindowSize(window, &initialWidth, &initialHeight);
    framebuffer_size_callback(window, initialWidth, initialHeight);


    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        drawSpacebar(spacebar, windowWidth); // Pass window width

        float labelY = spacebar.y + (SPACEBAR_HEIGHT * 0.5f) - 8.0f;
        renderText(spacebar.x, labelY, "Space", windowWidth);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
