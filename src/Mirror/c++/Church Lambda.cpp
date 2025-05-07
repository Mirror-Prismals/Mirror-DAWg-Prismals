// manual_smooth_stepping.cpp
//
// This program:
//  1) Parses a lambda expression (factorial(3)) and computes up to 1000 beta-reduction steps.
//  2) Builds Tromp-style "corner" diagrams for each step, scales & centers them.
//  3) Lets you manually step through the reduction sequence using UP/DOWN arrow keys.
//  4) When an arrow key is pressed, smoothly interpolates (both geometry and camera) from the current step to the target step.
//  5) If you try to step out of range, nothing happens.
//  6) Uses GLFW for windowing and OpenGL for rendering.
// 
// Compile example (Linux):
//   g++ -std=c++11 manual_smooth_stepping.cpp -o smooth_step -lglfw -ldl -lGL
//
// Run:
//   ./smooth_step
//

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <chrono>
#include <limits>

// -----------------------------------
// Global Tuning Variables
// -----------------------------------
static float SCALE_X = 40.f;      // horizontal scaling for geometry
static float SCALE_Y = 10.f;      // vertical scaling for geometry
static float BASE_VAR_SIZE = 1.f; // base size of variable corner
static float BASE_BOX_SIZE = 1.f; // extra margin for boxes
static float BASE_GAP = 1.f; // gap for application
static float BASE_LAM_OFF = 1.f; // vertical offset for lambda body
static float GLOBAL_MARGIN = 50.f; // margin for final camera bounding

// Duration to interpolate between steps (in seconds)
static float INTERP_DURATION = 0.5f;

// Vertex buffer size (in floats)
static const size_t VERTEX_BUFFER_SIZE = 20'000'000;

// -----------------------------------
// 1) Lambda AST
// -----------------------------------
class Expr {
public:
    virtual ~Expr() {}
    virtual std::string toString() const = 0;
};
using ExprPtr = std::shared_ptr<Expr>;

class Var : public Expr {
public:
    std::string name;
    Var(const std::string& n) : name(n) {}
    std::string toString() const override { return name; }
};

class Lam : public Expr {
public:
    std::string var;
    ExprPtr body;
    Lam(const std::string& v, ExprPtr b) : var(v), body(b) {}
    std::string toString() const override {
        return "(\\" + var + "." + body->toString() + ")";
    }
};

class App : public Expr {
public:
    ExprPtr func;
    ExprPtr arg;
    App(ExprPtr f, ExprPtr a) : func(f), arg(a) {}
    std::string toString() const override {
        return "(" + func->toString() + " " + arg->toString() + ")";
    }
};

// -----------------------------------
// 2) Parser (left-associative, comment skipping)
// -----------------------------------
class Parser {
public:
    Parser(const std::string& s) : input(s), pos(0) {}
    ExprPtr parseExpr() {
        skipWS();
        if (pos >= input.size()) throw std::runtime_error("Unexpected end");
        char c = input[pos];
        if (c == '\\') {
            pos++;
            skipWS();
            std::string var = parseIdent();
            skipWS();
            if (pos >= input.size() || input[pos] != '.') {
                throw std::runtime_error("Expected '.' in lambda");
            }
            pos++;
            ExprPtr body = parseExpr();
            return std::make_shared<Lam>(var, body);
        }
        else if (c == '(') {
            pos++;
            skipWS();
            ExprPtr left = parseExpr();
            skipWS();
            while (pos < input.size() && input[pos] != ')') {
                ExprPtr right = parseExpr();
                left = std::make_shared<App>(left, right);
                skipWS();
            }
            if (pos >= input.size() || input[pos] != ')') {
                throw std::runtime_error("Missing ')' in application");
            }
            pos++;
            return left;
        }
        else {
            std::string id = parseIdent();
            return std::make_shared<Var>(id);
        }
    }
private:
    std::string input;
    size_t pos;
    void skipWS() {
        while (pos < input.size()) {
            if (std::isspace(input[pos])) {
                pos++;
            }
            else if (input[pos] == ';') {
                while (pos < input.size() && input[pos] != '\n') pos++;
            }
            else {
                break;
            }
        }
    }
    std::string parseIdent() {
        skipWS();
        size_t start = pos;
        while (pos < input.size() && (std::isalnum(input[pos]) || input[pos] == '_')) {
            pos++;
        }
        if (start == pos) throw std::runtime_error("Expected identifier");
        return input.substr(start, pos - start);
    }
};

// -----------------------------------
// 3) Beta Reduction (Naive)
// -----------------------------------
ExprPtr substitute(const ExprPtr& expr, const std::string& var, const ExprPtr& val) {
    if (!expr) return expr;
    if (auto v = std::dynamic_pointer_cast<Var>(expr)) {
        if (v->name == var) return val;
        return expr;
    }
    if (auto l = std::dynamic_pointer_cast<Lam>(expr)) {
        if (l->var == var) {
            return expr;
        }
        else {
            return std::make_shared<Lam>(l->var, substitute(l->body, var, val));
        }
    }
    if (auto a = std::dynamic_pointer_cast<App>(expr)) {
        return std::make_shared<App>(
            substitute(a->func, var, val),
            substitute(a->arg, var, val)
        );
    }
    return expr;
}

std::pair<ExprPtr, bool> betaReduce(const ExprPtr& expr) {
    if (auto a = std::dynamic_pointer_cast<App>(expr)) {
        if (auto l = std::dynamic_pointer_cast<Lam>(a->func)) {
            ExprPtr replaced = substitute(l->body, l->var, a->arg);
            return { replaced, true };
        }
        else {
            auto [f2, changedF] = betaReduce(a->func);
            if (changedF) {
                return { std::make_shared<App>(f2, a->arg), true };
            }
            auto [arg2, changedA] = betaReduce(a->arg);
            if (changedA) {
                return { std::make_shared<App>(a->func, arg2), true };
            }
            return { expr, false };
        }
    }
    else if (auto l = std::dynamic_pointer_cast<Lam>(expr)) {
        auto [body2, changedB] = betaReduce(l->body);
        if (changedB) {
            return { std::make_shared<Lam>(l->var, body2), true };
        }
        return { expr, false };
    }
    return { expr, false };
}

std::vector<ExprPtr> reduceSteps(const ExprPtr& start, int maxSteps = 1000) {
    std::vector<ExprPtr> steps;
    steps.push_back(start);
    ExprPtr current = start;
    for (int i = 0; i < maxSteps; i++) {
        auto [next, changed] = betaReduce(current);
        steps.push_back(next);
        if (!changed) break;
        current = next;
    }
    return steps;
}

// -----------------------------------
// 4) Tromp-Style Corner Diagrams
// -----------------------------------
struct LineSegment {
    float x1, y1, x2, y2;
};
struct Diagram {
    float width = 0.f;
    float height = 0.f;
    std::vector<LineSegment> lines;
};

void offsetDiagram(Diagram& d, float dx, float dy) {
    for (auto& ln : d.lines) {
        ln.x1 += dx; ln.x2 += dx;
        ln.y1 += dy; ln.y2 += dy;
    }
}
void mergeDiagram(Diagram& dest, const Diagram& src) {
    dest.lines.insert(dest.lines.end(), src.lines.begin(), src.lines.end());
}

Diagram buildCornerDiagram(const ExprPtr& expr); // forward declaration

Diagram buildCornerVar(const std::string& v) {
    Diagram d;
    d.width = BASE_VAR_SIZE;
    d.height = BASE_VAR_SIZE;
    d.lines.push_back({ 0, 0, BASE_VAR_SIZE, 0 });
    d.lines.push_back({ 0, 0, 0, -BASE_VAR_SIZE });
    return d;
}
Diagram buildCornerLam(const std::string& var, const ExprPtr& body) {
    Diagram sub = buildCornerDiagram(body);
    float w = std::max(BASE_VAR_SIZE, sub.width) + BASE_BOX_SIZE;
    float h = sub.height + BASE_BOX_SIZE;
    Diagram d;
    d.lines.push_back({ 0, 0, w, 0 });
    d.lines.push_back({ w, 0, w, -h });
    d.lines.push_back({ 0, -h, w, -h });
    d.lines.push_back({ 0, 0, 0, -h });
    offsetDiagram(sub, 0, -BASE_LAM_OFF);
    mergeDiagram(d, sub);
    d.width = w;
    d.height = h;
    return d;
}
Diagram buildCornerApp(const ExprPtr& f, const ExprPtr& a) {
    Diagram df = buildCornerDiagram(f);
    Diagram da = buildCornerDiagram(a);
    offsetDiagram(da, df.width + BASE_GAP, 0);
    float w = df.width + BASE_GAP + da.width + BASE_BOX_SIZE;
    float h = std::max(df.height, da.height) + BASE_BOX_SIZE;
    Diagram d;
    d.lines.push_back({ 0, 0, w, 0 });
    d.lines.push_back({ w, 0, w, -h });
    d.lines.push_back({ 0, -h, w, -h });
    d.lines.push_back({ 0, 0, 0, -h });
    mergeDiagram(d, df);
    mergeDiagram(d, da);
    d.width = w;
    d.height = h;
    return d;
}
Diagram buildCornerDiagram(const ExprPtr& expr) {
    if (!expr) return Diagram{};
    if (auto v = std::dynamic_pointer_cast<Var>(expr))
        return buildCornerVar(v->name);
    else if (auto l = std::dynamic_pointer_cast<Lam>(expr))
        return buildCornerLam(l->var, l->body);
    else if (auto a = std::dynamic_pointer_cast<App>(expr))
        return buildCornerApp(a->func, a->arg);
    return Diagram{};
}

void centerDiagram(Diagram& d) {
    offsetDiagram(d, -d.width * 0.5f, d.height * 0.5f);
}
void scaleDiagram(Diagram& d, float sx, float sy) {
    for (auto& ln : d.lines) {
        ln.x1 *= sx; ln.x2 *= sx;
        ln.y1 *= sy; ln.y2 *= sy;
    }
    d.width *= sx;
    d.height *= sy;
}

// Bounding box struct and helper
struct BBox {
    float minx, maxx, miny, maxy;
};
BBox interpolateBox(const BBox& A, const BBox& B, float t) {
    BBox R;
    R.minx = A.minx * (1.f - t) + B.minx * t;
    R.maxx = A.maxx * (1.f - t) + B.maxx * t;
    R.miny = A.miny * (1.f - t) + B.miny * t;
    R.maxy = A.maxy * (1.f - t) + B.maxy * t;
    return R;
}
glm::mat4 orthoFromBox(const BBox& bb) {
    return glm::ortho(bb.minx, bb.maxx, bb.miny, bb.maxy, -1.f, 1.f);
}

// Interpolate vertices arrays
std::vector<float> interpolateVertices(const std::vector<float>& A, const std::vector<float>& B, float t) {
    size_t n = std::max(A.size(), B.size());
    std::vector<float> R(n, 0.f);
    float padA = A.empty() ? 0.f : A.back();
    float padB = B.empty() ? 0.f : B.back();
    for (size_t i = 0; i < n; i++) {
        float va = (i < A.size() ? A[i] : padA);
        float vb = (i < B.size() ? B[i] : padB);
        R[i] = va * (1.f - t) + vb * t;
    }
    return R;
}

// -----------------------------------
// 5) Factorial(3) Expression
// -----------------------------------
std::string factorialOf3Lambda() {
    return R"(
(
  (
    (\f.(\x.(f (x x))) (\x.(f (x x))))
    (\fact.\n.
      (
         ((\n.n (\x.(\a.(\b.b)) (\a.(\b.a))) n)
          (\s.\z.s z))
         (
           ((\m.\n.\s.\z.m (n s) z) n)
           ((\fact.\n.((\n.\s.\z.n (\g.\h.h (g s)) (\u.z) (\u.u)) n)) fact)
         )
      )
    )
  )
  (\s.\z.s (s (s z)))
)
)";
}

// -----------------------------------
// 6) OpenGL + Manual Step with Smooth Transition (try-catch for key events)
// -----------------------------------
const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;
uniform mat4 projection;
void main(){
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
}
)glsl";
const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;
uniform vec3 lineColor;
void main(){
    FragColor = vec4(lineColor, 1.0);
}
)glsl";

unsigned int compileShader(unsigned int type, const char* src) {
    unsigned int sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    int success;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(sh, 512, nullptr, infoLog);
        std::cerr << "Shader compile error:\n" << infoLog << std::endl;
    }
    return sh;
}
unsigned int createShaderProgram(const char* vs, const char* fs) {
    unsigned int v = compileShader(GL_VERTEX_SHADER, vs);
    unsigned int f = compileShader(GL_FRAGMENT_SHADER, fs);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glLinkProgram(prog);
    int success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(prog, 512, nullptr, infoLog);
        std::cerr << "Shader link error:\n" << infoLog << std::endl;
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return prog;
}

int main() {
    // Parse expression
    std::cout << "Parsing factorial(3) expression...\n";
    ExprPtr expr;
    {
        std::string input = factorialOf3Lambda();
        try {
            Parser p(input);
            expr = p.parseExpr();
            std::cout << "Parsed expression:\n" << expr->toString() << "\n\n";
        }
        catch (std::exception& ex) {
            std::cerr << "Parse error: " << ex.what() << std::endl;
            return 1;
        }
    }

    // Reduce
    std::cout << "Reducing up to 1000 steps...\n";
    auto steps = reduceSteps(expr, 1000);
    std::cout << "Total steps: " << steps.size() << "\n";

    // Build diagrams (scale, center), flatten vertices, compute bounding boxes
    std::vector<std::vector<float>> diagVerts(steps.size());
    std::vector<BBox> diagBoxes(steps.size());
    for (size_t i = 0; i < steps.size(); i++) {
        Diagram d = buildCornerDiagram(steps[i]);
        scaleDiagram(d, SCALE_X, SCALE_Y);
        centerDiagram(d);
        float minx = std::numeric_limits<float>::max();
        float maxx = -std::numeric_limits<float>::max();
        float miny = std::numeric_limits<float>::max();
        float maxy = -std::numeric_limits<float>::max();
        for (auto& ln : d.lines) {
            diagVerts[i].push_back(ln.x1);
            diagVerts[i].push_back(ln.y1);
            diagVerts[i].push_back(ln.x2);
            diagVerts[i].push_back(ln.y2);
            minx = std::min({ minx, ln.x1, ln.x2 });
            maxx = std::max({ maxx, ln.x1, ln.x2 });
            miny = std::min({ miny, ln.y1, ln.y2 });
            maxy = std::max({ maxy, ln.y1, ln.y2 });
        }
        if (d.lines.empty()) {
            diagVerts[i].push_back(0.f);
            diagVerts[i].push_back(0.f);
            diagVerts[i].push_back(0.f);
            diagVerts[i].push_back(0.f);
            minx = maxx = miny = maxy = 0.f;
        }
        float margin = 5.f;
        minx -= margin; maxx += margin;
        miny -= margin; maxy += margin;
        diagBoxes[i] = { minx, maxx, miny, maxy };
    }

    // Init GLFW and OpenGL fullscreen
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(mon);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Manual Stepping with Smooth Transition", mon, nullptr);
    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n";
        return 1;
    }
    glViewport(0, 0, mode->width, mode->height);

    unsigned int shader = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shader);
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, VERTEX_BUFFER_SIZE * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    int projLoc = glGetUniformLocation(shader, "projection");
    int lineColorLoc = glGetUniformLocation(shader, "lineColor");
    glUniform3f(lineColorLoc, 1.f, 1.f, 1.f);

    // Manual stepping state variables
    int currentIndex = 0;
    int targetIndex = 0;
    bool animating = false;
    double transitionStartTime = 0.0;

    // For edge detection of key presses
    bool upPressedLast = false;
    bool downPressedLast = false;

    // Function to set the current step geometry and camera
    auto updateDisplay = [&]() {
        // If not animating, display current step
        std::vector<float> currentVerts = diagVerts.at(currentIndex);
        glm::mat4 proj = orthoFromBox(diagBoxes.at(currentIndex));
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, currentVerts.size() * sizeof(float), currentVerts.data());
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
        };

    // Initialize display with step 0
    updateDisplay();

    std::cout << "Use UP arrow to move forward, DOWN arrow to move backward.\n";

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        bool upPressed = (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
        bool downPressed = (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS);

        // Trigger new transition on rising edge, if not animating
        if (!animating) {
            if (upPressed && !upPressedLast) {
                if (currentIndex < (int)steps.size() - 1) {
                    targetIndex = currentIndex + 1;
                    animating = true;
                    transitionStartTime = glfwGetTime();
                }
            }
            else if (downPressed && !downPressedLast) {
                if (currentIndex > 0) {
                    targetIndex = currentIndex - 1;
                    animating = true;
                    transitionStartTime = glfwGetTime();
                }
            }
        }
        upPressedLast = upPressed;
        downPressedLast = downPressed;

        // If animating, interpolate geometry & camera
        if (animating) {
            double now = glfwGetTime();
            float t = float((now - transitionStartTime) / INTERP_DURATION);
            if (t >= 1.f) {
                // Transition complete
                currentIndex = targetIndex;
                animating = false;
                updateDisplay();
            }
            else {
                // Interpolate geometry and bounding box
                std::vector<float> interpVerts = interpolateVertices(diagVerts.at(currentIndex), diagVerts.at(targetIndex), t);
                BBox interpBox = interpolateBox(diagBoxes.at(currentIndex), diagBoxes.at(targetIndex), t);
                glm::mat4 proj = orthoFromBox(interpBox);
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, interpVerts.size() * sizeof(float), interpVerts.data());
                glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
            }
        }

        // Render frame
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader);
        glBindVertexArray(VAO);
        int vertexCount = (int)diagVerts.at(currentIndex).size() / 2;
        // If animating, use interpolated vertex count (should be same as others)
        if (animating) {
            vertexCount = (int)(std::max(diagVerts.at(currentIndex).size(), diagVerts.at(targetIndex).size()) / 2);
        }
        glDrawArrays(GL_LINES, 0, vertexCount);

        glfwSwapBuffers(window);
    }

    std::cout << "Exiting program with code 0.\n";
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader);
    glfwTerminate();
    return 0;
}
