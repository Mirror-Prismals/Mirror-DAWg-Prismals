// SoundMachine3D_Final_FrontToBack.cpp
//
// A skeuomorphic 3D UI that toggles buttons and shortens them front-to-back
// when pressed, rather than shrinking them left-to-right.
// 
// Key Features:
// 1) Full 3D sides (front, top, right, bottom, left).
// 2) Buttons are centered in a fullscreen window.
// 3) Press animation sinks in, shifts left, and compresses depth.
// 4) Toggling behavior for sound buttons and ON/OFF button.
// 5) Labels drawn every frame (always visible).
// 6) A button click sound plays every time a button is clicked.
// 
// Uses GLFW/GLM and immediate-mode OpenGL. Requires stb_easy_font.h in the same folder.
// Compile with Visual Studio (link against glfw3.lib, opengl32.lib, user32.lib, gdi32.lib).

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// Use your local stb_easy_font.h (make sure it's in your source folder)
#include "stb_easy_font.h"

#ifndef PI
constexpr double PI = 3.14159265358979323846;
#endif

// -------------------------
// Chuck Process Control for sound scripts
// -------------------------
PROCESS_INFORMATION g_chuckProc = { 0 };
bool g_chuckRunning = false;

bool launchChuck(const std::string& scriptFile)
{
    std::string cmdLine = "chuck.exe " + scriptFile; // adjust path if needed
    STARTUPINFOA si = { sizeof(si) };
    ZeroMemory(&g_chuckProc, sizeof(g_chuckProc));
    if (!CreateProcessA(
        NULL,
        const_cast<char*>(cmdLine.c_str()),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &g_chuckProc))
    {
        std::cerr << "Failed to launch Chuck: " << GetLastError() << "\n";
        return false;
    }
    g_chuckRunning = true;
    return true;
}

void stopChuck()
{
    if (g_chuckRunning) {
        TerminateProcess(g_chuckProc.hProcess, 0);
        CloseHandle(g_chuckProc.hProcess);
        CloseHandle(g_chuckProc.hThread);
        g_chuckRunning = false;
    }
}

void writeChuckScript(const std::string& filename, const std::string& scriptContent)
{
    std::ofstream ofs(filename);
    if (ofs) {
        ofs << scriptContent;
        ofs.close();
    }
}

// -------------------------
// Play Button Click Sound
// -------------------------
// When a button is clicked, we write the click sound script and launch a Chuck process
// that plays a short sine/ADSR "click" sound.
void playButtonClickSound()
{
    std::string buttonClickScript = R"(
// Button click sound
SinOsc s => ADSR env => dac;
8000 => s.freq;
0.8 => s.gain;
1::ms => env.attackTime;
5::ms => env.decayTime;
0.0 => env.sustainLevel;
5::ms => env.releaseTime;
env.keyOn();
1::ms => now;
env.keyOff();
20::ms => now;
)";
    writeChuckScript("button_click.ck", buttonClickScript);

    PROCESS_INFORMATION buttonProc = { 0 };
    STARTUPINFOA si = { sizeof(si) };
    std::string cmdLine = "chuck.exe button_click.ck";
    if (CreateProcessA(NULL, const_cast<char*>(cmdLine.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &buttonProc))
    {
        // Wait briefly (100ms) for the click sound to play then clean up.
        WaitForSingleObject(buttonProc.hProcess, 100);
        CloseHandle(buttonProc.hProcess);
        CloseHandle(buttonProc.hThread);
    }
    else {
        std::cerr << "Failed to launch button click sound: " << GetLastError() << "\n";
    }
}

// -------------------------
// UI Button Structure
// -------------------------
struct Button {
    glm::vec2 pos;         // Center position (window coordinates)
    glm::vec2 size;        // Half-width and half-height
    std::string label;
    std::string chuckScript;

    bool isPressed = false;     // True while mouse is down on this button
    bool isSelected = false;    // True if toggled "on" (pressed in)
    double pressTime = 0.0;     // Timestamp of last mouse press
    float pressAnim = 0.0f;     // 0.0 -> not pressed, 0.5 -> fully pressed
};

// We'll store all sound buttons in a vector
std::vector<Button> g_soundButtons;

// The central ON/OFF button
Button g_onOffButton;

// Currently selected sound script
std::string g_currentSoundScript;

// Whether the user is playing a sound
bool g_playing = false;

// Timing constants
constexpr double PRESS_FEEDBACK_DURATION = 0.15; // time to animate to pressed
constexpr double LAUNCH_DELAY = 0.1;            // short delay before launching Chuck

// For animation updates
double g_lastFrameTime = 0.0;

// -------------------------
// 3D Button Drawing with Press Animation
// Shortens the "depth" dimension rather than the button width,
// and shifts the button left slightly while keeping width the same.
// -------------------------
void drawButton3D(float bx, float by, float bw, float bh, float depth, float pressAnim)
{
    // pressAnim in [0, 0.5]. We interpret 0.5 as "fully pressed."
    // SHIFT: move the button left by up to 10 px
    float shiftLeft = 10.0f * pressAnim;

    // SINK: the front face is offset deeper into the screen by pressOffsetZ
    float pressOffsetZ = depth * pressAnim;

    // COMPRESS: reduce the "depth" dimension from the front face to the back face
    // We'll shrink the overall thickness by up to 50% at full press
    float newDepth = depth * (1.0f - 0.5f * pressAnim);

    // The front face's corners are at (bx, by, -pressOffsetZ)
    float x = bx - shiftLeft;
    float y = by;

    // FRONT FACE color: 0.8 -> 0.6 as pressAnim goes 0->0.5
    float frontColor = 0.8f - 0.2f * (pressAnim * 2.0f);

    // FRONT FACE
    glColor3f(frontColor, frontColor, frontColor);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glEnd();

    // TOP FACE (connect front top edge to back top edge)
    glColor3f(0.9f, 0.9f, 0.9f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    // RIGHT FACE
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_QUADS);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    // BOTTOM FACE
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    // LEFT FACE
    glColor3f(0.65f, 0.65f, 0.65f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();
}

// -------------------------
// Text Rendering Function
// -------------------------
void renderText(float x, float y, const char* text)
{
    char buffer[99999];
    int num_quads = stb_easy_font_print(x, y, const_cast<char*>(text), NULL, buffer, sizeof(buffer));

    // Disable depth test so text isn't occluded
    glDisable(GL_DEPTH_TEST);

    glColor3f(0.0f, 0.0f, 0.0f); // black text
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);

    glEnable(GL_DEPTH_TEST);
}

// -------------------------
// Hit Testing
// -------------------------
bool isInside(const Button& btn, float x, float y)
{
    float left = btn.pos.x - btn.size.x;
    float right = btn.pos.x + btn.size.x;
    float top = btn.pos.y - btn.size.y;
    float bottom = btn.pos.y + btn.size.y;
    return (x >= left && x <= right && y >= top && y <= bottom);
}

// -------------------------
// Chuck Scripts for various sound effects
// -------------------------
std::string oceanWavesScript = R"(
    // Ocean waves sound generator
    Gain g => dac;
    Noise n => LPF f => g;
    f.freq(500.0);
    f.Q(1.0);
    SinOsc mod1 => blackhole;
    SinOsc mod2 => blackhole;
    mod1.freq(0.2);
    mod2.freq(0.1);
    g.gain(0.5);
    fun float mod_amplitude() {
        return 0.5 + 0.4 * mod1.last() + 0.2 * mod2.last();
    }
    while (true) {
        n.gain(mod_amplitude());
        1::ms => now;
    }
)";

std::string springRainScript = R"(
    // Spring rain with big raindrops and occasional thunder
    Gain g => dac;
    Noise rainNoise => HPF hpfRain => LPF lpfRain => g;
    hpfRain.freq(500.0);
    lpfRain.freq(2000.0);
    Noise bigRaindropNoise => BPF bpfBig => g;
    bpfBig.freq(800.0);
    bpfBig.Q(5.0);
    Phasor thunderOsc => ADSR envThunder => Gain thunderDist => JCRev rev => Gain thunderGain => HPF hpfThunder => LPF lpfThunder => g;
    thunderOsc.freq(50.0);
    envThunder.set(0.41, 0.5, 0.2, 0.5);
    envThunder.keyOff();
    thunderOsc => envThunder => thunderDist => thunderGain => hpfThunder => lpfThunder => rev;
    hpfThunder.freq(8000.0);
    rev.mix(5.2);
    thunderDist.gain(20.0);
    0.01 => float thunderGainControl;
    SinOsc modRain => blackhole;
    modRain.freq(0.1);
    modRain.gain(0.02);
    g.gain(0.5);
    fun float dbToGain(float db) {
        return Math.pow(10, db / 20.0);
    }
    while (true) {
        rainNoise.gain(0.1 + modRain.last() * 0.05);
        100::ms => now;
        if (Math.random2(0, 100) < 10) {
            bigRaindropNoise.gain(0.5);
            50::ms => now;
            bigRaindropNoise.gain(0.0);
        }
        if (Math.random2(0, 50) < 1) {
            hpfThunder.freq(Math.random2f(1000.0, 8000.0));
            lpfThunder.freq(Math.random2f(330.0, 400.0));
            thunderGain.gain(thunderGainControl * dbToGain(Math.random2f(-30.0, -20.0)));
            envThunder.keyOn();
            2::second => now;
            envThunder.keyOff();
            3::second => now;
        }
    }
)";

std::string mountainStreamScript = R"(
    // Mountain Stream Effect
    Gain g => dac;
    g.gain(0.5);
    Noise mountainStream => LPF lpfStream => g;
    mountainStream.gain(0.3);
    lpfStream.freq(200.0);
    while (true) {
        mountainStream.gain(0.3 + 0.1 * Math.random2f(-0.5, 0.5));
        100::ms => now;
    }
)";

std::string heartbeatScript = R"(
    // Heartbeat sound
    Gain g => dac;
    g.gain(0.5);
    SinOsc s => g;
    fun void heartbeat() {
        while (true) {
            50 => s.freq;
            0.5 => s.gain;
            0.1::second => now;
            0 => s.gain;
            0.4::second => now;
            50 => s.freq;
            0.5 => s.gain;
            0.1::second => now;
            0 => s.gain;
            1.3::second => now;
        }
    }
    spork ~ heartbeat();
    while (true) {
        1::second => now;
    }
)";

std::string volcanoLavaScript = R"(
    // Volcano lava erupting sound
    Gain g => dac;
    Noise rumbleNoise => HPF hpfRumble => LPF lpfRumble => g;
    hpfRumble.freq(20.0);
    lpfRumble.freq(200.0);
    Noise crackleNoise => BPF bpfCrackle => g;
    bpfCrackle.freq(1000.0);
    bpfCrackle.Q(5.0);
    SinOsc modRumble => blackhole;
    modRumble.freq(0.1);
    modRumble.gain(0.02);
    g.gain(0.5);
    fun float lowBitDepth(float input, int bits) {
        return Math.round(input * bits) / bits;
    }
    while (true) {
        rumbleNoise.gain(0.2 + modRumble.last() * 0.1);
        crackleNoise.gain(lowBitDepth(Math.random2f(0.0, 0.5), 8));
        50::ms => now;
    }
)";

std::string summerNightScript = R"(
    // Summer night with cicada sounds
    Gain g => dac;
    g.gain(0.5);
    fun void cicada() {
        SinOsc s => LPF f => JCRev r => g;
        s.freq(3000);
        0.01 => s.gain;
        80 => f.freq;
        0.1 => r.mix;
        while (true) {
            Math.random2f(0.01, 0.03) :: second => dur chirp_dur;
            s.freq(3000 + Math.random2f(-100, 100));
            Math.random2f(0.05, 0.2) => s.gain;
            chirp_dur => now;
        }
    }
    for (0 => int i; i < 10; i++) {
        spork ~ cicada();
        0.2::second => now;
    }
    Noise n => LPF f => JCRev r => g;
    1.1 => n.gain;
    500 => f.freq;
    0.2 => r.mix;
    while (true) {
        Math.random2f(0.005, 0.01) => n.gain;
        1::second => now;
    }
)";

std::string jungleNoiseScript = R"(
    // Jungle noise with birds and insects
    Gain g => dac;
    g.gain(0.5);
    fun void bird_chirp() {
        SinOsc bird => ADSR env => g;
        1200 + Math.random2f(300, 800) => bird.freq;
        env.set(0.01, 0.2, 0.2, 0.01);
        0.2 => bird.gain;
        env.keyOn();
        0.3::second => now;
        env.keyOff();
        0.1::second => now;
    }
    fun void insect_sound() {
        Noise insect => BPF filter => ADSR env => g;
        5000 + Math.random2f(1000, 2000) => filter.freq;
        1.01 => filter.Q;
        env.set(0.01, 0.1, 0.2, 0.01);
        0.1 => insect.gain;
        env.keyOn();
        0.2::second => now;
        env.keyOff();
        0.3::second => now;
    }
    fun void water_stream() {
        Noise water => LPF filter => g;
        2000 => filter.freq;
        0.2 => filter.Q;
        0.1 => water.gain;
        while (true) {
            0.1::second => now;
        }
    }
    spork ~ water_stream();
    while (true) {
        if (Math.random2f(0, 1) < 0.5) {
            spork ~ bird_chirp();
        } else {
            spork ~ insect_sound();
        }
        0.5::second + Math.random2f(1, 3)::second => now;
    }
)";

std::string insideVolcanoScript = R"(
    // Inside Volcano
    Gain g => dac;
    Delay delay => JCRev reverb => g;
    delay.delay(1::ms);
    reverb.mix(0.5);
    Noise rumbleNoise => HPF hpfRumble => LPF lpfRumble => delay;
    hpfRumble.freq(20.0);
    lpfRumble.freq(100.0);
    Noise popNoise => BPF bpfPop => delay;
    bpfPop.freq(500.0);
    bpfPop.Q(10.0);
    SinOsc bubbleOsc1 => Gain bubbleGain1 => delay;
    bubbleOsc1.freq(0.5);
    SinOsc bubbleOsc2 => Gain bubbleGain2 => delay;
    bubbleOsc2.freq(0.7);
    g.gain(0.5);
    while (true) {
        0.2 + 0.1 * Math.sin(now / second) => rumbleNoise.gain;
        if (Math.random2(0, 100) < 20) {
            0.5 + 0.5 * Math.random2f(-1.0, 1.0) => popNoise.gain;
            10::ms => now;
            0.0 => popNoise.gain;
        }
        (0.5 + 0.5 * bubbleOsc1.last() + 0.5 * bubbleOsc2.last()) * Math.random2f(0.1, 0.3) => bubbleGain1.gain;
        (0.5 + 0.5 * bubbleOsc2.last() + 0.5 * bubbleOsc1.last()) * Math.random2f(0.1, 0.3) => bubbleGain2.gain;
        50::ms => now;
    }
)";

// -------------------------
// Initialize UI
// We'll place 8 sound buttons in a 2×4 grid, plus 1 ON/OFF button near bottom
// -------------------------
void initUI(int screenWidth, int screenHeight)
{
    // Each button is 120 wide, 40 tall in total (half-sizes: 60, 20).
    float halfW = 60.0f;
    float halfH = 20.0f;

    // We want 2 rows, 4 columns
    float xSpacing = 200.0f;
    float ySpacing = 200.0f;

    int columns = 4;
    int rows = 2;

    float totalWidth = (columns - 1) * xSpacing;
    float totalHeight = (rows - 1) * ySpacing;

    float startX = (screenWidth * 0.5f) - (totalWidth * 0.5f);
    float startY = (screenHeight * 0.4f) - (totalHeight * 0.5f);

    // Prepare an array of labels and scripts
    struct SoundInfo {
        const char* label;
        const std::string* script;
    } soundInfos[8] = {
        { "Ocean Waves",     &oceanWavesScript     },
        { "Spring Rain",     &springRainScript     },
        { "Mountain Stream", &mountainStreamScript },
        { "Heartbeat",       &heartbeatScript      },
        { "Volcano Lava",    &volcanoLavaScript    },
        { "Summer Night",    &summerNightScript    },
        { "Jungle Noise",    &jungleNoiseScript    },
        { "Inside Volcano",  &insideVolcanoScript  },
    };

    g_soundButtons.clear();
    g_soundButtons.resize(8);

    for (int i = 0; i < 8; i++) {
        int col = i % 4;
        int row = i / 4;

        float posX = startX + col * xSpacing;
        float posY = startY + row * ySpacing;

        g_soundButtons[i].pos = glm::vec2(posX, posY);
        g_soundButtons[i].size = glm::vec2(halfW, halfH);
        g_soundButtons[i].label = soundInfos[i].label;
        g_soundButtons[i].chuckScript = *soundInfos[i].script;
    }

    // ON/OFF button near bottom center
    g_onOffButton.pos = glm::vec2(screenWidth * 0.5f, screenHeight - 100.0f);
    g_onOffButton.size = glm::vec2(75.0f, 20.0f);
    g_onOffButton.label = "OFF/RESUME";
    g_onOffButton.isPressed = false;
    g_onOffButton.isSelected = false;
}

// -------------------------
// Mouse Button Callback
// - Toggling behavior for sound buttons and ON/OFF button,
// - And playing the button click sound on release.
// -------------------------
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    double currentTime = glfwGetTime();
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Mark button as pressed if inside
            for (auto& btn : g_soundButtons) {
                if (isInside(btn, (float)mx, (float)my)) {
                    btn.isPressed = true;
                    btn.pressTime = currentTime;
                }
            }
            if (isInside(g_onOffButton, (float)mx, (float)my)) {
                g_onOffButton.isPressed = true;
                g_onOffButton.pressTime = currentTime;
            }
        }
        else if (action == GLFW_RELEASE) {
            // Sound buttons: toggle selection and play click sound
            for (auto& btn : g_soundButtons) {
                if (btn.isPressed && isInside(btn, (float)mx, (float)my)) {
                    playButtonClickSound();
                    // If it's already selected, unselect it
                    if (btn.isSelected) {
                        btn.isSelected = false;
                        // If this was the currently selected sound, clear script
                        if (btn.chuckScript == g_currentSoundScript) {
                            g_currentSoundScript.clear();
                            // If ON/OFF is on, we also stop Chuck
                            if (g_onOffButton.isSelected) {
                                stopChuck();
                                g_onOffButton.isSelected = false;
                                g_playing = false;
                            }
                        }
                    }
                    else {
                        // Unselect all others
                        for (auto& other : g_soundButtons) {
                            other.isSelected = false;
                        }
                        // Select this one
                        btn.isSelected = true;
                        // Update current script
                        g_currentSoundScript = btn.chuckScript;

                        // If ON/OFF is already on, we re‐launch Chuck with the new script
                        if (g_onOffButton.isSelected) {
                            stopChuck();
                            writeChuckScript("sound_script.ck", g_currentSoundScript);
                            if (launchChuck("sound_script.ck")) {
                                g_playing = true;
                            }
                        }
                    }
                }
                btn.isPressed = false;
            }

            // ON/OFF button: toggle on/off and play click sound
            if (g_onOffButton.isPressed && isInside(g_onOffButton, (float)mx, (float)my)) {
                playButtonClickSound();
                // Wait a brief moment for press feedback
                while (glfwGetTime() - g_onOffButton.pressTime < LAUNCH_DELAY) {
                    glfwPollEvents();
                }

                // Toggle
                g_onOffButton.isSelected = !g_onOffButton.isSelected;
                if (g_onOffButton.isSelected) {
                    // Turn ON
                    // If there's a selected sound, launch it
                    if (!g_currentSoundScript.empty()) {
                        writeChuckScript("sound_script.ck", g_currentSoundScript);
                        if (launchChuck("sound_script.ck")) {
                            g_playing = true;
                        }
                    }
                    else {
                        // No sound is selected => revert ON/OFF to off
                        g_onOffButton.isSelected = false;
                    }
                }
                else {
                    // Turn OFF
                    stopChuck();
                    g_playing = false;
                }
            }
            g_onOffButton.isPressed = false;
        }
    }
}

// -------------------------
// Update Press Animations
// For both toggled state and actual press
// - If button is pressed or selected => animate to 0.5
// - Otherwise => animate to 0.0
// -------------------------
void updateButtonAnimations(std::vector<Button>& buttons, float deltaTime)
{
    float animSpeed = (float)(0.5 / PRESS_FEEDBACK_DURATION); // 0->0.5 in PRESS_FEEDBACK_DURATION
    for (auto& btn : buttons) {
        bool shouldPress = (btn.isPressed || btn.isSelected);
        float target = shouldPress ? 0.5f : 0.0f;

        if (btn.pressAnim < target) {
            btn.pressAnim += animSpeed * deltaTime;
            if (btn.pressAnim > target) btn.pressAnim = target;
        }
        else if (btn.pressAnim > target) {
            btn.pressAnim -= animSpeed * deltaTime;
            if (btn.pressAnim < target) btn.pressAnim = target;
        }
    }
}

void updateButtonAnimations(Button& btn, float deltaTime)
{
    float animSpeed = (float)(0.5 / PRESS_FEEDBACK_DURATION);
    bool shouldPress = (btn.isPressed || btn.isSelected);
    float target = shouldPress ? 0.5f : 0.0f;

    if (btn.pressAnim < target) {
        btn.pressAnim += animSpeed * deltaTime;
        if (btn.pressAnim > target) btn.pressAnim = target;
    }
    else if (btn.pressAnim > target) {
        btn.pressAnim -= animSpeed * deltaTime;
        if (btn.pressAnim < target) btn.pressAnim = target;
    }
}

// -------------------------
// Main
// -------------------------
int main()
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Fullscreen
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int fullWidth = mode->width;
    int fullHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(fullWidth, fullHeight, "Sound Machine 3D - FrontToBack", monitor, nullptr);
    if (!window) {
        glfwTerminate();
        std::cerr << "Failed to create fullscreen window\n";
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, fullWidth, fullHeight, 0, -100, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    initUI(fullWidth, fullHeight);
    g_lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - g_lastFrameTime);
        g_lastFrameTime = currentTime;

        // Update press animations
        updateButtonAnimations(g_soundButtons, deltaTime);
        updateButtonAnimations(g_onOffButton, deltaTime);

        // Clear background
        glClearColor(0.0f, 0.375f, 0.375f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Draw sound selection buttons
        for (const auto& btn : g_soundButtons) {
            float bx = btn.pos.x - btn.size.x;
            float by = btn.pos.y - btn.size.y;
            float bw = btn.size.x * 2.0f;
            float bh = btn.size.y * 2.0f;
            float depth = 10.0f;

            drawButton3D(bx, by, bw, bh, depth, btn.pressAnim);
            // Render label (center it a bit)
            renderText(bx + 10, by + bh / 2 - 5, btn.label.c_str());
        }

        // Draw ON/OFF button
        {
            float cx = g_onOffButton.pos.x - g_onOffButton.size.x;
            float cy = g_onOffButton.pos.y - g_onOffButton.size.y;
            float cw = g_onOffButton.size.x * 2.0f;
            float ch = g_onOffButton.size.y * 2.0f;
            float depth = 10.0f;

            drawButton3D(cx, cy, cw, ch, depth, g_onOffButton.pressAnim);
            renderText(cx + 15, cy + ch / 2 - 5, g_onOffButton.label.c_str());
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    stopChuck();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
