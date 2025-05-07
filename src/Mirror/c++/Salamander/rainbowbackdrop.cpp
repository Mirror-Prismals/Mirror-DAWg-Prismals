// main.cpp
#define _CRT_SECURE_NO_WARNINGS

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <vector>
#include <string>
#include <sstream>
#include "stb_easy_font.h"

// ----------------------------------------------------
// Global Definitions & Helper Structures

struct Color {
    float r, g, b, a;
};

const Color sampleCol = { 0.501f, 0.188f, 0.188f, 1.0f };
const Color fxchainCol = { 0.439f, 0.063f, 0.063f, 1.0f };
const Color textColor = { 0.9f, 0.9f, 0.9f, 1.0f };
const Color timelineCol = { 0.376f, 0.125f, 0.125f, 1.0f };
const Color defaultPortCol = { 0.8f, 0.8f, 0.8f, 1.0f };
// Two default colors for the node halves.
const Color defaultNodeColor = { 0.3f, 0.7f, 0.3f, 1.0f };
const Color defaultNodeColor2 = { 0.3f, 0.3f, 0.7f, 1.0f };

Color hexToColor(const std::string& hex) {
    unsigned int ri = 255, gi = 255, bi = 255;
    const char* str = hex.c_str();
    if (hex[0] == '#')
        sscanf(str, "#%02x%02x%02x", &ri, &gi, &bi);
    else
        sscanf(str, "%02x%02x%02x", &ri, &gi, &bi);
    return { ri / 255.0f, gi / 255.0f, bi / 255.0f, 1.0f };
}

// ----------------------------------------------------
// Global Animation and Window State

float sampleAnim = 0.0f;
float fxAnim = 0.0f;
bool sampleHidden = false;
bool fxHidden = false;
const float animSpeed = 4.0f;

int winWidth = 1024, winHeight = 768;
GLFWmonitor* primaryMonitor = nullptr;
float sampleWidth = 0;
float fxHeight = 0;

// ----------------------------------------------------
// Node-Based UI Structures and Global State

struct Node {
    glm::vec2 pos;
    float size;
    // Two half colors for the front face.
    Color halfColor1;
    Color halfColor2;
    // Port colors.
    Color portTop, portBottom, portLeft, portRight;
    bool dragging = false;
    glm::vec2 dragOffset;
    // Node text and its color.
    std::string label;
    Color labelColor;

    Node() : pos(0, 0), size(100.0f),
        halfColor1(defaultNodeColor), halfColor2(defaultNodeColor2),
        portTop(defaultPortCol), portBottom(defaultPortCol),
        portLeft(defaultPortCol), portRight(defaultPortCol),
        label("Node"), labelColor(textColor) {
    }

    glm::vec2 getPortPos(const std::string& port) const {
        if (port == "top")    return pos + glm::vec2(size * 0.5f, 0);
        if (port == "bottom") return pos + glm::vec2(size * 0.5f, size);
        if (port == "left")   return pos + glm::vec2(0, size * 0.5f);
        if (port == "right")  return pos + glm::vec2(size, size * 0.5f);
        return pos;
    }
    Color getPortColor(const std::string& port) const {
        if (port == "top")    return portTop;
        if (port == "bottom") return portBottom;
        if (port == "left")   return portLeft;
        if (port == "right")  return portRight;
        return defaultPortCol;
    }
    bool contains(const glm::vec2& p) const {
        return (p.x >= pos.x && p.x <= pos.x + size &&
            p.y >= pos.y && p.y <= pos.y + size);
    }
};

struct Connection {
    int fromNode;
    std::string fromPort;
    int toNode;
    std::string toPort;
};

std::vector<Node> nodes;
std::vector<Connection> connections;

glm::vec2 timelineOffset;
float timelineWidth = 0;
float timelineHeight = 0;

bool mouseDown = false;
double mouseX, mouseY;
int activeNodeIndex = -1;
bool draggingWire = false;
int wireFromNode = -1;
std::string wireFromPort = "";
glm::vec2 wireStart, wireEnd;

// Editing states for half-colors.
bool isColorEditing = false;
int activeColorNode = -1;
std::string hexInput = "";
int activeHalf = 0; // 1 for halfColor1, 2 for halfColor2.

// Port color editing.
bool isPortColorEditing = false;
int activePortNode = -1;
std::string activePortName = "";
std::string portHexInput = "";

// Node text editing.
bool isTextEditing = false;
int activeTextNode = -1;
std::string textInput = "";

// Node text color editing.
bool isTextColorEditing = false;
int activeTextColorNode = -1;
std::string textColorInput = "";

// Helper to determine if a point is inside the node’s text region.
bool isInsideTextRegion(const Node& node, const glm::vec2& pt) {
    float textX = node.pos.x + node.size * 0.3f;
    float textY = node.pos.y + node.size * 0.4f;
    float textW = node.size * 0.4f;
    float textH = node.size * 0.2f;
    return (pt.x >= textX && pt.x <= textX + textW &&
        pt.y >= textY && pt.y <= textY + textH);
}

// ----------------------------------------------------
// Dynamic Gradient Background (single definition)
void renderDynamicBackground(double time) {
    float r1 = 0.5f + 0.5f * sin(time + 0.0f);
    float g1 = 0.5f + 0.5f * sin(time + 2.0f);
    float b1 = 0.5f + 0.5f * sin(time + 4.0f);

    float r2 = 0.5f + 0.5f * sin(time + 1.0f);
    float g2 = 0.5f + 0.5f * sin(time + 3.0f);
    float b2 = 0.5f + 0.5f * sin(time + 5.0f);

    float r3 = 0.5f + 0.5f * sin(time + 2.0f);
    float g3 = 0.5f + 0.5f * sin(time + 4.0f);
    float b3 = 0.5f + 0.5f * sin(time + 6.0f);

    float r4 = 0.5f + 0.5f * sin(time + 3.0f);
    float g4 = 0.5f + 0.5f * sin(time + 5.0f);
    float b4 = 0.5f + 0.5f * sin(time + 7.0f);

    glDisable(GL_DEPTH_TEST);
    glBegin(GL_QUADS);
    glColor3f(r1, g1, b1); glVertex2f(0, 0);
    glColor3f(r2, g2, b2); glVertex2f(winWidth, 0);
    glColor3f(r3, g3, b3); glVertex2f(winWidth, winHeight);
    glColor3f(r4, g4, b4); glVertex2f(0, winHeight);
    glEnd();
    glEnable(GL_DEPTH_TEST);
}

// ----------------------------------------------------
// Utility Drawing Functions (Existing ones)

// Original 3D panel drawing helper (used for panels)
void drawPanel3D(float bx, float by, float bw, float bh, float depth,
    const Color& baseColor, float pressAnim, bool darkTheme)
{
    float shift = 10.0f * pressAnim;
    float pressOffsetZ = depth * pressAnim;
    float newDepth = depth * (1.0f - 0.5f * pressAnim);
    float x = bx - shift;
    float y = by;

    glColor4f(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glEnd();

    glColor4f(baseColor.r * 1.1f, baseColor.g * 1.1f, baseColor.b * 1.1f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    glColor4f(baseColor.r * 0.9f, baseColor.g * 0.9f, baseColor.b * 0.9f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    glColor4f(baseColor.r * 1.05f, baseColor.g * 1.05f, baseColor.b * 1.05f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    glColor4f(baseColor.r * 0.95f, baseColor.g * 0.95f, baseColor.b * 0.95f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();
}

// New helper: Draw a node with a non‑inverted, diagonally split 3D effect.
void drawNode3D(float bx, float by, float bw, float bh, float depth, float pressAnim,
    const Color& color1, const Color& color2)
{
    float shiftLeft = 10.0f * pressAnim;
    float pressOffsetZ = depth * pressAnim;
    float newDepth = depth * (1.0f - 0.5f * pressAnim);
    float x = bx - shiftLeft;
    float y = by;
    float frontFactor = 0.8f - 0.2f * (pressAnim * 2.0f);

    glBegin(GL_TRIANGLES);
    glColor3f(color1.r * frontFactor, color1.g * frontFactor, color1.b * frontFactor);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);

    glColor3f(color2.r * frontFactor, color2.g * frontFactor, color2.b * frontFactor);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glEnd();

    Color avgColor = { (color1.r + color2.r) * 0.5f,
                       (color1.g + color2.g) * 0.5f,
                       (color1.b + color2.b) * 0.5f,
                       1.0f };

    glColor3f(avgColor.r * 0.9f, avgColor.g * 0.9f, avgColor.b * 0.9f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    glColor3f(avgColor.r * 0.6f, avgColor.g * 0.6f, avgColor.b * 0.6f);
    glBegin(GL_QUADS);
    glVertex3f(x + bw, y, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x + bw - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    glColor3f(avgColor.r * 0.7f, avgColor.g * 0.7f, avgColor.b * 0.7f);
    glBegin(GL_QUADS);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x + bw, y + bh, -pressOffsetZ);
    glVertex3f(x + bw - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glEnd();

    glColor3f(avgColor.r * 0.65f, avgColor.g * 0.65f, avgColor.b * 0.65f);
    glBegin(GL_QUADS);
    glVertex3f(x, y, -pressOffsetZ);
    glVertex3f(x, y + bh, -pressOffsetZ);
    glVertex3f(x - newDepth, y + bh - newDepth, -(pressOffsetZ + newDepth));
    glVertex3f(x - newDepth, y - newDepth, -(pressOffsetZ + newDepth));
    glEnd();
}

// Render text.
void renderText(float x, float y, const char* text, const Color& col)
{
    char buffer[99999];
    int quads = stb_easy_font_print(x, y, (char*)text, nullptr, buffer, sizeof(buffer));
    glColor4f(col.r, col.g, col.b, col.a);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// 2D helpers.
void drawRect(float x, float y, float w, float h, const Color& col) {
    glColor4f(col.r, col.g, col.b, col.a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawText2D(float x, float y, const char* text, const Color& col) {
    char buffer[99999];
    int quads = stb_easy_font_print(x, y, (char*)text, nullptr, buffer, sizeof(buffer));
    glColor4f(col.r, col.g, col.b, col.a);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void drawCircle(float cx, float cy, float radius, const Color& col) {
    glColor4f(col.r, col.g, col.b, col.a);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    const int segments = 20;
    for (int i = 0; i <= segments; i++) {
        float theta = i * 2.0f * 3.1415926f / segments;
        glVertex2f(cx + radius * cosf(theta), cy + radius * sinf(theta));
    }
    glEnd();
}

void drawRibbonBezier(const glm::vec2& p0, const glm::vec2& p1,
    const Color& colStart, const Color& colEnd, float baseWidth = 4.0f) {
    const int segments = 30;
    double timeOffset = glfwGetTime();
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= segments; i++) {
        float t = i / float(segments);
        float u = 1.0f - t;
        glm::vec2 center = u * p0 + t * p1;
        glm::vec2 dir = glm::normalize(p1 - p0);
        glm::vec2 perp = glm::vec2(-dir.y, dir.x);
        float offset = 3.0f * sinf(2.0f * 3.1415926f * (10 * t + timeOffset));
        center += perp * offset;
        float dynamicWidth = baseWidth + 2.0f * sinf(2.0f * 3.1415926f * (5 * t + timeOffset));
        glm::vec2 left = center + perp * (dynamicWidth * 0.5f);
        glm::vec2 right = center - perp * (dynamicWidth * 0.5f);
        Color col;
        col.r = colStart.r * (1 - t) + colEnd.r * t;
        col.g = colStart.g * (1 - t) + colEnd.g * t;
        col.b = colStart.b * (1 - t) + colEnd.b * t;
        col.a = colStart.a * (1 - t) + colEnd.a * t;
        glColor4f(col.r, col.g, col.b, col.a);
        glVertex2f(left.x, left.y);
        glVertex2f(right.x, right.y);
    }
    glEnd();
}

std::pair<int, std::string> checkPortHit(const glm::vec2& pt, float hitRadius = 10.0f) {
    for (size_t i = 0; i < nodes.size(); i++) {
        const Node& node = nodes[i];
        std::vector<std::string> ports = { "top", "bottom", "left", "right" };
        for (const auto& port : ports) {
            glm::vec2 portPos = node.getPortPos(port);
            float dx = pt.x - portPos.x;
            float dy = pt.y - portPos.y;
            if (sqrtf(dx * dx + dy * dy) <= hitRadius)
                return { (int)i, port };
        }
    }
    return { -1, "" };
}

// ----------------------------------------------------
// Animation Update Function

void updateAnimations(double dt) {
    float targetSample = sampleHidden ? 1.0f : 0.0f;
    float targetFx = fxHidden ? 1.0f : 0.0f;
    sampleAnim += (targetSample - sampleAnim) * dt * animSpeed;
    fxAnim += (targetFx - fxAnim) * dt * animSpeed;
}

// ----------------------------------------------------
// Rendering Functions

void renderPanels() {
    sampleWidth = winWidth * 0.2f * (1.0f - sampleAnim);
    fxHeight = winHeight * 0.3f * (1.0f - fxAnim);
    float panelDepth = 15.0f;
    float samplePanelX = 0 - sampleAnim * (winWidth * 0.2f);
    float samplePanelY = 10;
    float samplePanelW = winWidth * 0.2f;
    float samplePanelH = winHeight - 10;
    glPushMatrix();
    float centerX = samplePanelX + samplePanelW * 0.5f;
    float centerY = samplePanelY + samplePanelH * 0.5f;
    glTranslatef(centerX, centerY, 0);
    glRotatef(180.0f, 0, 1, 0);
    glTranslatef(-centerX, -centerY, 0);
    drawPanel3D(samplePanelX, samplePanelY, samplePanelW, samplePanelH,
        panelDepth, sampleCol, 0.0f, false);
    renderText(samplePanelX + 10, samplePanelY + 30, "SAMPLE MANAGER", textColor);
    glPopMatrix();

    float fxPanelX = sampleWidth;
    float fxPanelY = winHeight - fxHeight;
    float fxPanelW = winWidth - sampleWidth;
    float fxPanelH = fxHeight;
    drawPanel3D(fxPanelX, fxPanelY, fxPanelW, fxPanelH,
        panelDepth, fxchainCol, 0.0f, false);
    renderText(fxPanelX + 10, fxPanelY + 30, "FX CHAIN", textColor);
}

void renderNodeUI() {
    sampleWidth = winWidth * 0.2f * (1.0f - sampleAnim);
    fxHeight = winHeight * 0.3f * (1.0f - fxAnim);
    timelineOffset = glm::vec2(sampleWidth, 0);
    timelineWidth = winWidth - sampleWidth;
    timelineHeight = winHeight - fxHeight;

    drawRect(timelineOffset.x, 0, timelineWidth, winHeight - fxHeight, timelineCol);

    glPushMatrix();
    glTranslatef(timelineOffset.x, 0, 0);

    for (const auto& conn : connections) {
        if (conn.fromNode < 0 || conn.fromNode >= (int)nodes.size() ||
            conn.toNode < 0 || conn.toNode >= (int)nodes.size())
            continue;
        const Node& nFrom = nodes[conn.fromNode];
        const Node& nTo = nodes[conn.toNode];
        Color effectiveFrom = {
            (nFrom.halfColor1.r + nFrom.getPortColor(conn.fromPort).r) / 2.0f,
            (nFrom.halfColor1.g + nFrom.getPortColor(conn.fromPort).g) / 2.0f,
            (nFrom.halfColor1.b + nFrom.getPortColor(conn.fromPort).b) / 2.0f,
            (nFrom.halfColor1.a + nFrom.getPortColor(conn.fromPort).a) / 2.0f
        };
        Color effectiveTo = {
            (nTo.halfColor1.r + nTo.getPortColor(conn.toPort).r) / 2.0f,
            (nTo.halfColor1.g + nTo.getPortColor(conn.toPort).g) / 2.0f,
            (nTo.halfColor1.b + nTo.getPortColor(conn.toPort).b) / 2.0f,
            (nTo.halfColor1.a + nTo.getPortColor(conn.toPort).a) / 2.0f
        };
        glm::vec2 start = nFrom.getPortPos(conn.fromPort);
        glm::vec2 end = nTo.getPortPos(conn.toPort);
        drawRibbonBezier(start, end, effectiveFrom, effectiveTo, 6.0f);
    }

    if (draggingWire) {
        if (wireFromNode >= 0 && wireFromNode < (int)nodes.size()) {
            const Node& nFrom = nodes[wireFromNode];
            Color effective = {
                (nFrom.halfColor1.r + nFrom.getPortColor(wireFromPort).r) / 2.0f,
                (nFrom.halfColor1.g + nFrom.getPortColor(wireFromPort).g) / 2.0f,
                (nFrom.halfColor1.b + nFrom.getPortColor(wireFromPort).b) / 2.0f,
                (nFrom.halfColor1.a + nFrom.getPortColor(wireFromPort).a) / 2.0f
            };
            drawRibbonBezier(wireStart, wireEnd, effective, effective, 6.0f);
        }
    }

    for (const auto& node : nodes) {
        drawNode3D(node.pos.x, node.pos.y, node.size, node.size, 10.0f, 0.0f,
            node.halfColor1, node.halfColor2);
        std::vector<std::string> ports = { "top", "bottom", "left", "right" };
        for (const auto& port : ports) {
            glm::vec2 p = node.getPortPos(port);
            drawCircle(p.x, p.y, 6.0f, node.getPortColor(port));
        }
        drawText2D(node.pos.x + node.size * 0.35f, node.pos.y + node.size * 0.45f,
            node.label.c_str(), node.labelColor);
    }

    glPopMatrix();

    if (isColorEditing && activeColorNode >= 0 && activeColorNode < (int)nodes.size()) {
        Node node = nodes[activeColorNode];
        glm::vec2 screenPos = node.pos + timelineOffset;
        std::string prompt = (activeHalf == 1 ? "Half1 Hex: " : "Half2 Hex: ") + hexInput;
        drawText2D(screenPos.x, screenPos.y - 20, prompt.c_str(), textColor);
    }
    if (isTextEditing && activeTextNode >= 0 && activeTextNode < (int)nodes.size()) {
        Node node = nodes[activeTextNode];
        glm::vec2 screenPos = node.pos + timelineOffset;
        std::string prompt = "Text: " + textInput;
        drawText2D(screenPos.x, screenPos.y - 40, prompt.c_str(), textColor);
    }
    if (isTextColorEditing && activeTextColorNode >= 0 && activeTextColorNode < (int)nodes.size()) {
        Node node = nodes[activeTextColorNode];
        glm::vec2 screenPos = node.pos + timelineOffset;
        std::string prompt = "Text Color Hex: " + textColorInput;
        drawText2D(screenPos.x, screenPos.y - 40, prompt.c_str(), textColor);
    }
}

// ----------------------------------------------------
// GLFW Callback Functions

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glm::vec2 mousePos(mouseX - timelineOffset.x, mouseY);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mouseDown = true;
            auto portHit = checkPortHit(mousePos);
            if (portHit.first != -1) {
                draggingWire = true;
                wireFromNode = portHit.first;
                wireFromPort = portHit.second;
                wireStart = nodes[wireFromNode].getPortPos(wireFromPort);
                wireEnd = wireStart;
                return;
            }
            activeNodeIndex = -1;
            for (size_t i = 0; i < nodes.size(); i++) {
                if (nodes[i].contains(mousePos)) {
                    activeNodeIndex = (int)i;
                    nodes[i].dragging = true;
                    nodes[i].dragOffset = mousePos - nodes[i].pos;
                    break;
                }
            }
        }
        else if (action == GLFW_RELEASE) {
            mouseDown = false;
            if (activeNodeIndex != -1) {
                nodes[activeNodeIndex].dragging = false;
                activeNodeIndex = -1;
            }
            if (draggingWire) {
                auto portHit = checkPortHit(mousePos);
                if (portHit.first != -1 && portHit.first != wireFromNode && portHit.second != wireFromPort) {
                    Connection conn;
                    conn.fromNode = wireFromNode;
                    conn.fromPort = wireFromPort;
                    conn.toNode = portHit.first;
                    conn.toPort = portHit.second;
                    connections.push_back(conn);
                }
                draggingWire = false;
                wireFromNode = -1;
                wireFromPort = "";
            }
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        for (size_t i = 0; i < nodes.size(); i++) {
            if (nodes[i].contains(mousePos) && isInsideTextRegion(nodes[i], mousePos)) {
                isTextEditing = true;
                activeTextNode = i;
                textInput = nodes[i].label;
                break;
            }
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
        for (size_t i = 0; i < nodes.size(); i++) {
            if (nodes[i].contains(mousePos)) {
                if (isInsideTextRegion(nodes[i], mousePos)) {
                    isTextColorEditing = true;
                    activeTextColorNode = i;
                    textColorInput = "";
                }
                else {
                    isColorEditing = true;
                    activeColorNode = i;
                    float localX = mousePos.x - nodes[i].pos.x;
                    float localY = mousePos.y - nodes[i].pos.y;
                    activeHalf = (localX > localY) ? 1 : 2;
                    hexInput = "";
                }
                break;
            }
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    double dx = xpos - mouseX, dy = ypos - mouseY;
    mouseX = xpos; mouseY = ypos;
    glm::vec2 mousePos(mouseX - timelineOffset.x, mouseY);
    if (activeNodeIndex != -1 && nodes[activeNodeIndex].dragging) {
        nodes[activeNodeIndex].pos = mousePos - nodes[activeNodeIndex].dragOffset;
    }
    if (draggingWire) {
        wireEnd = mousePos;
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (isPortColorEditing) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            if (key == GLFW_KEY_BACKSPACE && !portHexInput.empty()) {
                portHexInput.pop_back();
            }
            else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
                if (portHexInput.length() == 6 && activePortNode >= 0 && activePortNode < (int)nodes.size()) {
                    Color newPortColor = hexToColor(portHexInput);
                    Node& node = nodes[activePortNode];
                    if (activePortName == "top")
                        node.portTop = newPortColor;
                    else if (activePortName == "bottom")
                        node.portBottom = newPortColor;
                    else if (activePortName == "left")
                        node.portLeft = newPortColor;
                    else if (activePortName == "right")
                        node.portRight = newPortColor;
                }
                isPortColorEditing = false;
                activePortNode = -1;
                activePortName = "";
                portHexInput = "";
            }
            else {
                if ((key >= GLFW_KEY_0 && key <= GLFW_KEY_9) ||
                    (key >= GLFW_KEY_A && key <= GLFW_KEY_F)) {
                    if (portHexInput.size() < 6) {
                        const char* keyName = glfwGetKeyName(key, 0);
                        if (keyName && keyName[0]) {
                            char c = keyName[0];
                            if (c >= 'a' && c <= 'f')
                                c = c - 'a' + 'A';
                            portHexInput.push_back(c);
                        }
                    }
                }
            }
        }
        return;
    }
    if (isColorEditing) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            if (key == GLFW_KEY_BACKSPACE && !hexInput.empty()) {
                hexInput.pop_back();
            }
            else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
                if (hexInput.length() == 6 && activeColorNode >= 0 && activeColorNode < (int)nodes.size()) {
                    if (activeHalf == 1)
                        nodes[activeColorNode].halfColor1 = hexToColor(hexInput);
                    else if (activeHalf == 2)
                        nodes[activeColorNode].halfColor2 = hexToColor(hexInput);
                }
                isColorEditing = false;
                activeColorNode = -1;
                hexInput = "";
                activeHalf = 0;
            }
            else {
                if ((key >= GLFW_KEY_0 && key <= GLFW_KEY_9) ||
                    (key >= GLFW_KEY_A && key <= GLFW_KEY_F)) {
                    if (hexInput.size() < 6) {
                        const char* keyName = glfwGetKeyName(key, 0);
                        if (keyName && keyName[0]) {
                            char c = keyName[0];
                            if (c >= 'a' && c <= 'f')
                                c = c - 'a' + 'A';
                            hexInput.push_back(c);
                        }
                    }
                }
            }
        }
        return;
    }
    if (isTextEditing) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            if (key == GLFW_KEY_BACKSPACE && !textInput.empty()) {
                textInput.pop_back();
            }
            else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
                if (activeTextNode >= 0 && activeTextNode < (int)nodes.size()) {
                    nodes[activeTextNode].label = textInput;
                }
                isTextEditing = false;
                activeTextNode = -1;
                textInput = "";
            }
            else {
                const char* keyName = glfwGetKeyName(key, 0);
                if (keyName && keyName[0]) {
                    textInput.push_back(keyName[0]);
                }
            }
        }
        return;
    }
    if (isTextColorEditing) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            if (key == GLFW_KEY_BACKSPACE && !textColorInput.empty()) {
                textColorInput.pop_back();
            }
            else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
                if (textColorInput.length() == 6 && activeTextColorNode >= 0 && activeTextColorNode < (int)nodes.size()) {
                    nodes[activeTextColorNode].labelColor = hexToColor(textColorInput);
                }
                isTextColorEditing = false;
                activeTextColorNode = -1;
                textColorInput = "";
            }
            else {
                if ((key >= GLFW_KEY_0 && key <= GLFW_KEY_9) ||
                    (key >= GLFW_KEY_A && key <= GLFW_KEY_F)) {
                    if (textColorInput.size() < 6) {
                        const char* keyName = glfwGetKeyName(key, 0);
                        if (keyName && keyName[0]) {
                            char c = keyName[0];
                            if (c >= 'a' && c <= 'f')
                                c = c - 'a' + 'A';
                            textColorInput.push_back(c);
                        }
                    }
                }
            }
        }
        return;
    }
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_N) {
            Node newNode;
            newNode.pos = glm::vec2(rand() % (int)(timelineWidth - newNode.size),
                rand() % (int)(timelineHeight - newNode.size));
            newNode.halfColor1 = defaultNodeColor;
            newNode.halfColor2 = defaultNodeColor2;
            newNode.label = "Node";
            newNode.labelColor = textColor;
            nodes.push_back(newNode);
        }
        else if (key == GLFW_KEY_S) {
            sampleHidden = !sampleHidden;
        }
        else if (key == GLFW_KEY_X) {
            fxHidden = !fxHidden;
        }
        else if (key == GLFW_KEY_F) {
            static bool isFullscreen = false;
            static int windowedX, windowedY, windowedWidth, windowedHeight;
            GLFWwindow* curr = glfwGetCurrentContext();
            if (!isFullscreen) {
                glfwGetWindowPos(curr, &windowedX, &windowedY);
                glfwGetWindowSize(curr, &windowedWidth, &windowedHeight);
                primaryMonitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
                winWidth = mode->width;
                winHeight = mode->height;
                glfwSetWindowMonitor(curr, primaryMonitor, 0, 0, winWidth, winHeight, mode->refreshRate);
            }
            else {
                winWidth = windowedWidth;
                winHeight = windowedHeight;
                glfwSetWindowMonitor(curr, nullptr, windowedX, windowedY, winWidth, winHeight, 0);
            }
            isFullscreen = !isFullscreen;
            glViewport(0, 0, winWidth, winHeight);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, winWidth, winHeight, 0, -100, 100);
        }
    }
}

// ----------------------------------------------------
// Main Entry Point

int main(void) {
    srand((unsigned int)time(nullptr));
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    winWidth = mode->width * 0.8;
    winHeight = mode->height * 0.8;

    GLFWwindow* window = glfwCreateWindow(winWidth, winHeight, "Merged 3D Panels and Node UI", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, winWidth, winHeight, 0, -100, 100);
    glViewport(0, 0, winWidth, winHeight);

    Node init;
    init.pos = glm::vec2(50, 50);
    init.halfColor1 = defaultNodeColor;
    init.halfColor2 = defaultNodeColor2;
    init.label = "Node";
    init.labelColor = textColor;
    nodes.push_back(init);

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double dt = currentTime - lastTime;
        lastTime = currentTime;
        updateAnimations(dt);

        // Render the dynamic background.
        renderDynamicBackground(currentTime);

        // Render timeline and nodes.
        renderNodeUI();
        // Render panels on top.
        renderPanels();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
