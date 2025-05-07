#include <jack/jack.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <mutex>
#include <iostream>

// --- Globals ---
constexpr int MAX_BUFFER = 16384;
std::vector<float> audioL(MAX_BUFFER, 0.f);
std::vector<float> audioR(MAX_BUFFER, 0.f);
std::mutex audioMutex;
size_t writePos = 0;

jack_port_t* inL = nullptr, * inR = nullptr;
jack_client_t* client = nullptr;

float gain = 1.0f;
int samplesToDraw = 8192;

// --- JACK audio callback ---
int process(jack_nframes_t nframes, void*) {
    auto* bufL = (jack_default_audio_sample_t*)jack_port_get_buffer(inL, nframes);
    auto* bufR = (jack_default_audio_sample_t*)jack_port_get_buffer(inR, nframes);
    std::lock_guard<std::mutex> lock(audioMutex);
    for (unsigned int i = 0; i < nframes; ++i) {
        audioL[writePos] = bufL[i];
        audioR[writePos] = bufR[i];
        writePos = (writePos + 1) % MAX_BUFFER;
    }
    return 0;
}

bool initJACK() {
    client = jack_client_open("xy_scope", JackNoStartServer, nullptr);
    if (!client) return false;

    inL = jack_port_register(client, "left", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    inR = jack_port_register(client, "right", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);

    jack_set_process_callback(client, process, nullptr);
    jack_activate(client);

    // Connect system capture ports if using mic input:
    jack_connect(client, "system:capture_1", jack_port_name(inL));
    jack_connect(client, "system:capture_2", jack_port_name(inR));
    // Or connect your playback output channels via JACK patchbay/ Catia.

    return true;
}

// --- Shader sources ---
const char* vtx_src = R"(#version 330 core
layout(location=0) in vec2 pos;
uniform mat4 projection;
uniform float gain;
void main() {
    gl_Position = projection * vec4(pos * gain, 0.0, 1.0);
})";

const char* frag_src = R"(#version 330 core
out vec4 FragColor;
void main() { FragColor = vec4(0,1,0,1); }
)";

GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(s, 512, nullptr, log); std::cerr << log << "\n"; }
    return s;
}

GLuint make_prog() {
    GLuint vs = compile(GL_VERTEX_SHADER, vtx_src);
    GLuint fs = compile(GL_FRAGMENT_SHADER, frag_src);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

int main() {
    if (!initJACK()) { std::cerr << "JACK failed\n"; return -1; }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(800, 600, "XY Scope", NULL, NULL);
    if (!win) return -1;
    glfwMakeContextCurrent(win);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    GLuint prog = make_prog();
    GLuint vao, vbo; glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * MAX_BUFFER, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

    glm::mat4 proj = glm::ortho(-1.f, 1.f, -1.f, 1.f);
    int lastn = samplesToDraw;

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS) gain *= 1.01f;
        if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS) gain /= 1.01f;
        if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS && samplesToDraw < MAX_BUFFER - 8) samplesToDraw += 8;
        if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS && samplesToDraw > 16) samplesToDraw -= 8;

        if (samplesToDraw != lastn) {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * samplesToDraw, nullptr, GL_DYNAMIC_DRAW);
            lastn = samplesToDraw;
        }

        std::vector<glm::vec2> points(samplesToDraw);
        {
            std::lock_guard<std::mutex> lock(audioMutex);
            size_t pos = writePos;
            for (int i = 0; i < samplesToDraw; ++i) {
                points[i].x = audioL[pos];
                points[i].y = audioR[pos];
                pos = (pos + 1) % MAX_BUFFER;
            }
        }
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2) * samplesToDraw, points.data());

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "projection"), 1, GL_FALSE, &proj[0][0]);
        glUniform1f(glGetUniformLocation(prog, "gain"), gain);

        glBindVertexArray(vao);
        glDrawArrays(GL_LINE_STRIP, 0, samplesToDraw);

        glfwSwapBuffers(win);
    }

    jack_client_close(client);
    glfwTerminate();
}
