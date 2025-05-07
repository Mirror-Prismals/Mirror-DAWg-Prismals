#include <iostream>
#include <cmath>
#include <atomic>
#include <thread>
#include <chrono>
#include <jack/jack.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// Global atomics for left and right channel levels (range [0,1]).
static std::atomic<float> g_level_left(0.0f);
static std::atomic<float> g_level_right(0.0f);

struct Color {
    float r, g, b;
};

// Linear interpolation between two colors.
Color lerpColor(const Color& a, const Color& b, float t) {
    Color result;
    result.r = a.r + t * (b.r - a.r);
    result.g = a.g + t * (b.g - a.g);
    result.b = a.b + t * (b.b - a.b);
    return result;
}

// Darken a color by a given factor (using 0.25 for 4× darker).
Color darkenColor(const Color& c, float factor = 0.25f) {
    return { c.r * factor, c.g * factor, c.b * factor };
}

// Data structure to hold JACK port pointers.
struct JackData {
    jack_port_t* left;
    jack_port_t* right;
};

// JACK process callback: computes peak absolute sample for left and right channels.
int jack_process(jack_nframes_t nframes, void* arg) {
    JackData* data = static_cast<JackData*>(arg);
    float* in_left = static_cast<float*>(jack_port_get_buffer(data->left, nframes));
    float* in_right = static_cast<float*>(jack_port_get_buffer(data->right, nframes));

    float max_left = 0.0f;
    float max_right = 0.0f;
    for (jack_nframes_t i = 0; i < nframes; i++) {
        float abs_left = std::fabs(in_left[i]);
        float abs_right = std::fabs(in_right[i]);
        if (abs_left > max_left)
            max_left = abs_left;
        if (abs_right > max_right)
            max_right = abs_right;
    }
    g_level_left.store(max_left);
    g_level_right.store(max_right);
    return 0;
}

// Map a decibel value (in [-60, 0]) to a Y coordinate within the meter.
float dBToY(float dB, float meter_bottom, float meter_height) {
    return meter_bottom + ((dB + 60.0f) / 60.0f) * meter_height;
}

// Render a filled stepped gradient for one channel in the given horizontal range.
// Note: This function does NOT draw the outer bounding box.
void renderChannelMeter(float meter_left, float meter_right,
    float meter_bottom, float meter_top,
    float meter_height, float level)
{
    // 60 steps spanning -60 dB to 0 dB.
    const float dB_min = -60.0f;
    const float dB_max = 0.0f;
    const int steps = 60;
    float step_dB = (dB_max - dB_min) / steps; // 1 dB per step.

    // Anchor colors (from bottom to top):
    // Blue, Cyan, Green, Yellow, Orange, Red.
    Color blue = { 0.0f, 0.0f, 1.0f };
    Color cyan = { 0.0f, 1.0f, 1.0f };
    Color green = { 0.0f, 1.0f, 0.0f };
    Color yellow = { 1.0f, 1.0f, 0.0f };
    Color orange = { 1.0f, 0.65f, 0.0f };
    Color red = { 1.0f, 0.0f, 0.0f };

    // Convert level (linear) to decibels.
    if (level < 0.000001f)
        level = 0.000001f;
    float dB = 20.0f * std::log10(level);
    if (dB > 0.0f)
        dB = 0.0f;
    if (dB < dB_min)
        dB = dB_min;

    // Render each of the 60 steps.
    for (int i = 0; i < steps; i++) {
        float seg_low_dB = dB_min + i * step_dB;
        float seg_high_dB = seg_low_dB + step_dB;

        float y_low = dBToY(seg_low_dB, meter_bottom, meter_height);
        float y_high = dBToY(seg_high_dB, meter_bottom, meter_height);

        // Normalized position (0 at bottom, 1 at top) for the center of the step.
        float t = ((seg_low_dB + seg_high_dB) * 0.5f - dB_min) / (dB_max - dB_min);
        Color fullColor;
        if (t < 0.2f) {
            float t0 = t / 0.2f;
            fullColor = lerpColor(blue, cyan, t0);
        }
        else if (t < 0.4f) {
            float t0 = (t - 0.2f) / 0.2f;
            fullColor = lerpColor(cyan, green, t0);
        }
        else if (t < 0.6f) {
            float t0 = (t - 0.4f) / 0.2f;
            fullColor = lerpColor(green, yellow, t0);
        }
        else if (t < 0.8f) {
            float t0 = (t - 0.6f) / 0.2f;
            fullColor = lerpColor(yellow, orange, t0);
        }
        else {
            float t0 = (t - 0.8f) / 0.2f;
            fullColor = lerpColor(orange, red, t0);
        }
        Color darkColor = darkenColor(fullColor, 0.25f); // 4× darker

        // Render the step according to fill level.
        if (dB >= seg_high_dB) {
            // Step fully filled.
            glColor3f(fullColor.r, fullColor.g, fullColor.b);
            glBegin(GL_QUADS);
            glVertex2f(meter_left, y_low);
            glVertex2f(meter_right, y_low);
            glVertex2f(meter_right, y_high);
            glVertex2f(meter_left, y_high);
            glEnd();
        }
        else if (dB <= seg_low_dB) {
            // Step not reached.
            glColor3f(darkColor.r, darkColor.g, darkColor.b);
            glBegin(GL_QUADS);
            glVertex2f(meter_left, y_low);
            glVertex2f(meter_right, y_low);
            glVertex2f(meter_right, y_high);
            glVertex2f(meter_left, y_high);
            glEnd();
        }
        else {
            // Partially filled step.
            float y_partial = dBToY(dB, meter_bottom, meter_height);
            // Filled portion.
            glColor3f(fullColor.r, fullColor.g, fullColor.b);
            glBegin(GL_QUADS);
            glVertex2f(meter_left, y_low);
            glVertex2f(meter_right, y_low);
            glVertex2f(meter_right, y_partial);
            glVertex2f(meter_left, y_partial);
            glEnd();
            // Unfilled (darkened) portion.
            glColor3f(darkColor.r, darkColor.g, darkColor.b);
            glBegin(GL_QUADS);
            glVertex2f(meter_left, y_partial);
            glVertex2f(meter_right, y_partial);
            glVertex2f(meter_right, y_high);
            glVertex2f(meter_left, y_high);
            glEnd();
        }
    }
}

int main() {
    // ----- 1) Initialize JACK -----
    jack_client_t* client = jack_client_open("StereoSoundMeter", JackNullOption, nullptr);
    if (!client) {
        std::cerr << "Failed to connect to JACK server.\n";
        return 1;
    }
    JackData jackData;
    jackData.left = jack_port_register(client, "input_L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    jackData.right = jack_port_register(client, "input_R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    if (!jackData.left || !jackData.right) {
        std::cerr << "Failed to register JACK input ports.\n";
        jack_client_close(client);
        return 1;
    }
    jack_set_process_callback(client, jack_process, &jackData);
    if (jack_activate(client)) {
        std::cerr << "Cannot activate JACK client.\n";
        jack_client_close(client);
        return 1;
    }

    // ----- 2) Initialize GLFW and create a window -----
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        jack_client_close(client);
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Stereo Sound Meter", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        jack_client_close(client);
        return 1;
    }
    glfwMakeContextCurrent(window);
    int fb_width, fb_height;
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    glViewport(0, 0, fb_width, fb_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // ----- 3) Define overall meter dimensions (one meter split into two halves) -----
    // Overall meter bounding box.
    float overall_left = -0.01f;
    float overall_right = 0.01f;
    float meter_bottom = -0.9f;
    float orig_meter_height = 1.8f * 0.75f; // ~1.35 units tall.
    float meter_height = orig_meter_height * 0.6f;
    float meter_top = meter_bottom + meter_height;

    // Compute the vertical divider for the two halves (not used for rendering now).
    float mid_x = (overall_left + overall_right) / 2.0f;

    // Define left half boundaries.
    float left_meter_left = overall_left;
    float left_meter_right = mid_x;
    // Define right half boundaries.
    float right_meter_left = mid_x;
    float right_meter_right = overall_right;

    // ----- 4) Main rendering loop with FPS cap at 24 -----
    const double frameDuration = 1.0 / 24.0;  // ~41.67 ms per frame
    double previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        if (currentTime - previousTime >= frameDuration) {
            previousTime = currentTime;
            glfwPollEvents();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Render left channel in left half.
            float leftLevel = g_level_left.load();
            renderChannelMeter(left_meter_left, left_meter_right,
                meter_bottom, meter_top, meter_height, leftLevel);

            // Render right channel in right half.
            float rightLevel = g_level_right.load();
            renderChannelMeter(right_meter_left, right_meter_right,
                meter_bottom, meter_top, meter_height, rightLevel);

            // Draw overall bounding box on top.
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(overall_left, meter_bottom);
            glVertex2f(overall_right, meter_bottom);
            glVertex2f(overall_right, meter_top);
            glVertex2f(overall_left, meter_top);
            glEnd();

            glfwSwapBuffers(window);
        }
        else {
            // Sleep briefly to avoid busy waiting.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // ----- 5) Cleanup -----
    glfwDestroyWindow(window);
    glfwTerminate();
    jack_deactivate(client);
    jack_client_close(client);
    return 0;
}
