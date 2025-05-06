// advanced_audio_visualizer_debug.cpp
//
// Compile with (example):
//   g++ advanced_audio_visualizer_debug.cpp -o advanced_audio_visualizer_debug -ljack -lGL -lglfw -ldl
//
// This file combines a real-time JACK input audio visualizer (using OpenGL/GLFW)
// with advanced processing features inspired by your Python v4 script:
//   • Virtual gain (VGAIN)
//   • A simple VEQ filter (switchable between none, lowpass, highpass)
//   • An echo/reverb effect (with adjustable delay and reverb parameters)
//   • A virtual oscillator (VOSC)
//   • A virtual gate (noise gate)
// 
// Additionally, a simple debug overlay is provided (using stb_easy_font)
// to display current parameter values. stb_easy_font is embedded below so no extra package is needed.

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include <cstring>   // memset, etc.
#include <cstdlib>
#include <algorithm> // std::clamp
#include <string>

// JACK headers
#include <jack/jack.h>
#include <jack/ringbuffer.h>

// GLAD MUST be included BEFORE GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM for math
#include <glm/glm.hpp>

// --------------------------
// Debug text rendering with stb_easy_font
// --------------------------
// This is a public-domain single-header library. Just drop it in your project.
// (No extra package installation is required.)
#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"  // Ensure this header is in your project directory

// --------------------------
// Configuration parameters
// --------------------------
const int SAMPLE_RATE = 44100;  // Hz
const int CHANNELS = 1;         // Mono
const int BLOCK_SIZE = 1024;    // Suggested JACK block size
const int FPS = 60;             // Frames per second for visualization
const int STRIP_WIDTH = 2;      // Pixel width for each color block
const int WINDOW_WIDTH = 1920;  // Window resolution width
const int WINDOW_HEIGHT = 1080; // Window resolution height

// --------------------------
// Advanced processing defaults
// --------------------------

// Virtual Gain (VGain) – original Python default was 96.0
float g_vgain = 96.0f;

// VEQ (filter) settings
enum class FilterType { None, Lowpass, Highpass };
FilterType g_filterType = FilterType::None;
float g_cutoffFreq = 5000.0f; // Hz

// Echo settings
bool  g_echoEnabled = false;
float g_echoDelayTime = 1.0f;   // seconds
float g_echoDelayDryWet = 0.5f; // mix ratio (0=dry, 1=wet)
float g_echoDelayFeedback = 0.5f;   // feedback factor
float g_echoDelayGain = 1.0f;   // gain for delayed signal (linear scale)
float g_reverbTime = 2.0f;      // seconds
float g_reverbDryWet = 0.5f;
float g_reverbGain = 1.0f;

// VOSC (virtual oscillator) settings
bool  g_voscPlaying = false;
float g_voscFreq = 60.0f;   // Hz
float g_voscAmp = 40.0f;    // amplitude

// Virtual Gate settings
bool  g_gateEnabled = false;
float g_gateThreshold = 0.1f;  // Normalized threshold (in range [0,1])

// --------------------------
// Global variables for JACK
// --------------------------
jack_client_t* jackClient = nullptr;
jack_port_t* inputPort = nullptr;
jack_ringbuffer_t* ringBuffer = nullptr;
const size_t RINGBUFFER_SIZE = 65536 * sizeof(float); // in bytes

// --------------------------
// Debug overlay flag and toggle timing
// --------------------------
bool debugMenuEnabled = false;
double lastDebugToggleTime = 0.0;

// --------------------------
// HSV to RGB conversion
// Returns a glm::vec3 with channels in [0,1]
glm::vec3 hsvToRgb(float h, float s, float v) {
    float r, g, b;
    int i = static_cast<int>(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    switch (i % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    case 5: r = v; g = p; b = q; break;
    default: r = g = b = 0; break;
    }
    return glm::vec3(r, g, b);
}

// Map a processed audio sample (assumed in [-1,1]) to an RGB color.
glm::vec3 sampleToColor(float sample) {
    sample = std::clamp(sample, -1.0f, 1.0f);
    float normalized = (sample + 1.0f) / 2.0f;
    normalized = std::pow(normalized, 3.0f);
    float hue = normalized;
    return hsvToRgb(hue, 1.0f, 1.0f);
}

// --------------------------
// JACK process callback
// --------------------------
int jackProcessCallback(jack_nframes_t nframes, void* arg) {
    float* in = static_cast<float*>(jack_port_get_buffer(inputPort, nframes));
    size_t bytesToWrite = nframes * sizeof(float);
    if (jack_ringbuffer_write_space(ringBuffer) < bytesToWrite)
        return 0; // drop samples if ringbuffer is full
    jack_ringbuffer_write(ringBuffer, reinterpret_cast<char*>(in), bytesToWrite);
    return 0;
}

// --------------------------
// Initialize JACK audio
// --------------------------
bool initJACK() {
    const char* clientName = "AdvancedAudioVisualizer";
    jack_options_t options = JackNullOption;
    jack_status_t status;
    jackClient = jack_client_open(clientName, options, &status, nullptr);
    if (jackClient == nullptr) {
        std::cerr << "Failed to open JACK client. Status: " << status << std::endl;
        return false;
    }
    inputPort = jack_port_register(jackClient, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    if (inputPort == nullptr) {
        std::cerr << "No available JACK ports." << std::endl;
        return false;
    }
    ringBuffer = jack_ringbuffer_create(RINGBUFFER_SIZE);
    if (!ringBuffer) {
        std::cerr << "Failed to create ring buffer." << std::endl;
        return false;
    }
    jack_ringbuffer_reset(ringBuffer);
    if (jack_set_process_callback(jackClient, jackProcessCallback, nullptr) != 0) {
        std::cerr << "Failed to set JACK process callback." << std::endl;
        return false;
    }
    if (jack_activate(jackClient)) {
        std::cerr << "Cannot activate JACK client." << std::endl;
        return false;
    }
    std::cout << "JACK client activated." << std::endl;
    return true;
}

// --------------------------
// Cleanup JACK
// --------------------------
void cleanupJACK() {
    if (jackClient) {
        jack_deactivate(jackClient);
        jack_client_close(jackClient);
        jackClient = nullptr;
    }
    if (ringBuffer) {
        jack_ringbuffer_free(ringBuffer);
        ringBuffer = nullptr;
    }
}

// --------------------------
// Audio Processing Classes
// --------------------------

// Simple one-pole lowpass filter
class LowpassFilter {
public:
    LowpassFilter(float cutoff, float sampleRate) : prevOutput(0.0f) {
        setCutoff(cutoff, sampleRate);
    }
    void setCutoff(float cutoff, float sampleRate) {
        alpha = 1.0f - std::exp(-2.0f * 3.14159265f * cutoff / sampleRate);
    }
    float process(float input) {
        float output = alpha * input + (1.0f - alpha) * prevOutput;
        prevOutput = output;
        return output;
    }
private:
    float alpha;
    float prevOutput;
};

// Simple one-pole highpass filter
class HighpassFilter {
public:
    HighpassFilter(float cutoff, float sampleRate) : prevInput(0.0f), prevOutput(0.0f) {
        setCutoff(cutoff, sampleRate);
    }
    void setCutoff(float cutoff, float sampleRate) {
        alpha = std::exp(-2.0f * 3.14159265f * cutoff / sampleRate);
    }
    float process(float input) {
        float output = alpha * (prevOutput + input - prevInput);
        prevInput = input;
        prevOutput = output;
        return output;
    }
private:
    float alpha;
    float prevInput;
    float prevOutput;
};

// Echo (delay + reverb) processor
class Echo {
public:
    Echo(float delayTime, float sampleRate) : delayIndex(0), reverbIndex(0) {
        setDelayTime(delayTime, sampleRate);
        setReverbTime(g_reverbTime, sampleRate);
        delayDryWet = g_echoDelayDryWet;
        delayFeedback = g_echoDelayFeedback;
        delayGain = g_echoDelayGain;
        reverbDryWet = g_reverbDryWet;
        reverbGain = g_reverbGain;
    }
    void setDelayTime(float delayTime, float sampleRate) {
        size_t size = static_cast<size_t>(delayTime * sampleRate);
        delayBuffer.assign(size, 0.0f);
        delayIndex = 0;
    }
    void setReverbTime(float reverbTime, float sampleRate) {
        size_t size = static_cast<size_t>(reverbTime * sampleRate);
        reverbBuffer.assign(size, 0.0f);
        reverbIndex = 0;
    }
    float process(float input) {
        float delayedSample = delayBuffer[delayIndex];
        float newDelaySample = input * delayGain + delayedSample * delayFeedback;
        delayBuffer[delayIndex] = newDelaySample;
        delayIndex = (delayIndex + 1) % delayBuffer.size();
        float delayOutput = (1.0f - delayDryWet) * input + delayDryWet * delayedSample;

        float reverbedSample = reverbBuffer[reverbIndex];
        float newReverbSample = delayOutput * reverbGain + reverbedSample * delayFeedback;
        reverbBuffer[reverbIndex] = newReverbSample;
        reverbIndex = (reverbIndex + 1) % reverbBuffer.size();
        float reverbOutput = (1.0f - reverbDryWet) * delayOutput + reverbDryWet * reverbedSample;

        return reverbOutput;
    }
    float delayDryWet, delayFeedback, delayGain;
    float reverbDryWet, reverbGain;
private:
    std::vector<float> delayBuffer;
    std::vector<float> reverbBuffer;
    size_t delayIndex;
    size_t reverbIndex;
};

// Virtual Oscillator (VOSC) – sine wave generator
class Vosc {
public:
    Vosc(float freq, float amp, float sampleRate)
        : frequency(freq), amplitude(amp), sampleRate(sampleRate), phase(0.0f), playing(false) {
    }
    void togglePlay() {
        playing = !playing;
        std::cout << "VOSC " << (playing ? "Playing" : "Stopped") << std::endl;
    }
    void setFrequency(float freq) {
        frequency = freq;
        std::cout << "VOSC Frequency set to: " << frequency << " Hz" << std::endl;
    }
    void setAmplitude(float amp) {
        amplitude = amp;
        std::cout << "VOSC Amplitude set to: " << amplitude << std::endl;
    }
    float process() {
        if (!playing) return 0.0f;
        float value = amplitude * std::sin(phase);
        phase += 2.0f * 3.14159265f * frequency / sampleRate;
        if (phase > 2.0f * 3.14159265f)
            phase -= 2.0f * 3.14159265f;
        return value;
    }
private:
    float frequency;
    float amplitude;
    float sampleRate;
    float phase;
    bool playing;
};

// --------------------------
// Debug Menu Drawing Function
// --------------------------
void drawDebugMenu() {
    std::string debugText;
    debugText += "DEBUG MENU\n";
    debugText += "VGAIN: " + std::to_string(g_vgain) + "\n";
    debugText += "Filter: ";
    if (g_filterType == FilterType::None)
        debugText += "None\n";
    else if (g_filterType == FilterType::Lowpass)
        debugText += "Lowpass\n";
    else if (g_filterType == FilterType::Highpass)
        debugText += "Highpass\n";
    debugText += "Cutoff Freq: " + std::to_string(g_cutoffFreq) + " Hz\n";
    debugText += "Echo: " + std::string(g_echoEnabled ? "Enabled" : "Disabled") + "\n";
    debugText += "Echo Delay Time: " + std::to_string(g_echoDelayTime) + " s\n";
    debugText += "Echo Delay Dry/Wet: " + std::to_string(g_echoDelayDryWet) + "\n";
    debugText += "Echo Delay Feedback: " + std::to_string(g_echoDelayFeedback) + "\n";
    debugText += "Echo Delay Gain: " + std::to_string(g_echoDelayGain) + "\n";
    debugText += "Reverb Time: " + std::to_string(g_reverbTime) + " s\n";
    debugText += "Reverb Dry/Wet: " + std::to_string(g_reverbDryWet) + "\n";
    debugText += "Reverb Gain: " + std::to_string(g_reverbGain) + "\n";
    debugText += "VOSC: " + std::string(g_voscPlaying ? "Playing" : "Stopped") + "\n";
    debugText += "VOSC Freq: " + std::to_string(g_voscFreq) + " Hz\n";
    debugText += "VOSC Amp: " + std::to_string(g_voscAmp) + "\n";
    debugText += "Gate: " + std::string(g_gateEnabled ? "Enabled" : "Disabled") + "\n";
    debugText += "Gate Threshold: " + std::to_string(g_gateThreshold) + "\n";

    char buffer[99999];
    int num_quads = stb_easy_font_print(10.0f, 10.0f, const_cast<char*>(debugText.c_str()), NULL, buffer, sizeof(buffer));
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glColor3f(1.0f, 1.0f, 1.0f); // White text
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// --------------------------
// Main function
// --------------------------
int main() {
    if (!initJACK()) {
        return EXIT_FAILURE;
    }

    // Auto-connect system (desktop) audio ports.
    const char** systemOutPorts = jack_get_ports(jackClient, "system", nullptr, JackPortIsOutput);
    if (systemOutPorts) {
        for (int i = 0; systemOutPorts[i] != nullptr; i++) {
            if (jack_connect(jackClient, systemOutPorts[i], jack_port_name(inputPort)) != 0)
                std::cerr << "Failed to connect system port: " << systemOutPorts[i] << std::endl;
            else
                std::cout << "Connected to system port: " << systemOutPorts[i] << std::endl;
        }
        jack_free(systemOutPorts);
    }
    else {
        std::cerr << "No system playback ports found. Connect desktop audio manually." << std::endl;
    }

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        cleanupJACK();
        return EXIT_FAILURE;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Advanced Real-Time Audio Visualizer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        cleanupJACK();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD." << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        cleanupJACK();
        return EXIT_FAILURE;
    }
    glfwSwapInterval(1); // Enable vsync

    int maxBlocks = WINDOW_WIDTH / STRIP_WIDTH;
    std::vector<glm::vec3> colorStrip;

    LowpassFilter* lowpassFilter = nullptr;
    HighpassFilter* highpassFilter = nullptr;
    Echo echo(g_echoDelayTime, SAMPLE_RATE);
    echo.setReverbTime(g_reverbTime, SAMPLE_RATE);
    Vosc vosc(g_voscFreq, g_voscAmp, SAMPLE_RATE);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // --- Keyboard Controls ---
        // VGAIN: Q increases, A decreases.
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            g_vgain *= 1.01f;
            std::cout << "VGAIN: " << g_vgain << std::endl;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            g_vgain /= 1.01f;
            std::cout << "VGAIN: " << g_vgain << std::endl;
        }
        // Cycle filter type with F: None -> Lowpass -> Highpass -> None.
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            if (g_filterType == FilterType::None) {
                g_filterType = FilterType::Lowpass;
                if (lowpassFilter) { delete lowpassFilter; lowpassFilter = nullptr; }
                lowpassFilter = new LowpassFilter(g_cutoffFreq, SAMPLE_RATE);
                if (highpassFilter) { delete highpassFilter; highpassFilter = nullptr; }
                std::cout << "Filter: Lowpass" << std::endl;
            }
            else if (g_filterType == FilterType::Lowpass) {
                g_filterType = FilterType::Highpass;
                if (highpassFilter) { delete highpassFilter; highpassFilter = nullptr; }
                highpassFilter = new HighpassFilter(g_cutoffFreq, SAMPLE_RATE);
                if (lowpassFilter) { delete lowpassFilter; lowpassFilter = nullptr; }
                std::cout << "Filter: Highpass" << std::endl;
            }
            else if (g_filterType == FilterType::Highpass) {
                g_filterType = FilterType::None;
                if (lowpassFilter) { delete lowpassFilter; lowpassFilter = nullptr; }
                if (highpassFilter) { delete highpassFilter; highpassFilter = nullptr; }
                std::cout << "Filter: None" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        // Adjust cutoff frequency: W increases, S decreases.
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            g_cutoffFreq *= 1.01f;
            std::cout << "Cutoff Frequency: " << g_cutoffFreq << " Hz" << std::endl;
            if (g_filterType == FilterType::Lowpass && lowpassFilter)
                lowpassFilter->setCutoff(g_cutoffFreq, SAMPLE_RATE);
            else if (g_filterType == FilterType::Highpass && highpassFilter)
                highpassFilter->setCutoff(g_cutoffFreq, SAMPLE_RATE);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            g_cutoffFreq /= 1.01f;
            std::cout << "Cutoff Frequency: " << g_cutoffFreq << " Hz" << std::endl;
            if (g_filterType == FilterType::Lowpass && lowpassFilter)
                lowpassFilter->setCutoff(g_cutoffFreq, SAMPLE_RATE);
            else if (g_filterType == FilterType::Highpass && highpassFilter)
                highpassFilter->setCutoff(g_cutoffFreq, SAMPLE_RATE);
        }
        // Toggle echo with E.
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            g_echoEnabled = !g_echoEnabled;
            std::cout << "Echo Enabled: " << (g_echoEnabled ? "Yes" : "No") << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        // Toggle VOSC with V.
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
            vosc.togglePlay();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        // Adjust VOSC frequency: R increases, T decreases.
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            g_voscFreq *= 1.01f;
            vosc.setFrequency(g_voscFreq);
        }
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
            g_voscFreq /= 1.01f;
            vosc.setFrequency(g_voscFreq);
        }
        // Adjust VOSC amplitude: Y increases, U decreases.
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
            g_voscAmp *= 1.01f;
            vosc.setAmplitude(g_voscAmp);
        }
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
            g_voscAmp /= 1.01f;
            vosc.setAmplitude(g_voscAmp);
        }
        // --- Virtual Gate Controls ---
        // Toggle virtual gate with G.
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
            g_gateEnabled = !g_gateEnabled;
            std::cout << "Gate " << (g_gateEnabled ? "Enabled" : "Disabled") << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        // Increase gate threshold with H.
        if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
            g_gateThreshold *= 1.01f;
            std::cout << "Gate Threshold: " << g_gateThreshold << std::endl;
        }
        // Decrease gate threshold with N.
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
            g_gateThreshold /= 1.01f;
            std::cout << "Gate Threshold: " << g_gateThreshold << std::endl;
        }
        // Toggle debug menu with D (with simple debounce)
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            double t = glfwGetTime();
            if (t - lastDebugToggleTime > 0.3) {
                debugMenuEnabled = !debugMenuEnabled;
                lastDebugToggleTime = t;
            }
        }

        // --- Echo Parameter Controls (F2-F12, KP_ADD/KP_SUBTRACT) ---
        if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) {
            g_echoDelayTime += 0.1f;
            if (g_echoDelayTime > 30.0f) g_echoDelayTime = 30.0f;
            echo.setDelayTime(g_echoDelayTime, SAMPLE_RATE);
            std::cout << "Echo Delay Time: " << g_echoDelayTime << " s" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) {
            g_echoDelayTime -= 0.1f;
            if (g_echoDelayTime < 0.1f) g_echoDelayTime = 0.1f;
            echo.setDelayTime(g_echoDelayTime, SAMPLE_RATE);
            std::cout << "Echo Delay Time: " << g_echoDelayTime << " s" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS) {
            g_echoDelayDryWet += 0.01f;
            if (g_echoDelayDryWet > 1.0f) g_echoDelayDryWet = 1.0f;
            echo.delayDryWet = g_echoDelayDryWet;
            std::cout << "Echo Delay Dry/Wet: " << g_echoDelayDryWet << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS) {
            g_echoDelayDryWet -= 0.01f;
            if (g_echoDelayDryWet < 0.0f) g_echoDelayDryWet = 0.0f;
            echo.delayDryWet = g_echoDelayDryWet;
            std::cout << "Echo Delay Dry/Wet: " << g_echoDelayDryWet << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (glfwGetKey(window, GLFW_KEY_F6) == GLFW_PRESS) {
            g_echoDelayFeedback += 0.1f;
            if (g_echoDelayFeedback > 10.0f) g_echoDelayFeedback = 10.0f;
            echo.delayFeedback = g_echoDelayFeedback;
            std::cout << "Echo Delay Feedback: " << g_echoDelayFeedback << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (glfwGetKey(window, GLFW_KEY_F7) == GLFW_PRESS) {
            g_echoDelayFeedback -= 0.1f;
            if (g_echoDelayFeedback < 0.0f) g_echoDelayFeedback = 0.0f;
            echo.delayFeedback = g_echoDelayFeedback;
            std::cout << "Echo Delay Feedback: " << g_echoDelayFeedback << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (glfwGetKey(window, GLFW_KEY_F8) == GLFW_PRESS) {
            g_echoDelayGain += 0.1f;
            if (g_echoDelayGain > 24.0f) g_echoDelayGain = 24.0f;
            echo.delayGain = g_echoDelayGain;
            std::cout << "Echo Delay Gain: " << g_echoDelayGain << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (glfwGetKey(window, GLFW_KEY_F9) == GLFW_PRESS) {
            g_echoDelayGain -= 0.1f;
            if (g_echoDelayGain < 0.0f) g_echoDelayGain = 0.0f;
            echo.delayGain = g_echoDelayGain;
            std::cout << "Echo Delay Gain: " << g_echoDelayGain << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (glfwGetKey(window, GLFW_KEY_F10) == GLFW_PRESS) {
            g_reverbTime += 0.1f;
            if (g_reverbTime > 30.0f) g_reverbTime = 30.0f;
            echo.setReverbTime(g_reverbTime, SAMPLE_RATE);
            std::cout << "Reverb Time: " << g_reverbTime << " s" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
            g_reverbTime -= 0.1f;
            if (g_reverbTime < 0.1f) g_reverbTime = 0.1f;
            echo.setReverbTime(g_reverbTime, SAMPLE_RATE);
            std::cout << "Reverb Time: " << g_reverbTime << " s" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (glfwGetKey(window, GLFW_KEY_F12) == GLFW_PRESS) {
            g_reverbDryWet += 0.01f;
            if (g_reverbDryWet > 1.0f) g_reverbDryWet = 1.0f;
            echo.reverbDryWet = g_reverbDryWet;
            std::cout << "Reverb Dry/Wet: " << g_reverbDryWet << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) {
            g_reverbGain += 0.1f;
            if (g_reverbGain > 24.0f) g_reverbGain = 24.0f;
            echo.reverbGain = g_reverbGain;
            std::cout << "Reverb Gain: " << g_reverbGain << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {
            g_reverbGain -= 0.1f;
            if (g_reverbGain < 0.0f) g_reverbGain = 0.0f;
            echo.reverbGain = g_reverbGain;
            std::cout << "Reverb Gain: " << g_reverbGain << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // --- Process audio samples from JACK ring buffer ---
        size_t availableBytes = jack_ringbuffer_read_space(ringBuffer);
        int numSamples = availableBytes / sizeof(float);
        if (numSamples > 0) {
            std::vector<float> samples(numSamples);
            jack_ringbuffer_read(ringBuffer, reinterpret_cast<char*>(samples.data()), numSamples * sizeof(float));
            for (float sample : samples) {
                float processed = sample * 32767.0f;
                processed += vosc.process() * 32767.0f;
                processed *= g_vgain;
                if (g_filterType == FilterType::Lowpass && lowpassFilter)
                    processed = lowpassFilter->process(processed);
                else if (g_filterType == FilterType::Highpass && highpassFilter)
                    processed = highpassFilter->process(processed);
                if (g_echoEnabled)
                    processed = echo.process(processed);
                processed = std::clamp(processed, -32767.0f, 32767.0f);

                // Normalize for visualization
                float displaySample = processed / 32767.0f;

                // Apply virtual gate: if enabled and sample amplitude is below threshold, zero it.
                if (g_gateEnabled && std::abs(displaySample) < g_gateThreshold) {
                    displaySample = 0.0f;
                }

                glm::vec3 color = sampleToColor(displaySample);
                colorStrip.push_back(color);
                if (colorStrip.size() > static_cast<size_t>(maxBlocks))
                    colorStrip.erase(colorStrip.begin());
            }
        }

        // --- Render visualization ---
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glBegin(GL_QUADS);
        for (size_t i = 0; i < colorStrip.size(); ++i) {
            glm::vec3 col = colorStrip[i];
            int x = i * STRIP_WIDTH;
            glColor3f(col.r, col.g, col.b);
            glVertex2i(x, 0);
            glVertex2i(x + STRIP_WIDTH, 0);
            glVertex2i(x + STRIP_WIDTH, WINDOW_HEIGHT);
            glVertex2i(x, WINDOW_HEIGHT);
        }
        glEnd();

        // --- Draw Debug Menu if enabled ---
        if (debugMenuEnabled) {
            drawDebugMenu();
        }

        glfwSwapBuffers(window);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FPS));
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    cleanupJACK();
    if (lowpassFilter) { delete lowpassFilter; lowpassFilter = nullptr; }
    if (highpassFilter) { delete highpassFilter; highpassFilter = nullptr; }

    return EXIT_SUCCESS;
}
