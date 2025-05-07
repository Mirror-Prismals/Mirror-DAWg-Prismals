#include <GLFW/glfw3.h>
#include <jack/jack.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <atomic>
#include <mutex>

// Constants
const float SPACEBAR_HEIGHT = 60.0f;
const float SPACEBAR_DEPTH = 18.0f;
const size_t SAMPLE_RATE = 44100;           // 44100 samples per second
const size_t MAX_VISUAL_SAMPLES = SAMPLE_RATE * 5; // Keep last 5 seconds for display

// Global state for our "burning" mode demo.
// Recording is controlled via R key; but the burning indicator is always on (orange).
std::atomic<bool> isRecording{ true }; // Start recording automatically.
std::mutex visMutex;
std::vector<float> audioDataCopy;    // For waveform visualization.

jack_client_t* jackClient = nullptr;
jack_port_t* inputPort = nullptr;

// JACK process callback: just copy incoming audio samples to our visualization vector.
// We “burn” the audio by not writing it anywhere.
int process(jack_nframes_t nframes, void* /*arg*/) {
    jack_default_audio_sample_t* in = (jack_default_audio_sample_t*)jack_port_get_buffer(inputPort, nframes);

    if (isRecording) {
        std::lock_guard<std::mutex> lock(visMutex);
        // Append samples to the visualization vector.
        for (jack_nframes_t i = 0; i < nframes; ++i) {
            audioDataCopy.push_back(in[i]);
        }
        // Cap the vector to the last MAX_VISUAL_SAMPLES.
        if (audioDataCopy.size() > MAX_VISUAL_SAMPLES) {
            // Erase older samples.
            audioDataCopy.erase(audioDataCopy.begin(), audioDataCopy.begin() + (audioDataCopy.size() - MAX_VISUAL_SAMPLES));
        }
    }
    return 0;
}

// Draw the keycap and overlay the waveform.
void drawSpacebarKeycap(float windowWidth, float windowHeight) {
    float bx = 0, by = (windowHeight - SPACEBAR_HEIGHT) / 2.0f;
    float bw = windowWidth;
    float bh = SPACEBAR_HEIGHT;
    float x = bx, y = by;
    float baseR = 0.93f, baseG = 0.93f, baseB = 0.88f;

    // Top face
    glColor3f(baseR, baseG, baseB);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + bw, y, 0);
    glVertex3f(x + bw, y + bh, 0);
    glVertex3f(x, y + bh, 0);
    glEnd();

    // Front face
    glColor3f(baseR + 0.07f, baseG + 0.07f, baseB + 0.07f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + bw, y, 0);
    glVertex3f(x + bw - SPACEBAR_DEPTH, y - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glVertex3f(x - SPACEBAR_DEPTH, y - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glEnd();

    // Right face
    glColor3f(baseR - 0.05f, baseG - 0.05f, baseB - 0.05f);
    glBegin(GL_QUADS);
    glVertex3f(x + bw, y, 0);
    glVertex3f(x + bw, y + bh, 0);
    glVertex3f(x + bw - SPACEBAR_DEPTH, y + bh - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glVertex3f(x + bw - SPACEBAR_DEPTH, y - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glEnd();

    // Top edge
    glColor3f(baseR - 0.02f, baseG - 0.02f, baseB - 0.02f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + bh, 0);
    glVertex3f(x + bw, y + bh, 0);
    glVertex3f(x + bw - SPACEBAR_DEPTH, y + bh - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glVertex3f(x - SPACEBAR_DEPTH, y + bh - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glEnd();

    // Left face
    glColor3f(baseR - 0.03f, baseG - 0.03f, baseB - 0.03f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x, y + bh, 0);
    glVertex3f(x - SPACEBAR_DEPTH, y + bh - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glVertex3f(x - SPACEBAR_DEPTH, y - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glEnd();

    // Draw waveform overlay from audioDataCopy.
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_STRIP);
    {
        std::lock_guard<std::mutex> lock(visMutex);
        size_t dataSize = audioDataCopy.size();
        if (dataSize > 1) {
            float waveformHeight = bh * 0.8f;
            float waveformYOffset = (bh - waveformHeight) / 2.0f;
            for (size_t i = 0; i < dataSize; ++i) {
                float sample = audioDataCopy[i];
                // Normalize sample from [-1,1] to [0,1]
                float normalized = (sample + 1.0f) / 2.0f;
                float yPos = y + waveformYOffset + normalized * waveformHeight;
                float xPos = x + (static_cast<float>(i) / (dataSize - 1)) * bw;
                glVertex3f(xPos, yPos, 1.0f);
            }
        }
    }
    glEnd();
}

// Always draw the burning indicator as an orange circle in the top middle.
void drawBurningIndicator(int windowWidth) {
    float radius = 20.0f;
    float centerX = windowWidth / 2.0f;
    float centerY = 50.0f; // 50 pixels from the top
    // Always orange: (1.0, 0.65, 0.0)
    glColor3f(1.0f, 0.65f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(centerX, centerY);
    int numSegments = 30;
    for (int i = 0; i <= numSegments; i++) {
        float angle = 2.0f * 3.1415926f * i / numSegments;
        float dx = cos(angle) * radius;
        float dy = sin(angle) * radius;
        glVertex2f(centerX + dx, centerY + dy);
    }
    glEnd();
}

// GLFW framebuffer resize callback.
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
}

// GLFW key callback: Press R to toggle recording on/off.
void key_callback(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        isRecording = !isRecording;
        if (!isRecording) {
            std::cout << "Recording stopped (burning mode: audio is discarded)" << std::endl;
        }
        else {
            std::cout << "Recording started (burning mode: audio is discarded)" << std::endl;
            // Clear visualization buffer when starting.
            std::lock_guard<std::mutex> lock(visMutex);
            audioDataCopy.clear();
        }
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main() {
    if (!glfwInit())
        return -1;

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Burning DAW Demo", primaryMonitor, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    framebuffer_size_callback(window, width, height);

    // Open JACK client.
    jackClient = jack_client_open("BurningDAW", JackNullOption, nullptr);
    if (!jackClient) {
        std::cerr << "JACK server not running?" << std::endl;
        glfwTerminate();
        return -1;
    }
    jack_set_process_callback(jackClient, process, nullptr);
    inputPort = jack_port_register(jackClient, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    if (!inputPort) {
        std::cerr << "Could not register JACK input port." << std::endl;
        jack_client_close(jackClient);
        glfwTerminate();
        return -1;
    }
    if (jack_activate(jackClient)) {
        std::cerr << "Cannot activate JACK client." << std::endl;
        jack_client_close(jackClient);
        glfwTerminate();
        return -1;
    }
    const char** ports = jack_get_ports(jackClient, nullptr, JACK_DEFAULT_AUDIO_TYPE, JackPortIsPhysical | JackPortIsOutput);
    if (ports) {
        if (jack_connect(jackClient, ports[0], jack_port_name(inputPort))) {
            std::cerr << "Cannot connect input port" << std::endl;
        }
        jack_free(ports);
    }

    // Main loop.
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glfwGetWindowSize(window, &width, &height);
        drawSpacebarKeycap(width, height);
        drawBurningIndicator(width);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    jack_deactivate(jackClient);
    jack_client_close(jackClient);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
