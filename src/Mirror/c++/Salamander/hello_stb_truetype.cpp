#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <fstream>
#include <iostream>

// Font atlas settings
#define FONT_BITMAP_W 512
#define FONT_BITMAP_H 512
#define FONT_SIZE 32.0f

unsigned char fontBitmap[FONT_BITMAP_W * FONT_BITMAP_H];
stbtt_bakedchar cdata[96]; // ASCII 32..126

std::vector<unsigned char> loadFile(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(file)), {});
}

void bakeFontAtlas(const char* fontPath) {
    auto fontBuffer = loadFile(fontPath);
    if (fontBuffer.empty()) {
        std::cerr << "Failed to load font file!\n";
        return;
    }
    stbtt_BakeFontBitmap(fontBuffer.data(), 0, FONT_SIZE, fontBitmap, FONT_BITMAP_W, FONT_BITMAP_H, 32, 96, cdata);
}

GLuint createFontTexture() {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FONT_BITMAP_W, FONT_BITMAP_H, 0, GL_RED, GL_UNSIGNED_BYTE, fontBitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return tex;
}

// FLIP THE Y-COORDINATES HERE!
void renderText(float x, float y, const char* text, GLuint fontTex) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    glColor3f(1.0f, 0.0f, 0.0f); // Red text

    glBegin(GL_QUADS);
    while (*text) {
        if (*text >= 32 && *text < 128) {
            stbtt_bakedchar* b = cdata + (*text - 32);

            float x0 = x + b->xoff;
            float y0 = y + b->yoff; // FLIP: use + instead of -
            float x1 = x0 + (b->x1 - b->x0);
            float y1 = y0 + (b->y1 - b->y0); // FLIP: use + instead of -

            float s0 = b->x0 / (float)FONT_BITMAP_W;
            float s1 = b->x1 / (float)FONT_BITMAP_W;
            float t0 = b->y0 / (float)FONT_BITMAP_H;
            float t1 = b->y1 / (float)FONT_BITMAP_H;

            glTexCoord2f(s0, t0); glVertex2f(x0, y0);
            glTexCoord2f(s1, t0); glVertex2f(x1, y0);
            glTexCoord2f(s1, t1); glVertex2f(x1, y1);
            glTexCoord2f(s0, t1); glVertex2f(x0, y1);

            x += b->xadvance;
        }
        ++text;
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(800, 600, "stb_truetype Demo", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    bakeFontAtlas("Roboto-Regular.ttf");
    GLuint fontTex = createFontTexture();

    // Standard OpenGL 2D projection (y increases downward)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 800, 600, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        renderText(50, 100, "Hello, stb_truetype!", fontTex);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
