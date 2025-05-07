// ======================================================================
// FlightSimNoGravity.cpp
// Single–file "flight" simulator with no gravity.
// You accelerate forward (W to throttle up, S to throttle down)
// but never fall. Pitch up/down only changes the camera angle.
//
// Controls:
//   - W/S: Increase/decrease throttle (forward acceleration)
//   - UP/DOWN: Pitch nose up/down (visual only; no altitude change)
//   - LEFT/RIGHT: Yaw left/right
// ======================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

// =====================================================
// Window, “Flight” Parameters (no gravity)
// =====================================================
const unsigned int WINDOW_WIDTH = 1280;
const unsigned int WINDOW_HEIGHT = 720;

glm::vec3 aircraftPos(0.0f, 150.0f, 0.0f);
float aircraftPitch = 0.0f;  // around X-axis
float aircraftYaw = 90.0f; // around Y-axis
float throttle = 0.2f;
glm::vec3 velocity(0.0f);

const float maxThrust = 60.0f;
// float gravity = 9.81f;  // <— removed entirely
const float drag = 0.995f;
const float pitchSpeed = 60.0f; // deg/sec
const float yawSpeed = 60.0f; // deg/sec

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// =====================================================
// Callback: adjust viewport on window resize
// =====================================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// =====================================================
// Build a rotation matrix from Yaw then Pitch (no roll)
// =====================================================
glm::mat4 getRotationMatrix() {
    glm::mat4 rotY = glm::rotate(glm::mat4(1.0f),
        glm::radians(aircraftYaw),
        glm::vec3(0, 1, 0));
    glm::mat4 rotX = glm::rotate(glm::mat4(1.0f),
        glm::radians(aircraftPitch),
        glm::vec3(1, 0, 0));
    return rotY * rotX;
}

// Extract forward and up vectors
glm::vec3 getForwardVector() {
    glm::mat4 rot = getRotationMatrix();
    return glm::normalize(glm::vec3(rot * glm::vec4(1, 0, 0, 0)));
}
glm::vec3 getUpVector() {
    glm::mat4 rot = getRotationMatrix();
    return glm::normalize(glm::vec3(rot * glm::vec4(0, 1, 0, 0)));
}

// =====================================================
// Process flight input: throttle, pitch, yaw. NO gravity
// =====================================================
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        throttle += 0.5f * deltaTime;
        if (throttle > 1.0f) throttle = 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        throttle -= 0.5f * deltaTime;
        if (throttle < 0.0f) throttle = 0.0f;
    }
    // Pitch: up/down arrow
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        aircraftPitch += pitchSpeed * deltaTime; // tilt nose up
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        aircraftPitch -= pitchSpeed * deltaTime; // tilt nose down
    }
    // Yaw: left/right arrow
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        aircraftYaw += yawSpeed * deltaTime;  // turn left
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        aircraftYaw -= yawSpeed * deltaTime;  // turn right
    }
    // Escape closes
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

// =====================================================
// Shader Utility
// =====================================================
GLuint compileShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSrc, nullptr);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "Vertex Shader Error:\n" << infoLog << "\n";
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSrc, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cout << "Fragment Shader Error:\n" << infoLog << "\n";
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cout << "Shader Linking Error:\n" << infoLog << "\n";
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// =====================================================
// Object Shader for terrain and trees
// =====================================================
const char* objectVertexShaderSrc = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 FragPos;
out vec3 Normal;
void main(){
    FragPos = vec3(model * vec4(aPos,1.0));
    Normal  = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos,1.0);
}
)";

const char* objectFragmentShaderSrc = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;
uniform vec3 objectColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
void main(){
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * objectColor;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * objectColor;
    FragColor = vec4(ambient + diffuse, 1.0);
}
)";

// =====================================================
// Terrain Generation
// =====================================================
const int   terrainResolution = 200;
const float terrainSize = 1000.f;
std::vector<float> terrainVertices;
std::vector<unsigned int> terrainIndices;

float getTerrainHeight(float x, float z) {
    // Keep a wave function or something
    return 50.0f * sin(0.002f * x) * cos(0.002f * z);
}

void generateTerrainMesh() {
    terrainVertices.clear();
    terrainIndices.clear();
    // Build vertex array
    for (int z = 0; z < terrainResolution; z++) {
        for (int x = 0; x < terrainResolution; x++) {
            float relX = (float)x / (terrainResolution - 1);
            float relZ = (float)z / (terrainResolution - 1);
            float worldX = relX * terrainSize - terrainSize * 0.5f;
            float worldZ = relZ * terrainSize - terrainSize * 0.5f;
            float worldY = getTerrainHeight(worldX, worldZ);
            // pos
            terrainVertices.push_back(worldX);
            terrainVertices.push_back(worldY);
            terrainVertices.push_back(worldZ);
            // normal placeholder
            terrainVertices.push_back(0.f);
            terrainVertices.push_back(1.f);
            terrainVertices.push_back(0.f);
        }
    }
    // Build index array
    for (int z = 0; z < terrainResolution - 1; z++) {
        for (int x = 0; x < terrainResolution - 1; x++) {
            int topLeft = z * terrainResolution + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * terrainResolution + x;
            int bottomRight = bottomLeft + 1;
            terrainIndices.push_back(topLeft);
            terrainIndices.push_back(bottomLeft);
            terrainIndices.push_back(topRight);
            terrainIndices.push_back(topRight);
            terrainIndices.push_back(bottomLeft);
            terrainIndices.push_back(bottomRight);
        }
    }
    // Compute approximate normals
    for (int z = 0; z < terrainResolution; z++) {
        for (int x = 0; x < terrainResolution; x++) {
            int idx = (z * terrainResolution + x) * 6;
            float wx = terrainVertices[idx + 0];
            float wz = terrainVertices[idx + 2];
            float hL = getTerrainHeight(wx - 1.f, wz);
            float hR = getTerrainHeight(wx + 1.f, wz);
            float hD = getTerrainHeight(wx, wz - 1.f);
            float hU = getTerrainHeight(wx, wz + 1.f);
            glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.f, hD - hU));
            terrainVertices[idx + 3] = normal.x;
            terrainVertices[idx + 4] = normal.y;
            terrainVertices[idx + 5] = normal.z;
        }
    }
}

// =====================================================
// Simple Cylinder and Cone for Trees
// =====================================================
struct Mesh {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO, VBO, EBO;
};

Mesh createCylinder(float radius, float height, int segments) {
    Mesh mesh;
    for (int i = 0; i <= segments; i++) {
        float theta = i * 2.f * 3.14159f / segments;
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);
        // bottom
        mesh.vertices.insert(mesh.vertices.end(), { x,0.f,z,  x,0.f,z });
        // top
        mesh.vertices.insert(mesh.vertices.end(), { x,height,z, x,0.f,z });
    }
    for (int i = 0; i < segments; i++) {
        int i0 = i * 2;
        int i1 = i0 + 1;
        int i2 = i0 + 2;
        int i3 = i0 + 3;
        mesh.indices.push_back(i0);
        mesh.indices.push_back(i1);
        mesh.indices.push_back(i2);
        mesh.indices.push_back(i2);
        mesh.indices.push_back(i1);
        mesh.indices.push_back(i3);
    }
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return mesh;
}

Mesh createCone(float radius, float height, int segments) {
    Mesh mesh;
    // apex
    mesh.vertices.insert(mesh.vertices.end(), { 0.f,height,0.f,  0.f,1.f,0.f });
    for (int i = 0; i <= segments; i++) {
        float theta = i * 2.f * 3.14159f / segments;
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);
        glm::vec3 n = glm::normalize(glm::vec3(x, radius, z));
        mesh.vertices.insert(mesh.vertices.end(), { x,0.f,z, n.x,n.y,n.z });
    }
    for (int i = 1; i <= segments; i++) {
        mesh.indices.push_back(0);
        mesh.indices.push_back(i);
        mesh.indices.push_back(i + 1);
    }
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return mesh;
}

// =====================================================
// Trees
// =====================================================
struct Tree {
    glm::vec3 position;
    float trunkHeight;
    float trunkRadius;
};

std::vector<Tree> trees;
float randomRange(float minVal, float maxVal) {
    return minVal + (maxVal - minVal) * ((float)std::rand() / RAND_MAX);
}

void generateTrees(int count) {
    trees.clear();
    for (int i = 0; i < count; i++) {
        float x = randomRange(-terrainSize * 0.5f, terrainSize * 0.5f);
        float z = randomRange(-terrainSize * 0.5f, terrainSize * 0.5f);
        float y = getTerrainHeight(x, z);
        trees.push_back({ glm::vec3(x,y,z), 20.f, 2.f });
    }
}

// =====================================================
// Main
// =====================================================
int main() {
    if (!glfwInit()) {
        std::cout << "Failed to init GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "FlightSimNoGravity", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to init GLAD\n";
        return -1;
    }

    // Compile object shader
    GLuint objectShader = compileShaderProgram(objectVertexShaderSrc, objectFragmentShaderSrc);

    // Generate terrain
    generateTerrainMesh();
    GLuint terrainVAO, terrainVBO, terrainEBO;
    glGenVertexArrays(1, &terrainVAO);
    glGenBuffers(1, &terrainVBO);
    glGenBuffers(1, &terrainEBO);

    glBindVertexArray(terrainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, terrainVertices.size() * sizeof(float), terrainVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrainIndices.size() * sizeof(unsigned int), terrainIndices.data(), GL_STATIC_DRAW);

    // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Create trunk & cone
    Mesh trunkMesh = createCylinder(1.f, 20.f, 16);
    Mesh coneMesh = createCone(6.f, 20.f, 16);

    // Generate trees
    generateTrees(50);

    glEnable(GL_DEPTH_TEST);

    // Light
    glm::vec3 lightPos(100.f, 300.f, 100.f);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Physics update (no gravity)
        glm::vec3 forward = getForwardVector();
        glm::vec3 thrust = forward * (throttle * maxThrust);
        velocity += thrust * deltaTime;
        velocity *= drag;
        // No vertical changes => velocity.y remains as is (should be zero)
        // so we stay at the same altitude
        aircraftPos += velocity * deltaTime;

        // Camera
        glm::vec3 camPos = aircraftPos + getUpVector() * 2.f;
        glm::mat4 view = glm::lookAt(camPos, camPos + getForwardVector(), getUpVector());
        glm::mat4 projection = glm::perspective(glm::radians(75.f),
            (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
            0.1f, 5000.f);

        glClearColor(0.53f, 0.81f, 0.92f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(objectShader);
        glUniform3fv(glGetUniformLocation(objectShader, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(objectShader, "viewPos"), 1, glm::value_ptr(camPos));
        glUniformMatrix4fv(glGetUniformLocation(objectShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(objectShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Draw terrain
        glm::mat4 terrainModel(1.f);
        glUniformMatrix4fv(glGetUniformLocation(objectShader, "model"), 1, GL_FALSE, glm::value_ptr(terrainModel));
        glUniform3f(glGetUniformLocation(objectShader, "objectColor"), 0.3f, 0.8f, 0.2f);
        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)terrainIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Draw trees
        for (const auto& tree : trees) {
            // trunk
            glm::mat4 trunkModel(1.f);
            trunkModel = glm::translate(trunkModel, tree.position);
            glUniformMatrix4fv(glGetUniformLocation(objectShader, "model"), 1, GL_FALSE, glm::value_ptr(trunkModel));
            glUniform3f(glGetUniformLocation(objectShader, "objectColor"), 0.55f, 0.27f, 0.07f);
            glBindVertexArray(trunkMesh.VAO);
            glDrawElements(GL_TRIANGLES, (GLsizei)trunkMesh.indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            // foliage
            glm::mat4 coneModel(1.f);
            coneModel = glm::translate(coneModel, tree.position + glm::vec3(0.f, tree.trunkHeight, 0.f));
            glUniformMatrix4fv(glGetUniformLocation(objectShader, "model"), 1, GL_FALSE, glm::value_ptr(coneModel));
            glUniform3f(glGetUniformLocation(objectShader, "objectColor"), 0.f, 0.5f, 0.f);
            glBindVertexArray(coneMesh.VAO);
            glDrawElements(GL_TRIANGLES, (GLsizei)coneMesh.indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
    glDeleteBuffers(1, &terrainEBO);
    glDeleteProgram(objectShader);
    glDeleteVertexArrays(1, &trunkMesh.VAO);
    glDeleteBuffers(1, &trunkMesh.VBO);
    glDeleteBuffers(1, &trunkMesh.EBO);
    glDeleteVertexArrays(1, &coneMesh.VAO);
    glDeleteBuffers(1, &coneMesh.VBO);
    glDeleteBuffers(1, &coneMesh.EBO);

    glfwTerminate();
    return 0;
}
