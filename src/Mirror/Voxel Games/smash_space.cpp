// 
//
// A voxel demo that creates an "infinite" superflat world of voxel cubes.
// Each cube is rendered with a white stone base and a tight black grid overlay.
// The player spawns on a platform so that the collision box’s feet (relative to the eye)
// exactly rest on top of the cubes (i.e. the player’s feet are at y=1).
//
// Movement modes:
//   0 = Normal (standing/walking with sprinting enabled)
//   1 = Prone (on–ground, one block tall, moves slower, sprint disabled)
//   3 = Paragliding (activated when in the air; uses reduced gravity)
// 
// • When on the ground, pressing P toggles between normal and prone.
//   (When going prone, the camera is lowered so that the collision box spans from y = -0.5 to +0.5,
//    meaning the feet (camera.y – 0.5) are exactly at y=1. When returning to normal, the camera is raised back.)
// • When in the air, pressing P activates paraglider mode.
// • Horizontal movement is computed using only yaw (ignoring pitch).
// • In normal mode you can sprint (via left–control) and jump (via SPACE); not in prone or paragliding.
// • When landing from paraglider mode, gravity “lands” you gradually and the mode reverts to normal.
// • A simple ray–cast selects the block you’re looking at and draws a white wireframe outline over it.
// • The projection FOV is set to 103°.
//
// Smash–style attacks:
//   • Left click sets the attack color. On the ground left click gives red and right click gives blue.
//   • In the air, left click uses different colors:
//         • No movement key: neutral aerial → orange (#ff4700)
//         • W held: up aerial → lime (#00ff00)
//         • A or D held: side aerial → purple (#8000FF)
//         • S held: down aerial → magenta (#ff00ff)
//   • The HUD circle then fades back to white over 1 second.
//   • (Movement modifier colors for ground attacks are handled as before.)
//
// Compile with (for example):
//    g++ VoxelFPS_Superflat.cpp -lglfw -ldl -lGL -lX11 -lpthread -lXrandr -lXi
//

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <cmath>

// -------------------- Settings --------------------
const unsigned int WINDOW_WIDTH = 800;    // initial window size (fullscreen later)
const unsigned int WINDOW_HEIGHT = 600;

const float BLOCK_SIZE = 1.0f; // each block is 1 unit

// For the grid overlay, we simulate a virtual 24×24 resolution per face.
const float GRID_TEXTURE_SIZE = 24.0f;
const float GRID_LINE_THICKNESS = 0.02f;

// Render radius (in blocks) around the player for the "infinite" ground.
const int RENDER_RADIUS = 50;

// -------------------- Timing --------------------
float deltaTime = 0.0f, lastFrame = 0.0f;

// -------------------- Player (FPS) State --------------------
// Collision box is defined relative to the camera (eye).
// In normal mode the box spans from (-0.3, -1.6, -0.3) to (0.3, 0.4, 0.3).
// Thus, the player's feet are at cameraPos.y - 1.6.
// We spawn so that feet = 2.6 - 1.6 = 1.0.
glm::vec3 cameraPos(0.0f, 2.6f, 0.0f);
float cameraYaw = -90.0f;
float cameraPitch = 0.0f;
glm::vec3 velocity(0.0f);
bool onGround = false;

// Movement modes:
//   0 = Normal, 1 = Prone, 3 = Paragliding.
int playerMode = 0;
const float moveSpeed = 10.0f;
const float jumpSpeed = 5.0f;
const float gravity = 9.81f;
const float sprintMultiplier = 2.0f;

// -------------------- Mouse Handling --------------------
bool firstMouse = true;
float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;

// -------------------- Global VAO/VBO Handles --------------------
GLuint cubeVAO, cubeVBO, instanceVBO;
GLuint outlineVAO;

// HUD VAO/VBO and shader.
GLuint hudVAO, hudVBO, hudShaderProgram;
int numCircleVertices = 0;

// -------------------- HUD Attack/Fade Variables --------------------
const float hudFadeDuration = 1.0f; // 1 second fade
float hudFadeTimer = hudFadeDuration; // starts finished (white)
glm::vec3 hudAttackColor(1.0f); // base attack color (set on click)
// Movement modifier color (only used for ground attacks split mode)
glm::vec3 hudMoveColor(1.0f);

// -------------------- Callback Functions --------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // y reversed: bottom->top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    cameraYaw += xoffset;
    cameraPitch += yoffset;
    if (cameraPitch > 89.0f)
        cameraPitch = 89.0f;
    if (cameraPitch < -89.0f)
        cameraPitch = -89.0f;
}

// Mouse button callback for attacks.
// For left click:
//   - If on ground: set attack color to red (as before).
//   - If in air: set the color according to movement keys:
//         * W: lime (#00ff00)
//         * S: magenta (#ff00ff)
//         * A or D: purple (#8000FF)
//         * Otherwise: neutral aerial → orange (#ff4700)
// For right click, we use blue (ground or air).
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (!onGround) { // aerial left click
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    hudAttackColor = glm::vec3(0.0f, 1.0f, 0.0f); // lime
                }
                else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    hudAttackColor = glm::vec3(1.0f, 0.0f, 1.0f); // magenta
                }
                else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                    hudAttackColor = glm::vec3(0.502f, 0.0f, 1.0f); // purple (#8000FF)
                }
                else {
                    hudAttackColor = glm::vec3(1.0f, 0.278f, 0.0f); // neutral aerial: orange (#ff4700)
                }
            }
            else {
                hudAttackColor = glm::vec3(1.0f, 0.0f, 0.0f); // ground left click: red
            }
            hudFadeTimer = 0.0f;
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            // For right click, use blue regardless (or you can modify as needed).
            hudAttackColor = glm::vec3(0.0f, 0.0f, 1.0f);
            hudFadeTimer = 0.0f;
        }
    }
}

// -------------------- Collision Box Functions --------------------
glm::vec3 getPlayerBoxMin() {
    if (playerMode == 1)
        return glm::vec3(-0.3f, -0.5f, -0.3f);
    else
        return glm::vec3(-0.3f, -1.6f, -0.3f);
}
glm::vec3 getPlayerBoxMax() {
    if (playerMode == 1)
        return glm::vec3(0.3f, 0.5f, 0.3f);
    else
        return glm::vec3(0.3f, 0.4f, 0.3f);
}

// -------------------- Shader Compilation --------------------
GLuint compileShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSrc, NULL);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex Shader Compilation Error:\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSrc, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment Shader Compilation Error:\n" << infoLog << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Shader Program Linking Error:\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// -------------------- Raycasting for Block Selection --------------------
glm::ivec3 raycastForBlock(bool /*place*/) {
    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front = glm::normalize(front);
    float t = 0.0f;
    while (t < 10.0f) {
        glm::vec3 p = cameraPos + t * front;
        if (p.y >= 0.0f && p.y <= 1.0f) {
            int bx = static_cast<int>(floor(p.x));
            int bz = static_cast<int>(floor(p.z));
            return glm::ivec3(bx, 0, bz);
        }
        t += 0.1f;
    }
    return glm::ivec3(-10000, -10000, -10000);
}

// -------------------- Process Input --------------------
void processInput(GLFWwindow* window) {
    static bool pKeyWasDown = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        if (!pKeyWasDown) {
            if (onGround) {
                if (playerMode == 0) {
                    playerMode = 1;
                    cameraPos.y = 1.5f;
                }
                else if (playerMode == 1) {
                    playerMode = 0;
                    cameraPos.y = 2.6f;
                }
            }
            else {
                if (playerMode != 3)
                    playerMode = 3;
            }
            pKeyWasDown = true;
        }
    }
    else {
        pKeyWasDown = false;
    }

    glm::vec3 frontHoriz = glm::normalize(glm::vec3(cos(glm::radians(cameraYaw)), 0.0f, sin(glm::radians(cameraYaw))));
    glm::vec3 right = glm::normalize(glm::cross(frontHoriz, glm::vec3(0, 1, 0)));

    glm::vec3 moveDir(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        moveDir += frontHoriz;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        moveDir -= frontHoriz;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        moveDir -= right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        moveDir += right;
    if (glm::length(moveDir) > 0.001f)
        moveDir = glm::normalize(moveDir);

    float speed = moveSpeed;
    if (playerMode == 0 && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        speed *= sprintMultiplier;
    if (playerMode == 1)
        speed *= 0.5f;

    glm::vec3 horizVel = moveDir * speed;
    velocity.x = horizVel.x;
    velocity.z = horizVel.z;

    if (playerMode == 0 && onGround && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        velocity.y = jumpSpeed;
        onGround = false;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// -------------------- Handle Collision --------------------
void handleCollision() {
    float feet = cameraPos.y + getPlayerBoxMin().y;
    if (feet < 1.0f) {
        if (playerMode == 3) {
            float targetY = 1.0f - getPlayerBoxMin().y;
            cameraPos.y = glm::mix(cameraPos.y, targetY, 0.1f);
            if (std::abs((cameraPos.y + getPlayerBoxMin().y) - 1.0f) < 0.05f) {
                cameraPos.y = targetY;
                velocity.y = 0.0f;
                onGround = true;
                playerMode = 0;
                cameraPos.y = 2.6f;
            }
        }
        else {
            cameraPos.y = 1.0f - getPlayerBoxMin().y;
            velocity.y = 0.0f;
            onGround = true;
        }
    }
}

// -------------------- Cube Geometry Data --------------------
float cubeVertices[] = {
    // Front face
   -0.5f, -0.5f,  0.5f,    0,0,1,   0.0f, 0.0f,
    0.5f, -0.5f,  0.5f,    0,0,1,   1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,    0,0,1,   1.0f, 1.0f,
    0.5f,  0.5f,  0.5f,    0,0,1,   1.0f, 1.0f,
   -0.5f,  0.5f,  0.5f,    0,0,1,   0.0f, 1.0f,
   -0.5f, -0.5f,  0.5f,    0,0,1,   0.0f, 0.0f,
   // Right face
   0.5f, -0.5f,  0.5f,    1,0,0,   0.0f, 0.0f,
   0.5f, -0.5f, -0.5f,    1,0,0,   1.0f, 0.0f,
   0.5f,  0.5f, -0.5f,    1,0,0,   1.0f, 1.0f,
   0.5f,  0.5f, -0.5f,    1,0,0,   1.0f, 1.0f,
   0.5f,  0.5f,  0.5f,    1,0,0,   0.0f, 1.0f,
   0.5f, -0.5f,  0.5f,    1,0,0,   0.0f, 0.0f,
   // Back face
   0.5f, -0.5f, -0.5f,    0,0,-1,  0.0f, 0.0f,
  -0.5f, -0.5f, -0.5f,    0,0,-1,  1.0f, 0.0f,
  -0.5f,  0.5f, -0.5f,    0,0,-1,  1.0f, 1.0f,
  -0.5f,  0.5f, -0.5f,    0,0,-1,  1.0f, 1.0f,
   0.5f,  0.5f, -0.5f,    0,0,-1,  0.0f, 1.0f,
   0.5f, -0.5f, -0.5f,    0,0,-1,  0.0f, 0.0f,
   // Left face
  -0.5f, -0.5f, -0.5f,   -1,0,0,   0.0f, 0.0f,
  -0.5f, -0.5f,  0.5f,   -1,0,0,   1.0f, 0.0f,
  -0.5f,  0.5f,  0.5f,   -1,0,0,   1.0f, 1.0f,
  -0.5f,  0.5f,  0.5f,   -1,0,0,   1.0f, 1.0f,
  -0.5f,  0.5f, -0.5f,   -1,0,0,   0.0f, 1.0f,
  -0.5f, -0.5f, -0.5f,   -1,0,0,   0.0f, 0.0f,
  // Top face
 -0.5f,  0.5f,  0.5f,    0,1,0,   0.0f, 0.0f,
  0.5f,  0.5f,  0.5f,    0,1,0,   1.0f, 0.0f,
  0.5f,  0.5f, -0.5f,    0,1,0,   1.0f, 1.0f,
  0.5f,  0.5f, -0.5f,    0,1,0,   1.0f, 1.0f,
 -0.5f,  0.5f, -0.5f,    0,1,0,   0.0f, 1.0f,
 -0.5f,  0.5f,  0.5f,    0,1,0,   0.0f, 0.0f,
 // Bottom face
-0.5f, -0.5f, -0.5f,    0,-1,0,  0.0f, 0.0f,
 0.5f, -0.5f, -0.5f,    0,-1,0,  1.0f, 0.0f,
 0.5f, -0.5f,  0.5f,    0,-1,0,  1.0f, 1.0f,
 0.5f, -0.5f,  0.5f,    0,-1,0,  1.0f, 1.0f,
-0.5f, -0.5f,  0.5f,    0,-1,0,  0.0f, 1.0f,
-0.5f, -0.5f, -0.5f,    0,-1,0,  0.0f, 0.0f
};

// -------------------- Instance Data --------------------
std::vector<glm::vec3> getVisibleInstances(const glm::vec3& camPos) {
    std::vector<glm::vec3> offsets;
    int camX = static_cast<int>(floor(camPos.x));
    int camZ = static_cast<int>(floor(camPos.z));
    for (int z = camZ - RENDER_RADIUS; z <= camZ + RENDER_RADIUS; z++) {
        for (int x = camX - RENDER_RADIUS; x <= camX + RENDER_RADIUS; x++) {
            offsets.push_back(glm::vec3(x * BLOCK_SIZE, 0.0f, z * BLOCK_SIZE));
        }
    }
    return offsets;
}

// -------------------- Voxel Shader Sources --------------------
const char* voxelVertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aOffset;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 TexCoord;
void main(){
    vec3 pos = aPos + aOffset;
    gl_Position = projection * view * model * vec4(pos, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* voxelFragmentShaderSrc = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
const float gridSize = 24.0;
const float lineWidth = 0.02;
void main(){
    vec2 f = fract(TexCoord * gridSize);
    if(f.x < lineWidth || f.x > 1.0 - lineWidth ||
       f.y < lineWidth || f.y > 1.0 - lineWidth)
       FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    else
       FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

// -------------------- HUD Shader Sources --------------------
// The HUD vertex shader computes an angle from the center for each vertex.
const char* hudVertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 projection;
uniform mat4 model;
out float angle;
void main(){
    vec4 pos = model * vec4(aPos, 0.0, 1.0);
    gl_Position = projection * pos;
    angle = atan(aPos.y, aPos.x); // range [-pi, pi]
}
)";

// The HUD fragment shader chooses the color based on the angle.
// If useSplit is true, vertices with angle >= 0 use the attack color;
// vertices with angle < 0 use the movement modifier color.
// Both colors fade toward white according to fadeFactor.
const char* hudFragmentShaderSrc = R"(
#version 330 core
in float angle;
out vec4 FragColor;
uniform bool useSplit;
uniform vec3 hudAttackColor;
uniform vec3 hudMoveColor;
uniform float fadeFactor; // 0 = full color, 1 = white
vec3 finalColor() {
    vec3 baseColor;
    if(useSplit) {
        if(angle >= 0.0)
            baseColor = hudAttackColor;
        else
            baseColor = hudMoveColor;
    } else {
        baseColor = hudAttackColor;
    }
    return mix(baseColor, vec3(1.0), fadeFactor);
}
void main(){
    FragColor = vec4(finalColor(), 1.0);
}
)";

// -------------------- Helper Function to Generate HUD Circle Vertices --------------------
std::vector<glm::vec2> generateCircleVertices(float radius, int segments) {
    std::vector<glm::vec2> vertices;
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * 3.14159265f * i / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        vertices.push_back(glm::vec2(x, y));
    }
    return vertices;
}

// -------------------- Main Function --------------------
int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Voxel FPS Superflat", primaryMonitor, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // Compile shaders.
    GLuint voxelShaderProgram = compileShaderProgram(voxelVertexShaderSrc, voxelFragmentShaderSrc);
    hudShaderProgram = compileShaderProgram(hudVertexShaderSrc, hudFragmentShaderSrc);

    // -------------------- Setup Cube Geometry --------------------
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // -------------------- Setup Instance Buffer --------------------
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    glBindVertexArray(0);

    // -------------------- Setup Outline VAO --------------------
    glGenVertexArrays(1, &outlineVAO);
    glBindVertexArray(outlineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // -------------------- Setup HUD Circle --------------------
    const float hudRadius = 0.05f; // in NDC
    const int circleSegments = 32;
    std::vector<glm::vec2> circleVertices = generateCircleVertices(hudRadius, circleSegments);
    numCircleVertices = circleVertices.size();

    glGenVertexArrays(1, &hudVAO);
    glGenBuffers(1, &hudVBO);
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(glm::vec2),
        circleVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // -------------------- Main Render Loop --------------------
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Update fade timer.
        if (hudFadeTimer < hudFadeDuration)
            hudFadeTimer += deltaTime;
        if (hudFadeTimer > hudFadeDuration)
            hudFadeTimer = hudFadeDuration;
        float fadeFactor = hudFadeTimer / hudFadeDuration; // 0 = full attack color, 1 = white

        // For ground attacks, you may set a movement modifier color (split mode) as before.
        bool useSplit = false;
        if (hudFadeTimer < hudFadeDuration && onGround) {
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                hudMoveColor = glm::vec3(0.0f, 1.0f, 1.0f); // cyan
                useSplit = true;
            }
            else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                hudMoveColor = glm::vec3(1.0f, 0.0f, 1.0f); // magenta
                useSplit = true;
            }
            else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                hudMoveColor = glm::vec3(1.0f, 1.0f, 0.0f); // yellow
                useSplit = true;
            }
        }

        // Apply gravity.
        if (!onGround) {
            if (playerMode == 3)
                velocity.y -= (gravity * 0.3f) * deltaTime;
            else
                velocity.y -= gravity * deltaTime;
        }
        cameraPos += velocity * deltaTime;
        handleCollision();

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // sky color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Setup view and projection.
        glm::vec3 front;
        front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front.y = sin(glm::radians(cameraPitch));
        front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front = glm::normalize(front);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::perspective(glm::radians(103.0f),
            (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
            0.1f, 100.0f);

        // Update instance data.
        std::vector<glm::vec3> instanceOffsets = getVisibleInstances(cameraPos);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, instanceOffsets.size() * sizeof(glm::vec3),
            instanceOffsets.data(), GL_DYNAMIC_DRAW);

        // Draw voxel grid.
        glUseProgram(voxelShaderProgram);
        glm::mat4 modelMat = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(voxelShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
        glUniformMatrix4fv(glGetUniformLocation(voxelShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(voxelShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(cubeVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 36, instanceOffsets.size());
        glBindVertexArray(0);

        // Draw block selection outline.
        glm::ivec3 selectedBlock = raycastForBlock(false);
        if (selectedBlock.x != -10000) {
            glm::mat4 outlineMat = glm::translate(glm::mat4(1.0f),
                glm::vec3(selectedBlock.x * BLOCK_SIZE, 0.0f, selectedBlock.z * BLOCK_SIZE));
            outlineMat = glm::scale(outlineMat, glm::vec3(1.05f));
            glUseProgram(voxelShaderProgram);
            glUniformMatrix4fv(glGetUniformLocation(voxelShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(outlineMat));
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(2.0f);
            glBindVertexArray(outlineVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glBindVertexArray(0);
        }

        // Draw HUD circle.
        glDisable(GL_DEPTH_TEST);
        glUseProgram(hudShaderProgram);
        glm::mat4 hudProjection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
        glm::mat4 hudModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(hudShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(hudProjection));
        glUniformMatrix4fv(glGetUniformLocation(hudShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(hudModel));
        glUniform1f(glGetUniformLocation(hudShaderProgram, "fadeFactor"), fadeFactor);
        glUniform1i(glGetUniformLocation(hudShaderProgram, "useSplit"), useSplit);
        glUniform3fv(glGetUniformLocation(hudShaderProgram, "hudAttackColor"), 1, glm::value_ptr(hudAttackColor));
        glUniform3fv(glGetUniformLocation(hudShaderProgram, "hudMoveColor"), 1, glm::value_ptr(hudMoveColor));
        glBindVertexArray(hudVAO);
        glLineWidth(2.0f);
        glDrawArrays(GL_LINE_LOOP, 0, numCircleVertices);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup.
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteVertexArrays(1, &outlineVAO);
    glDeleteProgram(voxelShaderProgram);
    glDeleteVertexArrays(1, &hudVAO);
    glDeleteBuffers(1, &hudVBO);
    glDeleteProgram(hudShaderProgram);

    glfwTerminate();
    return 0;
}
