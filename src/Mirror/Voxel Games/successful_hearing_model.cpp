// voxel_perceptual_ambient.cpp
// Compile with: g++ voxel_perceptual_ambient.cpp glad.c -o voxel_perceptual_ambient -lglfw -lGL -ldl -ljack -lpthread -lm
// Requires: JACK, OpenGL, GLFW, GLM, GLAD (glad.c in your build)

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <jack/jack.h>
#include <atomic>
#include <thread>
#include <vector>
#include <random>
#include <cmath>
#include <iostream>
#include <chrono>

// ----------- AUDIO STATE SHARING -----------
std::atomic<float> g_yaw(-90.0f), g_pitch(0.0f);
std::atomic<bool> g_running(true);
static float g_jack_sample_rate = 48000.0f;

// ----------- AMBIENT SOUND GENERATOR -----------
inline float white_noise() {
    static thread_local std::default_random_engine rng(std::random_device{}());
    static thread_local std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    return dist(rng);
}

struct LPF {
    float y = 0.0f, a = 0.1f;
    void set(float cutoff, float sample_rate) {
        float x = expf(-2.0f * 3.14159265359f * cutoff / sample_rate);
        a = 1.0f - x;
    }
    float process(float x) {
        y += a * (x - y);
        return y;
    }
};

struct Crackle {
    float t = 0.0f;
    float duration = 0.002f;
    float freq = 4000.0f;
    float gain = 0.05f;
    bool active = false;
};

struct BPF {
    float z1 = 0, z2 = 0, a0 = 1, a1 = 0, a2 = 0, b1 = 0, b2 = 0;
    void set(float freq, float Q, float sample_rate) {
        float w0 = 2.0f * 3.14159265359f * freq / sample_rate;
        float alpha = sinf(w0) / (2.0f * Q);
        float c = cosf(w0);
        a0 = alpha;
        a1 = 0;
        a2 = -alpha;
        b1 = -2 * c;
        b2 = 1 - alpha;
    }
    float process(float x) {
        float y = a0 * x + a1 * z1 + a2 * z2 - b1 * z1 - b2 * z2;
        z2 = z1;
        z1 = y;
        return y;
    }
};

// ----------- JACK PROCESS CALLBACK -----------
int jack_process(jack_nframes_t nframes, void* arg) {
    static LPF hiss_lpf;
    static float last_cutoff = 8000.0f;
    static float sub_phase = 0.0f;
    static float sub_freq = 22.0f;
    static Crackle crackle;
    static float crackle_timer = 0.0f;
    static BPF crackle_bpf;
    static std::default_random_engine rng(std::random_device{}());
    static std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    jack_default_audio_sample_t* out = (jack_default_audio_sample_t*)jack_port_get_buffer((jack_port_t*)arg, nframes);

    // Get camera orientation
    float yaw = g_yaw.load();
    float pitch = g_pitch.load();

    // Map pitch to filter cutoff for perceptual filtering
    float cutoff = 4000.0f + 4000.0f * (glm::clamp(sinf(glm::radians(pitch)), -0.5f, 0.5f) + 0.5f);
    if (fabs(cutoff - last_cutoff) > 1.0f) {
        hiss_lpf.set(cutoff, g_jack_sample_rate);
        last_cutoff = cutoff;
    }

    for (jack_nframes_t i = 0; i < nframes; ++i) {
        // Hiss
        float hiss = hiss_lpf.process(white_noise()) * 0.05f;

        // Crackle
        float crackle_sample = 0.0f;
        crackle_timer -= 1.0f / g_jack_sample_rate;
        if (crackle.active) {
            crackle.t += 1.0f / g_jack_sample_rate;
            if (crackle.t < crackle.duration) {
                crackle_sample = crackle_bpf.process(white_noise()) * crackle.gain;
            }
            else {
                crackle.active = false;
                crackle.t = 0.0f;
            }
        }
        else if (crackle_timer <= 0.0f) {
            crackle.active = true;
            crackle.t = 0.0f;
            crackle.duration = 0.002f + dist01(rng) * 0.002f;
            crackle.freq = 4000.0f + (dist01(rng) - 0.5f) * 3000.0f;
            crackle.gain = 0.04f + dist01(rng) * 0.02f;
            crackle_bpf.set(crackle.freq, 20.0f, g_jack_sample_rate);
            crackle_timer = 0.1f + dist01(rng) * 0.6f;
        }

        // Subtle ambience (low sine)
        float sub = sinf(sub_phase) * 0.01f;
        sub_phase += 2.0f * 3.14159265359f * sub_freq / g_jack_sample_rate;
        if (sub_phase > 2.0f * 3.14159265359f) sub_phase -= 2.0f * 3.14159265359f;

        // Mix
        out[i] = hiss + crackle_sample + sub;
    }
    return 0;
}

// ----------- JACK AUDIO THREAD -----------
void jack_audio_thread() {
    jack_client_t* client = jack_client_open("voxel_perceptual_ambient", JackNullOption, nullptr);
    if (!client) { std::cerr << "Failed to connect to JACK.\n"; return; }
    jack_port_t* out_port = jack_port_register(client, "out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    g_jack_sample_rate = (float)jack_get_sample_rate(client);
    jack_set_process_callback(client, jack_process, out_port);
    if (jack_activate(client)) { std::cerr << "Cannot activate JACK client.\n"; return; }
    std::cout << "JACK audio running. Move camera to change sound color.\n";
    while (g_running.load()) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    jack_client_close(client);
}

// ----------- VOXEL ENGINE (YOUR CODE) -----------
const int SW = 800, SH = 600, RADIUS = 50;
float deltaTime = 0, lastFrame = 0;
glm::vec3 camPos(0, 2.6f, 0), vel(0);
float yaw = -90, pitch = 0, lastX = SW / 2, lastY = SH / 2;
bool onGround = false, firstMouse = true;
GLuint cubeVAO, cubeVBO, instVBO, outlVAO;

void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }
void mouse_callback(GLFWwindow*, double xposIn, double yposIn) {
    float xpos = (float)xposIn, ypos = (float)yposIn;
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float dx = xpos - lastX, dy = lastY - ypos; lastX = xpos; lastY = ypos;
    yaw += dx * 0.1f; pitch += dy * 0.1f;
    if (pitch > 89)pitch = 89; if (pitch < -89)pitch = -89;
    // Update audio orientation
    g_yaw.store(yaw);
    g_pitch.store(pitch);
}
GLuint compileShaderProgram(const char* v, const char* f) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER), fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs, 1, &v, nullptr); glCompileShader(vs);
    glShaderSource(fs, 1, &f, nullptr); glCompileShader(fs);
    GLuint p = glCreateProgram(); glAttachShader(p, vs); glAttachShader(p, fs); glLinkProgram(p);
    glDeleteShader(vs); glDeleteShader(fs); return p;
}
void processInput(GLFWwindow* w) {
    glm::vec3 d(0), f(glm::normalize(glm::vec3(cos(glm::radians(yaw)), 0, sin(glm::radians(yaw)))));
    glm::vec3 r = glm::cross(f, glm::vec3(0, 1, 0));
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS)d += f;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS)d -= f;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS)d -= r;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS)d += r;
    if (glm::length(d) > 0.01f)d = glm::normalize(d);
    float s = 10.0f; if (glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)s *= 2.0f;
    glm::vec3 hv = d * s; vel.x = hv.x; vel.z = hv.z;
    if (onGround && glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS) { vel.y = 5; onGround = false; }
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)glfwSetWindowShouldClose(w, true);
}
void handleCollision() {
    float feet = camPos.y - 1.6f;
    if (feet < 1.0f) { camPos.y = 2.6f; vel.y = 0; onGround = true; }
}
std::vector<float> genCube(float s = 1.0f) {
    float h = s * 0.5f;
    std::vector<glm::vec3> pos = {
        {-h,-h,h},{h,-h,h},{h,h,h},{h,h,h},{-h,h,h},{-h,-h,h},
        {h,-h,h},{h,-h,-h},{h,h,-h},{h,h,-h},{h,h,h},{h,-h,h},
        {h,-h,-h},{-h,-h,-h},{-h,h,-h},{-h,h,-h},{h,h,-h},{h,-h,-h},
        {-h,-h,-h},{-h,-h,h},{-h,h,h},{-h,h,h},{-h,h,-h},{-h,-h,-h},
        {-h,h,h},{h,h,h},{h,h,-h},{h,h,-h},{-h,h,-h},{-h,h,h},
        {-h,-h,-h},{h,-h,-h},{h,-h,h},{h,-h,h},{-h,-h,h},{-h,-h,-h}
    };
    std::vector<glm::vec3> norm = {
        {0,0,1},{0,0,1},{0,0,1},{0,0,1},{0,0,1},{0,0,1},
        {1,0,0},{1,0,0},{1,0,0},{1,0,0},{1,0,0},{1,0,0},
        {0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1},
        {-1,0,0},{-1,0,0},{-1,0,0},{-1,0,0},{-1,0,0},{-1,0,0},
        {0,1,0},{0,1,0},{0,1,0},{0,1,0},{0,1,0},{0,1,0},
        {0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0},{0,-1,0}
    };
    std::vector<glm::vec2> uv = { {0,0},{1,0},{1,1},{1,1},{0,1},{0,0},
        {0,0},{1,0},{1,1},{1,1},{0,1},{0,0},
        {0,0},{1,0},{1,1},{1,1},{0,1},{0,0},
        {0,0},{1,0},{1,1},{1,1},{0,1},{0,0},
        {0,0},{1,0},{1,1},{1,1},{0,1},{0,0},
        {0,0},{1,0},{1,1},{1,1},{0,1},{0,0} };
    std::vector<float> vtx; vtx.reserve(36 * 8);
    for (int i = 0; i < 36; ++i) {
        vtx.push_back(pos[i].x); vtx.push_back(pos[i].y); vtx.push_back(pos[i].z);
        vtx.push_back(norm[i].x); vtx.push_back(norm[i].y); vtx.push_back(norm[i].z);
        vtx.push_back(uv[i].x); vtx.push_back(uv[i].y);
    }
    return vtx;
}
std::vector<glm::vec3> getChunks(glm::vec3 p) {
    std::vector<glm::vec3> v; int cx = (int)floor(p.x), cz = (int)floor(p.z);
    for (int z = cz - RADIUS; z <= cz + RADIUS; z++)
        for (int x = cx - RADIUS; x <= cx + RADIUS; x++)
            v.push_back(glm::vec3(x, 0, z)); return v;
}

const char* vertSrc = R"(#version 330 core
layout(location=0)in vec3 aPos;layout(location=1)in vec3 aNorm;
layout(location=2)in vec2 texCoord;layout(location=3)in vec3 offset;
uniform mat4 model,view,proj;out vec2 TexCoord;
void main(){vec3 p=aPos+offset;gl_Position=proj*view*model*vec4(p,1);TexCoord=texCoord;})";

const char* fragSrc = R"(#version 330 core
in vec2 TexCoord;out vec4 FragColor;
const float gridSize=24,lineWidth=0.02;
void main(){vec2 f=fract(TexCoord*gridSize);
if(f.x<lineWidth||f.x>1.0-lineWidth||f.y<lineWidth||f.y>1.0-lineWidth)
FragColor=vec4(0,0,0,1);
else FragColor=vec4(1,1,1,1);})";

int main() {
    // Start JACK audio in background thread
    std::thread audio_thread(jack_audio_thread);

    // --- OpenGL/GLFW/GLM setup ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto mon = glfwGetPrimaryMonitor();
    auto m = glfwGetVideoMode(mon);
    auto* win = glfwCreateWindow(m->width, m->height, "Voxel Perceptual Ambient", mon, NULL);
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetCursorPosCallback(win, mouse_callback);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    GLuint shader = compileShaderProgram(vertSrc, fragSrc);
    auto verts = genCube();
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);

    glGenBuffers(1, &instVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(3); glVertexAttribDivisor(3, 1);
    glBindVertexArray(0);

    glGenVertexArrays(1, &outlVAO);
    glBindVertexArray(outlVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); glBindVertexArray(0);

    while (!glfwWindowShouldClose(win)) {
        float t = (float)glfwGetTime();
        deltaTime = t - lastFrame; lastFrame = t;
        processInput(win);
        if (!onGround)vel.y -= 9.81f * deltaTime;
        camPos += vel * deltaTime;
        handleCollision();
        glClearColor(0.53f, 0.81f, 0.92f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 f = glm::normalize(glm::vec3(cos(glm::radians(yaw)) * cos(glm::radians(pitch)), sin(glm::radians(pitch)), sin(glm::radians(yaw)) * cos(glm::radians(pitch))));
        glm::mat4 view = glm::lookAt(camPos, camPos + f, glm::vec3(0, 1, 0));
        glm::mat4 proj = glm::perspective(glm::radians(103.f), (float)SW / SH, 0.1f, 100.f);
        auto offs = getChunks(camPos);
        glBindBuffer(GL_ARRAY_BUFFER, instVBO);
        glBufferData(GL_ARRAY_BUFFER, offs.size() * sizeof(glm::vec3), offs.data(), GL_DYNAMIC_DRAW);

        glUseProgram(shader);
        glm::mat4 model(1);
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
        glBindVertexArray(cubeVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 36, (GLsizei)offs.size());
        glBindVertexArray(0);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    // Cleanup
    g_running.store(false);
    audio_thread.join();
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &instVBO);
    glDeleteVertexArrays(1, &outlVAO);
    glDeleteProgram(shader);
    glfwTerminate();
    return 0;
}
