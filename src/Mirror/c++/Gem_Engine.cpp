// main.cpp
#define _CRT_SECURE_NO_WARNINGS  // For sscanf warnings on MSVC

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // For perspective, lookAt, etc.
#include <glm/gtc/type_ptr.hpp>         // For value_ptr and project
#include <glm/gtc/constants.hpp>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <map>
#include <utility>

// stb_easy_font for text rendering
#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

// -----------------------------
// Helpers for Hex Conversion and Color Manipulation
// -----------------------------
bool isHexColor(const std::string& s) {
    if (s.size() != 7 || s[0] != '#') return false;
    for (size_t i = 1; i < s.size(); i++)
        if (!std::isxdigit(s[i])) return false;
    return true;
}

glm::vec3 hexToRGB(const std::string& s) {
    unsigned int r, g, b;
    sscanf(s.c_str(), "#%02x%02x%02x", &r, &g, &b);
    return glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);
}

std::string rgbToHex(const glm::vec3& col) {
    char buf[8];
    int r = static_cast<int>(glm::clamp(col.r, 0.0f, 1.0f) * 255);
    int g = static_cast<int>(glm::clamp(col.g, 0.0f, 1.0f) * 255);
    int b = static_cast<int>(glm::clamp(col.b, 0.0f, 1.0f) * 255);
    sprintf(buf, "#%02X%02X%02X", r, g, b);
    return std::string(buf);
}

glm::vec3 invertColor(const glm::vec3& col) {
    return glm::vec3(1.0f - col.r, 1.0f - col.g, 1.0f - col.b);
}

// -----------------------------
// Fake Levenshtein distance (for fuzzy matching)
// -----------------------------
int levenshteinDistance(const std::string& a, const std::string& b) {
    return std::abs((int)a.size() - (int)b.size());
}

// -----------------------------
// Data Structures for Graph & Simulation
// -----------------------------
struct Node {
    std::string label;
    glm::vec3 pos;       // 3D position
    glm::vec3 vel;       // Velocity for simulation
    glm::vec3 color;     // Node (gem) color
    glm::vec3 edgeColor; // For drawing edges
};

struct Edge {
    int from;
    int to;
};

std::vector<Node> g_nodes;
std::vector<Edge> g_edges;

// -----------------------------
// Global Free Flight Camera State
// -----------------------------
glm::vec3 cameraPos = glm::vec3(0.0f, 200.0f, 800.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraYaw = -90.0f;  // so that initial front is (0,0,-1)
float cameraPitch = 0.0f;

bool showLabels = true;   // Toggle for labels
bool outerMode = false;   // Toggle for "solid" gem (outer mode)
bool showNodes = true;    // Toggle for drawing node markers

// Timing and key toggle flags
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool firstMouse = true;
double lastX = 0.0, lastY = 0.0;
float mouseSensitivity = 0.1f;
float movementSpeed = 300.0f;
bool lKeyPressed = false;
bool oKeyPressed = false;
bool hKeyPressed = false;
bool leftKeyPressed = false;
bool rightKeyPressed = false;
bool rKeyPressed = false;

// Simulation runs for a short time
const float simulationDuration = 0.05f;  // 50ms

// -----------------------------
// Gem Palette Generation
// -----------------------------
glm::vec3 interpolateColor(const glm::vec3& start, const glm::vec3& end, float t) {
    return start * (1.0f - t) + end * t;
}

// Hardcoded color ranges for each gem type (use lowercase keys).
std::map<std::string, std::pair<std::string, std::string>> gemColorRanges = {
    {"garnet",     {"#400000", "#FF0000"}},  // deep red gradient
    {"amethyst",   {"#30005A", "#B19CD9"}},  // dark purple to light purple
    {"aquamarine", {"#006666", "#66FFFF"}},  // dark cyan to bright cyan
    {"diamond",    {"#CCCCCC", "#FFFFFF"}},  // gray to white
    {"emerald",    {"#004D00", "#00FF00"}},  // dark green to bright green
    {"alexandrite",{"#2B0030", "#00FF80"}},  // from provided alexandrite palette
    {"ruby",       {"#400000", "#FF4040"}},  // slightly different red than garnet
    {"peridot",    {"#405000", "#C0FF00"}},  // olive to lime green
    {"sapphire",   {"#000040", "#0000FF"}},  // dark blue to bright blue
    {"opal",       {"#800080", "#FFC0CB"}},  // purple to pink
    {"citrine",    {"#806000", "#FFFF00"}},  // brownish-yellow to bright yellow
    {"zircon",     {"#406080", "#A0D0FF"}}   // muted blue to light blue
};

std::vector<std::string> generateGemPalette(const std::string& gemName, int count) {
    std::string lowerGem = gemName;
    std::transform(lowerGem.begin(), lowerGem.end(), lowerGem.begin(), ::tolower);

    glm::vec3 startColor = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::vec3 endColor = glm::vec3(0.5f, 0.5f, 0.5f);

    if (gemColorRanges.count(lowerGem)) {
        startColor = hexToRGB(gemColorRanges[lowerGem].first);
        endColor = hexToRGB(gemColorRanges[lowerGem].second);
    }

    std::vector<std::string> palette;
    for (int i = 0; i < count; i++) {
        float t = float(i) / float(count - 1);
        // Randomize the interpolation factor a little
        float offset = ((rand() % 11) - 5) / 100.0f; // [-0.05, +0.05]
        float tRand = glm::clamp(t + offset, 0.0f, 1.0f);
        glm::vec3 col = interpolateColor(startColor, endColor, tRand);
        // Add small random offsets to each color channel
        for (int j = 0; j < 3; j++) {
            float channelOffset = ((rand() % 11) - 5) / 100.0f;
            if (j == 0) col.r = glm::clamp(col.r + channelOffset, 0.0f, 1.0f);
            if (j == 1) col.g = glm::clamp(col.g + channelOffset, 0.0f, 1.0f);
            if (j == 2) col.b = glm::clamp(col.b + channelOffset, 0.0f, 1.0f);
        }
        palette.push_back(rgbToHex(col));
    }
    return palette;
}

// -----------------------------
// Global Gem Carousel State
// -----------------------------
std::vector<std::string> gemTypes = {
    "garnet", "amethyst", "aquamarine", "diamond", "emerald",
    "alexandrite", "ruby", "peridot", "sapphire", "opal", "citrine", "zircon"
};
int currentGemIndex = 0;
int paletteCount = 24;  // Number of colors per gem palette
std::vector<std::string> currentPalette; // The palette for the currently selected gem

// -----------------------------
// Build Graph from Current Palette
// -----------------------------
void buildGemGraph() {
    g_nodes.clear();
    g_edges.clear();
    for (auto& col : currentPalette) {
        Node n;
        n.label = col;
        // Randomized initial position for each node
        n.pos = glm::vec3(
            float(rand() % 800 - 400),
            float(rand() % 600 - 300),
            float(rand() % 600 - 300)
        );
        n.vel = glm::vec3(0.0f);
        n.color = hexToRGB(col);
        n.edgeColor = invertColor(n.color);
        g_nodes.push_back(n);
    }
    // Build edges using fuzzy matching on labels
    for (size_t i = 0; i < g_nodes.size(); i++) {
        for (size_t j = i + 1; j < g_nodes.size(); j++) {
            int d = levenshteinDistance(g_nodes[i].label, g_nodes[j].label);
            if (d < 5)
                g_edges.push_back({ (int)i, (int)j });
        }
    }
}

// -----------------------------
// Input Callbacks and Process Input
// -----------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = float(xpos - lastX);
    float yoffset = float(lastY - ypos); // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;
    cameraYaw += xoffset;
    cameraPitch += yoffset;
    if (cameraPitch > 89.0f)  cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraPos += cameraFront * float(yoffset) * movementSpeed * 0.01f;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraFront * movementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraFront * movementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * movementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * movementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraUp * movementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraUp * movementSpeed * deltaTime;

    // Toggle labels with L key
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        if (!lKeyPressed) {
            showLabels = !showLabels;
            lKeyPressed = true;
        }
    }
    else { lKeyPressed = false; }

    // Toggle outer mode with O key
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        if (!oKeyPressed) {
            outerMode = !outerMode;
            oKeyPressed = true;
        }
    }
    else { oKeyPressed = false; }

    // Toggle node markers with H key
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        if (!hKeyPressed) {
            showNodes = !showNodes;
            hKeyPressed = true;
        }
    }
    else { hKeyPressed = false; }

    // Cycle gem type with left/right arrow keys
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        if (!leftKeyPressed) {
            currentGemIndex = (currentGemIndex - 1 + gemTypes.size()) % gemTypes.size();
            currentPalette = generateGemPalette(gemTypes[currentGemIndex], paletteCount);
            buildGemGraph();
            leftKeyPressed = true;
        }
    }
    else { leftKeyPressed = false; }

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        if (!rightKeyPressed) {
            currentGemIndex = (currentGemIndex + 1) % gemTypes.size();
            currentPalette = generateGemPalette(gemTypes[currentGemIndex], paletteCount);
            buildGemGraph();
            rightKeyPressed = true;
        }
    }
    else { rightKeyPressed = false; }

    // Regenerate current gem palette with R key
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!rKeyPressed) {
            currentPalette = generateGemPalette(gemTypes[currentGemIndex], paletteCount);
            buildGemGraph();
            rKeyPressed = true;
        }
    }
    else { rKeyPressed = false; }
}

// -----------------------------
// 3D Projection and View Setup using GLM
// -----------------------------
void set3DProjection(GLFWwindow* window, glm::mat4& proj, glm::mat4& view) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (height == 0) height = 1;
    float aspect = float(width) / float(height);
    proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 5000.0f);
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(proj));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(view));
}

// -----------------------------
// Drawing Helper: 2D Text using stb_easy_font
// -----------------------------
void drawText2D(const char* text, float x, float y, unsigned int color = 0xffffffff) {
    char buffer[99999];
    int num_quads = stb_easy_font_print(x, y, (char*)text, nullptr, buffer, sizeof(buffer));
    glColor4ub((color >> 24) & 0xFF,
        (color >> 16) & 0xFF,
        (color >> 8) & 0xFF,
        color & 0xFF);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, (const void*)buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// -----------------------------
// Force-Directed Simulation in 3D
// -----------------------------
void simulateGraph(float dt) {
    const float repulsion = 50000.0f;
    const float attraction = 0.1f;
    const float damping = 0.85f;
    const float idealLength = 150.0f;

    std::vector<glm::vec3> forces(g_nodes.size(), glm::vec3(0.0f));
    // Repulsion between nodes
    for (size_t i = 0; i < g_nodes.size(); i++) {
        for (size_t j = i + 1; j < g_nodes.size(); j++) {
            glm::vec3 diff = g_nodes[i].pos - g_nodes[j].pos;
            float dist = glm::length(diff) + 0.001f;
            glm::vec3 force = (repulsion / (dist * dist)) * (diff / dist);
            forces[i] += force;
            forces[j] -= force;
        }
    }
    // Attraction along graph edges
    for (auto& edge : g_edges) {
        glm::vec3 diff = g_nodes[edge.to].pos - g_nodes[edge.from].pos;
        float dist = glm::length(diff) + 0.001f;
        glm::vec3 force = attraction * (dist - idealLength) * (diff / dist);
        forces[edge.from] += force;
        forces[edge.to] -= force;
    }
    // Update positions and velocities
    for (size_t i = 0; i < g_nodes.size(); i++) {
        g_nodes[i].vel = (g_nodes[i].vel + forces[i] * dt) * damping;
        g_nodes[i].pos += g_nodes[i].vel * dt;
    }
}

// -----------------------------
// Triangle Structure and Build Triangles for Outer Mode Rendering
// -----------------------------
struct Triangle {
    int i, j, k;
    glm::vec3 normal;
    glm::vec3 centroid;
    bool isFrontFacing(const glm::vec3& cameraPos) const {
        glm::vec3 viewVec = glm::normalize(cameraPos - centroid);
        return glm::dot(normal, viewVec) > 0.0f;
    }
};

bool hasEdge(int a, int b) {
    for (auto& edge : g_edges) {
        if ((edge.from == a && edge.to == b) || (edge.from == b && edge.to == a))
            return true;
    }
    return false;
}

std::vector<Triangle> buildTriangles() {
    std::vector<Triangle> tris;
    int n = g_nodes.size();
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            for (int k = j + 1; k < n; k++) {
                if (hasEdge(i, j) && hasEdge(j, k) && hasEdge(i, k)) {
                    Triangle t;
                    t.i = i; t.j = j; t.k = k;
                    glm::vec3 A = g_nodes[i].pos;
                    glm::vec3 B = g_nodes[j].pos;
                    glm::vec3 C = g_nodes[k].pos;
                    t.normal = glm::normalize(glm::cross(B - A, C - A));
                    t.centroid = (A + B + C) / 3.0f;
                    tris.push_back(t);
                }
            }
        }
    }
    return tris;
}

// -----------------------------
// Main Program
// -----------------------------
int main() {
    srand((unsigned)time(nullptr));
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* vmode = glfwGetVideoMode(primary);
    GLFWwindow* window = glfwCreateWindow(vmode->width, vmode->height,
        "3D Fuzzy Graph Gemstone Maker (Free Flight)", primary, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize the graph with the first gem's palette.
    currentPalette = generateGemPalette(gemTypes[currentGemIndex], paletteCount);
    buildGemGraph();

    // Record simulation start time
    float simulationStartTime = float(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = float(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        // Run simulation for a short period
        if (currentFrame - simulationStartTime < simulationDuration) {
            simulateGraph(0.01f);
        }

        // Set up 3D view
        glm::mat4 proj, view;
        set3DProjection(window, proj, view);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (outerMode) {
            // Outer mode: build triangles from nodes to render a "solid" gemstone.
            std::vector<Triangle> triangles = buildTriangles();
            std::vector<int> frontTriangleIndices;
            for (int i = 0; i < triangles.size(); i++) {
                if (triangles[i].isFrontFacing(cameraPos))
                    frontTriangleIndices.push_back(i);
            }
            // Build an edge map for boundary edges.
            std::map<std::pair<int, int>, int> edgeMap;
            for (int idx : frontTriangleIndices) {
                Triangle t = triangles[idx];
                auto addEdge = [&](int a, int b) {
                    if (a > b) std::swap(a, b);
                    edgeMap[{a, b}]++;
                    };
                addEdge(t.i, t.j);
                addEdge(t.j, t.k);
                addEdge(t.i, t.k);
            }
            // Render front-facing triangles with translucent fills.
            for (int idx : frontTriangleIndices) {
                Triangle t = triangles[idx];
                glm::vec3 faceColor = (g_nodes[t.i].edgeColor + g_nodes[t.j].edgeColor + g_nodes[t.k].edgeColor) / 3.0f;
                glColor4f(faceColor.r, faceColor.g, faceColor.b, 0.5f);
                glBegin(GL_TRIANGLES);
                glVertex3f(g_nodes[t.i].pos.x, g_nodes[t.i].pos.y, g_nodes[t.i].pos.z);
                glVertex3f(g_nodes[t.j].pos.x, g_nodes[t.j].pos.y, g_nodes[t.j].pos.z);
                glVertex3f(g_nodes[t.k].pos.x, g_nodes[t.k].pos.y, g_nodes[t.k].pos.z);
                glEnd();
            }
            // Render boundary edges.
            glLineWidth(1.5f);
            glBegin(GL_LINES);
            for (auto& entry : edgeMap) {
                if (entry.second == 1) {
                    int a = entry.first.first, b = entry.first.second;
                    glm::vec3 lineColor = (g_nodes[a].edgeColor + g_nodes[b].edgeColor) * 0.5f;
                    glColor4f(lineColor.r, lineColor.g, lineColor.b, 0.8f);
                    glVertex3f(g_nodes[a].pos.x, g_nodes[a].pos.y, g_nodes[a].pos.z);
                    glVertex3f(g_nodes[b].pos.x, g_nodes[b].pos.y, g_nodes[b].pos.z);
                }
            }
            glEnd();
        }
        else {
            // Default mode: draw graph edges.
            glLineWidth(1.5f);
            glBegin(GL_LINES);
            for (auto& edge : g_edges) {
                Node& A = g_nodes[edge.from];
                Node& B = g_nodes[edge.to];
                glm::vec3 lineColor = (A.edgeColor + B.edgeColor) * 0.5f;
                glColor4f(lineColor.r, lineColor.g, lineColor.b, 0.3f);
                glVertex3f(A.pos.x, A.pos.y, A.pos.z);
                glVertex3f(B.pos.x, B.pos.y, B.pos.z);
            }
            glEnd();
        }

        // Draw node markers if enabled.
        if (showNodes) {
            for (auto& n : g_nodes) {
                float d = glm::length(cameraPos - n.pos);
                float scale = 5.0f * (d / 800.0f);
                glm::vec4 c(n.color, 1.0f);
                glPushMatrix();
                glTranslatef(n.pos.x, n.pos.y, n.pos.z);
                glScalef(scale, scale, scale);
                glColor4f(c.r, c.g, c.b, c.a);
                glBegin(GL_TRIANGLE_FAN);
                glVertex3f(0.0f, 0.0f, 0.0f);
                int slices = 16;
                for (int i = 0; i <= slices; i++) {
                    float theta = 2.0f * glm::pi<float>() * i / slices;
                    glVertex3f(cosf(theta), sinf(theta), 0.0f);
                }
                glEnd();
                glPopMatrix();
            }
        }

        // Precompute screen positions for node labels.
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glm::vec4 viewport(0, 0, width, height);
        std::vector<glm::vec3> screenPositions;
        screenPositions.reserve(g_nodes.size());
        for (auto& n : g_nodes) {
            screenPositions.push_back(glm::project(n.pos, view, proj, viewport));
        }

        // Draw labels as a 2D overlay.
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);  // Fixed 2D projection with top at 0.
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        if (showLabels) {
            for (size_t i = 0; i < g_nodes.size(); i++) {
                float labelX = screenPositions[i].x;
                // Flip the y-coordinate so labels are locked to the y-axis of the screen.
                float labelY = height - screenPositions[i].y;
                drawText2D(g_nodes[i].label.c_str(), labelX, labelY, 0xFFFFFFFF);
            }
        }
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

        // Draw a fixed debug overlay with the current gem type at the top center.
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        {
            std::string title = "Current Gem: " + gemTypes[currentGemIndex];
            drawText2D(title.c_str(), width / 2 - 100, 20, 0xFFFFFFFF);
        }
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
