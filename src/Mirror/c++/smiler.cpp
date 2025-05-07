// main.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM for matrix math:
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <deque>
#include <memory>
#include <cmath>

// --------------------- SHADER CLASS ---------------------
class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexSrc, const char* fragmentSrc) {
        // Vertex shader
        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexSrc, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Fragment shader
        unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentSrc, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // Shader program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // Delete shaders after linking
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() { glUseProgram(ID); }

    // Utility uniform functions
    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()),
            1, GL_FALSE, glm::value_ptr(mat));
    }
    void setVec4(const std::string& name, const glm::vec4& vec) const {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(vec));
    }

private:
    void checkCompileErrors(unsigned int shader, const std::string& type) {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "ERROR::SHADER_COMPILATION_ERROR (" << type << "):\n"
                    << infoLog << "\n";
            }
        }
        else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
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
    virtual void onKey(int key, int action) {}
    virtual ~VisualMode() {}
};

// --------------------- SHAPE GENERATOR MODE ---------------------
// This mode draws one large circle that moves in a sine–cosine pattern and leaves a tail.
class ShapeGeneratorMode : public VisualMode {
public:
    ShapeGeneratorMode()
        : currentPos(0.0f, 0.0f), tailMaxLength(50), timeAccum(0.0f)
    {
        // Build shader: simple transformation-based shader.
        const char* vertexShaderSrc = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            uniform mat4 uModel;
            uniform mat4 uProjection;
            void main() {
                gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
            }
        )";

        const char* fragmentShaderSrc = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec4 uColor;
            void main() {
                FragColor = uColor;
            }
        )";

        shader = std::make_unique<Shader>(vertexShaderSrc, fragmentShaderSrc);

        // Generate circle geometry (triangle fan)
        const int segments = 64;
        std::vector<float> circleVertices;
        // Center point
        circleVertices.push_back(0.0f);
        circleVertices.push_back(0.0f);
        const float radius = 0.15f;
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * 3.1415926f * float(i) / float(segments);
            float x = radius * cosf(angle);
            float y = radius * sinf(angle);
            circleVertices.push_back(x);
            circleVertices.push_back(y);
        }
        vertexCount = segments + 2;

        // Setup VAO/VBO for circle shape
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float),
            circleVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
            (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    ~ShapeGeneratorMode() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void update(float dt) override {
        timeAccum += dt;
        // Update current position along a sine-cosine curve.
        currentPos.x = 0.6f * cosf(timeAccum * 0.8f);
        currentPos.y = 0.6f * sinf(timeAccum * 1.1f);

        // Record tail (store the latest position)
        tail.push_back(currentPos);
        if (tail.size() > tailMaxLength)
            tail.pop_front();
    }

    void render(const glm::mat4& projection) override {
        shader->use();
        shader->setMat4("uProjection", projection);

        // Draw tail shapes from oldest (faded) to newest
        int index = 0;
        for (const auto& pos : tail) {
            // Compute a model matrix for this tail segment
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos, 0.0f));
            // Optional: add rotation over time for extra style
            model = glm::rotate(model, timeAccum * 0.2f, glm::vec3(0.0f, 0.0f, 1.0f));
            shader->setMat4("uModel", model);

            // Compute a fading factor: older segments are lighter
            float fade = float(index) / float(tail.size());
            // Render in black with varying intensity (or invert: black tail on white bg)
            glm::vec4 color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f - fade * 0.8f);
            shader->setVec4("uColor", color);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);
            ++index;
        }

        // Draw the current shape (full black, no fade)
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(currentPos, 0.0f));
        model = glm::rotate(model, timeAccum, glm::vec3(0.0f, 0.0f, 1.0f));
        shader->setMat4("uModel", model);
        shader->setVec4("uColor", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);
        glBindVertexArray(0);
    }

private:
    std::unique_ptr<Shader> shader;
    unsigned int VAO, VBO;
    int vertexCount;

    glm::vec2 currentPos;
    std::deque<glm::vec2> tail;
    const size_t tailMaxLength;
    float timeAccum;
};

// --------------------- SWIRL MODE ---------------------
// This mode renders a grid of large shapes that rotate and “swirl” in place.
class SwirlMode : public VisualMode {
public:
    SwirlMode()
        : timeAccum(0.0f)
    {
        // Use same shader as before.
        const char* vertexShaderSrc = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            uniform mat4 uModel;
            uniform mat4 uProjection;
            void main() {
                gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
            }
        )";

        const char* fragmentShaderSrc = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec4 uColor;
            void main() {
                FragColor = uColor;
            }
        )";

        shader = std::make_unique<Shader>(vertexShaderSrc, fragmentShaderSrc);

        // Create a square shape (or circle) for demonstration.
        // Here we create a simple circle as before.
        const int segments = 64;
        std::vector<float> circleVertices;
        circleVertices.push_back(0.0f); // center
        circleVertices.push_back(0.0f);
        const float radius = 0.1f;
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * 3.1415926f * float(i) / float(segments);
            circleVertices.push_back(radius * cosf(angle));
            circleVertices.push_back(radius * sinf(angle));
        }
        vertexCount = segments + 2;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float),
            circleVertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
            (void*)0);
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

        // Render a grid of shapes
        const int gridX = 5, gridY = 5;
        for (int i = 0; i < gridX; ++i) {
            for (int j = 0; j < gridY; ++j) {
                // Map grid coordinates to [-0.8, 0.8]
                float x = -0.8f + (1.6f * i) / (gridX - 1);
                float y = -0.8f + (1.6f * j) / (gridY - 1);

                glm::mat4 model = glm::translate(glm::mat4(1.0f),
                    glm::vec3(x, y, 0.0f));
                // Each shape rotates with a phase offset based on grid position
                float angle = timeAccum + (i + j) * 0.3f;
                model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
                shader->setMat4("uModel", model);

                // Render in black
                shader->setVec4("uColor", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                glBindVertexArray(VAO);
                glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);
            }
        }
        glBindVertexArray(0);
    }

private:
    std::unique_ptr<Shader> shader;
    unsigned int VAO, VBO;
    int vertexCount;

    float timeAccum;
};

// --------------------- ENGINE ---------------------
class Engine {
public:
    Engine(GLFWwindow* win)
        : window(win), currentModeIndex(0)
    {
        // Create two modes
        modes.push_back(std::make_unique<ShapeGeneratorMode>());
        modes.push_back(std::make_unique<SwirlMode>());
    }

    void processInput(int key, int action) {
        // Switch modes with keys 1 and 2:
        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_1) {
                currentModeIndex = 0;
            }
            else if (key == GLFW_KEY_2) {
                currentModeIndex = 1;
            }
        }
        // Pass key event to current mode
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

// --------------------- CALLBACKS ---------------------
Engine* gEngine = nullptr;
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (gEngine)
        gEngine->processInput(key, action);
}

// --------------------- MAIN ---------------------
int main()
{
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Set OpenGL version to 3.3 (Core Profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create Window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Creative VFX Engine", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set key callback
    glfwSetKeyCallback(window, keyCallback);

    // Load GLAD (using gladLoadGLLoader)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // Enable blending for tail fading
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create an orthographic projection for 2D drawing
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);

    // Create Engine and set global pointer for callbacks
    Engine engine(window);
    gEngine = &engine;

    // Timing variables
    float lastTime = glfwGetTime();

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Calculate delta time
        float currentTime = glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        // Process events
        glfwPollEvents();

        // Update current mode
        engine.update(dt);

        // Clear screen (white background)
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render current mode
        engine.render(projection);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
