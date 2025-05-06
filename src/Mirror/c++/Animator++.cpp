#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <thread>
#include <chrono>
#include <sstream>

// Fixed canvas dimensions.
const int WIDTH = 800;
const int HEIGHT = 600;

// Global pixel buffer for the current frame; initially all white.
std::vector<uint8_t> pixelBuffer(WIDTH * HEIGHT * 4, 255);

// Global flip-book: each frame is stored as a pixel buffer.
std::vector< std::vector<uint8_t> > frames;
int currentFrameIndex = 0;

// Undo stack.
std::vector< std::vector<uint8_t> > undoStack;

// Brush settings.
int brushSize = 10;

// Limited palette: 12 colors.
struct Color { uint8_t r, g, b; };
std::vector<Color> palette = {
    {0xff, 0x00, 0xff}, // Magenta
    {0x80, 0x00, 0xff}, // Violet
    {0x80, 0x80, 0xff}, // Indigo
    {0x00, 0x80, 0xff}, // Cerulean
    {0x00, 0x80, 0x80}, // Teal  
    {0x00, 0x80, 0x00}, // Green 
    {0x80, 0x80, 0x00}, // Olive 
    {0xff, 0x80, 0x00}, // Orange 
    {0xff, 0x00, 0x00}, // Red 
    {0xff, 0x00, 0x80}, // Pink 
    {0x00, 0x00, 0x00}, // Black
    {0xff, 0xff, 0xff}  // White
};
// Current pencil color (default from palette[0]).
uint8_t currentPencilR = palette[0].r;
uint8_t currentPencilG = palette[0].g;
uint8_t currentPencilB = palette[0].b;

// Eraser always draws white.
const uint8_t eraserR = 255, eraserG = 255, eraserB = 255;

// Tool mode.
enum class Tool { PENCIL, ERASER };
Tool currentTool = Tool::PENCIL;

// Playhead control modes.
enum class PlayheadMode { DIRECT, RC };
PlayheadMode playheadMode = PlayheadMode::DIRECT;

// Playhead position in canvas coordinates.
float playheadX = WIDTH / 2.0f;
float playheadY = HEIGHT / 2.0f;
// For RC mode, store velocity.
float playheadVelX = 0.0f, playheadVelY = 0.0f;

// Color selection mode.
bool colorSelectMode = false;
int currentPaletteIndex = 0;

// Global flag for playback.
bool isPlaying = false;
int playbackFrameIndex = 0;

// Global canvas texture ID.
unsigned int canvasTex = 0;

// Utility: Clamp an integer between min and max.
int clampInt(int val, int minVal, int maxVal) {
    return std::max(minVal, std::min(val, maxVal));
}

// Fade the given frame by blending each channel halfway toward white.
std::vector<uint8_t> fadeFrame(const std::vector<uint8_t>& frame) {
    std::vector<uint8_t> faded(frame.size());
    for (size_t i = 0; i < frame.size(); i += 4) {
        faded[i+0] = (frame[i+0] + 255) / 2;
        faded[i+1] = (frame[i+1] + 255) / 2;
        faded[i+2] = (frame[i+2] + 255) / 2;
        faded[i+3] = 255;
    }
    return faded;
}

// Draw a filled circle into the pixel buffer at (cx, cy) with the given color.
void drawCircleOnBuffer(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b) {
    int xStart = cx - radius;
    int xEnd   = cx + radius;
    int yStart = cy - radius;
    int yEnd   = cy + radius;
    for (int y = yStart; y <= yEnd; ++y) {
        for (int x = xStart; x <= xEnd; ++x) {
            int dx = x - cx;
            int dy = y - cy;
            if (dx*dx + dy*dy <= radius*radius) {
                int px = clampInt(x, 0, WIDTH - 1);
                int py = clampInt(y, 0, HEIGHT - 1);
                int idx = (py * WIDTH + px) * 4;
                pixelBuffer[idx+0] = r;
                pixelBuffer[idx+1] = g;
                pixelBuffer[idx+2] = b;
                pixelBuffer[idx+3] = 255;
            }
        }
    }
}

// Update the canvas texture from the pixel buffer.
void updateCanvasTexture(unsigned int tex) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer.data());
}

// --- Key Callback ---
// This function is defined in global scope.
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Process color selection mode exclusively.
    if (colorSelectMode) {
        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_LEFT)
                currentPaletteIndex = (currentPaletteIndex + palette.size() - 1) % palette.size();
            else if (key == GLFW_KEY_RIGHT)
                currentPaletteIndex = (currentPaletteIndex + 1) % palette.size();
            else if (key == GLFW_KEY_SPACE) {
                Color chosen = palette[currentPaletteIndex];
                currentPencilR = chosen.r;
                currentPencilG = chosen.g;
                currentPencilB = chosen.b;
                colorSelectMode = false;
            }
        }
        return;
    }
    
    if (action == GLFW_PRESS) {
        // Frame navigation:
        if (key == GLFW_KEY_RIGHT_BRACKET) { // ]
            std::vector<uint8_t> faded = fadeFrame(pixelBuffer);
            frames.push_back(faded);
            currentFrameIndex = frames.size() - 1;
            pixelBuffer = frames[currentFrameIndex];
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        } else if (key == GLFW_KEY_LEFT_BRACKET) { // [
            if (currentFrameIndex > 0) {
                currentFrameIndex--;
                pixelBuffer = frames[currentFrameIndex];
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        // Playback: Q plays from the very first frame.
        else if (key == GLFW_KEY_Q) {
            std::cout << "Playback starting from frame 0..." << std::endl;
            // Save current frame index (if needed).
            for (int i = 0; i < (int)frames.size(); i++) {
                pixelBuffer = frames[i];
                updateCanvasTexture(canvasTex);
                glfwSwapBuffers(window);
                glfwPollEvents();
                std::this_thread::sleep_for(std::chrono::milliseconds(42));
            }
            std::cout << "Playback finished." << std::endl;
            currentFrameIndex = frames.size() - 1;
        }
        // Toggle playhead mode with C.
        else if (key == GLFW_KEY_C) {
            if (playheadMode == PlayheadMode::DIRECT) {
                playheadMode = PlayheadMode::RC;
                playheadVelX = 0.0f;
                playheadVelY = 0.0f;
            } else {
                playheadMode = PlayheadMode::DIRECT;
            }
        }
    }
    
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_E)
            currentTool = Tool::ERASER;
        else if (key == GLFW_KEY_P)
            currentTool = Tool::PENCIL;
        else if (key == GLFW_KEY_U) {
            if (!undoStack.empty()) {
                pixelBuffer = undoStack.back();
                undoStack.pop_back();
            }
        }
        // In DIRECT mode, arrow keys reposition the playhead.
        if (playheadMode == PlayheadMode::DIRECT) {
            if (key == GLFW_KEY_UP) { playheadY += 5.0f; }
            if (key == GLFW_KEY_DOWN) { playheadY -= 5.0f; }
            if (key == GLFW_KEY_LEFT) { playheadX -= 5.0f; }
            if (key == GLFW_KEY_RIGHT) { playheadX += 5.0f; }
        }
        // Clamp playhead.
        playheadX = std::max(0.0f, std::min(playheadX, (float)WIDTH));
        playheadY = std::max(0.0f, std::min(playheadY, (float)HEIGHT));
    }
    
    // Update window title with frame info.
    {
        std::ostringstream oss;
        oss << "Drawing App - Frame " << currentFrameIndex << " / " << frames.size();
        glfwSetWindowTitle(window, oss.str().c_str());
    }
}

// --- Shader Sources ---
const char* quadVertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main(){
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* quadFragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D canvasTexture;
void main(){
    FragColor = texture(canvasTexture, TexCoord);
}
)";

const char* overlayVertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
void main(){
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* overlayFragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 overlayColor;
void main(){
    FragColor = overlayColor;
}
)";

unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
         glGetShaderInfoLog(shader, 512, nullptr, infoLog);
         std::cerr << "Shader compilation error: " << infoLog << std::endl;
    }
    return shader;
}

unsigned int createShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
         glGetProgramInfoLog(program, 512, nullptr, infoLog);
         std::cerr << "Shader linking error: " << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// Global VAOs and shader IDs.
unsigned int quadVAO, quadVBO;
unsigned int quadShader;
unsigned int overlayVAO, overlayVBO;
unsigned int overlayShader;

void setupQuad() {
    float quadVertices[] = {
        // positions      // texCoords
        -1.0f,  1.0f,     0.0f, 1.0f,
        -1.0f, -1.0f,     0.0f, 0.0f,
         1.0f, -1.0f,     1.0f, 0.0f,
        
        -1.0f,  1.0f,     0.0f, 1.0f,
         1.0f, -1.0f,     1.0f, 0.0f,
         1.0f,  1.0f,     1.0f, 1.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    quadShader = createShaderProgram(quadVertexShaderSrc, quadFragmentShaderSrc);
}

void setupOverlay() {
    glGenVertexArrays(1, &overlayVAO);
    glGenBuffers(1, &overlayVBO);
    overlayShader = createShaderProgram(overlayVertexShaderSrc, overlayFragmentShaderSrc);
}

// Draw the playhead crosshair.
void drawPlayheadOverlay() {
    float ndcX = (playheadX / (WIDTH / 2.0f)) - 1.0f;
    float ndcY = (playheadY / (HEIGHT / 2.0f)) - 1.0f;
    float crossSize = 0.02f;
    float crossVertices[] = {
        ndcX - crossSize, ndcY,  ndcX + crossSize, ndcY,
        ndcX, ndcY - crossSize,  ndcX, ndcY + crossSize
    };
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crossVertices), crossVertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glUseProgram(overlayShader);
    int colorLoc = glGetUniformLocation(overlayShader, "overlayColor");
    glUniform4f(colorLoc, 0.0f, 1.0f, 0.0f, 1.0f);
    glBindVertexArray(overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINES, 0, 4);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Draw the palette overlay.
void drawPaletteOverlay() {
    const int squareSize = 50;
    const int spacing = 10;
    int totalWidth = palette.size() * squareSize + (palette.size() - 1) * spacing;
    int startX = (WIDTH - totalWidth) / 2;
    int yPos = 20;
    
    std::vector<float> vertices;
    for (size_t i = 0; i < palette.size(); i++) {
        int x = startX + i * (squareSize + spacing);
        int y = yPos;
        float x0 = (x / (WIDTH / 2.0f)) - 1.0f;
        float y0 = (y / (HEIGHT / 2.0f)) - 1.0f;
        float x1 = ((x + squareSize) / (WIDTH / 2.0f)) - 1.0f;
        float y1 = (((y + squareSize)) / (HEIGHT / 2.0f)) - 1.0f;
        vertices.insert(vertices.end(), { x0, y0, x1, y0, x1, y1 });
        vertices.insert(vertices.end(), { x0, y0, x1, y1, x0, y1 });
    }
    
    glBindVertexArray(overlayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glUseProgram(overlayShader);
    for (size_t i = 0; i < palette.size(); i++) {
        Color col = palette[i];
        int colorLoc = glGetUniformLocation(overlayShader, "overlayColor");
        if ((int)i == currentPaletteIndex)
            glUniform4f(colorLoc, 0.0f, 0.0f, 0.0f, 1.0f);
        else
            glUniform4f(colorLoc, col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 1.0f);
        glDrawArrays(GL_TRIANGLES, i * 6, 6);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Drawing App", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    glfwSetKeyCallback(window, key_callback);
    
    // Initialize flip-book with initial blank frame.
    frames.push_back(pixelBuffer);
    currentFrameIndex = 0;
    
    // Create canvas texture.
    glGenTextures(1, &canvasTex);
    glBindTexture(GL_TEXTURE_2D, canvasTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer.data());
    
    setupQuad();
    setupOverlay();
    
    // RC mode constants.
    const float rcAcceleration = 0.5f;
    const float rcFriction = 0.95f;
    
    while (!glfwWindowShouldClose(window)) {
        // In RC mode, update playhead with inertia.
        if (playheadMode == PlayheadMode::RC) {
            if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) { playheadVelY += rcAcceleration; }
            if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) { playheadVelY -= rcAcceleration; }
            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { playheadVelX -= rcAcceleration; }
            if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { playheadVelX += rcAcceleration; }
            playheadX += playheadVelX;
            playheadY += playheadVelY;
            playheadVelX *= rcFriction;
            playheadVelY *= rcFriction;
        }
        
        // Process painting actions.
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            undoStack.push_back(pixelBuffer);
            if (currentTool == Tool::PENCIL)
                drawCircleOnBuffer((int)playheadX, (int)playheadY, brushSize, currentPencilR, currentPencilG, currentPencilB);
            else
                drawCircleOnBuffer((int)playheadX, (int)playheadY, brushSize, eraserR, eraserG, eraserB);
        }
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            undoStack.push_back(pixelBuffer);
            drawCircleOnBuffer((int)playheadX, (int)playheadY, brushSize, eraserR, eraserG, eraserB);
        }
        
        // Clamp playhead.
        playheadX = std::max(0.0f, std::min(playheadX, (float)WIDTH));
        playheadY = std::max(0.0f, std::min(playheadY, (float)HEIGHT));
        
        // Update window title with frame info.
        {
            std::ostringstream oss;
            oss << "Drawing App - Frame " << currentFrameIndex << " / " << frames.size();
            glfwSetWindowTitle(window, oss.str().c_str());
        }
        
        // Set viewport: centered 800x600.
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        int viewX = (fbWidth - WIDTH) / 2;
        int viewY = (fbHeight - HEIGHT) / 2;
        glViewport(viewX, viewY, WIDTH, HEIGHT);
        
        updateCanvasTexture(canvasTex);
        
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Render the canvas quad.
        glUseProgram(quadShader);
        int zoomLoc = glGetUniformLocation(quadShader, "zoom");
        int panLoc = glGetUniformLocation(quadShader, "pan");
        glUniform1f(zoomLoc, 1.0f);
        glUniform2f(panLoc, 0.0f, 0.0f);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // Render playhead crosshair.
        drawPlayheadOverlay();
        
        // Render palette overlay if in color selection mode.
        if (colorSelectMode)
            drawPaletteOverlay();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteTextures(1, &canvasTex);
    glDeleteVertexArrays(1, &overlayVAO);
    glDeleteBuffers(1, &overlayVBO);
    glDeleteProgram(quadShader);
    glDeleteProgram(overlayShader);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
