#include <GLFW/glfw3.h>
#include <jack/jack.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <atomic>
#include <mutex>

// Constants for keycap and display
const float SPACEBAR_HEIGHT = 60.0f;
const float SPACEBAR_DEPTH = 18.0f;
const size_t SAMPLE_RATE = 44100;           // Samples per second
const size_t MAX_VISUAL_SAMPLES = SAMPLE_RATE * 5; // Last 5 seconds of audio

// Envelope and spawn parameters (adjustable)
float amplitudeThreshold = 0.8f; // Minimum envelope value to spawn a blood cell
const float RELEASE_TIME = 0.034f;      // 34ms release time for smoothing
float spawnRate = 0.5f; // Time (in seconds) between spawns

// Global control variables
std::atomic<bool> isRecording{ true };
bool showEnvelopeLine = true;   // Toggle for envelope line visibility

std::mutex visMutex;
std::vector<float> audioDataCopy;    // Raw incoming audio samples
std::vector<float> gEnvelopeData;    // Smoothed (post-release) envelope values

// JACK globals
jack_client_t* jackClient = nullptr;
jack_port_t* inputPort = nullptr;

// Structure for a blood cell (spawned on the envelope)
// Now includes a fixed y position.
struct BloodCell {
    float x;   // x position (in pixels) along the keycap
    float y;   // baked y position computed at spawn time
    float t;   // time since spawn (for squishy animation)
};

std::vector<BloodCell> bloodCells;
float spawnCooldown = 0.0f;  // seconds

// ----------------------------------------------------------------------
// JACK process callback: copy incoming samples for visualization.
int process(jack_nframes_t nframes, void* /*arg*/) {
    jack_default_audio_sample_t* in = (jack_default_audio_sample_t*)jack_port_get_buffer(inputPort, nframes);
    if (isRecording) {
        std::lock_guard<std::mutex> lock(visMutex);
        for (jack_nframes_t i = 0; i < nframes; ++i) {
            audioDataCopy.push_back(in[i]);
        }
        if (audioDataCopy.size() > MAX_VISUAL_SAMPLES) {
            audioDataCopy.erase(audioDataCopy.begin(), audioDataCopy.begin() + (audioDataCopy.size() - MAX_VISUAL_SAMPLES));
        }
    }
    return 0;
}

// ----------------------------------------------------------------------
// Utility: Draw a torus using immediate mode.
// innerRadius: radius of the tube, outerRadius: distance from tube center to torus center,
// sides: subdivisions around the tube, rings: subdivisions around the main circle.
void drawTorus(float innerRadius, float outerRadius, int sides, int rings) {
    for (int i = 0; i < rings; i++) {
        float theta = (float)i * 2.0f * 3.1415926f / rings;
        float nextTheta = (float)(i + 1) * 2.0f * 3.1415926f / rings;
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= sides; j++) {
            float phi = (float)j * 2.0f * 3.1415926f / sides;
            float cosPhi = cos(phi);
            float sinPhi = sin(phi);
            // Vertex on current ring
            float x1 = (outerRadius + innerRadius * cosPhi) * cos(theta);
            float y1 = (outerRadius + innerRadius * cosPhi) * sin(theta);
            float z1 = innerRadius * sinPhi;
            glVertex3f(x1, y1, z1);
            // Vertex on next ring
            float x2 = (outerRadius + innerRadius * cosPhi) * cos(nextTheta);
            float y2 = (outerRadius + innerRadius * cosPhi) * sin(nextTheta);
            float z2 = innerRadius * sinPhi;
            glVertex3f(x2, y2, z2);
        }
        glEnd();
    }
}

// ----------------------------------------------------------------------
// Draw the keycap and the envelope line.
// The envelope line is drawn using the global gEnvelopeData (post-release).
void drawSpacebarKeycap(float windowWidth, float windowHeight) {
    float bx = 0;
    float by = (windowHeight - SPACEBAR_HEIGHT) / 2.0f;
    float bw = windowWidth;
    float bh = SPACEBAR_HEIGHT;
    float x = bx, y = by;

    // Top face (#800000)
    glColor3f(0.5f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + bw, y, 0);
    glVertex3f(x + bw, y + bh, 0);
    glVertex3f(x, y + bh, 0);
    glEnd();

    // Front face (#400000)
    glColor3f(0.25f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x + bw, y, 0);
    glVertex3f(x + bw - SPACEBAR_DEPTH, y - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glVertex3f(x - SPACEBAR_DEPTH, y - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glEnd();

    // Right face (#200000) – shadow effect.
    glColor3f(0.125f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3f(x + bw, y, 0);
    glVertex3f(x + bw, y + bh, 0);
    glVertex3f(x + bw - SPACEBAR_DEPTH, y + bh - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glVertex3f(x + bw - SPACEBAR_DEPTH, y - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glEnd();

    // Top edge (#800000)
    glColor3f(0.5f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + bh, 0);
    glVertex3f(x + bw, y + bh, 0);
    glVertex3f(x + bw - SPACEBAR_DEPTH, y + bh - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glVertex3f(x - SPACEBAR_DEPTH, y + bh - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glEnd();

    // Left face (#400000)
    glColor3f(0.25f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, 0);
    glVertex3f(x, y + bh, 0);
    glVertex3f(x - SPACEBAR_DEPTH, y + bh - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glVertex3f(x - SPACEBAR_DEPTH, y - SPACEBAR_DEPTH, -SPACEBAR_DEPTH);
    glEnd();

    // Optionally draw the envelope line.
    if (showEnvelopeLine) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_STRIP);
        {
            size_t dataSize = gEnvelopeData.size();
            if (dataSize > 1) {
                float waveformHeight = bh * 0.8f;
                for (size_t i = 0; i < dataSize; ++i) {
                    float env = gEnvelopeData[i];
                    float yPos = (y + bh) - env * waveformHeight;
                    float xPos = x + (static_cast<float>(i) / (dataSize - 1)) * bw;
                    glVertex3f(xPos, yPos, 1.0f);
                }
            }
        }
        glEnd();
    }
}

// ----------------------------------------------------------------------
// Update and draw blood cells.
// Now, each cell's y position is fixed when spawned.
// Cells only update their x position (and timer) over time.
void updateAndDrawBloodCells(float windowWidth, float windowHeight, const std::vector<float>& envelopeData, float deltaTime) {
    float bw = windowWidth;
    // Base rate for leftward motion (windowWidth/5 pixels per second).
    float shiftSpeed = windowWidth / 5.0f;

    // Update each blood cell: move left and update its timer.
    for (auto it = bloodCells.begin(); it != bloodCells.end(); ) {
        it->x -= shiftSpeed * deltaTime;
        it->t += deltaTime;
        if (it->x < 0) {
            it = bloodCells.erase(it);
        }
        else {
            ++it;
        }
    }

    // Draw blood cells.
    // Their color is interpolated based on the envelope value at spawn.
    for (auto& cell : bloodCells) {
        glPushMatrix();
        glTranslatef(cell.x, cell.y, 0);
        // Apply a squishy transformation: non-uniform scaling oscillates over time.
        float squish = 0.2f * sin(2.0f * 3.1415926f * cell.t);
        float scaleX = 1.0f + squish;
        float scaleY = 1.0f - squish;
        glScalef(scaleX, scaleY, 1.0f);
        // For simplicity, use the same color as computed at spawn.
        // (Alternatively, you could store a color in the BloodCell struct.)
        glColor3f(1.0f, 0.0f, 0.0f);
        drawTorus(2.0f, 4.0f, 16, 16);
        glPopMatrix();
    }
}

// ----------------------------------------------------------------------
// GLFW framebuffer resize callback.
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
}

// ----------------------------------------------------------------------
// GLFW key callback.
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_R) {
            isRecording = !isRecording;
            if (!isRecording) {
                std::cout << "Recording stopped (burning mode: audio is discarded)" << std::endl;
            }
            else {
                std::cout << "Recording started (burning mode: audio is discarded)" << std::endl;
                std::lock_guard<std::mutex> lock(visMutex);
                audioDataCopy.clear();
            }
        }
        else if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        else if (key == GLFW_KEY_L) {
            showEnvelopeLine = !showEnvelopeLine;
            std::cout << "Envelope line display: " << (showEnvelopeLine ? "ON" : "OFF") << std::endl;
        }
        else if (key == GLFW_KEY_T) {
            amplitudeThreshold += 0.05f;
            std::cout << "Amplitude threshold increased to " << amplitudeThreshold << std::endl;
        }
        else if (key == GLFW_KEY_G) {
            amplitudeThreshold -= 0.05f;
            if (amplitudeThreshold < 0.0f)
                amplitudeThreshold = 0.0f;
            std::cout << "Amplitude threshold decreased to " << amplitudeThreshold << std::endl;
        }
        else if (key == GLFW_KEY_Y) {
            spawnRate -= 0.05f;
            if (spawnRate < 0.05f)
                spawnRate = 0.05f;
            std::cout << "Spawn interval decreased to " << spawnRate << " seconds" << std::endl;
        }
        else if (key == GLFW_KEY_H) {
            spawnRate += 0.05f;
            std::cout << "Spawn interval increased to " << spawnRate << " seconds" << std::endl;
        }
    }
}

// ----------------------------------------------------------------------
// Main function.
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

    double lastTime = glfwGetTime();

    // Main loop.
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        spawnCooldown -= deltaTime;

        // Compute the post-release envelope from the raw audio.
        {
            std::lock_guard<std::mutex> lock(visMutex);
            size_t dataSize = audioDataCopy.size();
            gEnvelopeData.resize(dataSize);
            float alpha = exp(-1.0f / (RELEASE_TIME * SAMPLE_RATE));
            float prevEnv = 0.0f;
            for (size_t i = 0; i < dataSize; i++) {
                float sample = (audioDataCopy[i] > 0.0f) ? audioDataCopy[i] : 0.0f;
                float env = (sample < prevEnv) ? prevEnv * alpha : sample;
                gEnvelopeData[i] = env;
                prevEnv = env;
            }
        }

        // Compute the global envelope peak from gEnvelopeData.
        float envelopeMax = 0.0f;
        size_t maxIndex = 0;
        size_t dataSize = gEnvelopeData.size();
        for (size_t i = 0; i < dataSize; i++) {
            if (gEnvelopeData[i] > envelopeMax) {
                envelopeMax = gEnvelopeData[i];
                maxIndex = i;
            }
        }

        // Spawn a blood cell if the smoothed envelope peak exceeds the adjustable threshold.
        if (dataSize > 0 && envelopeMax > amplitudeThreshold && spawnCooldown <= 0.0f) {
            float waveformHeight = SPACEBAR_HEIGHT * 0.8f;
            float keycapY = (height - SPACEBAR_HEIGHT) / 2.0f;
            // Always spawn at the very right side.
            BloodCell newCell;
            newCell.x = width;
            newCell.t = 0.0f;
            // Bake the y position from the envelope at the right edge.
            float envVal = gEnvelopeData[dataSize - 1]; // envelope value at far right
            newCell.y = keycapY + SPACEBAR_HEIGHT - envVal * waveformHeight;
            bloodCells.push_back(newCell);
            spawnCooldown = spawnRate;
        }

        // Clear the background (#200000).
        glClearColor(0.125f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glfwGetWindowSize(window, &width, &height);

        // Draw the keycap and envelope line.
        drawSpacebarKeycap(width, height);

        // Draw the burning indicator (a circle at the top middle).
        {
            float radius = 20.0f;
            float centerX = width / 2.0f;
            float centerY = 50.0f;
            glColor3f(0.25f, 0.0f, 0.0f);
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

        // Update and draw blood cells (with fixed y positions).
        updateAndDrawBloodCells(width, height, gEnvelopeData, deltaTime);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    jack_deactivate(jackClient);
    jack_client_close(jackClient);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
