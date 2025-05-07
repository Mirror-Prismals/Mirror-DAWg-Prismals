#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>

// Window dimensions
const int WIDTH = 800, HEIGHT = 600;

// Game grid dimensions
const int GRID_WIDTH = 80;
const int GRID_HEIGHT = 60;
const float CELL_SIZE = 10.0f;

// Cell states
bool currentGrid[GRID_HEIGHT][GRID_WIDTH] = {false};
bool nextGrid[GRID_HEIGHT][GRID_WIDTH] = {false};

// Game state
bool gameRunning = false;
int simulationSpeed = 100; // ms between updates
int generation = 0;
bool keyPressed = false;

// Function prototypes
void initGrid();
void processInput(GLFWwindow* window);
void updateGrid();
void renderGrid(GLFWwindow* window);
int countNeighbors(int x, int y);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void addGlider(int startX, int startY);
void addPulsar(int startX, int startY);
void randomizeGrid();

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Conway's Game of Life - GLFW", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);
    
    // Set callbacks
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    
    // Initialize the grid with some patterns
    initGrid();
    
    // Main loop
    double lastTime = glfwGetTime();
    double updateTimer = 0.0;
    
    while (!glfwWindowShouldClose(window)) {
        // Calculate delta time
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // Process input
        processInput(window);
        
        // Update grid based on simulation speed
        if (gameRunning) {
            updateTimer += deltaTime;
            if (updateTimer >= simulationSpeed / 1000.0) {
                updateGrid();
                updateTimer = 0.0;
            }
        }
        
        // Render
        glClear(GL_COLOR_BUFFER_BIT);
        renderGrid(window);
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}

void initGrid() {
    // Clear the grid
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            currentGrid[y][x] = false;
        }
    }
    
    // Add some initial patterns
    addGlider(10, 10);
    addPulsar(30, 30);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
        
    // Space key toggles simulation
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!keyPressed) {
            gameRunning = !gameRunning;
            keyPressed = true;
        }
    } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        keyPressed = false;
    }
}

void updateGrid() {
    generation++;
    
    // Compute next generation
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            int neighbors = countNeighbors(x, y);
            bool currentState = currentGrid[y][x];
            
            // Apply Conway's rules
            if (currentState && (neighbors < 2 || neighbors > 3)) {
                // Cell dies from under/overpopulation
                nextGrid[y][x] = false;
            } else if (!currentState && neighbors == 3) {
                // Cell becomes alive from reproduction
                nextGrid[y][x] = true;
            } else {
                // State remains the same
                nextGrid[y][x] = currentState;
            }
        }
    }
    
    // Swap grids
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            currentGrid[y][x] = nextGrid[y][x];
        }
    }
}

void renderGrid(GLFWwindow* window) {
    // Set up orthographic projection
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Set background color (dark blue)
    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
    
    // Draw cells
    glColor3f(0.2f, 0.6f, 0.8f); // Cell color (light blue)
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (currentGrid[y][x]) {
                glBegin(GL_QUADS);
                glVertex2f(x * CELL_SIZE, y * CELL_SIZE);
                glVertex2f(x * CELL_SIZE + CELL_SIZE - 1, y * CELL_SIZE);
                glVertex2f(x * CELL_SIZE + CELL_SIZE - 1, y * CELL_SIZE + CELL_SIZE - 1);
                glVertex2f(x * CELL_SIZE, y * CELL_SIZE + CELL_SIZE - 1);
                glEnd();
            }
        }
    }
    
    // Draw grid lines
    glColor3f(0.3f, 0.3f, 0.3f); // Grid line color (gray)
    glBegin(GL_LINES);
    
    // Horizontal lines
    for (int y = 0; y <= GRID_HEIGHT; y++) {
        glVertex2f(0, y * CELL_SIZE);
        glVertex2f(GRID_WIDTH * CELL_SIZE, y * CELL_SIZE);
    }
    
    // Vertical lines
    for (int x = 0; x <= GRID_WIDTH; x++) {
        glVertex2f(x * CELL_SIZE, 0);
        glVertex2f(x * CELL_SIZE, GRID_HEIGHT * CELL_SIZE);
    }
    
    glEnd();
    
    // Display generation count (if you have a font rendering system)
    // For this basic example, we just output to console
    if (generation % 10 == 0 && gameRunning) {
        std::cout << "Generation: " << generation << std::endl;
    }
}

int countNeighbors(int x, int y) {
    int count = 0;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue; // Skip the cell itself
            
            // Handle wrapping around edges (toroidal grid)
            int nx = (x + dx + GRID_WIDTH) % GRID_WIDTH;
            int ny = (y + dy + GRID_HEIGHT) % GRID_HEIGHT;
            
            if (currentGrid[ny][nx]) {
                count++;
            }
        }
    }
    
    return count;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Get cursor position
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        // Convert to grid coordinates
        int gridX = static_cast<int>(xpos / CELL_SIZE);
        int gridY = static_cast<int>(ypos / CELL_SIZE);
        
        // Toggle cell if within grid bounds
        if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
            currentGrid[gridY][gridX] = !currentGrid[gridY][gridX];
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Declare variables outside switch statement to fix the scope issue
    double xpos, ypos;
    int gridX, gridY;
    
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_R:
                // Reset grid
                initGrid();
                generation = 0;
                break;
            case GLFW_KEY_SPACE:
                // Toggle simulation (handled in processInput)
                break;
            case GLFW_KEY_G:
                // Add glider at cursor position
                glfwGetCursorPos(window, &xpos, &ypos);
                gridX = static_cast<int>(xpos / CELL_SIZE);
                gridY = static_cast<int>(ypos / CELL_SIZE);
                addGlider(gridX, gridY);
                break;
            case GLFW_KEY_P:
                // Add pulsar at cursor position
                glfwGetCursorPos(window, &xpos, &ypos);
                gridX = static_cast<int>(xpos / CELL_SIZE);
                gridY = static_cast<int>(ypos / CELL_SIZE);
                addPulsar(gridX, gridY);
                break;
            case GLFW_KEY_N:
                // Single step
                if (!gameRunning) {
                    updateGrid();
                }
                break;
            case GLFW_KEY_UP:
                // Increase speed
                simulationSpeed = std::max(10, simulationSpeed - 10);
                std::cout << "Speed: " << simulationSpeed << "ms" << std::endl;
                break;
            case GLFW_KEY_DOWN:
                // Decrease speed
                simulationSpeed = std::min(500, simulationSpeed + 10);
                std::cout << "Speed: " << simulationSpeed << "ms" << std::endl;
                break;
            case GLFW_KEY_X:
                // Randomize grid
                randomizeGrid();
                break;
        }
    }
}

void addGlider(int startX, int startY) {
    bool glider[3][3] = {
        {false, true, false},
        {false, false, true},
        {true, true, true}
    };
    
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            int worldX = (startX + x) % GRID_WIDTH;
            int worldY = (startY + y) % GRID_HEIGHT;
            
            currentGrid[worldY][worldX] = glider[y][x];
        }
    }
}

void addPulsar(int startX, int startY) {
    bool pulsar[13][13] = {
        {false, false, true, true, true, false, false, false, true, true, true, false, false},
        {false, false, false, false, false, false, false, false, false, false, false, false, false},
        {true, false, false, false, false, true, false, true, false, false, false, false, true},
        {true, false, false, false, false, true, false, true, false, false, false, false, true},
        {true, false, false, false, false, true, false, true, false, false, false, false, true},
        {false, false, true, true, true, false, false, false, true, true, true, false, false},
        {false, false, false, false, false, false, false, false, false, false, false, false, false},
        {false, false, true, true, true, false, false, false, true, true, true, false, false},
        {true, false, false, false, false, true, false, true, false, false, false, false, true},
        {true, false, false, false, false, true, false, true, false, false, false, false, true},
        {true, false, false, false, false, true, false, true, false, false, false, false, true},
        {false, false, false, false, false, false, false, false, false, false, false, false, false},
        {false, false, true, true, true, false, false, false, true, true, true, false, false}
    };
    
    for (int y = 0; y < 13; y++) {
        for (int x = 0; x < 13; x++) {
            int worldX = (startX + x) % GRID_WIDTH;
            int worldY = (startY + y) % GRID_HEIGHT;
            
            currentGrid[worldY][worldX] = pulsar[y][x];
        }
    }
}

void randomizeGrid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            currentGrid[y][x] = (dis(gen) > 0.7); // ~30% chance of being alive
        }
    }
}
