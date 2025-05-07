#include <GLFW/glfw3.h>
#include <jack/jack.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <atomic>
#include <mutex>
#include <cstdlib>  // for rand()

// Constants for lane dimensions and audio rate
static constexpr float LANE_HEIGHT = 60.0f;   // Height of the DAW lane
static constexpr float LANE_DEPTH = 18.0f;    // Depth for 3D effect
static constexpr size_t SAMPLE_RATE = 44100;  // Samples per second

// Stepped gradient settings:
// We use 4 base colors: redish, orange, yellow, green.
// 23 steps per transition => TOTAL_STEPS = 23 * (4 - 1) = 69.
static constexpr int STEPS_PER_COLOR = 23;
static constexpr int NUM_BASE_COLORS = 4;
static constexpr int TOTAL_STEPS = STEPS_PER_COLOR * (NUM_BASE_COLORS - 1);

// Base colors in [0,1]: redish, orange, yellow, green.
const float BASE_COLORS[NUM_BASE_COLORS][3] = {
    {1.0f, 0.2f, 0.3f},  // Redish
    {1.0f, 0.6f, 0.2f},  // Orange
    {1.0f, 1.0f, 0.2f},  // Yellow
    {0.2f, 1.0f, 0.2f}   // Green
};

// Global state for recording
std::atomic<bool> isRecording{ false };  // Toggle with R
std::mutex dataMutex;

// Audio data and per-sample modulation parameters:
// audioData: all recorded samples
// samplePeriods: randomized period (in seconds) per sample for color modulation
// samplePhases: randomized phase offset (in radians) per sample
std::vector<float> audioData;
std::vector<float> samplePeriods;
std::vector<float> samplePhases;

// JACK globals
jack_client_t* jackClient = nullptr;
jack_port_t* inputPort = nullptr;

//-----------------------------------------------------------------------------
// JACK process callback: Append incoming audio and, for each sample,
// generate a random period and phase.
int processCallback(jack_nframes_t nframes, void* /*arg*/) {
    auto* in = (jack_default_audio_sample_t*)jack_port_get_buffer(inputPort, nframes);
    if (isRecording) {
        std::lock_guard<std::mutex> lock(dataMutex);
        // Append samples.
        audioData.insert(audioData.end(), in, in + nframes);
        // For each sample, generate random period and phase.
        // Here, period is chosen between 0.2 and 1.0 seconds.
        // Phase is chosen between 0 and 2*pi.
        for (jack_nframes_t i = 0; i < nframes; i++) {
            float period = 0.2f + (static_cast<float>(rand()) / RAND_MAX) * 0.8f;
            float phase = (static_cast<float>(rand()) / RAND_MAX) * 6.28318530718f;
            samplePeriods.push_back(period);
            samplePhases.push_back(phase);
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
// This function maps a sample's amplitude ([-1,1]) into a stepped index
// in our 69-color palette, then adds a time-based offset (using the sample's
// period and phase) to animate along the palette. The resulting modulated index
// is then converted back into an RGB color by interpolating between base colors.
inline void modulatedAmplitudeToColor(float amplitude,
    float period,
    float phase,
    float time,
    float& r, float& g, float& b) {
    // Normalize amplitude from [-1,1] to [0,1]
    float norm = (amplitude + 1.f) * 0.5f;
    if (norm < 0.f) norm = 0.f;
    if (norm > 1.f) norm = 1.f;

    // Map normalized value to a base index in [0, TOTAL_STEPS]
    float baseIndex = norm * TOTAL_STEPS;

    // Compute a time-based offset that oscillates between 0 and STEPS_PER_COLOR.
    float offset = (std::sin(2.f * 3.14159265359f * (time / period) + phase) + 1.f) * 0.5f * STEPS_PER_COLOR;

    // Combine to get a modulated index. Clamp to TOTAL_STEPS.
    float modIndex = baseIndex + offset;
    if (modIndex > TOTAL_STEPS)
        modIndex = TOTAL_STEPS;

    // Determine which segment (base color pair) the index falls into.
    float segmentF = modIndex / float(STEPS_PER_COLOR);
    int segment = int(segmentF);
    if (segment >= NUM_BASE_COLORS - 1) {
        segment = NUM_BASE_COLORS - 2;
        segmentF = float(NUM_BASE_COLORS - 1);
    }
    float t = segmentF - float(segment); // fractional part
    // Optional: Apply smoothstep for a smoother transition.
    t = t * t * (3.f - 2.f * t);

    // Interpolate between the two base colors.
    r = BASE_COLORS[segment][0] + t * (BASE_COLORS[segment + 1][0] - BASE_COLORS[segment][0]);
    g = BASE_COLORS[segment][1] + t * (BASE_COLORS[segment + 1][1] - BASE_COLORS[segment][1]);
    b = BASE_COLORS[segment][2] + t * (BASE_COLORS[segment + 1][2] - BASE_COLORS[segment][2]);
}

//-----------------------------------------------------------------------------
// Draw the skeuomorphic DAW lane with the animated waveform overlay.
// For each sample, we compute its color by modulating its amplitude-mapped stepped index
// using its individual period and phase.
void drawSkeuomorphicLane(float windowWidth, float windowHeight) {
    float x = 0.f;
    float y = (windowHeight - LANE_HEIGHT) * 0.5f;
    float w = windowWidth;
    float h = LANE_HEIGHT;
    float d = LANE_DEPTH; // Depth for 3D effect

    // Define face colors for the lane.
    float topR = 0.93f, topG = 0.93f, topB = 0.88f; // Top face.
    float frontR = topR + 0.07f, frontG = topG + 0.07f, frontB = topB + 0.07f; // Front face.
    float sideR = topR - 0.05f, sideG = topG - 0.05f, sideB = topB - 0.05f;       // Right side face.

    // Draw top face.
    glColor3f(topR, topG, topB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x, y + h, 0);
    glEnd();

    // Draw front face (beveled edge).
    glColor3f(frontR, frontG, frontB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w - d, y - d, -d);
    glVertex3f(x - d, y - d, -d);
    glEnd();

    // Draw right side face.
    glColor3f(sideR, sideG, sideB);
    glBegin(GL_QUADS);
    glVertex3f(x + w, y, 0);
    glVertex3f(x + w, y + h, 0);
    glVertex3f(x + w - d, y + h - d, -d);
    glVertex3f(x + w - d, y - d, -d);
    glEnd();

    // Retrieve local copy of audio data and modulation parameters.
    std::vector<float> localAudio;
    std::vector<float> localPeriods;
    std::vector<float> localPhases;
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        localAudio = audioData;
        localPeriods = samplePeriods;
        localPhases = samplePhases;
    }
    size_t size = localAudio.size();
    if (size < 2) return;
    float scaleX = w / static_cast<float>(size - 1);
    float waveH = h * 0.8f;
    float offsetY = (h - waveH) * 0.5f;
    double currentTime = glfwGetTime(); // Global clock in seconds

    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < size; i++) {
        // For each sample, compute its modulated color.
        float mod_r, mod_g, mod_b;
        // Use sample's period and phase if available; otherwise use defaults.
        float period = (i < localPeriods.size()) ? localPeriods[i] : 0.5f;
        float phase = (i < localPhases.size()) ? localPhases[i] : 0.f;
        modulatedAmplitudeToColor(localAudio[i], period, phase, (float)currentTime,
            mod_r, mod_g, mod_b);
        glColor3f(mod_r, mod_g, mod_b);

        float norm = (localAudio[i] + 1.f) * 0.5f;  // map [-1,1] -> [0,1]
        float xx = x + static_cast<float>(i) * scaleX;
        float yy = y + offsetY + norm * waveH;
        glVertex3f(xx, yy, 0.5f);  // Draw above top face.
    }
    glEnd();
}

//-----------------------------------------------------------------------------
// Draw the burning indicator: a fixed orange circle in the top center.
void drawBurningIndicator(int windowWidth) {
    float radius = 20.f;
    float cx = windowWidth * 0.5f;
    float cy = 50.f; // 50 pixels from the top.
    glColor3f(1.f, 0.65f, 0.f); // Orange.
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    const int segments = 30;
    for (int i = 0; i <= segments; i++) {
        float angle = 2.f * 3.14159265359f * i / segments;
        float dx = cosf(angle) * radius;
        float dy = sinf(angle) * radius;
        glVertex2f(cx + dx, cy + dy);
    }
    glEnd();
}

//-----------------------------------------------------------------------------
// GLFW framebuffer resize callback.
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
}

//-----------------------------------------------------------------------------
// GLFW key callback: R toggles recording (clearing previous waveform); ESC quits.
void key_callback(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        isRecording = !isRecording;
        if (!isRecording) {
            std::cout << "[Recording] Stopped.\n";
        }
        else {
            std::cout << "[Recording] Started. Clearing previous waveform...\n";
            std::lock_guard<std::mutex> lock(dataMutex);
            audioData.clear();
            samplePeriods.clear();
            samplePhases.clear();
        }
    }
    else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

//-----------------------------------------------------------------------------
// Main function.
int main() {
    if (!glfwInit())
        return -1;

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height,
        "Skeuomorphic DAW Lane (Animated Stepped Colors)",
        primary, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    framebuffer_size_callback(window, w, h);

    // JACK initialization.
    jackClient = jack_client_open("SkeuomorphicDAW", JackNullOption, nullptr);
    if (!jackClient) {
        std::cerr << "JACK server not running?\n";
        glfwTerminate();
        return -1;
    }
    jack_set_process_callback(jackClient, processCallback, nullptr);

    inputPort = jack_port_register(jackClient, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    if (!inputPort) {
        std::cerr << "Could not register JACK input port.\n";
        jack_client_close(jackClient);
        glfwTerminate();
        return -1;
    }
    if (jack_activate(jackClient)) {
        std::cerr << "Cannot activate JACK client.\n";
        jack_client_close(jackClient);
        glfwTerminate();
        return -1;
    }
    const char** ports = jack_get_ports(jackClient, nullptr, JACK_DEFAULT_AUDIO_TYPE,
        JackPortIsPhysical | JackPortIsOutput);
    if (ports) {
        if (jack_connect(jackClient, ports[0], jack_port_name(inputPort))) {
            std::cerr << "Cannot connect input port.\n";
        }
        jack_free(ports);
    }

    // Main render loop.
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.f, 0.5f, 0.5f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glfwGetWindowSize(window, &w, &h);
        drawSkeuomorphicLane((float)w, (float)h);
        drawBurningIndicator(w);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup.
    jack_deactivate(jackClient);
    jack_client_close(jackClient);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
