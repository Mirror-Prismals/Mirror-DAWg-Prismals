// ferrofluid_tangent_envelope.cpp
//
// Compile (Linux example):
//   g++ ferrofluid_tangent_envelope.cpp -o ferrofluid_tangent_envelope \
//       -lglfw -ljack -ldl -lGL -pthread
//
// Make sure GLAD’s loader is generated for your OpenGL version and GLM is installed.

#include <iostream>
#include <vector>
#include <mutex>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <algorithm>

// JACK audio includes
#include <jack/jack.h>

// Include GLAD before GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM for math
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

constexpr float PI = 3.14159265f;

// --------------------------------------------------
// Global audio buffer + mutex
// --------------------------------------------------
std::mutex audioMutex;
std::vector<float> audioBuffer;
const size_t AUDIO_BUFFER_SIZE = 4096;

// --------------------------------------------------
// JACK process callback
// --------------------------------------------------
int jackProcessCallback(jack_nframes_t nframes, void* arg) {
    jack_port_t* inputPort = reinterpret_cast<jack_port_t*>(arg);
    auto* in = reinterpret_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(inputPort, nframes));
    std::lock_guard<std::mutex> lock(audioMutex);
    for (jack_nframes_t i = 0; i < nframes; i++) {
        audioBuffer.push_back(in[i]);
    }
    if (audioBuffer.size() > AUDIO_BUFFER_SIZE) {
        audioBuffer.erase(audioBuffer.begin(), audioBuffer.begin() + (audioBuffer.size() - AUDIO_BUFFER_SIZE));
    }
    return 0;
}

// --------------------------------------------------
// Shaders for drawing circles (as point sprites) and the envelope polygon
// --------------------------------------------------
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 projection;
uniform float pointSize;
void main()
{
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = pointSize;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
uniform bool usePointCutout;
void main()
{
    if(usePointCutout) {
        vec2 center = gl_PointCoord - vec2(0.5);
        if(length(center) > 0.5)
            discard;
    }
    FragColor = vec4(color, 1.0);
}
)";

// Utility: compile shader
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "ERROR::SHADER_COMPILATION_ERROR\n" << infoLog << std::endl;
    }
    return shader;
}

// --------------------------------------------------
// Particle definition: each particle is a circle with position and velocity.
// --------------------------------------------------
struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
};

// Helper: rotate a 2D vector by -90° (clockwise)
glm::vec2 rotateCW(const glm::vec2& v) {
    return glm::vec2(v.y, -v.x);
}

// Helper: sort indices in counterclockwise order around the centroid.
std::vector<int> sortIndicesCCW(const std::vector<Particle>& particles) {
    std::vector<int> indices(particles.size());
    glm::vec2 centroid(0.0f);
    for (size_t i = 0; i < particles.size(); i++) {
        indices[i] = i;
        centroid += particles[i].position;
    }
    centroid /= static_cast<float>(particles.size());
    std::sort(indices.begin(), indices.end(), [&particles, centroid](int a, int b) {
        float angleA = atan2(particles[a].position.y - centroid.y, particles[a].position.x - centroid.x);
        float angleB = atan2(particles[b].position.y - centroid.y, particles[b].position.x - centroid.x);
        return angleA < angleB;
    });
    return indices;
}

// --------------------------------------------------
// Main
// --------------------------------------------------
int main(){
    // 1) JACK setup for audio input
    jack_client_t* client;
    const char* client_name = "ferrofluid_tangent_envelope";
    jack_options_t options = JackNullOption;
    jack_status_t status;
    client = jack_client_open(client_name, options, &status);
    if(!client) {
        std::cerr << "Failed to open JACK client." << std::endl;
        return 1;
    }
    jack_port_t* input_port = jack_port_register(client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    if(!input_port) {
        std::cerr << "Failed to register JACK input port." << std::endl;
        jack_client_close(client);
        return 1;
    }
    jack_set_process_callback(client, jackProcessCallback, input_port);
    if(jack_activate(client)) {
        std::cerr << "Cannot activate JACK client." << std::endl;
        jack_client_close(client);
        return 1;
    }

    // 2) GLFW and GLAD in fullscreen mode
    if(!glfwInit()){
        std::cerr << "Failed to initialize GLFW." << std::endl;
        jack_client_close(client);
        return 1;
    }
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Ferrofluid Tangent Envelope", primaryMonitor, nullptr);
    if(!window){
        std::cerr << "Failed to create fullscreen GLFW window." << std::endl;
        glfwTerminate();
        jack_client_close(client);
        return 1;
    }
    glfwMakeContextCurrent(window);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cerr << "Failed to initialize GLAD." << std::endl;
        glfwTerminate();
        jack_client_close(client);
        return 1;
    }
    glViewport(0, 0, mode->width, mode->height);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // 3) Compile and link shader program
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    {
        int success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if(!success){
            char infoLog[512];
            glGetProgramInfoLog(shaderProgram, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR\n" << infoLog << std::endl;
        }
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 4) Setup eight particles
    const int NUM_PARTICLES = 8;
    std::vector<Particle> particles(NUM_PARTICLES);
    float initRadius = 0.5f; // distance from center in NDC for initial arrangement
    float angleStep = 2.0f * PI / NUM_PARTICLES;
    for (int i = 0; i < NUM_PARTICLES; i++){
        float angle = i * angleStep;
        particles[i].position = glm::vec2(cos(angle), sin(angle)) * initRadius;
        particles[i].velocity = glm::vec2(0.0f);
    }
    // Each circle’s radius (in NDC)
    float circleRadius = 0.15f;

    // Setup VAO/VBO for drawing circles as point sprites
    GLuint circleVAO, circleVBO;
    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);
    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Setup separate VAO/VBO for the envelope polygon
    GLuint envelopeVAO, envelopeVBO;
    glGenVertexArrays(1, &envelopeVAO);
    glGenBuffers(1, &envelopeVBO);

    // Orthographic projection from -1 to 1 (NDC)
    glm::mat4 projection = glm::ortho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);
    float pixelRadius = circleRadius * mode->height / 2.0f;
    float pointSizePixels = pixelRadius * 2.0f; // diameter in pixels

    // 5) Main loop
    auto lastTime = std::chrono::high_resolution_clock::now();
    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Get audio amplitude (average absolute value)
        float amplitude = 0.0f;
        {
            std::lock_guard<std::mutex> lock(audioMutex);
            if(!audioBuffer.empty()){
                for (float s : audioBuffer)
                    amplitude += std::fabs(s);
                amplitude /= audioBuffer.size();
            }
        }
        float forceScale = amplitude * 2.0f; // tweak as desired

        // Update particle physics: add random "womp" and basic collision/bounce
        for(auto& p : particles){
            float rx = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
            float ry = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
            p.velocity += glm::vec2(rx, ry) * forceScale * deltaTime;
            p.velocity *= 0.95f;
            p.position += p.velocity * deltaTime;
            // Bounce off screen boundaries
            if(p.position.x < -1.0f + circleRadius) { p.position.x = -1.0f + circleRadius; p.velocity.x = -p.velocity.x; }
            if(p.position.x >  1.0f - circleRadius) { p.position.x =  1.0f - circleRadius; p.velocity.x = -p.velocity.x; }
            if(p.position.y < -1.0f + circleRadius) { p.position.y = -1.0f + circleRadius; p.velocity.y = -p.velocity.y; }
            if(p.position.y >  1.0f - circleRadius) { p.position.y =  1.0f - circleRadius; p.velocity.y = -p.velocity.y; }
        }
        // Simple packing: resolve collisions between particles
        for (int i = 0; i < NUM_PARTICLES; i++){
            for (int j = i+1; j < NUM_PARTICLES; j++){
                glm::vec2 diff = particles[j].position - particles[i].position;
                float dist = glm::length(diff);
                float minDist = circleRadius * 2.0f;
                if(dist < minDist && dist > 0.0001f){
                    float overlap = minDist - dist;
                    glm::vec2 dir = diff / dist;
                    particles[i].position -= 0.5f * overlap * dir;
                    particles[j].position += 0.5f * overlap * dir;
                    particles[i].velocity = glm::reflect(particles[i].velocity, dir) * 0.5f;
                    particles[j].velocity = glm::reflect(particles[j].velocity, -dir) * 0.5f;
                }
            }
        }
        
        // Upload circle positions to GPU
        {
            std::vector<glm::vec2> circlePositions;
            circlePositions.reserve(NUM_PARTICLES);
            for(auto& p : particles)
                circlePositions.push_back(p.position);
            glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, circlePositions.size() * sizeof(glm::vec2), circlePositions.data());
        }
        
        // Compute envelope polygon following the tangents:
        std::vector<int> sortedIndices = sortIndicesCCW(particles);
        std::vector<glm::vec2> envelopePoints;
        // For each particle in convex order, compute tangent points based on its neighbors.
        for (int idx = 0; idx < NUM_PARTICLES; idx++){
            int i = sortedIndices[idx];
            int prev = sortedIndices[(idx + NUM_PARTICLES - 1) % NUM_PARTICLES];
            int next = sortedIndices[(idx + 1) % NUM_PARTICLES];
            glm::vec2 C = particles[i].position;
            glm::vec2 P = particles[prev].position;
            glm::vec2 N = particles[next].position;
            // For equal-radius circles, the external tangent direction is given by rotating the vector from neighbor to current.
            glm::vec2 n1 = glm::normalize(rotateCW(C - P)); // tangent from previous edge
            glm::vec2 n2 = glm::normalize(rotateCW(N - C)); // tangent from next edge
            // Tangent points on circle i:
            glm::vec2 T1 = C + circleRadius * n1;
            glm::vec2 T2 = C + circleRadius * n2;
            // Compute angles for these points (relative to center C)
            float angle1 = atan2(T1.y - C.y, T1.x - C.x);
            float angle2 = atan2(T2.y - C.y, T2.x - C.x);
            // Ensure we take the smaller (external) arc.
            if(angle2 < angle1)
                angle2 += 2.0f * PI;
            // Sample the arc with a sufficient number of points (adjust arcSamples for smoothness)
            const int arcSamples = 30;
            for (int j = 0; j <= arcSamples; j++){
                float t = (float)j / arcSamples;
                float a = angle1 + (angle2 - angle1) * t;
                glm::vec2 pt = C + circleRadius * glm::vec2(cos(a), sin(a));
                envelopePoints.push_back(pt);
            }
        }
        
        // Upload envelope polygon points to GPU
        glBindVertexArray(envelopeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, envelopeVBO);
        glBufferData(GL_ARRAY_BUFFER, envelopePoints.size() * sizeof(glm::vec2), envelopePoints.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        
        // Render
        glClearColor(1.f, 1.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // 1) Draw envelope polygon (filled) in black (disable point cutout)
        GLint usePointCutoutLoc = glGetUniformLocation(shaderProgram, "usePointCutout");
        glUniform1i(usePointCutoutLoc, 0);
        GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);
        glBindVertexArray(envelopeVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, envelopePoints.size());
        glBindVertexArray(0);
        
        // 2) Draw circles on top (with point cutout enabled)
        glUniform1i(usePointCutoutLoc, 1);
        glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);
        GLint pointSizeLoc = glGetUniformLocation(shaderProgram, "pointSize");
        glUniform1f(pointSizeLoc, pointSizePixels);
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
        glBindVertexArray(0);
        
        glfwSwapBuffers(window);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Cleanup
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
    glDeleteVertexArrays(1, &envelopeVAO);
    glDeleteBuffers(1, &envelopeVBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    jack_client_close(client);
    return 0;
}
