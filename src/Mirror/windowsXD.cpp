#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <ctime>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const int TASKBAR_HEIGHT = 48;
const int START_BTN_WIDTH = 130;
const int START_BTN_HEIGHT = TASKBAR_HEIGHT;
const int CLOCK_WIDTH = 110;
const int CLOCK_HEIGHT = TASKBAR_HEIGHT;

// XP Colors
const float COLOR_START_BTN[3] = { 0.15f, 0.58f, 0.22f }; // XP green
const float COLOR_START_BTN_LIGHT[3] = { 0.38f, 0.80f, 0.36f }; // XP highlight
const float COLOR_START_BTN_DARK[3] = { 0.10f, 0.38f, 0.13f }; // XP shadow
const float COLOR_START_BTN_BORDER[3] = { 0.09f, 0.28f, 0.10f }; // XP dark border
const float COLOR_START_BTN_HOVER[3] = { 0.22f, 0.70f, 0.32f };
const float COLOR_START_BTN_ACTIVE[3] = { 0.10f, 0.38f, 0.13f };
const float COLOR_TASKBAR_TOP[3] = { 0.38f, 0.60f, 0.91f }; // Lighter blue
const float COLOR_TASKBAR_BOTTOM[3] = { 0.16f, 0.32f, 0.64f }; // Darker blue

// XP clock blue: even more vibrant azure than the taskbar
const float COLOR_CLOCK_FACE[3] = { 0.32f, 0.63f, 1.00f }; // Azure blue
const float COLOR_CLOCK_LIGHT[3] = { 0.55f, 0.82f, 1.00f }; // Highlight
const float COLOR_CLOCK_DARK[3] = { 0.18f, 0.38f, 0.70f }; // Shadow
const float COLOR_CLOCK_BORDER[3] = { 0.10f, 0.22f, 0.50f }; // Border

// Utility: draw a rectangle
void draw_rect(float x, float y, float w, float h, const float color[3]) {
    glColor3fv(color);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// Utility: draw a rectangle border
void draw_rect_border(float x, float y, float w, float h, float thickness, const float color[3]) {
    // Top
    draw_rect(x, y, w, thickness, color);
    // Left
    draw_rect(x, y, thickness, h, color);
    // Right
    draw_rect(x + w - thickness, y, thickness, h, color);
    // Bottom
    draw_rect(x, y + h - thickness, w, thickness, color);
}

// Utility: draw a vertical gradient rectangle
void draw_gradient_rect(float x, float y, float w, float h, const float top[3], const float bottom[3]) {
    glBegin(GL_QUADS);
    glColor3fv(top);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glColor3fv(bottom);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// Utility: draw a simplified logo (single white circle with black outline)
void draw_simple_logo(float x, float y, float radius) {
    int segments = 36;
    float cx = x + radius;
    float cy = y + radius;
    // Draw black outline
    float outline_r = radius + 2.0f;
    glColor3f(0, 0, 0);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * M_PI * float(i) / float(segments);
        glVertex2f(cx + outline_r * cos(theta), cy + outline_r * sin(theta));
    }
    glEnd();
    // Draw main face (white)
    glColor3f(1, 1, 1);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; ++i) {
        float theta = 2.0f * M_PI * float(i) / float(segments);
        glVertex2f(cx + radius * cos(theta), cy + radius * sin(theta));
    }
    glEnd();
}

// Mouse state
struct MouseState {
    double x, y;
    bool left_pressed;
    bool left_held;
} mouse = { 0, 0, false, false };

void mouse_button_callback(GLFWwindow*, int button, int action, int /*mods*/) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) mouse.left_pressed = true, mouse.left_held = true;
        else if (action == GLFW_RELEASE) mouse.left_held = false;
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    mouse.x = xpos;
    mouse.y = ypos;
}

// Draw XP-style 3D Start button (rectangular, not rounded)
void draw_xp_start_button(float x, float y, float w, float h, bool hover, bool active) {
    // 3D effect: highlight (top/left), shadow (bottom/right)
    float highlight[3], shadow[3], face[3], border[3];
    for (int i = 0; i < 3; ++i) {
        highlight[i] = COLOR_START_BTN_LIGHT[i];
        shadow[i] = COLOR_START_BTN_DARK[i];
        border[i] = COLOR_START_BTN_BORDER[i];
    }
    if (active) {
        for (int i = 0; i < 3; ++i) face[i] = COLOR_START_BTN_ACTIVE[i];
    }
    else if (hover) {
        for (int i = 0; i < 3; ++i) face[i] = COLOR_START_BTN_HOVER[i];
    }
    else {
        for (int i = 0; i < 3; ++i) face[i] = COLOR_START_BTN[i];
    }

    // Outer border
    draw_rect_border(x, y, w, h, 2.0f, border);

    // Highlight (top and left)
    draw_rect(x + 2, y + 2, w - 4, 7, highlight); // Top highlight
    draw_rect(x + 2, y + 2, 7, h - 4, highlight); // Left highlight

    // Shadow (bottom and right)
    draw_rect(x + 2, y + h - 9, w - 4, 7, shadow); // Bottom shadow
    draw_rect(x + w - 9, y + 2, 7, h - 4, shadow); // Right shadow

    // Main face (slight vertical gradient for realism)
    float top_face[3], bottom_face[3];
    for (int i = 0; i < 3; ++i) {
        top_face[i] = std::min(face[i] + 0.08f, 1.0f);
        bottom_face[i] = std::max(face[i] - 0.05f, 0.0f);
    }
    draw_gradient_rect(x + 4, y + 4, w - 8, h - 8, top_face, bottom_face);
}

// Draw XP-style 3D clock (rectangular, not rounded)
void draw_xp_clock(float x, float y, float w, float h) {
    // 3D effect: highlight (top/left), shadow (bottom/right), border, gradient
    float highlight[3], shadow[3], face[3], border[3];
    for (int i = 0; i < 3; ++i) {
        highlight[i] = COLOR_CLOCK_LIGHT[i];
        shadow[i] = COLOR_CLOCK_DARK[i];
        border[i] = COLOR_CLOCK_BORDER[i];
        face[i] = COLOR_CLOCK_FACE[i];
    }

    // Outer border
    draw_rect_border(x, y, w, h, 2.0f, border);

    // Highlight (top and left)
    draw_rect(x + 2, y + 2, w - 4, 7, highlight); // Top highlight
    draw_rect(x + 2, y + 2, 7, h - 4, highlight); // Left highlight

    // Shadow (bottom and right)
    draw_rect(x + 2, y + h - 9, w - 4, 7, shadow); // Bottom shadow
    draw_rect(x + w - 9, y + 2, 7, h - 4, shadow); // Right shadow

    // Main face (vertical gradient)
    float top_face[3], bottom_face[3];
    for (int i = 0; i < 3; ++i) {
        top_face[i] = std::min(face[i] + 0.09f, 1.0f);
        bottom_face[i] = std::max(face[i] - 0.07f, 0.0f);
    }
    draw_gradient_rect(x + 4, y + 4, w - 8, h - 8, top_face, bottom_face);
}

int main() {
    if (!glfwInit()) return -1;

    // Get primary monitor and its video mode for fullscreen
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int WINDOW_WIDTH = mode->width;
    int WINDOW_HEIGHT = mode->height;

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "XP Taskbar Lookalike", monitor, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    while (!glfwWindowShouldClose(window)) {
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClearColor(0.5f, 0.7f, 1.0f, 1.0f); // XP sky blue
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Draw taskbar gradient
        draw_gradient_rect(0, WINDOW_HEIGHT - TASKBAR_HEIGHT, WINDOW_WIDTH, TASKBAR_HEIGHT, COLOR_TASKBAR_TOP, COLOR_TASKBAR_BOTTOM);

        // Start button position (no margin, full height)
        float btn_x = 0;
        float btn_y = WINDOW_HEIGHT - TASKBAR_HEIGHT;
        float btn_w = START_BTN_WIDTH;
        float btn_h = START_BTN_HEIGHT;

        // Mouse hover/click detection
        bool hover = mouse.x >= btn_x && mouse.x <= btn_x + btn_w && mouse.y >= btn_y && mouse.y <= btn_y + btn_h;
        bool active = hover && mouse.left_held;

        // Draw XP-style Start button (rectangular, 3D, correct green)
        draw_xp_start_button(btn_x, btn_y, btn_w, btn_h, hover, active);

        // Draw simplified logo (bigger, with black outline)
        float logo_radius = 18.0f;
        float logo_x = btn_x + 13.0f;
        float logo_y = btn_y + (btn_h - 2 * logo_radius) / 2.0f;
        draw_simple_logo(logo_x, logo_y, logo_radius);

        // Draw 3D clock (full height, azure blue, right side)
        float clock_x = WINDOW_WIDTH - CLOCK_WIDTH;
        float clock_y = WINDOW_HEIGHT - CLOCK_HEIGHT;
        float clock_w = CLOCK_WIDTH;
        float clock_h = CLOCK_HEIGHT;
        draw_xp_clock(clock_x, clock_y, clock_w, clock_h);

        // Reset mouse click (so click is only detected once per press)
        mouse.left_pressed = false;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
