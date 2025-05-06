#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <random>
#include <array>

// --- FPS Meter Variables ---
double fps_last_time = 0.0;
int fps_frames = 0;

// --- END FPS Meter Variables ---

constexpr int CHUNK_SIZE = 16;
constexpr int RENDER_DIST = 5; // chunks in each direction

enum BlockType { AIR = 0, SOLID = 1 };

struct ChunkKey {
    int x, y, z;
    bool operator==(const ChunkKey& o) const { return x == o.x && y == o.y && z == o.z; }
};
namespace std {
    template<> struct hash<ChunkKey> {
        size_t operator()(const ChunkKey& k) const {
            return ((hash<int>()(k.x) ^ (hash<int>()(k.y) << 1)) >> 1) ^ (hash<int>()(k.z) << 1);
        }
    };
}

struct VoxelGrid {
    int xmin, xmax, ymin, ymax, zmin, zmax;
    int W, H, D;
    std::vector<BlockType> data;

    VoxelGrid()
        : xmin(0), xmax(-1), ymin(0), ymax(-1), zmin(0), zmax(-1), W(0), H(0), D(0) {
    }

    VoxelGrid(int xmin_, int xmax_, int ymin_, int ymax_, int zmin_, int zmax_)
        : xmin(xmin_), xmax(xmax_), ymin(ymin_), ymax(ymax_), zmin(zmin_), zmax(zmax_) {
        W = xmax - xmin + 1;
        H = ymax - ymin + 1;
        D = zmax - zmin + 1;
        data.resize(W * H * D, AIR);
    }
    int idx(int x, int y, int z) const {
        return (x - xmin) + W * ((y - ymin) + H * (z - zmin));
    }
    BlockType get(int x, int y, int z) const {
        if (x < xmin || x > xmax || y < ymin || y > ymax || z < zmin || z > zmax) return AIR;
        return data[idx(x, y, z)];
    }
    void set(int x, int y, int z, BlockType t) {
        if (x < xmin || x > xmax || y < ymin || y > ymax || z < zmin || z > zmax) return;
        data[idx(x, y, z)] = t;
    }
};

// --- BEGIN PERLIN NOISE IMPLEMENTATION (Ken Perlin style, public domain) ---
static int p[512];
void init_perlin(unsigned int seed = 1337) {
    std::vector<int> perm(256);
    for (int i = 0; i < 256; ++i) perm[i] = i;
    std::mt19937 rng(seed);
    std::shuffle(perm.begin(), perm.end(), rng);
    for (int i = 0; i < 512; ++i) p[i] = perm[i & 255];
}
float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
float my_lerp(float a, float b, float t) { return a + t * (b - a); }
float grad(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y, v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}
float perlin(float x, float y, float z) {
    int X = (int)floor(x) & 255, Y = (int)floor(y) & 255, Z = (int)floor(z) & 255;
    x -= floor(x); y -= floor(y); z -= floor(z);
    float u = fade(x), v = fade(y), w = fade(z);
    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
    int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;
    return my_lerp(
        my_lerp(
            my_lerp(grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z), u),
            my_lerp(grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z), u),
            v
        ),
        my_lerp(
            my_lerp(grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1), u),
            my_lerp(grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1), u),
            v
        ),
        w
    );
}
// --- END PERLIN NOISE IMPLEMENTATION ---

void generate_terrain_grid(VoxelGrid& grid) {
    float freq1 = 0.07f, freq2 = 0.15f;
    float erosion_strength = 0.6f;
    int surfaceY = 32;
    for (int x = grid.xmin; x <= grid.xmax; ++x) {
        for (int z = grid.zmin; z <= grid.zmax; ++z) {
            for (int y = grid.ymax; y >= grid.ymin; --y) {
                float base = perlin(x * freq1, y * freq1, z * freq1);
                float erosion = perlin(x * freq2 + 100, y * freq2 + 100, z * freq2 + 100);
                float density = base - erosion * erosion_strength - ((float)y - surfaceY) * 0.03f;
                if (density > 0.0f) {
                    grid.set(x, y, z, SOLID);
                }
            }
        }
    }
}

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    float thickness;
};

struct Chunk {
    ChunkKey key;
    VoxelGrid voxels;
    std::vector<Vertex> verts;
    GLuint vbo = 0;
    size_t vert_count = 0;
    bool meshed = false;
    bool dirty = true;

    Chunk() : key{ 0,0,0 }, voxels(), verts(), vbo(0), vert_count(0), meshed(false), dirty(true) {}
    Chunk(const ChunkKey& k)
        : key(k),
        voxels(k.x* CHUNK_SIZE, k.x* CHUNK_SIZE + CHUNK_SIZE - 1,
            k.y* CHUNK_SIZE, k.y* CHUNK_SIZE + CHUNK_SIZE - 1,
            k.z* CHUNK_SIZE, k.z* CHUNK_SIZE + CHUNK_SIZE - 1),
        vbo(0), vert_count(0), meshed(false), dirty(true) {
    }
};

// --- SURFACE NETS MESHING IMPLEMENTATION ---
void surface_nets_mesh(const VoxelGrid& grid, std::vector<Vertex>& verts) {
    static const std::array<std::array<int, 3>, 8> corners = { {
        {0,0,0},{1,0,0},{1,1,0},{0,1,0},
        {0,0,1},{1,0,1},{1,1,1},{0,1,1}
    } };
    static const std::array<std::array<int, 4>, 6> faces = { {
        {0,1,2,3}, // -Z
        {4,5,6,7}, // +Z
        {0,1,5,4}, // -Y
        {2,3,7,6}, // +Y
        {1,2,6,5}, // +X
        {0,3,7,4}  // -X
    } };
    int W = grid.W, H = grid.H, D = grid.D;
    int xmin = grid.xmin, ymin = grid.ymin, zmin = grid.zmin;
    float thickness = 0.06f;

    for (int x = xmin; x < xmin + W - 1; ++x)
        for (int y = ymin; y < ymin + H - 1; ++y)
            for (int z = zmin; z < zmin + D - 1; ++z) {
                int cube = 0;
                for (int i = 0; i < 8; ++i) {
                    int vx = x + corners[i][0];
                    int vy = y + corners[i][1];
                    int vz = z + corners[i][2];
                    if (grid.get(vx, vy, vz) == SOLID) cube |= (1 << i);
                }
                if (cube == 0 || cube == 255) continue; // Fully solid or fully air

                // Find average position of surface crossing
                glm::vec3 avg(0);
                int count = 0;
                for (int i = 0; i < 8; ++i) {
                    int vx = x + corners[i][0];
                    int vy = y + corners[i][1];
                    int vz = z + corners[i][2];
                    if (grid.get(vx, vy, vz) != grid.get(x, y, z)) {
                        avg += glm::vec3(vx, vy, vz);
                        count++;
                    }
                }
                if (count == 0) continue;
                avg /= float(count);

                // Emit a quad for each face with a sign change
                for (int f = 0; f < 6; ++f) {
                    int d = 0;
                    for (int i = 0; i < 4; ++i) {
                        int vx = x + corners[faces[f][i]][0];
                        int vy = y + corners[faces[f][i]][1];
                        int vz = z + corners[faces[f][i]][2];
                        d += (grid.get(vx, vy, vz) == SOLID) ? 1 : -1;
                    }
                    if (d == 0) continue; // No sign change

                    // Build quad vertices
                    glm::vec3 v[4];
                    for (int i = 0; i < 4; ++i) {
                        int vx = x + corners[faces[f][i]][0];
                        int vy = y + corners[faces[f][i]][1];
                        int vz = z + corners[faces[f][i]][2];
                        v[i] = glm::vec3(vx, vy, vz);
                    }
                    glm::vec3 normal = glm::normalize(glm::cross(v[1] - v[0], v[2] - v[0]));
                    verts.push_back({ v[0], normal, thickness });
                    verts.push_back({ v[1], normal, thickness });
                    verts.push_back({ v[2], normal, thickness });
                    verts.push_back({ v[2], normal, thickness });
                    verts.push_back({ v[3], normal, thickness });
                    verts.push_back({ v[0], normal, thickness });
                }
            }
}

const char* vert = R"(#version 330 core
layout(location=0)in vec3 p;
layout(location=1)in vec3 n;
layout(location=2)in float thickness;
uniform mat4 view,proj;
out vec3 world;
out vec3 normal;
out float grid_thickness;
void main(){
    world = p;
    normal = n;
    grid_thickness = thickness;
    gl_Position=proj*view*vec4(p,1);
})";

const char* frag = R"(#version 330 core
in vec3 world;
in vec3 normal;
in float grid_thickness;
out vec4 f;
const float g = 24.0;
void main() {
    float l = grid_thickness;
    float alpha = 0.0;
    if (abs(normal.y) > 0.9) {
        vec2 q = fract(world.xz * g);
        alpha = (q.x < l || q.x > 1. - l || q.y < l || q.y > 1. - l) ? 1.0 : 0.0;
    } else if (abs(normal.x) > 0.9) {
        vec2 q = fract(world.yz * g);
        alpha = (q.x < l || q.x > 1. - l || q.y < l || q.y > 1. - l) ? 1.0 : 0.0;
    } else {
        vec2 q = fract(world.xy * g);
        alpha = (q.x < l || q.x > 1. - l || q.y < l || q.y > 1. - l) ? 1.0 : 0.0;
    }
    f = vec4(0,0,0,alpha);
})";

int SW = 800, SH = 600;
float dt = 0, lastT = 0, yaw = -90, pitch = 0, lastX = 400, lastY = 300;
bool onGround = true, firstMouse = true;
glm::vec3 cam(0, 40.6f, 0), vel(0);

void framebuffer(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }
void mouse(GLFWwindow*, double x, double y) {
    if (firstMouse) { lastX = x; lastY = y; firstMouse = false; }
    float dx = x - lastX, dy = lastY - y; lastX = x; lastY = y;
    yaw += dx * 0.1f; pitch += dy * 0.1f;
    pitch = glm::clamp(pitch, -89.f, 89.f);
}

GLuint makeProg(const char* vS, const char* fS) {
    GLuint v = glCreateShader(GL_VERTEX_SHADER), f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(v, 1, &vS, nullptr); glCompileShader(v);
    glShaderSource(f, 1, &fS, nullptr); glCompileShader(f);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f); return p;
}

void process(GLFWwindow* w) {
    glm::vec3 d(0), f(glm::normalize(glm::vec3(cos(glm::radians(yaw)), 0, sin(glm::radians(yaw)))));
    glm::vec3 r = glm::cross(f, glm::vec3(0, 1, 0));
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS)d += f;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS)d -= f;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS)d -= r;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS)d += r;
    if (glm::length(d) > 0.01f) d = glm::normalize(d);
    float s = 10; if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)s *= 2;
    vel.x = d.x * s; vel.z = d.z * s;
    if (onGround && glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS) { vel.y = 5; onGround = false; }
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)glfwSetWindowShouldClose(w, true);
}

void collision() {
    float feet = cam.y - 1.6f;
    if (feet < 1) { cam.y = 2.6f; vel.y = 0; onGround = true; }
}

void crosshair() {
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glLineWidth(2);
    glBegin(GL_LINES); glColor3f(0, 1, 0);
    glVertex2f(-0.02f, 0); glVertex2f(0.02f, 0);
    glVertex2f(0, -0.02f); glVertex2f(0, 0.02f);
    glEnd();
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glEnable(GL_DEPTH_TEST);
}

std::unordered_map<ChunkKey, Chunk> chunks;

void update_chunk_mesh_if_dirty(Chunk& chunk) {
    if (chunk.dirty) {
        chunk.verts.clear();
        surface_nets_mesh(chunk.voxels, chunk.verts); // <-- Use Surface Nets instead of greedy_mesh
        if (chunk.vbo == 0) glGenBuffers(1, &chunk.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * chunk.verts.size(), chunk.verts.data(), GL_STATIC_DRAW);
        chunk.vert_count = chunk.verts.size();
        chunk.meshed = true;
        chunk.dirty = false;
    }
}

void set_block_in_chunk(const ChunkKey& key, int x, int y, int z, BlockType t) {
    auto it = chunks.find(key);
    if (it == chunks.end()) return;
    it->second.voxels.set(x, y, z, t);
    it->second.dirty = true;
}

void load_chunk(const ChunkKey& key) {
    if (chunks.count(key)) return;
    Chunk chunk(key);
    generate_terrain_grid(chunk.voxels);
    chunk.dirty = true;
    chunks[key] = std::move(chunk);
}

void unload_far_chunks(const glm::ivec3& player_chunk) {
    std::vector<ChunkKey> to_remove;
    for (const auto& [key, chunk] : chunks) {
        if (std::abs(key.x - player_chunk.x) > RENDER_DIST ||
            std::abs(key.y - player_chunk.y) > 1 ||
            std::abs(key.z - player_chunk.z) > RENDER_DIST) {
            glDeleteBuffers(1, &chunk.vbo);
            to_remove.push_back(key);
        }
    }
    for (const auto& key : to_remove) chunks.erase(key);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    GLFWmonitor* mon = glfwGetPrimaryMonitor();
    auto mode = glfwGetVideoMode(mon);
    SW = mode->width; SH = mode->height; lastX = SW / 2; lastY = SH / 2;
    GLFWwindow* win = glfwCreateWindow(SW, SH, "Chunked Voxel Terrain - Surface Nets Mesh", &*mon, nullptr);
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer);
    glfwSetCursorPosCallback(win, mouse);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint prog = makeProg(vert, frag);

    GLuint vao;
    glGenVertexArrays(1, &vao);

    init_perlin(1337);

    int chunk_y = 0;

    // --- FPS Meter Initialization ---
    fps_last_time = glfwGetTime();
    fps_frames = 0;
    // --- END FPS Meter Initialization ---

    while (!glfwWindowShouldClose(win)) {
        float t = glfwGetTime();
        dt = t - lastT; lastT = t;
        process(win);
        if (!onGround) vel.y -= 9.81f * dt;
        cam += vel * dt;
        collision();

        glClearColor(0.53f, 0.81f, 0.92f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 dir = glm::normalize(glm::vec3(
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch))));
        glm::mat4 V = glm::lookAt(cam, cam + dir, glm::vec3(0, 1, 0));
        glm::mat4 P = glm::perspective(glm::radians(103.f), (float)SW / SH, 0.1f, 200.f);

        glm::ivec3 player_chunk(
            int(floor(cam.x / float(CHUNK_SIZE))),
            chunk_y,
            int(floor(cam.z / float(CHUNK_SIZE)))
        );

        for (int dx = -RENDER_DIST; dx <= RENDER_DIST; ++dx)
            for (int dz = -RENDER_DIST; dz <= RENDER_DIST; ++dz) {
                ChunkKey key{ player_chunk.x + dx, chunk_y, player_chunk.z + dz };
                load_chunk(key);
            }
        unload_far_chunks(player_chunk);

        glBindVertexArray(vao);
        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "view"), 1, 0, glm::value_ptr(V));
        glUniformMatrix4fv(glGetUniformLocation(prog, "proj"), 1, 0, glm::value_ptr(P));

        for (auto& [key, chunk] : chunks) {
            update_chunk_mesh_if_dirty(chunk);
            if (!chunk.meshed || chunk.vert_count == 0) continue;
            glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0); glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3)); glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 6)); glEnableVertexAttribArray(2);
            glDrawArrays(GL_TRIANGLES, 0, chunk.vert_count);
        }

        crosshair();
        glfwSwapBuffers(win);
        glfwPollEvents();

        // --- FPS Meter Update ---
        fps_frames++;
        double current_time = glfwGetTime();
        if (current_time - fps_last_time >= 1.0) {
            std::cout << "FPS: " << fps_frames << std::endl;
            fps_frames = 0;
            fps_last_time = current_time;
        }
        // --- END FPS Meter Update ---
    }
    glfwTerminate();
    return 0;
}
