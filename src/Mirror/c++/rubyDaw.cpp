#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <random>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

// Constants
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

// Utility functions
float lerp(float a, float b, float t) { return a + (b - a) * t; }
glm::vec4 hexToRgba(uint32_t hex, float alpha = 1.0f) {
    return glm::vec4(
        ((hex >> 16) & 0xFF) / 255.0f,
        ((hex >> 8) & 0xFF) / 255.0f,
        (hex & 0xFF) / 255.0f,
        alpha
    );
}

// Theme colors
struct Theme {
    glm::vec4 bgCenter = hexToRgba(0x400000);
    glm::vec4 bgEdge = hexToRgba(0x100000);
    glm::vec4 sampleColor = hexToRgba(0x803030);
    glm::vec4 timelineColor = hexToRgba(0x602020);
    glm::vec4 fxchainColor = hexToRgba(0x701010);
    glm::vec4 rubyBg = hexToRgba(0x701010);
    glm::vec4 rubyBorder = hexToRgba(0x500000);
    glm::vec4 rubyHover = hexToRgba(0x803030);
    glm::vec4 rubyText = hexToRgba(0xf0f0f0);
    glm::vec4 rubyMuted = hexToRgba(0xc0c0c0);
    glm::vec4 wireRed = hexToRgba(0xff4444);
    glm::vec4 circleRed = hexToRgba(0xff6666);
    glm::vec4 circleRedBorder = hexToRgba(0xcc3333);
    glm::vec4 laneBg = hexToRgba(0x701010, 0.5f);
    glm::vec4 laneAddBg = hexToRgba(0x501010, 0.5f);
    glm::vec4 laneAddButton = hexToRgba(0xff6666);
    glm::vec4 gridLine = hexToRgba(0x431616, 0.7f);
    glm::vec4 gridLineInner = hexToRgba(0x431616, 0.5f);
};

// Forward declarations
class Node;
struct Connection;

// Connection point
struct IOCircle {
    glm::vec2 position;
    bool isInput;
    glm::vec4 color;
    glm::vec4 borderColor;
    int portIndex;
    float radius = 6.0f;

    IOCircle(glm::vec2 pos = glm::vec2(0.0f), bool input = false, int index = 0) :
        position(pos), isInput(input), portIndex(index) {
        color = hexToRgba(input ? 0x80ff80 : 0xff6666);
        borderColor = hexToRgba(input ? 0x60cc60 : 0xcc3333);
    }

    glm::vec2 getWorldPosition(const glm::vec2& nodePos) const {
        return nodePos + position;
    }

    // Fixed: Added const qualifier to the draw method
    void draw(const glm::vec2& nodePos) const {
        glm::vec2 worldPos = getWorldPosition(nodePos);

        // Draw circle fill
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(color.r, color.g, color.b, color.a);
        glVertex2f(worldPos.x, worldPos.y);
        for (int i = 0; i <= 20; i++) {
            float angle = i * 2.0f * 3.14159f / 20.0f;
            glVertex2f(worldPos.x + radius * cos(angle), worldPos.y + radius * sin(angle));
        }
        glEnd();

        // Draw circle border
        glLineWidth(1.0f);
        glBegin(GL_LINE_LOOP);
        glColor4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        for (int i = 0; i < 20; i++) {
            float angle = i * 2.0f * 3.14159f / 20.0f;
            glVertex2f(worldPos.x + radius * cos(angle), worldPos.y + radius * sin(angle));
        }
        glEnd();
    }
};

// Wire properties
struct WireLayer {
    float baseOffset = 0.0f;
    float variationAmplitude = 5.0f;
    float phase = 0.0f;
    float baseWidth = 2.0f;
    float widthVariation = 1.0f;
    float opacity = 0.4f;

    WireLayer() {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        baseOffset = std::uniform_real_distribution<float>(-10.0f, 10.0f)(gen);
        variationAmplitude = std::uniform_real_distribution<float>(5.0f, 10.0f)(gen);
        phase = std::uniform_real_distribution<float>(0.0f, 2.0f * 3.14159f)(gen);
        baseWidth = std::uniform_real_distribution<float>(2.0f, 4.0f)(gen);
        widthVariation = std::uniform_real_distribution<float>(1.0f, 2.0f)(gen);
    }
};

// Sparkle effect
struct Sparkle {
    glm::vec2 position;
    float lifetime = 0.0f;
    float maxLifetime = 1.0f;
    float size = 2.0f;

    Sparkle(glm::vec2 pos) : position(pos) {}

    bool update(float deltaTime) {
        lifetime += deltaTime;
        return lifetime < maxLifetime;
    }

    void draw() {
        float progress = lifetime / maxLifetime;
        float scale = progress < 0.5f ? 0.5f + 2.0f * progress : 2.5f - 2.0f * progress;
        float alpha = 1.0f - progress;

        glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glVertex2f(position.x, position.y);
        for (int i = 0; i <= 12; i++) {
            float angle = i * 2.0f * 3.14159f / 12.0f;
            glVertex2f(position.x + size * scale * cos(angle), position.y + size * scale * sin(angle));
        }
        glEnd();
    }
};

// Connection between nodes
struct Connection {
    int fromNodeId;
    int fromPortIndex;
    int toNodeId;
    int toPortIndex;
    std::vector<WireLayer> layers;

    Connection(int fromNode, int fromPort, int toNode, int toPort) :
        fromNodeId(fromNode), fromPortIndex(fromPort), toNodeId(toNode), toPortIndex(toPort) {

        // Create 3 layers for visual effect
        for (int i = 0; i < 3; i++) {
            WireLayer layer;
            layer.opacity = (i == 1) ? 0.8f : 0.4f;  // Middle layer is more opaque
            layers.push_back(layer);
        }
    }

    void draw(const std::vector<std::shared_ptr<Node>>& nodes, float time);
};

// Helper function to draw text
void drawText(const std::string& text, float x, float y, float scale = 1.0f) {
    static char buffer[9999];

    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);

    int quads = stb_easy_font_print(0, 0, const_cast<char*>(text.c_str()), nullptr, buffer, sizeof(buffer));

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();
}

// Base Node class
class Node {
public:
    int id;
    glm::vec2 position;
    glm::vec2 size;
    std::string title;
    std::vector<IOCircle> inputCircles;
    std::vector<IOCircle> outputCircles;
    bool isDragging = false;
    glm::vec2 dragOffset;

    Node(int nodeId, glm::vec2 pos, glm::vec2 sz, std::string ttl) :
        id(nodeId), position(pos), size(sz), title(ttl) {
    }

    virtual ~Node() {}

    virtual void draw(const Theme& theme) = 0;

    bool isPointInside(const glm::vec2& point) const {
        return point.x >= position.x && point.x <= position.x + size.x &&
            point.y >= position.y && point.y <= position.y + size.y;
    }

    bool isPointInHeader(const glm::vec2& point) const {
        return point.x >= position.x && point.x <= position.x + size.x &&
            point.y >= position.y && point.y <= position.y + 24.0f;
    }

    IOCircle* getCircleAtPoint(const glm::vec2& point) {
        for (auto& circle : inputCircles) {
            if (glm::distance(circle.getWorldPosition(position), point) <= circle.radius) {
                return &circle;
            }
        }
        for (auto& circle : outputCircles) {
            if (glm::distance(circle.getWorldPosition(position), point) <= circle.radius) {
                return &circle;
            }
        }
        return nullptr;
    }

protected:
    void drawNodeBase(const Theme& theme) {
        // Draw body
        glBegin(GL_QUADS);
        glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
        glVertex2f(position.x, position.y);
        glVertex2f(position.x + size.x, position.y);
        glVertex2f(position.x + size.x, position.y + size.y);
        glVertex2f(position.x, position.y + size.y);
        glEnd();

        // Draw header
        glBegin(GL_QUADS);
        glColor4f(theme.rubyBg.r, theme.rubyBg.g, theme.rubyBg.b, theme.rubyBg.a);
        glVertex2f(position.x, position.y);
        glVertex2f(position.x + size.x, position.y);
        glVertex2f(position.x + size.x, position.y + 24.0f);
        glVertex2f(position.x, position.y + 24.0f);
        glEnd();

        // Draw border
        glLineWidth(1.0f);
        glBegin(GL_LINE_LOOP);
        glColor4f(theme.rubyBorder.r, theme.rubyBorder.g, theme.rubyBorder.b, theme.rubyBorder.a);
        glVertex2f(position.x, position.y);
        glVertex2f(position.x + size.x, position.y);
        glVertex2f(position.x + size.x, position.y + size.y);
        glVertex2f(position.x, position.y + size.y);
        glEnd();

        // Draw header/body separator
        glBegin(GL_LINES);
        glVertex2f(position.x, position.y + 24.0f);
        glVertex2f(position.x + size.x, position.y + 24.0f);
        glEnd();

        // Draw title
        glColor4f(theme.rubyText.r, theme.rubyText.g, theme.rubyText.b, theme.rubyText.a);
        drawText(title, position.x + 10.0f, position.y + 14.0f);
    }
};

// Node implementations
class PrimitiveNode : public Node {
public:
    PrimitiveNode(int nodeId, glm::vec2 pos) :
        Node(nodeId, pos, glm::vec2(140.0f, 80.0f), "Primitive") {
        outputCircles.push_back(IOCircle(glm::vec2(size.x, size.y * 0.5f), false, 0));
    }

    void draw(const Theme& theme) override {
        drawNodeBase(theme);
        drawText("(No real audio logic)", position.x + 10.0f, position.y + 40.0f);
        for (const auto& circle : outputCircles) {
            circle.draw(position);
        }
    }
};

class NoteNode : public Node {
public:
    std::string content = "(Type your note here...)";

    NoteNode(int nodeId, glm::vec2 pos) :
        Node(nodeId, pos, glm::vec2(160.0f, 120.0f), "Note") {
        inputCircles.push_back(IOCircle(glm::vec2(0.0f, size.y * 0.5f), true, 0));
        outputCircles.push_back(IOCircle(glm::vec2(size.x, size.y * 0.5f), false, 0));
    }

    void draw(const Theme& theme) override {
        drawNodeBase(theme);
        drawText(content, position.x + 10.0f, position.y + 40.0f);
        for (const auto& circle : inputCircles) { circle.draw(position); }
        for (const auto& circle : outputCircles) { circle.draw(position); }
    }
};

// Application state
struct AppState {
    GLFWwindow* window = nullptr;
    Theme theme;

    // Layout
    bool sampleManagerHidden = false;
    bool fxChainHidden = false;
    glm::vec4 timelineArea = glm::vec4(0.2f * WINDOW_WIDTH, 0, 0.8f * WINDOW_WIDTH, 0.7f * WINDOW_HEIGHT);
    glm::vec4 sampleManagerArea = glm::vec4(0, 0, 0.2f * WINDOW_WIDTH, 0.7f * WINDOW_HEIGHT);
    glm::vec4 fxChainArea = glm::vec4(0.2f * WINDOW_WIDTH, 0.7f * WINDOW_HEIGHT, 0.8f * WINDOW_WIDTH, 0.3f * WINDOW_HEIGHT);

    // Pan/zoom
    glm::vec2 gridOffset = glm::vec2(0.0f);
    float zoom = 1.0f;
    bool isTimelineDragging = false;
    glm::vec2 dragStart = glm::vec2(0.0f);

    // Transport
    glm::vec2 transportPos = glm::vec2(10.0f, 40.0f);
    bool isTransportDragging = false;
    glm::vec2 transportDragOffset = glm::vec2(0.0f);

    // Context menu
    bool showContextMenu = false;
    glm::vec2 contextMenuPos = glm::vec2(0.0f);

    // Wire dragging
    bool isDraggingWire = false;
    int dragFromNodeId = -1;
    int dragFromPortIndex = -1;
    bool dragFromIsInput = false;
    glm::vec2 wireStart = glm::vec2(0.0f);
    glm::vec2 wireEnd = glm::vec2(0.0f);
    std::vector<WireLayer> tempWireLayers;

    // Nodes and connections
    int nextNodeId = 0;
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<Connection> connections;

    // Animation
    float globalTime = 0.0f;
    std::vector<Sparkle> sparkles;
    float sparkleTimer = 0.0f;

    AppState() {
        // Initialize temporary wire layers
        for (int i = 0; i < 3; i++) {
            tempWireLayers.push_back(WireLayer());
        }
    }
};

// Drawing functions declarations
void drawBackground(const Theme& theme);
void drawTimeline(AppState& state);
void drawTransportNode(const AppState& state);
void drawUI(AppState& state);
std::shared_ptr<Node> getNodeAtPoint(AppState& state, const glm::vec2& point);
IOCircle* getCircleAtPoint(AppState& state, const glm::vec2& point, int& nodeId, bool& isInput);
void createNode(AppState& state, const std::string& nodeType, float x, float y);

// Additional input callbacks to handle mouse dragging of nodes, wire connections, etc.
void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
    AppState* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    glm::vec2 mousePos(xpos, ypos);

    static glm::vec2 lastMousePos(0.0f, 0.0f);
    glm::vec2 mouseDelta = mousePos - lastMousePos;
    lastMousePos = mousePos;

    // Handle wire dragging
    if (state->isDraggingWire) {
        state->wireEnd = mousePos;
    }

    // Handle node dragging
    for (auto& node : state->nodes) {
        if (node->isDragging) {
            node->position += mouseDelta;
        }
    }

    // Handle timeline panning
    if (state->isTimelineDragging) {
        state->gridOffset += mouseDelta;
    }

    // Handle transport dragging
    if (state->isTransportDragging) {
        state->transportPos += mouseDelta;
    }
}

// Mouse button callback to handle interaction with nodes and connection points
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    AppState* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    glm::vec2 mousePos(x, y);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // First check if we're clicking on a node
            std::shared_ptr<Node> node = getNodeAtPoint(*state, mousePos);
            if (node) {
                if (node->isPointInHeader(mousePos)) {
                    // Dragging the node by its header
                    node->isDragging = true;
                    node->dragOffset = mousePos - node->position;
                    // Bring to front by moving to end of vector
                    auto it = std::find(state->nodes.begin(), state->nodes.end(), node);
                    if (it != state->nodes.end()) {
                        std::rotate(it, it + 1, state->nodes.end());
                    }
                }
                else {
                    // Check if we're interacting with an IO circle
                    int nodeId;
                    bool isInput;
                    IOCircle* circle = getCircleAtPoint(*state, mousePos, nodeId, isInput);
                    if (circle) {
                        // Start dragging a wire
                        state->isDraggingWire = true;
                        state->dragFromNodeId = nodeId;
                        state->dragFromPortIndex = circle->portIndex;
                        state->dragFromIsInput = isInput;
                        if (isInput) {
                            state->wireStart = circle->getWorldPosition(node->position);
                            state->wireEnd = mousePos;
                        }
                        else {
                            state->wireStart = circle->getWorldPosition(node->position);
                            state->wireEnd = mousePos;
                        }
                    }
                }
            }
            else if (mousePos.x >= state->timelineArea.x &&
                mousePos.x <= state->timelineArea.x + state->timelineArea.z &&
                mousePos.y >= state->timelineArea.y &&
                mousePos.y <= state->timelineArea.y + state->timelineArea.w) {
                // Clicking in the timeline area (not on a node) - start panning
                state->isTimelineDragging = true;
                state->dragStart = mousePos;
            }
            else if (mousePos.x >= state->transportPos.x &&
                mousePos.x <= state->transportPos.x + 200.0f &&
                mousePos.y >= state->transportPos.y &&
                mousePos.y <= state->transportPos.y + 40.0f) {
                // Clicking on transport - start dragging
                state->isTransportDragging = true;
                state->transportDragOffset = mousePos - state->transportPos;
            }
        }
        else if (action == GLFW_RELEASE) {
            // Release any dragging operations
            for (auto& node : state->nodes) {
                node->isDragging = false;
            }

            // Handle wire connection completion
            if (state->isDraggingWire) {
                int nodeId;
                bool isInput;
                IOCircle* targetCircle = getCircleAtPoint(*state, mousePos, nodeId, isInput);

                if (targetCircle && nodeId != state->dragFromNodeId &&
                    isInput != state->dragFromIsInput) {
                    // Valid connection - different nodes and opposite port types
                    if (state->dragFromIsInput) {
                        // From input to output
                        state->connections.push_back(
                            Connection(nodeId, targetCircle->portIndex,
                                state->dragFromNodeId, state->dragFromPortIndex));
                    }
                    else {
                        // From output to input
                        state->connections.push_back(
                            Connection(state->dragFromNodeId, state->dragFromPortIndex,
                                nodeId, targetCircle->portIndex));
                    }
                }

                state->isDraggingWire = false;
            }

            state->isTimelineDragging = false;
            state->isTransportDragging = false;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        // Show context menu
        state->showContextMenu = true;
        state->contextMenuPos = mousePos;
    }
}

// Drawing functions implementations
void drawBackground(const Theme& theme) {
    // Draw gradient background
    glBegin(GL_QUADS);
    glColor4f(theme.bgCenter.r, theme.bgCenter.g, theme.bgCenter.b, theme.bgCenter.a);
    glVertex2f(WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f); // Center

    glColor4f(theme.bgEdge.r, theme.bgEdge.g, theme.bgEdge.b, theme.bgEdge.a);
    glVertex2f(0.0f, 0.0f); // Top-left
    glVertex2f(WINDOW_WIDTH, 0.0f); // Top-right
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT); // Bottom-right
    glVertex2f(0.0f, WINDOW_HEIGHT); // Bottom-left
    glVertex2f(0.0f, 0.0f); // Top-left again to complete the fan
    glEnd();
}

void drawTimeline(AppState& state) {
    // Draw timeline area
    glBegin(GL_QUADS);
    glColor4f(state.theme.timelineColor.r, state.theme.timelineColor.g,
        state.theme.timelineColor.b, state.theme.timelineColor.a);
    glVertex2f(state.timelineArea.x, state.timelineArea.y);
    glVertex2f(state.timelineArea.x + state.timelineArea.z, state.timelineArea.y);
    glVertex2f(state.timelineArea.x + state.timelineArea.z, state.timelineArea.y + state.timelineArea.w);
    glVertex2f(state.timelineArea.x, state.timelineArea.y + state.timelineArea.w);
    glEnd();

    // Draw timeline grid
    const float gridSize = 32.0f * state.zoom;
    const float offsetX = state.gridOffset.x - (int)(state.gridOffset.x / gridSize) * gridSize;
    const float offsetY = state.gridOffset.y - (int)(state.gridOffset.y / gridSize) * gridSize;

    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glColor4f(state.theme.gridLine.r, state.theme.gridLine.g, state.theme.gridLine.b, state.theme.gridLine.a);

    // Vertical lines
    for (float x = offsetX + state.timelineArea.x; x < state.timelineArea.x + state.timelineArea.z; x += gridSize) {
        glVertex2f(x, state.timelineArea.y);
        glVertex2f(x, state.timelineArea.y + state.timelineArea.w);
    }

    // Horizontal lines
    for (float y = offsetY + state.timelineArea.y; y < state.timelineArea.y + state.timelineArea.w; y += gridSize) {
        glVertex2f(state.timelineArea.x, y);
        glVertex2f(state.timelineArea.x + state.timelineArea.z, y);
    }
    glEnd();

    // Draw node connections
    for (auto& conn : state.connections) {
        conn.draw(state.nodes, state.globalTime);
    }

    // Draw temporary wire when dragging
    if (state.isDraggingWire) {
        // Draw wire layers for temporary connection
        for (auto& layer : state.tempWireLayers) {
            float dx = state.wireEnd.x - state.wireStart.x;
            float dy = state.wireEnd.y - state.wireStart.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len < 0.0001f) len = 0.0001f;

            float perpX = -dy / len;
            float perpY = dx / len;

            float offset = layer.baseOffset;
            float mx = (state.wireStart.x + state.wireEnd.x) * 0.5f;
            float my = (state.wireStart.y + state.wireEnd.y) * 0.5f;
            float cp_x = mx + perpX * offset;
            float cp_y = my + perpY * offset;

            float width = layer.baseWidth;

            glLineWidth(width);
            glBegin(GL_LINE_STRIP);

            const int steps = 20;
            for (int j = 0; j <= steps; j++) {
                float t = (float)j / steps;
                float u = 1.0f - t;
                float tt = t * t;
                float uu = u * u;

                float x = uu * state.wireStart.x + 2 * u * t * cp_x + tt * state.wireEnd.x;
                float y = uu * state.wireStart.y + 2 * u * t * cp_y + tt * state.wireEnd.y;

                glm::vec4 wireColor = state.theme.wireRed;
                glColor4f(wireColor.r, wireColor.g, wireColor.b, layer.opacity);

                glVertex2f(x, y);
            }
            glEnd();
        }
    }

    // Draw nodes
    for (auto& node : state.nodes) {
        node->draw(state.theme);
    }
}

void drawTransportNode(const AppState& state) {
    // Draw transport controls box
    glBegin(GL_QUADS);
    glColor4f(state.theme.rubyBg.r, state.theme.rubyBg.g, state.theme.rubyBg.b, state.theme.rubyBg.a);
    glVertex2f(state.transportPos.x, state.transportPos.y);
    glVertex2f(state.transportPos.x + 200.0f, state.transportPos.y);
    glVertex2f(state.transportPos.x + 200.0f, state.transportPos.y + 40.0f);
    glVertex2f(state.transportPos.x, state.transportPos.y + 40.0f);
    glEnd();

    // Draw border
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glColor4f(state.theme.rubyBorder.r, state.theme.rubyBorder.g, state.theme.rubyBorder.b, state.theme.rubyBorder.a);
    glVertex2f(state.transportPos.x, state.transportPos.y);
    glVertex2f(state.transportPos.x + 200.0f, state.transportPos.y);
    glVertex2f(state.transportPos.x + 200.0f, state.transportPos.y + 40.0f);
    glVertex2f(state.transportPos.x, state.transportPos.y + 40.0f);
    glEnd();

    // Draw play button (triangle)
    glBegin(GL_TRIANGLES);
    glColor4f(state.theme.rubyText.r, state.theme.rubyText.g, state.theme.rubyText.b, state.theme.rubyText.a);
    glVertex2f(state.transportPos.x + 20.0f, state.transportPos.y + 10.0f);
    glVertex2f(state.transportPos.x + 40.0f, state.transportPos.y + 20.0f);
    glVertex2f(state.transportPos.x + 20.0f, state.transportPos.y + 30.0f);
    glEnd();

    // Draw stop button (square)
    glBegin(GL_QUADS);
    glVertex2f(state.transportPos.x + 50.0f, state.transportPos.y + 10.0f);
    glVertex2f(state.transportPos.x + 70.0f, state.transportPos.y + 10.0f);
    glVertex2f(state.transportPos.x + 70.0f, state.transportPos.y + 30.0f);
    glVertex2f(state.transportPos.x + 50.0f, state.transportPos.y + 30.0f);
    glEnd();

    // Draw BPM text
    glColor4f(state.theme.rubyText.r, state.theme.rubyText.g, state.theme.rubyText.b, state.theme.rubyText.a);
    drawText("120 BPM", state.transportPos.x + 80.0f, state.transportPos.y + 20.0f);
}

void drawUI(AppState& state) {
    // Draw sample manager if not hidden
    if (!state.sampleManagerHidden) {
        glBegin(GL_QUADS);
        glColor4f(state.theme.sampleColor.r, state.theme.sampleColor.g, state.theme.sampleColor.b, state.theme.sampleColor.a);
        glVertex2f(state.sampleManagerArea.x, state.sampleManagerArea.y);
        glVertex2f(state.sampleManagerArea.x + state.sampleManagerArea.z, state.sampleManagerArea.y);
        glVertex2f(state.sampleManagerArea.x + state.sampleManagerArea.z, state.sampleManagerArea.y + state.sampleManagerArea.w);
        glVertex2f(state.sampleManagerArea.x, state.sampleManagerArea.y + state.sampleManagerArea.w);
        glEnd();

        // Draw sample manager title
        glColor4f(state.theme.rubyText.r, state.theme.rubyText.g, state.theme.rubyText.b, state.theme.rubyText.a);
        drawText("Sample Manager", state.sampleManagerArea.x + 10.0f, state.sampleManagerArea.y + 20.0f);
    }

    // Draw fx chain if not hidden
    if (!state.fxChainHidden) {
        glBegin(GL_QUADS);
        glColor4f(state.theme.fxchainColor.r, state.theme.fxchainColor.g, state.theme.fxchainColor.b, state.theme.fxchainColor.a);
        glVertex2f(state.fxChainArea.x, state.fxChainArea.y);
        glVertex2f(state.fxChainArea.x + state.fxChainArea.z, state.fxChainArea.y);
        glVertex2f(state.fxChainArea.x + state.fxChainArea.z, state.fxChainArea.y + state.fxChainArea.w);
        glVertex2f(state.fxChainArea.x, state.fxChainArea.y + state.fxChainArea.w);
        glEnd();

        // Draw fx chain title
        glColor4f(state.theme.rubyText.r, state.theme.rubyText.g, state.theme.rubyText.b, state.theme.rubyText.a);
        drawText("Effect Chain", state.fxChainArea.x + 10.0f, state.fxChainArea.y + 20.0f);
    }

    // Draw context menu if visible
    if (state.showContextMenu) {
        float menuWidth = 120.0f;
        float menuHeight = 60.0f;

        // Ensure menu stays within window
        float menuX = std::min(state.contextMenuPos.x, WINDOW_WIDTH - menuWidth);
        float menuY = std::min(state.contextMenuPos.y, WINDOW_HEIGHT - menuHeight);

        // Draw menu background
        glBegin(GL_QUADS);
        glColor4f(state.theme.rubyBg.r, state.theme.rubyBg.g, state.theme.rubyBg.b, 0.9f);
        glVertex2f(menuX, menuY);
        glVertex2f(menuX + menuWidth, menuY);
        glVertex2f(menuX + menuWidth, menuY + menuHeight);
        glVertex2f(menuX, menuY + menuHeight);
        glEnd();

        // Draw menu border
        glLineWidth(1.0f);
        glBegin(GL_LINE_LOOP);
        glColor4f(state.theme.rubyBorder.r, state.theme.rubyBorder.g, state.theme.rubyBorder.b, state.theme.rubyBorder.a);
        glVertex2f(menuX, menuY);
        glVertex2f(menuX + menuWidth, menuY);
        glVertex2f(menuX + menuWidth, menuY + menuHeight);
        glVertex2f(menuX, menuY + menuHeight);
        glEnd();

        // Draw menu items
        glColor4f(state.theme.rubyText.r, state.theme.rubyText.g, state.theme.rubyText.b, state.theme.rubyText.a);
        drawText("Add Primitive", menuX + 10.0f, menuY + 20.0f);
        drawText("Add Note", menuX + 10.0f, menuY + 40.0f);
    }
}

// Helper function implementations
std::shared_ptr<Node> getNodeAtPoint(AppState& state, const glm::vec2& point) {
    // Check from front to back (last nodes are on top)
    for (int i = state.nodes.size() - 1; i >= 0; i--) {
        if (state.nodes[i]->isPointInside(point)) {
            return state.nodes[i];
        }
    }
    return nullptr;
}

IOCircle* getCircleAtPoint(AppState& state, const glm::vec2& point, int& nodeId, bool& isInput) {
    for (auto& node : state.nodes) {
        IOCircle* circle = node->getCircleAtPoint(point);
        if (circle) {
            nodeId = node->id;
            isInput = circle->isInput;
            return circle;
        }
    }
    nodeId = -1;
    return nullptr;
}

void createNode(AppState& state, const std::string& nodeType, float x, float y) {
    if (nodeType == "Primitive") {
        state.nodes.push_back(std::make_shared<PrimitiveNode>(state.nextNodeId++, glm::vec2(x, y)));
    }
    else if (nodeType == "Note") {
        state.nodes.push_back(std::make_shared<NoteNode>(state.nextNodeId++, glm::vec2(x, y)));
    }
}

// Main application flow
void update(AppState& state, float deltaTime) {
    state.globalTime += deltaTime;

    // Update sparkles
    state.sparkleTimer -= deltaTime;
    if (state.sparkleTimer <= 0.0f) {
        state.sparkleTimer = 0.2f;
        float x = (float)(rand() % WINDOW_WIDTH);
        float y = (float)(rand() % WINDOW_HEIGHT);
        state.sparkles.push_back(Sparkle(glm::vec2(x, y)));
    }

    // Update existing sparkles
    for (int i = state.sparkles.size() - 1; i >= 0; i--) {
        if (!state.sparkles[i].update(deltaTime)) {
            state.sparkles.erase(state.sparkles.begin() + i);
        }
    }
}

void render(AppState& state) {
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // Setup projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw elements
    drawBackground(state.theme);
    drawTimeline(state);
    drawTransportNode(state);
    drawUI(state);

    // Draw sparkles (on top)
    for (auto& sparkle : state.sparkles) {
        sparkle.draw();
    }

    glfwSwapBuffers(state.window);
}



// Main entry point
// Additional helper functions for node management
void toggleSampleManager(AppState& state) {
    state.sampleManagerHidden = !state.sampleManagerHidden;
    // Adjust timeline area based on visibility
    if (state.sampleManagerHidden) {
        state.timelineArea.x = 0;
        state.timelineArea.z = WINDOW_WIDTH;
    }
    else {
        state.timelineArea.x = state.sampleManagerArea.z;
        state.timelineArea.z = WINDOW_WIDTH - state.sampleManagerArea.z;
    }
}

void toggleFxChain(AppState& state) {
    state.fxChainHidden = !state.fxChainHidden;
    // Adjust timeline area based on visibility
    if (state.fxChainHidden) {
        state.timelineArea.w = WINDOW_HEIGHT;
    }
    else {
        state.timelineArea.w = WINDOW_HEIGHT - state.fxChainArea.w;
    }
}

// Function to handle keypresses
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    AppState* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            // Hide context menu if visible
            state->showContextMenu = false;
        }
        else if (key == GLFW_KEY_S) {
            // Toggle sample manager with 'S' key
            toggleSampleManager(*state);
        }
        else if (key == GLFW_KEY_F) {
            // Toggle FX chain with 'F' key
            toggleFxChain(*state);
        }
        else if (key == GLFW_KEY_N && mods & GLFW_MOD_CONTROL) {
            // Create new node with Ctrl+N near the center of the screen
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            createNode(*state, "Primitive", x, y);
        }
        else if (key == GLFW_KEY_DELETE) {
            // Delete the selected node
            // This would require tracking the currently selected node
            // Not implemented yet
        }
    }
}


// Function to handle scrolling for zoom
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    AppState* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));

    // Check if mouse is over a node first
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    std::shared_ptr<Node> nodeAtPoint = getNodeAtPoint(*state, glm::vec2(x, y));

    if (!nodeAtPoint &&
        x >= state->timelineArea.x &&
        x <= state->timelineArea.x + state->timelineArea.z &&
        y >= state->timelineArea.y &&
        y <= state->timelineArea.y + state->timelineArea.w) {
        // Zoom timeline
        if (yoffset > 0) {
            state->zoom *= 1.1f;
        }
        else {
            state->zoom *= 0.9f;
        }
        state->zoom = std::max(0.1f, std::min(state->zoom, 5.0f));
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Node Editor", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Setup state
    AppState state;
    state.window = window;

    // Set window user pointer for callbacks
    glfwSetWindowUserPointer(window, &state);

    // Set callbacks
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // Add some initial nodes for testing
    createNode(state, "Primitive", 300.0f, 200.0f);
    createNode(state, "Note", 500.0f, 300.0f);

    // Connect nodes for testing
    if (state.nodes.size() >= 2) {
        state.connections.push_back(Connection(0, 0, 1, 0));
    }

    // Main loop
    while (!glfwWindowShouldClose(state.window)) {
        glfwPollEvents();

        float currentTime = glfwGetTime();
        static float lastTime = currentTime;
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        update(state, deltaTime);
        render(state);
    }

    glfwTerminate();
    return 0;
}

// Implementation of Connection::draw
void Connection::draw(const std::vector<std::shared_ptr<Node>>& nodes, float time) {
    // Find source and target nodes
    std::shared_ptr<Node> fromNode = nullptr;
    std::shared_ptr<Node> toNode = nullptr;

    for (auto& node : nodes) {
        if (node->id == fromNodeId) fromNode = node;
        if (node->id == toNodeId) toNode = node;
    }

    if (!fromNode || !toNode) return;

    // Get circle world positions
    glm::vec2 fromPos;
    glm::vec2 toPos;
    glm::vec4 fromColor;
    glm::vec4 toColor;

    if (fromPortIndex < (int)fromNode->outputCircles.size()) {
        fromPos = fromNode->outputCircles[fromPortIndex].getWorldPosition(fromNode->position);
        fromColor = fromNode->outputCircles[fromPortIndex].color;
    }
    else return;

    if (toPortIndex < (int)toNode->inputCircles.size()) {
        toPos = toNode->inputCircles[toPortIndex].getWorldPosition(toNode->position);
        toColor = toNode->inputCircles[toPortIndex].color;
    }
    else return;

    // Draw curved wire between nodes
    float dx = toPos.x - fromPos.x;
    float dy = toPos.y - fromPos.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.0001f) len = 0.0001f;

    float perpX = -dy / len;
    float perpY = dx / len;

    // Draw wire layers
    for (auto& layer : layers) {
        float offset = layer.baseOffset + std::sin(time / 20.0f + layer.phase) * layer.variationAmplitude;
        float mx = (fromPos.x + toPos.x) * 0.5f;
        float my = (fromPos.y + toPos.y) * 0.5f;
        float cp_x = mx + perpX * offset;
        float cp_y = my + perpY * offset;

        float width = layer.baseWidth + std::sin(time / 10.0f + layer.phase) * layer.widthVariation;

        glLineWidth(width);
        glBegin(GL_LINE_STRIP);

        // Draw bezier curve with color gradient
        const int steps = 20;
        for (int j = 0; j <= steps; j++) {
            float t = (float)j / steps;
            float u = 1.0f - t;
            float tt = t * t;
            float uu = u * u;

            float x = uu * fromPos.x + 2 * u * t * cp_x + tt * toPos.x;
            float y = uu * fromPos.y + 2 * u * t * cp_y + tt * toPos.y;

            glm::vec4 color = fromColor * (1.0f - t) + toColor * t;
            glColor4f(color.r, color.g, color.b, layer.opacity);

            glVertex2f(x, y);
        }
        glEnd();
    }
}
