// main.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM for math:
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <deque>
#include <memory>
#include <cmath>

// Define our own max function instead of std::max
template <typename T>
inline T max_value(const T& a, const T& b) {
    return (a > b) ? a : b;
}

// --------------------- SHADER CLASS ---------------------
class Shader {
public:
    unsigned int ID;
    Shader(const char* vertexSrc, const char* fragmentSrc) {
        // Vertex shader
        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexSrc, nullptr);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // Fragment shader
        unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentSrc, nullptr);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // Shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
    void use() { glUseProgram(ID); }
    // Uniform setters:
    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()),
            1, GL_FALSE, glm::value_ptr(mat));
    }
    void setVec4(const std::string& name, const glm::vec4& vec) const {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
    }
    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
private:
    void checkCompileErrors(unsigned int shader, const std::string& type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "ERROR::SHADER_COMPILATION_ERROR (" << type << "):\n"
                    << infoLog << "\n";
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                std::cerr << "ERROR::PROGRAM_LINKING_ERROR (" << type << "):\n"
                    << infoLog << "\n";
            }
        }
    }
};

// --------------------- VISUAL MODE INTERFACE ---------------------
class VisualMode {
public:
    virtual void update(float dt) = 0;
    virtual void render(const glm::mat4& projection) = 0;
    // Each mode can override onKey to adjust parameters
    virtual void onKey(int key, int action) {}
    virtual ~VisualMode() {}
};

// --------------------- MODE 1: SHAPE GENERATOR MODE ---------------------
// A moving circle that leaves a fading tail.
// Adjustable parameters:
//   - Tail length (Q/A)
//   - Movement speed multiplier (W/S)
class ShapeGeneratorMode : public VisualMode {
public:
    ShapeGeneratorMode()
        : currentPos(0.0f, 0.0f), tailMaxLength(50), timeAccum(0.0f), speedMultiplier(1.0f)
    {
        // Build shader
        const char* vertexSrc = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            uniform mat4 uModel;
            uniform mat4 uProjection;
            void main(){
                gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
            }
        )";
        const char* fragmentSrc = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec4 uColor;
            void main(){
                FragColor = uColor;
            }
        )";
        shader = std::make_unique<Shader>(vertexSrc, fragmentSrc);
        // Generate circle geometry (triangle fan)
        const int segments = 64;
        std::vector<float> circleVerts;
        circleVerts.push_back(0.0f); circleVerts.push_back(0.0f); // center
        const float radius = 0.15f;
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * 3.1415926f * i / segments;
            circleVerts.push_back(radius * cosf(angle));
            circleVerts.push_back(radius * sinf(angle));
        }
        vertexCount = segments + 2;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, circleVerts.size() * sizeof(float), circleVerts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
    ~ShapeGeneratorMode() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    void update(float dt) override {
        timeAccum += dt * speedMultiplier;
        // Sine-cosine motion scaled by speedMultiplier
        currentPos.x = 0.6f * cosf(timeAccum * 0.8f);
        currentPos.y = 0.6f * sinf(timeAccum * 1.1f);
        tail.push_back(currentPos);
        if (tail.size() > tailMaxLength)
            tail.pop_front();
    }
    void render(const glm::mat4& projection) override {
        shader->use();
        shader->setMat4("uProjection", projection);
        // Draw tail (older segments faded)
        int idx = 0;
        for (const auto& pos : tail) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos, 0.0f));
            model = glm::rotate(model, timeAccum * 0.2f, glm::vec3(0, 0, 1));
            shader->setMat4("uModel", model);
            float fade = float(idx) / float(tail.size());
            shader->setVec4("uColor", glm::vec4(0, 0, 0, 1.0f - fade * 0.8f));
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);
            idx++;
        }
        // Draw current shape solid
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(currentPos, 0.0f));
        model = glm::rotate(model, timeAccum, glm::vec3(0, 0, 1));
        shader->setMat4("uModel", model);
        shader->setVec4("uColor", glm::vec4(0, 0, 0, 1));
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);
        glBindVertexArray(0);
    }
    // Adjust parameters via keys:
    void onKey(int key, int action) override {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            if (key == GLFW_KEY_Q) {
                tailMaxLength += 5;
                std::cout << "Tail Length: " << tailMaxLength << "\n";
            }
            if (key == GLFW_KEY_A) {
                // Cast literal to size_t for proper matching:
                tailMaxLength = max_value(static_cast<size_t>(5), tailMaxLength - 5);
                std::cout << "Tail Length: " << tailMaxLength << "\n";
            }
            if (key == GLFW_KEY_W) {
                speedMultiplier += 0.1f;
                std::cout << "Speed Multiplier: " << speedMultiplier << "\n";
            }
            if (key == GLFW_KEY_S) {
                speedMultiplier = max_value(0.1f, speedMultiplier - 0.1f);
                std::cout << "Speed Multiplier: " << speedMultiplier << "\n";
            }
        }
    }
private:
    std::unique_ptr<Shader> shader;
    unsigned int VAO, VBO;
    int vertexCount;
    glm::vec2 currentPos;
    std::deque<glm::vec2> tail;
    size_t tailMaxLength;
    float timeAccum;
    float speedMultiplier;
};

// --------------------- MODE 2: SWIRL MODE ---------------------
// A grid of rotating circles.
// Adjustable parameters:
//   - Grid resolution (Page Up/Page Down)
//   - Rotation speed (Up/Down arrows)
//   - Shape scale (Left/Right arrows)
class SwirlMode : public VisualMode {
public:
    SwirlMode()
        : timeAccum(0.0f), gridX(5), gridY(5), rotationSpeed(1.0f), shapeScale(0.1f)
    {
        // Vertex shader with adjustable scale
        const char* vertexSrc = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            uniform mat4 uModel;
            uniform mat4 uProjection;
            uniform float uScale;
            void main(){
                gl_Position = uProjection * uModel * vec4(aPos * uScale, 0.0, 1.0);
            }
        )";
        const char* fragmentSrc = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec4 uColor;
            void main(){
                FragColor = uColor;
            }
        )";
        shader = std::make_unique<Shader>(vertexSrc, fragmentSrc);
        // Create a circle shape for the grid:
        const int segments = 64;
        std::vector<float> circleVerts;
        circleVerts.push_back(0.0f); circleVerts.push_back(0.0f);
        const float baseRadius = 1.0f; // unit circle; will be scaled by uScale
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * 3.1415926f * i / segments;
            circleVerts.push_back(baseRadius * cosf(angle));
            circleVerts.push_back(baseRadius * sinf(angle));
        }
        vertexCount = segments + 2;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, circleVerts.size() * sizeof(float), circleVerts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
    ~SwirlMode() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    void update(float dt) override {
        timeAccum += dt;
    }
    void render(const glm::mat4& projection) override {
        shader->use();
        shader->setMat4("uProjection", projection);
        // Loop over a grid
        for (int i = 0; i < gridX; ++i) {
            for (int j = 0; j < gridY; ++j) {
                float x = -0.8f + (1.6f * i) / (gridX - 1);
                float y = -0.8f + (1.6f * j) / (gridY - 1);
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
                float angle = timeAccum * rotationSpeed + (i + j) * 0.3f;
                model = glm::rotate(model, angle, glm::vec3(0, 0, 1));
                shader->setMat4("uModel", model);
                shader->setFloat("uScale", shapeScale);
                shader->setVec4("uColor", glm::vec4(0, 0, 0, 1));
                glBindVertexArray(VAO);
                glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);
            }
        }
        glBindVertexArray(0);
    }
    // Adjust parameters:
    void onKey(int key, int action) override {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            if (key == GLFW_KEY_PAGE_UP) {
                gridX++; gridY++;
                std::cout << "Grid: " << gridX << " x " << gridY << "\n";
            }
            if (key == GLFW_KEY_PAGE_DOWN) {
                gridX = (gridX - 1 < 2) ? 2 : gridX - 1;
                gridY = (gridY - 1 < 2) ? 2 : gridY - 1;
                std::cout << "Grid: " << gridX << " x " << gridY << "\n";
            }
            if (key == GLFW_KEY_UP) {
                rotationSpeed += 0.1f;
                std::cout << "Rotation Speed: " << rotationSpeed << "\n";
            }
            if (key == GLFW_KEY_DOWN) {
                rotationSpeed = max_value(0.1f, rotationSpeed - 0.1f);
                std::cout << "Rotation Speed: " << rotationSpeed << "\n";
            }
            if (key == GLFW_KEY_RIGHT) {
                shapeScale += 0.01f;
                std::cout << "Shape Scale: " << shapeScale << "\n";
            }
            if (key == GLFW_KEY_LEFT) {
                shapeScale = max_value(0.01f, shapeScale - 0.01f);
                std::cout << "Shape Scale: " << shapeScale << "\n";
            }
        }
    }
private:
    std::unique_ptr<Shader> shader;
    unsigned int VAO, VBO;
    int vertexCount;
    float timeAccum;
    int gridX, gridY;
    float rotationSpeed;
    float shapeScale;
};

// --------------------- MODE 3: FRACTAL DISPLACEMENT MODE ---------------------
// A grid mesh deformed by fractal noise.
// Adjustable parameters:
//   - Noise scale (Left/Right arrows)
//   - Displacement amplitude (Up/Down arrows)
class FractalDisplacementMode : public VisualMode {
public:
    FractalDisplacementMode()
        : timeAccum(0.0f), noiseScale(3.0f), dispAmplitude(0.6f)
    {
        // Vertex shader with extra uniforms for noise scale and displacement amplitude
        const char* vertexSrc = R"(
            #version 330 core
            layout(location = 0) in vec2 aPos;
            uniform float uTime;
            uniform mat4 uProjection;
            uniform mat4 uModel;
            uniform float uNoiseScale;
            uniform float uDispAmp;
            // Simple hash function
            float hash(vec2 p) {
                return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
            }
            // Basic 2D noise
            float noise(vec2 p) {
                vec2 i = floor(p);
                vec2 f = fract(p);
                float a = hash(i);
                float b = hash(i + vec2(1.0, 0.0));
                float c = hash(i + vec2(0.0, 1.0));
                float d = hash(i + vec2(1.0, 1.0));
                vec2 u = f * f * (3.0 - 2.0 * f);
                return mix(a, b, u.x) + (c - a)*u.y*(1.0 - u.x) + (d - b)*u.x*u.y;
            }
            // Fractal noise summing multiple octaves
            float fractalNoise(vec2 p) {
                float total = 0.0;
                float amplitude = 1.0;
                float frequency = 1.0;
                for(int i = 0; i < 4; i++){
                    total += noise(p * frequency) * amplitude;
                    frequency *= 2.0;
                    amplitude *= 0.5;
                }
                return total;
            }
            void main(){
                vec2 pos = aPos;
                float displacement = fractalNoise(pos * uNoiseScale + uTime * 0.2);
                displacement = (displacement - 0.5) * uDispAmp;
                vec4 worldPos = uModel * vec4(pos, displacement, 1.0);
                gl_Position = uProjection * worldPos;
            }
        )";
        const char* fragmentSrc = R"(
            #version 330 core
            out vec4 FragColor;
            void main(){
                FragColor = vec4(0,0,0,1);
            }
        )";
        shader = std::make_unique<Shader>(vertexSrc, fragmentSrc);
        // Generate grid vertices over [-1, 1] x [-1, 1]
        const int gridSize = 50;
        for (int j = 0; j <= gridSize; j++) {
            for (int i = 0; i <= gridSize; i++) {
                float x = -1.0f + 2.0f * i / gridSize;
                float y = -1.0f + 2.0f * j / gridSize;
                vertices.push_back(x);
                vertices.push_back(y);
            }
        }
        // Build indices for triangles
        for (int j = 0; j < gridSize; j++) {
            for (int i = 0; i < gridSize; i++) {
                int row1 = j * (gridSize + 1);
                int row2 = (j + 1) * (gridSize + 1);
                indices.push_back(row1 + i);
                indices.push_back(row2 + i);
                indices.push_back(row1 + i + 1);

                indices.push_back(row1 + i + 1);
                indices.push_back(row2 + i);
                indices.push_back(row2 + i + 1);
            }
        }
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
    ~FractalDisplacementMode() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
    void update(float dt) override {
        timeAccum += dt;
    }
    void render(const glm::mat4& projection) override {
        shader->use();
        shader->setMat4("uProjection", projection);
        glm::mat4 model = glm::mat4(1.0f);
        shader->setMat4("uModel", model);
        shader->setFloat("uTime", timeAccum);
        shader->setFloat("uNoiseScale", noiseScale);
        shader->setFloat("uDispAmp", dispAmplitude);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    // Adjust parameters:
    void onKey(int key, int action) override {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            // Left/Right to adjust noiseScale
            if (key == GLFW_KEY_LEFT) {
                noiseScale = max_value(0.1f, noiseScale - 0.1f);
                std::cout << "Noise Scale: " << noiseScale << "\n";
            }
            if (key == GLFW_KEY_RIGHT) {
                noiseScale += 0.1f;
                std::cout << "Noise Scale: " << noiseScale << "\n";
            }
            // Up/Down to adjust displacement amplitude
            if (key == GLFW_KEY_UP) {
                dispAmplitude += 0.05f;
                std::cout << "Disp Amplitude: " << dispAmplitude << "\n";
            }
            if (key == GLFW_KEY_DOWN) {
                dispAmplitude = max_value(0.05f, dispAmplitude - 0.05f);
                std::cout << "Disp Amplitude: " << dispAmplitude << "\n";
            }
        }
    }
private:
    std::unique_ptr<Shader> shader;
    unsigned int VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    float timeAccum;
    float noiseScale;
    float dispAmplitude;
};

// --------------------- ENGINE ---------------------
class Engine {
public:
    Engine(GLFWwindow* win)
        : window(win), currentModeIndex(0)
    {
        modes.push_back(std::make_unique<ShapeGeneratorMode>());
        modes.push_back(std::make_unique<SwirlMode>());
        modes.push_back(std::make_unique<FractalDisplacementMode>());
    }
    void processInput(int key, int action) {
        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_1)
                currentModeIndex = 0;
            else if (key == GLFW_KEY_2)
                currentModeIndex = 1;
            else if (key == GLFW_KEY_3)
                currentModeIndex = 2;
        }
        modes[currentModeIndex]->onKey(key, action);
    }
    void update(float dt) {
        modes[currentModeIndex]->update(dt);
    }
    void render(const glm::mat4& projection) {
        modes[currentModeIndex]->render(projection);
    }
private:
    GLFWwindow* window;
    std::vector<std::unique_ptr<VisualMode>> modes;
    int currentModeIndex;
};

Engine* gEngine = nullptr;
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (gEngine)
        gEngine->processInput(key, action);
}

// --------------------- MAIN ---------------------
int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    // OpenGL 3.3 Core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    GLFWwindow* window = glfwCreateWindow(800, 600, "Interactive VFX Engine", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
    // Enable blending (for transparency effects)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 2D orthographic projection
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);

    Engine engine(window);
    gEngine = &engine;

    float lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        glfwPollEvents();
        engine.update(dt);

        // Clear white background for high-contrast visuals
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        engine.render(projection);
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}
