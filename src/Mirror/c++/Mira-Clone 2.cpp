// main.cpp
#include <iostream>
#include <vector>
#include <cmath>
#include <mutex>
#include <sstream>
#include <cstring>
#include <chrono>
#include <thread>

// JACK for audio
#include <jack/jack.h>

// GLFW and OpenGL for graphics
#include <GLFW/glfw3.h>
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
#endif

// GLM for math
#include <glm/glm.hpp>

// stb_easy_font for text rendering
#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

// ------------------------
// Custom to_string replacement
// ------------------------
template <typename T>
std::string my_to_string(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// ------------------------
// Configuration Parameters
// ------------------------
const int SAMPLE_RATE = 36669;       // 36,669 Hz
const int CHUNK = 666;               // Number of samples per frame

// Audio parameters
float VGAIN = 96.0f;
const float VGAIN_MIN = 0.0f;
const float VGAIN_MAX = 960.0f;

// VEQ parameters
std::string FILTER_TYPE = "None";    // "None", "Lowpass", "Highpass"
float CUTOFF_FREQ = 5000.0f;           // in Hz
const int FILTER_ORDER = 5;

// Echo parameters
bool ECHO_ENABLED = false;
float ECHO_DELAY_TIME = 1.0f;        // seconds
float ECHO_DELAY_DRY_WET = 0.5f;
float ECHO_DELAY_FEEDBACK = 0.5f;
float ECHO_DELAY_GAIN = 1.0f;        // dB

float ECHO_REVERB_TIME = 2.0f;       // seconds
float ECHO_REVERB_DRY_WET = 0.5f;
float ECHO_REVERB_GAIN = 1.0f;        // dB

// VOSC parameters
float VOSC_DEFAULT_FREQ = 60.0f;
float VOSC_DEFAULT_AMP = 40.0f;
const float VOSC_MAX_FREQ = 40000.0f;
const float VOSC_MAX_AMP = 96.0f;

// Visualization
int WINDOW_WIDTH = 800;
int WINDOW_HEIGHT = 600;
bool showInfo = false;

// Global audio output buffer (for visualization)
std::vector<int16_t> g_audioBuffer(CHUNK, 0);
std::mutex audioMutex;

// ------------------------
// Utility: Color Structure
// ------------------------
struct Color {
    unsigned char r, g, b;
};

Color sampleToColor(float sample, float vgain, float max_val = 32767.0f) {
    float adjusted = sample * vgain;
    if (adjusted > max_val) adjusted = max_val;
    if (adjusted < -max_val) adjusted = -max_val;
    float normalized = (adjusted + max_val) / (2 * max_val);
    int red   = std::min(int(normalized * 255.0f), 255);
    int blue  = std::min(int((1.0f - normalized) * 255.0f), 255);
    int green = std::min(int(std::fabs(normalized - 0.5f) * 510.0f), 255);
    return { static_cast<unsigned char>(red), static_cast<unsigned char>(green), static_cast<unsigned char>(blue) };
}

// ------------------------
// VOSC Class: Virtual Oscillator
// ------------------------
class VOSC {
public:
    float frequency;
    float amplitude;
    float phase;
    bool isPlaying;
    
    VOSC(float freq = VOSC_DEFAULT_FREQ, float amp = VOSC_DEFAULT_AMP)
        : frequency(freq), amplitude(amp), phase(0.0f), isPlaying(false) {}

    void togglePlay() {
        isPlaying = !isPlaying;
        std::cout << "VOSC " << (isPlaying ? "Playing" : "Stopped") << std::endl;
    }

    void setFrequency(float freq) {
        frequency = freq;
        std::cout << "VOSC Frequency set to: " << frequency << " Hz" << std::endl;
    }

    void setAmplitude(float amp) {
        amplitude = amp;
        std::cout << "VOSC Amplitude set to: " << amplitude << std::endl;
    }

    // Generates a sine wave buffer of length CHUNK.
    std::vector<float> generateWave() {
        std::vector<float> wave(CHUNK, 0.0f);
        if (!isPlaying || amplitude == 0.0f)
            return wave;
        float phaseIncrement = 2.0f * 3.14159265f * frequency / SAMPLE_RATE;
        for (int i = 0; i < CHUNK; ++i) {
            wave[i] = amplitude * std::sin(phase + phaseIncrement * i);
        }
        phase += phaseIncrement * CHUNK;
        phase = std::fmod(phase, 2.0f * 3.14159265f);
        return wave;
    }
};

// ------------------------
// Butterworth Filter (IIR) Implementation
// ------------------------
struct Filter {
    std::vector<float> a;
    std::vector<float> b;
    std::vector<float> x_history;
    std::vector<float> y_history;
    int order;

    Filter() : order(0) {}
    
    void reset() {
        x_history.assign(order, 0.0f);
        y_history.assign(order, 0.0f);
    }

    // Apply filter to a buffer (in place)
    void apply(std::vector<float>& data) {
        for (size_t n = 0; n < data.size(); n++) {
            float input = data[n];
            float output = b[0] * input;
            for (int i = 1; i < order + 1; ++i) {
                output += b[i] * (i <= (int)x_history.size() ? x_history[i-1] : 0.0f)
                          - a[i] * (i <= (int)y_history.size() ? y_history[i-1] : 0.0f);
            }
            if(order > 0) {
                for (int i = order - 1; i > 0; --i) {
                    x_history[i] = x_history[i-1];
                    y_history[i] = y_history[i-1];
                }
                x_history[0] = input;
                y_history[0] = output;
            }
            data[n] = output;
        }
    }
};

void designLowpass(Filter &filter, float cutoff, int order, int sampleRate) {
    filter.order = order;
    filter.a.resize(order+1);
    filter.b.resize(order+1);
    filter.x_history.assign(order, 0.0f);
    filter.y_history.assign(order, 0.0f);
    // Placeholder single-pole RC lowpass filter approximation:
    float RC = 1.0f / (2.0f * 3.14159265f * cutoff);
    float dt = 1.0f / sampleRate;
    float alpha = dt / (RC + dt);
    filter.b[0] = alpha;
    filter.b[1] = 0.0f;
    filter.a[0] = 1.0f;
    filter.a[1] = alpha - 1.0f;
}

void designHighpass(Filter &filter, float cutoff, int order, int sampleRate) {
    filter.order = order;
    filter.a.resize(order+1);
    filter.b.resize(order+1);
    filter.x_history.assign(order, 0.0f);
    filter.y_history.assign(order, 0.0f);
    // Simplified single-pole RC highpass filter approximation:
    float RC = 1.0f / (2.0f * 3.14159265f * cutoff);
    float dt = 1.0f / sampleRate;
    float alpha = RC / (RC + dt);
    filter.b[0] = alpha;
    filter.b[1] = -alpha;
    filter.a[0] = 1.0f;
    filter.a[1] = alpha - 1.0f;
}

// Global filter instance (for VEQ)
Filter g_filter;

// ------------------------
// Echo Class
// ------------------------
class Echo {
public:
    float delay_time;
    float delay_dry_wet;
    float delay_feedback;
    float delay_gain;
    std::vector<float> delay_buffer;
    size_t delay_index;

    float reverb_time;
    float reverb_dry_wet;
    float reverb_gain;
    std::vector<float> reverb_buffer;
    size_t reverb_index;

    Echo(float d_time, float d_drywet, float d_feedback, float d_gain,
         float r_time, float r_drywet, float r_gain)
         : delay_time(d_time), delay_dry_wet(d_drywet),
           delay_feedback(d_feedback), delay_gain(d_gain),
           delay_index(0),
           reverb_time(r_time), reverb_dry_wet(r_drywet), reverb_gain(r_gain),
           reverb_index(0)
    {
        delay_buffer.resize(static_cast<size_t>(delay_time * SAMPLE_RATE), 0.0f);
        reverb_buffer.resize(static_cast<size_t>(reverb_time * SAMPLE_RATE), 0.0f);
    }

    void updateParameters(float d_time, float d_drywet, float d_feedback, float d_gain,
                          float r_time, float r_drywet, float r_gain) {
        if (d_time != delay_time) {
            delay_time = d_time;
            delay_buffer.assign(static_cast<size_t>(delay_time * SAMPLE_RATE), 0.0f);
            delay_index = 0;
        }
        delay_dry_wet = d_drywet;
        delay_feedback = d_feedback;
        delay_gain = d_gain;
        if (r_time != reverb_time) {
            reverb_time = r_time;
            reverb_buffer.assign(static_cast<size_t>(reverb_time * SAMPLE_RATE), 0.0f);
            reverb_index = 0;
        }
        reverb_dry_wet = r_drywet;
        reverb_gain = r_gain;
    }

    std::vector<float> process(const std::vector<float>& samples) {
        std::vector<float> output(samples.size(), 0.0f);
        std::vector<float> delay_out(samples.size(), 0.0f);
        for (size_t i = 0; i < samples.size(); ++i) {
            float delayed = delay_buffer[delay_index];
            delay_out[i] = delayed;
            delay_buffer[delay_index] = samples[i] * delay_gain + delayed * delay_feedback;
            delay_index = (delay_index + 1) % delay_buffer.size();
        }
        std::vector<float> mixed(samples.size(), 0.0f);
        for (size_t i = 0; i < samples.size(); ++i) {
            mixed[i] = (1.0f - delay_dry_wet) * samples[i] + delay_dry_wet * delay_out[i];
        }
        std::vector<float> reverb_out(samples.size(), 0.0f);
        for (size_t i = 0; i < samples.size(); ++i) {
            float reverbed = reverb_buffer[reverb_index];
            reverb_out[i] = reverbed;
            reverb_buffer[reverb_index] = mixed[i] * reverb_gain + reverbed * delay_feedback;
            reverb_index = (reverb_index + 1) % reverb_buffer.size();
        }
        for (size_t i = 0; i < samples.size(); ++i) {
            output[i] = (1.0f - reverb_dry_wet) * mixed[i] + reverb_dry_wet * reverb_out[i];
        }
        return output;
    }
};

// ------------------------
// Global Instances
// ------------------------
VOSC g_vosc;
Echo g_echo(ECHO_DELAY_TIME, ECHO_DELAY_DRY_WET, ECHO_DELAY_FEEDBACK, ECHO_DELAY_GAIN,
           ECHO_REVERB_TIME, ECHO_REVERB_DRY_WET, ECHO_REVERB_GAIN);

// ------------------------
// JACK Audio Callback
// ------------------------
jack_client_t* client = nullptr;

int processAudio(jack_nframes_t nframes, void* arg) {
    jack_default_audio_sample_t* in = (jack_default_audio_sample_t*) jack_port_get_buffer((jack_port_t*) arg, nframes);
    std::vector<float> samples(nframes, 0.0f);
    for (jack_nframes_t i = 0; i < nframes && i < (unsigned)CHUNK; i++) {
        samples[i] = static_cast<float>(in[i]);
    }
    if (nframes < CHUNK)
        samples.resize(CHUNK, 0.0f);

    std::vector<float> voscWave = g_vosc.generateWave();
    for (int i = 0; i < CHUNK; i++) {
        samples[i] += voscWave[i] * 32767.0f;
    }
    for (int i = 0; i < CHUNK; i++) {
        samples[i] *= VGAIN;
    }
    if (FILTER_TYPE != "None") {
        if (FILTER_TYPE == "Lowpass") {
            designLowpass(g_filter, CUTOFF_FREQ, 1, SAMPLE_RATE);
        } else if (FILTER_TYPE == "Highpass") {
            designHighpass(g_filter, CUTOFF_FREQ, 1, SAMPLE_RATE);
        }
        g_filter.apply(samples);
    }
    if (ECHO_ENABLED) {
        std::vector<float> echoOut = g_echo.process(samples);
        float echoGainLinear = std::pow(10.0f, ECHO_DELAY_GAIN / 20.0f);
        for (int i = 0; i < CHUNK; i++) {
            samples[i] += echoOut[i] * echoGainLinear;
        }
    }
    std::vector<int16_t> outSamples(CHUNK, 0);
    for (int i = 0; i < CHUNK; i++) {
        float sample = std::max(-32767.0f, std::min(samples[i], 32767.0f));
        outSamples[i] = static_cast<int16_t>(sample);
    }
    {
        std::lock_guard<std::mutex> lock(audioMutex);
        g_audioBuffer = outSamples;
    }
    return 0;
}

// ------------------------
// JACK Shutdown Callback
// ------------------------
void jackShutdown(void* arg) {
    std::cerr << "JACK server shutdown!" << std::endl;
    exit(1);
}

// ------------------------
// Helper: Adjust Logarithmically
// ------------------------
float adjustLogarithmic(float value, const std::string& direction, float min_value, float max_value, float factor = 1.1f) {
    if (direction == "up") {
        float newValue = value * factor;
        return (newValue > max_value ? max_value : newValue);
    } else if (direction == "down") {
        float newValue = value / factor;
        return (newValue < min_value ? min_value : newValue);
    }
    return value;
}

// ------------------------
// GLFW Key Callback
// ------------------------
enum EffectType { VGAIN_EFFECT, VEQ_EFFECT, ECHO_EFFECT, VOSC_EFFECT };
EffectType currentEffect = VGAIN_EFFECT;
int currentParamIndex = 0;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    static auto lastAdjustTime = std::chrono::steady_clock::now();
    const int adjustDelayMs = 100;
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    if (key == GLFW_KEY_F1) {
        showInfo = !showInfo;
    } else if (key == GLFW_KEY_LEFT) {
        currentEffect = static_cast<EffectType>((currentEffect + 3) % 4);
        currentParamIndex = 0;
    } else if (key == GLFW_KEY_RIGHT) {
        currentEffect = static_cast<EffectType>((currentEffect + 1) % 4);
        currentParamIndex = 0;
    } else if (key == GLFW_KEY_TAB) {
        currentParamIndex++; // cycle parameters
    } else if (key == GLFW_KEY_P) {
        g_vosc.togglePlay();
    } else if (key == GLFW_KEY_F) {
        g_vosc.setFrequency(g_vosc.frequency + 100.0f);
    } else if (key == GLFW_KEY_V) {
        g_vosc.setFrequency(g_vosc.frequency - 100.0f);
    } else if (key == GLFW_KEY_A) {
        g_vosc.setAmplitude(std::min(g_vosc.amplitude + 0.1f, VOSC_MAX_AMP));
    } else if (key == GLFW_KEY_D) {
        g_vosc.setAmplitude(std::max(g_vosc.amplitude - 0.1f, 0.0f));
    } else if (key == GLFW_KEY_E) {
        ECHO_ENABLED = !ECHO_ENABLED;
        std::cout << "Echo Enabled: " << (ECHO_ENABLED ? "True" : "False") << std::endl;
    }
    auto now = std::chrono::steady_clock::now();
    auto msSince = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAdjustTime).count();
    if (msSince < adjustDelayMs) return;
    lastAdjustTime = now;

    if (currentEffect == VGAIN_EFFECT) {
        if (key == GLFW_KEY_UP) {
            VGAIN = adjustLogarithmic(VGAIN, "up", VGAIN_MIN, VGAIN_MAX);
        } else if (key == GLFW_KEY_DOWN) {
            VGAIN = adjustLogarithmic(VGAIN, "down", VGAIN_MIN, VGAIN_MAX);
        }
    }
    else if (currentEffect == VEQ_EFFECT) {
        if (currentParamIndex % 2 == 0) {
            if (key == GLFW_KEY_UP) {
                CUTOFF_FREQ = adjustLogarithmic(CUTOFF_FREQ, "up", 100.0f, SAMPLE_RATE / 2.0f - 100.0f);
            } else if (key == GLFW_KEY_DOWN) {
                CUTOFF_FREQ = adjustLogarithmic(CUTOFF_FREQ, "down", 100.0f, SAMPLE_RATE / 2.0f - 100.0f);
            }
        } else {
            if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
                if (FILTER_TYPE == "None")
                    FILTER_TYPE = "Lowpass";
                else if (FILTER_TYPE == "Lowpass")
                    FILTER_TYPE = "Highpass";
                else
                    FILTER_TYPE = "None";
            }
        }
    }
    else if (currentEffect == ECHO_EFFECT) {
        int param = currentParamIndex % 8;
        if (param == 0) {
            // Echo enabled already toggled
        } else if (param == 1) {
            if (key == GLFW_KEY_UP)
                ECHO_DELAY_TIME = adjustLogarithmic(ECHO_DELAY_TIME, "up", 0.1f, 30.0f);
            else if (key == GLFW_KEY_DOWN)
                ECHO_DELAY_TIME = adjustLogarithmic(ECHO_DELAY_TIME, "down", 0.1f, 30.0f);
            g_echo.updateParameters(ECHO_DELAY_TIME, ECHO_DELAY_DRY_WET, ECHO_DELAY_FEEDBACK, ECHO_DELAY_GAIN,
                                    ECHO_REVERB_TIME, ECHO_REVERB_DRY_WET, ECHO_REVERB_GAIN);
        } else if (param == 2) {
            if (key == GLFW_KEY_UP)
                ECHO_DELAY_DRY_WET = std::min(ECHO_DELAY_DRY_WET + 0.05f, 1.0f);
            else if (key == GLFW_KEY_DOWN)
                ECHO_DELAY_DRY_WET = std::max(ECHO_DELAY_DRY_WET - 0.05f, 0.0f);
            g_echo.updateParameters(ECHO_DELAY_TIME, ECHO_DELAY_DRY_WET, ECHO_DELAY_FEEDBACK, ECHO_DELAY_GAIN,
                                    ECHO_REVERB_TIME, ECHO_REVERB_DRY_WET, ECHO_REVERB_GAIN);
        } else if (param == 3) {
            if (key == GLFW_KEY_UP)
                ECHO_DELAY_FEEDBACK = std::min(ECHO_DELAY_FEEDBACK + 0.1f, 10.0f);
            else if (key == GLFW_KEY_DOWN)
                ECHO_DELAY_FEEDBACK = std::max(ECHO_DELAY_FEEDBACK - 0.1f, 0.0f);
            g_echo.updateParameters(ECHO_DELAY_TIME, ECHO_DELAY_DRY_WET, ECHO_DELAY_FEEDBACK, ECHO_DELAY_GAIN,
                                    ECHO_REVERB_TIME, ECHO_REVERB_DRY_WET, ECHO_REVERB_GAIN);
        } else if (param == 4) {
            if (key == GLFW_KEY_UP)
                ECHO_DELAY_GAIN = std::min(ECHO_DELAY_GAIN + 0.5f, 24.0f);
            else if (key == GLFW_KEY_DOWN)
                ECHO_DELAY_GAIN = std::max(ECHO_DELAY_GAIN - 0.5f, 0.0f);
            g_echo.updateParameters(ECHO_DELAY_TIME, ECHO_DELAY_DRY_WET, ECHO_DELAY_FEEDBACK, ECHO_DELAY_GAIN,
                                    ECHO_REVERB_TIME, ECHO_REVERB_DRY_WET, ECHO_REVERB_GAIN);
        } else if (param == 5) {
            if (key == GLFW_KEY_UP)
                ECHO_REVERB_TIME = adjustLogarithmic(ECHO_REVERB_TIME, "up", 0.1f, 30.0f);
            else if (key == GLFW_KEY_DOWN)
                ECHO_REVERB_TIME = adjustLogarithmic(ECHO_REVERB_TIME, "down", 0.1f, 30.0f);
            g_echo.updateParameters(ECHO_DELAY_TIME, ECHO_DELAY_DRY_WET, ECHO_DELAY_FEEDBACK, ECHO_DELAY_GAIN,
                                    ECHO_REVERB_TIME, ECHO_REVERB_DRY_WET, ECHO_REVERB_GAIN);
        } else if (param == 6) {
            if (key == GLFW_KEY_UP)
                ECHO_REVERB_DRY_WET = std::min(ECHO_REVERB_DRY_WET + 0.05f, 1.0f);
            else if (key == GLFW_KEY_DOWN)
                ECHO_REVERB_DRY_WET = std::max(ECHO_REVERB_DRY_WET - 0.05f, 0.0f);
            g_echo.updateParameters(ECHO_DELAY_TIME, ECHO_DELAY_DRY_WET, ECHO_DELAY_FEEDBACK, ECHO_DELAY_GAIN,
                                    ECHO_REVERB_TIME, ECHO_REVERB_DRY_WET, ECHO_REVERB_GAIN);
        } else if (param == 7) {
            if (key == GLFW_KEY_UP)
                ECHO_REVERB_GAIN = std::min(ECHO_REVERB_GAIN + 0.5f, 24.0f);
            else if (key == GLFW_KEY_DOWN)
                ECHO_REVERB_GAIN = std::max(ECHO_REVERB_GAIN - 0.5f, 0.0f);
            g_echo.updateParameters(ECHO_DELAY_TIME, ECHO_DELAY_DRY_WET, ECHO_DELAY_FEEDBACK, ECHO_DELAY_GAIN,
                                    ECHO_REVERB_TIME, ECHO_REVERB_DRY_WET, ECHO_REVERB_GAIN);
        }
    }
    else if (currentEffect == VOSC_EFFECT) {
        int param = currentParamIndex % 3;
        if (param == 0) {
            if (key == GLFW_KEY_UP)
                g_vosc.setFrequency(adjustLogarithmic(g_vosc.frequency, "up", -20000.0f, VOSC_MAX_FREQ));
            else if (key == GLFW_KEY_DOWN)
                g_vosc.setFrequency(adjustLogarithmic(g_vosc.frequency, "down", -20000.0f, VOSC_MAX_FREQ));
        } else if (param == 1) {
            if (key == GLFW_KEY_UP)
                g_vosc.setAmplitude(adjustLogarithmic(g_vosc.amplitude, "up", 0.0f, VOSC_MAX_AMP));
            else if (key == GLFW_KEY_DOWN)
                g_vosc.setAmplitude(adjustLogarithmic(g_vosc.amplitude, "down", 0.0f, VOSC_MAX_AMP));
        } else if (param == 2) {
            if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN)
                g_vosc.togglePlay();
        }
    }
}

// ------------------------
// Render Text using stb_easy_font
// ------------------------
void renderText(const char* text, float x, float y, float r, float g, float b) {
    char buffer[99999];
    int num_quads = stb_easy_font_print(x, y, (char*)text, nullptr, buffer, sizeof(buffer));
    glColor3f(r, g, b);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// ------------------------
// Main Function
// ------------------------
int main() {
    // --- JACK Initialization ---
    const char* client_name = "cpp_visualizer";
    jack_status_t status;
    client = jack_client_open(client_name, JackNullOption, &status);
    if (client == nullptr) {
        std::cerr << "JACK client not started, error: " << status << std::endl;
        return 1;
    }
    jack_set_process_callback(client, processAudio, jack_port_register(client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0));
    jack_on_shutdown(client, jackShutdown, 0);
    if (jack_activate(client)) {
        std::cerr << "Cannot activate JACK client." << std::endl;
        return 1;
    }

    // --- GLFW / OpenGL Initialization ---
    if (!glfwInit()) {
        std::cerr << "GLFW init failed" << std::endl;
        return -1;
    }
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    WINDOW_WIDTH = mode->width;
    WINDOW_HEIGHT = mode->height;

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Real-Time Audio Visualizer", primary, nullptr);
    if (!window) {
        std::cerr << "GLFW window creation failed" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        std::vector<int16_t> samples;
        {
            std::lock_guard<std::mutex> lock(audioMutex);
            samples = g_audioBuffer;
        }

        float bandWidth = (float)WINDOW_WIDTH / CHUNK;
        for (int i = 0; i < CHUNK; ++i) {
            Color col = sampleToColor((float)samples[i], VGAIN);
            glColor3ub(col.r, col.g, col.b);
            float x = i * bandWidth;
            glBegin(GL_QUADS);
                glVertex2f(x, 0);
                glVertex2f(x + bandWidth, 0);
                glVertex2f(x + bandWidth, WINDOW_HEIGHT);
                glVertex2f(x, WINDOW_HEIGHT);
            glEnd();
        }

        if (showInfo) {
            std::string info = "Effect: ";
            switch (currentEffect) {
                case VGAIN_EFFECT: info += "VGain"; break;
                case VEQ_EFFECT: info += "VEQ"; break;
                case ECHO_EFFECT: info += "Echo"; break;
                case VOSC_EFFECT: info += "VOSC"; break;
            }
            info += "\n";
            if (currentEffect == VGAIN_EFFECT) {
                info += "VGain: " + my_to_string(VGAIN);
            } else if (currentEffect == VEQ_EFFECT) {
                info += "Cutoff Frequency: " + my_to_string(CUTOFF_FREQ) + " Hz\n";
                info += "Filter Type: " + FILTER_TYPE;
            } else if (currentEffect == ECHO_EFFECT) {
                info += "Echo Enabled: " + std::string(ECHO_ENABLED ? "True" : "False") + "\n";
                info += "Delay Time: " + my_to_string(ECHO_DELAY_TIME) + " sec\n";
                info += "Delay Dry/Wet: " + my_to_string(ECHO_DELAY_DRY_WET) + "\n";
                info += "Delay Feedback: " + my_to_string(ECHO_DELAY_FEEDBACK) + "\n";
                info += "Delay Gain: " + my_to_string(ECHO_DELAY_GAIN) + " dB\n";
                info += "Reverb Time: " + my_to_string(ECHO_REVERB_TIME) + " sec\n";
                info += "Reverb Dry/Wet: " + my_to_string(ECHO_REVERB_DRY_WET) + "\n";
                info += "Reverb Gain: " + my_to_string(ECHO_REVERB_GAIN) + " dB";
            } else if (currentEffect == VOSC_EFFECT) {
                info += "VOSC Frequency: " + my_to_string(g_vosc.frequency) + " Hz\n";
                info += "VOSC Amplitude: " + my_to_string(g_vosc.amplitude) + "\n";
                info += "VOSC State: " + std::string(g_vosc.isPlaying ? "Playing" : "Stopped");
            }
            renderText(info.c_str(), 10, 10, 1.0f, 1.0f, 1.0f);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    jack_client_close(client);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
