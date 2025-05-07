#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>

// Window dimensions
constexpr int WINDOW_WIDTH = 1000;
constexpr int WINDOW_HEIGHT = 600;

// Text positioning and sizing
constexpr float LETTER_WIDTH = 20.0f;
constexpr float LETTER_HEIGHT = 30.0f;
constexpr float LETTER_SPACING = 10.0f;
constexpr float LINE_SPACING = 50.0f;

// Character tracking
int currentChar = 0;
const int CHARS_PER_LINE = 13;
float textStartX = 50.0f;
float textStartY = 50.0f;

// Function to draw a single line-based character
void drawCharacter(char c, float x, float y) {
    float w = LETTER_WIDTH;
    float h = LETTER_HEIGHT;

    glColor3f(1.0f, 1.0f, 1.0f);  // White color
    glLineWidth(2.0f);            // Slightly thicker lines
    glBegin(GL_LINES);

    switch (c) {
    case 'A':
        // Corrected A (pointed top)
        glVertex2f(x, y); glVertex2f(x + w / 2, y - h);         // Left diagonal
        glVertex2f(x + w / 2, y - h); glVertex2f(x + w, y);       // Right diagonal
        glVertex2f(x + w * 0.25f, y - h / 2); glVertex2f(x + w * 0.75f, y - h / 2);  // Middle horizontal
        break;

    case 'B':
        // Corrected B
        glVertex2f(x, y); glVertex2f(x, y - h);             // Vertical stem
        glVertex2f(x, y); glVertex2f(x + w * 0.8f, y);        // Top horizontal
        glVertex2f(x + w * 0.8f, y); glVertex2f(x + w, y - h * 0.2f);    // Top right curve
        glVertex2f(x + w, y - h * 0.2f); glVertex2f(x + w * 0.8f, y - h * 0.4f);  // Top to middle curve
        glVertex2f(x, y - h * 0.5f); glVertex2f(x + w * 0.8f, y - h * 0.5f);    // Middle horizontal
        glVertex2f(x + w * 0.8f, y - h * 0.5f); glVertex2f(x + w, y - h * 0.7f);  // Middle to bottom curve
        glVertex2f(x + w, y - h * 0.7f); glVertex2f(x + w * 0.8f, y - h);       // Bottom right curve
        glVertex2f(x, y - h); glVertex2f(x + w * 0.8f, y - h);              // Bottom horizontal
        break;

    case 'C':
        // Corrected C
        glVertex2f(x + w, y - h * 0.2f); glVertex2f(x + w * 0.7f, y);      // Top right curve
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w * 0.3f, y);        // Top horizontal
        glVertex2f(x + w * 0.3f, y); glVertex2f(x, y - h * 0.2f);        // Top left curve
        glVertex2f(x, y - h * 0.2f); glVertex2f(x, y - h * 0.8f);        // Left vertical
        glVertex2f(x, y - h * 0.8f); glVertex2f(x + w * 0.3f, y - h);      // Bottom left curve
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x + w * 0.7f, y - h);    // Bottom horizontal
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w, y - h * 0.8f);    // Bottom right curve
        break;

    case 'D':
        // Corrected D
        glVertex2f(x, y); glVertex2f(x, y - h);                    // Vertical stem
        glVertex2f(x, y); glVertex2f(x + w * 0.7f, y);               // Top horizontal
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w, y - h * 0.3f);      // Top right curve
        glVertex2f(x + w, y - h * 0.3f); glVertex2f(x + w, y - h * 0.7f);    // Right vertical
        glVertex2f(x + w, y - h * 0.7f); glVertex2f(x + w * 0.7f, y - h);    // Bottom right curve
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x, y - h);           // Bottom horizontal
        break;

    case 'E':
        // Corrected E
        glVertex2f(x, y); glVertex2f(x, y - h);                    // Vertical stem
        glVertex2f(x, y); glVertex2f(x + w, y);                    // Top horizontal
        glVertex2f(x, y - h / 2); glVertex2f(x + w * 0.8f, y - h / 2);       // Middle horizontal
        glVertex2f(x, y - h); glVertex2f(x + w, y - h);                // Bottom horizontal
        break;

    case 'F':
        // Corrected F
        glVertex2f(x, y); glVertex2f(x, y - h);                    // Vertical stem
        glVertex2f(x, y); glVertex2f(x + w, y);                    // Top horizontal
        glVertex2f(x, y - h / 2); glVertex2f(x + w * 0.8f, y - h / 2);       // Middle horizontal
        break;

    case 'G':
        // Corrected G
        glVertex2f(x + w, y - h * 0.2f); glVertex2f(x + w * 0.7f, y);      // Top right curve
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w * 0.3f, y);        // Top horizontal
        glVertex2f(x + w * 0.3f, y); glVertex2f(x, y - h * 0.2f);        // Top left curve
        glVertex2f(x, y - h * 0.2f); glVertex2f(x, y - h * 0.8f);        // Left vertical
        glVertex2f(x, y - h * 0.8f); glVertex2f(x + w * 0.3f, y - h);      // Bottom left curve
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x + w * 0.7f, y - h);    // Bottom horizontal
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w, y - h * 0.8f);    // Bottom right curve
        glVertex2f(x + w, y - h * 0.8f); glVertex2f(x + w, y - h * 0.5f);    // Right vertical
        glVertex2f(x + w, y - h * 0.5f); glVertex2f(x + w * 0.6f, y - h * 0.5f); // G's middle horizontal
        break;

    case 'H':
        // Corrected H
        glVertex2f(x, y); glVertex2f(x, y - h);                    // Left vertical
        glVertex2f(x + w, y); glVertex2f(x + w, y - h);                // Right vertical
        glVertex2f(x, y - h / 2); glVertex2f(x + w, y - h / 2);            // Middle horizontal
        break;

    case 'I':
        // Corrected I
        glVertex2f(x + w / 2, y); glVertex2f(x + w / 2, y - h);            // Vertical stem
        glVertex2f(x, y); glVertex2f(x + w, y);                    // Top horizontal
        glVertex2f(x, y - h); glVertex2f(x + w, y - h);                // Bottom horizontal
        break;

    case 'J':
        // Corrected J
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w * 0.7f, y - h * 0.7f);  // Vertical stem
        glVertex2f(x + w * 0.7f, y - h * 0.7f); glVertex2f(x + w * 0.5f, y - h); // Bottom right curve
        glVertex2f(x + w * 0.5f, y - h); glVertex2f(x + w * 0.3f, y - h);      // Bottom horizontal
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x + w * 0.1f, y - h * 0.7f); // Bottom left curve
        glVertex2f(x + w * 0.3f, y); glVertex2f(x + w, y);               // Top horizontal
        break;

    case 'K':
        // Corrected K
        glVertex2f(x, y); glVertex2f(x, y - h);                    // Vertical stem
        glVertex2f(x, y - h / 2); glVertex2f(x + w, y);                // Top diagonal
        glVertex2f(x, y - h / 2); glVertex2f(x + w, y - h);              // Bottom diagonal
        break;

    case 'L':
        // Corrected L
        glVertex2f(x, y); glVertex2f(x, y - h);                    // Vertical stem
        glVertex2f(x, y - h); glVertex2f(x + w, y - h);                // Bottom horizontal
        break;

    case 'M':
        // Corrected M
        glVertex2f(x, y); glVertex2f(x, y - h);                    // Left vertical
        glVertex2f(x, y); glVertex2f(x + w / 2, y - h / 2);              // First diagonal
        glVertex2f(x + w / 2, y - h / 2); glVertex2f(x + w, y);            // Second diagonal
        glVertex2f(x + w, y); glVertex2f(x + w, y - h);                // Right vertical
        break;

    case 'N':
        // Corrected N
        glVertex2f(x, y); glVertex2f(x, y - h);                    // Left vertical
        glVertex2f(x, y); glVertex2f(x + w, y - h);                  // Diagonal
        glVertex2f(x + w, y - h); glVertex2f(x + w, y);                // Right vertical
        break;

    case 'O':
        // Corrected O
        glVertex2f(x + w * 0.3f, y); glVertex2f(x + w * 0.7f, y);        // Top horizontal
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w, y - h * 0.3f);      // Top right curve
        glVertex2f(x + w, y - h * 0.3f); glVertex2f(x + w, y - h * 0.7f);    // Right vertical
        glVertex2f(x + w, y - h * 0.7f); glVertex2f(x + w * 0.7f, y - h);    // Bottom right curve
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w * 0.3f, y - h);    // Bottom horizontal
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x, y - h * 0.7f);      // Bottom left curve
        glVertex2f(x, y - h * 0.7f); glVertex2f(x, y - h * 0.3f);        // Left vertical
        glVertex2f(x, y - h * 0.3f); glVertex2f(x + w * 0.3f, y);        // Top left curve
        break;

    case 'P':
        // Corrected P
        glVertex2f(x, y); glVertex2f(x, y - h);                    // Vertical stem
        glVertex2f(x, y); glVertex2f(x + w * 0.7f, y);               // Top horizontal
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w, y - h * 0.3f);      // Top right curve
        glVertex2f(x + w, y - h * 0.3f); glVertex2f(x + w, y - h * 0.5f);    // Right vertical
        glVertex2f(x + w, y - h * 0.5f); glVertex2f(x + w * 0.7f, y - h * 0.6f); // Bottom right curve
        glVertex2f(x + w * 0.7f, y - h * 0.6f); glVertex2f(x, y - h * 0.6f); // Middle horizontal
        break;

    case 'Q':
        // Corrected Q
        glVertex2f(x + w * 0.3f, y); glVertex2f(x + w * 0.7f, y);        // Top horizontal
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w, y - h * 0.3f);      // Top right curve
        glVertex2f(x + w, y - h * 0.3f); glVertex2f(x + w, y - h * 0.7f);    // Right vertical
        glVertex2f(x + w, y - h * 0.7f); glVertex2f(x + w * 0.7f, y - h);    // Bottom right curve
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w * 0.3f, y - h);    // Bottom horizontal
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x, y - h * 0.7f);      // Bottom left curve
        glVertex2f(x, y - h * 0.7f); glVertex2f(x, y - h * 0.3f);        // Left vertical
        glVertex2f(x, y - h * 0.3f); glVertex2f(x + w * 0.3f, y);        // Top left curve
        glVertex2f(x + w * 0.5f, y - h * 0.7f); glVertex2f(x + w + w * 0.2f, y - h - h * 0.2f); // Q's tail
        break;

    case 'R':
        // Corrected R
        glVertex2f(x, y); glVertex2f(x, y - h);                    // Vertical stem
        glVertex2f(x, y); glVertex2f(x + w * 0.7f, y);               // Top horizontal
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w, y - h * 0.3f);      // Top right curve
        glVertex2f(x + w, y - h * 0.3f); glVertex2f(x + w, y - h * 0.5f);    // Right vertical
        glVertex2f(x + w, y - h * 0.5f); glVertex2f(x + w * 0.7f, y - h * 0.6f); // Bottom right curve
        glVertex2f(x + w * 0.7f, y - h * 0.6f); glVertex2f(x, y - h * 0.6f); // Middle horizontal
        glVertex2f(x + w * 0.5f, y - h * 0.6f); glVertex2f(x + w, y - h);    // R's leg
        break;

    case 'S':
        // Corrected S
        glVertex2f(x + w, y - h * 0.2f); glVertex2f(x + w * 0.7f, y);      // Top right curve
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w * 0.3f, y);        // Top horizontal
        glVertex2f(x + w * 0.3f, y); glVertex2f(x, y - h * 0.2f);        // Top left curve
        glVertex2f(x, y - h * 0.2f); glVertex2f(x, y - h * 0.4f);        // Left top vertical
        glVertex2f(x, y - h * 0.4f); glVertex2f(x + w * 0.3f, y - h * 0.5f); // Middle left curve
        glVertex2f(x + w * 0.3f, y - h * 0.5f); glVertex2f(x + w * 0.7f, y - h * 0.5f); // Middle horizontal
        glVertex2f(x + w * 0.7f, y - h * 0.5f); glVertex2f(x + w, y - h * 0.6f); // Middle right curve
        glVertex2f(x + w, y - h * 0.6f); glVertex2f(x + w, y - h * 0.8f);    // Right bottom vertical
        glVertex2f(x + w, y - h * 0.8f); glVertex2f(x + w * 0.7f, y - h);    // Bottom right curve
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w * 0.3f, y - h);    // Bottom horizontal
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x, y - h * 0.8f);      // Bottom left curve
        break;

    case 'T':
        // Corrected T
        glVertex2f(x + w / 2, y); glVertex2f(x + w / 2, y - h);            // Vertical stem
        glVertex2f(x, y); glVertex2f(x + w, y);                    // Top horizontal
        break;

    case 'U':
        // Corrected U
        glVertex2f(x, y); glVertex2f(x, y - h * 0.7f);               // Left vertical
        glVertex2f(x, y - h * 0.7f); glVertex2f(x + w * 0.3f, y - h);      // Bottom left curve
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x + w * 0.7f, y - h);    // Bottom horizontal
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w, y - h * 0.7f);    // Bottom right curve
        glVertex2f(x + w, y - h * 0.7f); glVertex2f(x + w, y);           // Right vertical
        break;

    case 'V':
        // Corrected V
        glVertex2f(x, y); glVertex2f(x + w / 2, y - h);                // Left diagonal
        glVertex2f(x + w / 2, y - h); glVertex2f(x + w, y);              // Right diagonal
        break;

    case 'W':
        // Corrected W
        glVertex2f(x, y); glVertex2f(x + w * 0.25f, y - h);            // First diagonal
        glVertex2f(x + w * 0.25f, y - h); glVertex2f(x + w * 0.5f, y - h * 0.5f); // Second diagonal
        glVertex2f(x + w * 0.5f, y - h * 0.5f); glVertex2f(x + w * 0.75f, y - h); // Third diagonal
        glVertex2f(x + w * 0.75f, y - h); glVertex2f(x + w, y);          // Fourth diagonal
        break;

    case 'X':
        // Corrected X
        glVertex2f(x, y); glVertex2f(x + w, y - h);                  // Forward diagonal
        glVertex2f(x + w, y); glVertex2f(x, y - h);                  // Backward diagonal
        break;

    case 'Y':
        // Corrected Y
        glVertex2f(x, y); glVertex2f(x + w / 2, y - h / 2);              // Top left diagonal
        glVertex2f(x + w / 2, y - h / 2); glVertex2f(x + w, y);            // Top right diagonal
        glVertex2f(x + w / 2, y - h / 2); glVertex2f(x + w / 2, y - h);        // Vertical stem
        break;

    case 'Z':
        // Corrected Z
        glVertex2f(x, y); glVertex2f(x + w, y);                    // Top horizontal
        glVertex2f(x + w, y); glVertex2f(x, y - h);                  // Diagonal
        glVertex2f(x, y - h); glVertex2f(x + w, y - h);                // Bottom horizontal
        break;

        // Digits
    case '0':
        // Corrected 0
        glVertex2f(x + w * 0.3f, y); glVertex2f(x + w * 0.7f, y);        // Top horizontal
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w, y - h * 0.3f);      // Top right curve
        glVertex2f(x + w, y - h * 0.3f); glVertex2f(x + w, y - h * 0.7f);    // Right vertical
        glVertex2f(x + w, y - h * 0.7f); glVertex2f(x + w * 0.7f, y - h);    // Bottom right curve
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w * 0.3f, y - h);    // Bottom horizontal
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x, y - h * 0.7f);      // Bottom left curve
        glVertex2f(x, y - h * 0.7f); glVertex2f(x, y - h * 0.3f);        // Left vertical
        glVertex2f(x, y - h * 0.3f); glVertex2f(x + w * 0.3f, y);        // Top left curve
        glVertex2f(x + w * 0.3f, y - h * 0.3f); glVertex2f(x + w * 0.7f, y - h * 0.7f); // Diagonal line for 0
        break;

    case '1':
        // Corrected 1
        glVertex2f(x + w * 0.5f, y); glVertex2f(x + w * 0.5f, y - h);      // Vertical stem
        glVertex2f(x + w * 0.3f, y - h * 0.2f); glVertex2f(x + w * 0.5f, y); // Top diagonal
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x + w * 0.7f, y - h);    // Base
        break;

    case '2':
        // Corrected 2
        glVertex2f(x, y - h); glVertex2f(x + w, y - h);                // Bottom horizontal
        glVertex2f(x + w, y - h); glVertex2f(x, y - h * 0.4f);           // Diagonal
        glVertex2f(x, y - h * 0.4f); glVertex2f(x + w * 0.3f, y);        // Top left curve
        glVertex2f(x + w * 0.3f, y); glVertex2f(x + w * 0.7f, y);        // Top horizontal
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w, y - h * 0.2f);      // Top right curve
        break;

    case '3':
        // Corrected 3
        glVertex2f(x, y - h * 0.2f); glVertex2f(x + w * 0.3f, y);        // Top left curve
        glVertex2f(x + w * 0.3f, y); glVertex2f(x + w * 0.7f, y);        // Top horizontal
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w, y - h * 0.2f);      // Top right curve
        glVertex2f(x + w, y - h * 0.2f); glVertex2f(x + w, y - h * 0.4f);    // Right top vertical
        glVertex2f(x + w, y - h * 0.4f); glVertex2f(x + w * 0.7f, y - h * 0.5f); // Middle right curve
        glVertex2f(x + w * 0.7f, y - h * 0.5f); glVertex2f(x + w * 0.5f, y - h * 0.5f); // Middle horizontal
        glVertex2f(x + w * 0.7f, y - h * 0.5f); glVertex2f(x + w, y - h * 0.6f); // Middle right curve
        glVertex2f(x + w, y - h * 0.6f); glVertex2f(x + w, y - h * 0.8f);    // Right bottom vertical
        glVertex2f(x + w, y - h * 0.8f); glVertex2f(x + w * 0.7f, y - h);    // Bottom right curve
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w * 0.3f, y - h);    // Bottom horizontal
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x, y - h * 0.8f);      // Bottom left curve
        break;

    case '4':
        // Corrected 4
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w * 0.7f, y - h);      // Right vertical
        glVertex2f(x + w, y - h * 0.6f); glVertex2f(x, y - h * 0.6f);      // Middle horizontal
        glVertex2f(x + w * 0.3f, y); glVertex2f(x + w * 0.3f, y - h * 0.6f); // Left segment
        break;

    case '5':
        // Corrected 5
        glVertex2f(x + w, y); glVertex2f(x, y);                    // Top horizontal
        glVertex2f(x, y); glVertex2f(x, y - h * 0.5f);               // Left vertical
        glVertex2f(x, y - h * 0.5f); glVertex2f(x + w * 0.7f, y - h * 0.5f); // Middle horizontal
        glVertex2f(x + w * 0.7f, y - h * 0.5f); glVertex2f(x + w, y - h * 0.7f); // Middle right curve
        glVertex2f(x + w, y - h * 0.7f); glVertex2f(x + w, y - h * 0.9f);    // Right bottom vertical
        glVertex2f(x + w, y - h * 0.9f); glVertex2f(x + w * 0.7f, y - h);    // Bottom right curve
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w * 0.3f, y - h);    // Bottom horizontal
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x, y - h * 0.8f);      // Bottom left curve
        break;

    case '6':
        // Corrected 6
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w * 0.3f, y);        // Top horizontal
        glVertex2f(x + w * 0.3f, y); glVertex2f(x, y - h * 0.3f);        // Top left curve
        glVertex2f(x, y - h * 0.3f); glVertex2f(x, y - h * 0.7f);        // Left vertical
        glVertex2f(x, y - h * 0.7f); glVertex2f(x + w * 0.3f, y - h);      // Bottom left curve
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x + w * 0.7f, y - h);    // Bottom horizontal
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w, y - h * 0.7f);    // Bottom right curve
        glVertex2f(x + w, y - h * 0.7f); glVertex2f(x + w, y - h * 0.5f);    // Right vertical
        glVertex2f(x + w, y - h * 0.5f); glVertex2f(x + w * 0.7f, y - h * 0.4f); // Middle right curve
        glVertex2f(x + w * 0.7f, y - h * 0.4f); glVertex2f(x + w * 0.3f, y - h * 0.4f); // Middle horizontal
        glVertex2f(x + w * 0.3f, y - h * 0.4f); glVertex2f(x, y - h * 0.5f); // Middle left curve
        break;

    case '7':
        // Corrected 7
        glVertex2f(x, y); glVertex2f(x + w, y);                    // Top horizontal
        glVertex2f(x + w, y); glVertex2f(x + w * 0.3f, y - h);           // Diagonal
        break;

    case '8':
        // Top circle
        glVertex2f(x + w * 0.3f, y); glVertex2f(x + w * 0.7f, y);            // Top horizontal
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w, y - h * 0.2f);          // Top right curve
        glVertex2f(x + w, y - h * 0.2f); glVertex2f(x + w, y - h * 0.4f);        // Right top vertical
        glVertex2f(x + w, y - h * 0.4f); glVertex2f(x + w * 0.7f, y - h * 0.5f);   // Middle right curve
        glVertex2f(x + w * 0.7f, y - h * 0.5f); glVertex2f(x + w * 0.3f, y - h * 0.5f); // Middle horizontal
        glVertex2f(x + w * 0.3f, y - h * 0.5f); glVertex2f(x, y - h * 0.4f);     // Middle left curve
        glVertex2f(x, y - h * 0.4f); glVertex2f(x, y - h * 0.2f);            // Left top vertical
        glVertex2f(x, y - h * 0.2f); glVertex2f(x + w * 0.3f, y);            // Top left curve

        // Bottom circle
        glVertex2f(x + w * 0.3f, y - h * 0.5f); glVertex2f(x, y - h * 0.6f);     // Middle left curve
        glVertex2f(x, y - h * 0.6f); glVertex2f(x, y - h * 0.8f);            // Left bottom vertical
        glVertex2f(x, y - h * 0.8f); glVertex2f(x + w * 0.3f, y - h);          // Bottom left curve
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x + w * 0.7f, y - h);        // Bottom horizontal
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w, y - h * 0.8f);        // Bottom right curve
        glVertex2f(x + w, y - h * 0.8f); glVertex2f(x + w, y - h * 0.6f);        // Right bottom vertical
        glVertex2f(x + w, y - h * 0.6f); glVertex2f(x + w * 0.7f, y - h * 0.5f);   // Middle right curve
        break;

    case '9':
        // Corrected 9
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x + w * 0.7f, y - h);    // Bottom horizontal
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w, y - h * 0.7f);    // Bottom right curve
        glVertex2f(x + w, y - h * 0.7f); glVertex2f(x + w, y - h * 0.3f);    // Right vertical
        glVertex2f(x + w, y - h * 0.3f); glVertex2f(x + w * 0.7f, y);      // Top right curve
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w * 0.3f, y);        // Top horizontal
        glVertex2f(x + w * 0.3f, y); glVertex2f(x, y - h * 0.3f);        // Top left curve
        glVertex2f(x, y - h * 0.3f); glVertex2f(x, y - h * 0.5f);        // Left vertical
        glVertex2f(x, y - h * 0.5f); glVertex2f(x + w * 0.3f, y - h * 0.6f); // Middle left curve
        glVertex2f(x + w * 0.3f, y - h * 0.6f); glVertex2f(x + w * 0.7f, y - h * 0.6f); // Middle horizontal
        glVertex2f(x + w * 0.7f, y - h * 0.6f); glVertex2f(x + w, y - h * 0.5f); // Middle right curve
        break;

        // Basic punctuation
    case '.':
        // Corrected period
        glVertex2f(x + w * 0.4f, y - h); glVertex2f(x + w * 0.6f, y - h);          // Bottom
        glVertex2f(x + w * 0.6f, y - h); glVertex2f(x + w * 0.6f, y - h * 0.8f);     // Right
        glVertex2f(x + w * 0.6f, y - h * 0.8f); glVertex2f(x + w * 0.4f, y - h * 0.8f); // Top
        glVertex2f(x + w * 0.4f, y - h * 0.8f); glVertex2f(x + w * 0.4f, y - h);     // Left
        break;

    case ',':
        // Corrected comma
        glVertex2f(x + w * 0.4f, y - h - h * 0.2f); glVertex2f(x + w * 0.6f, y - h);    // Diagonal
        glVertex2f(x + w * 0.6f, y - h); glVertex2f(x + w * 0.6f, y - h * 0.8f);      // Top vertical
        glVertex2f(x + w * 0.6f, y - h * 0.8f); glVertex2f(x + w * 0.4f, y - h * 0.8f); // Top
        glVertex2f(x + w * 0.4f, y - h * 0.8f); glVertex2f(x + w * 0.4f, y - h);      // Left side
        break;

    case ':':
        // Corrected colon - Top dot
        glVertex2f(x + w * 0.4f, y - h * 0.4f); glVertex2f(x + w * 0.6f, y - h * 0.4f);
        glVertex2f(x + w * 0.6f, y - h * 0.4f); glVertex2f(x + w * 0.6f, y - h * 0.2f);
        glVertex2f(x + w * 0.6f, y - h * 0.2f); glVertex2f(x + w * 0.4f, y - h * 0.2f);
        glVertex2f(x + w * 0.4f, y - h * 0.2f); glVertex2f(x + w * 0.4f, y - h * 0.4f);

        // Bottom dot
        glVertex2f(x + w * 0.4f, y - h * 0.8f); glVertex2f(x + w * 0.6f, y - h * 0.8f);
        glVertex2f(x + w * 0.6f, y - h * 0.8f); glVertex2f(x + w * 0.6f, y - h * 0.6f);
        glVertex2f(x + w * 0.6f, y - h * 0.6f); glVertex2f(x + w * 0.4f, y - h * 0.6f);
        glVertex2f(x + w * 0.4f, y - h * 0.6f); glVertex2f(x + w * 0.4f, y - h * 0.8f);
        break;

    case ';':
        // Corrected semicolon - Top dot
        glVertex2f(x + w * 0.4f, y - h * 0.4f); glVertex2f(x + w * 0.6f, y - h * 0.4f);
        glVertex2f(x + w * 0.6f, y - h * 0.4f); glVertex2f(x + w * 0.6f, y - h * 0.2f);
        glVertex2f(x + w * 0.6f, y - h * 0.2f); glVertex2f(x + w * 0.4f, y - h * 0.2f);
        glVertex2f(x + w * 0.4f, y - h * 0.2f); glVertex2f(x + w * 0.4f, y - h * 0.4f);

        // Bottom comma
        glVertex2f(x + w * 0.4f, y - h); glVertex2f(x + w * 0.6f, y - h * 0.8f);
        glVertex2f(x + w * 0.6f, y - h * 0.8f); glVertex2f(x + w * 0.6f, y - h * 0.6f);
        glVertex2f(x + w * 0.6f, y - h * 0.6f); glVertex2f(x + w * 0.4f, y - h * 0.6f);
        glVertex2f(x + w * 0.4f, y - h * 0.6f); glVertex2f(x + w * 0.4f, y - h * 0.8f);
        break;

    case '!':
        // Corrected exclamation mark
        glVertex2f(x + w * 0.5f, y - h * 0.8f); glVertex2f(x + w * 0.5f, y);  // Main line

        // Bottom dot
        glVertex2f(x + w * 0.4f, y - h); glVertex2f(x + w * 0.6f, y - h);
        glVertex2f(x + w * 0.6f, y - h); glVertex2f(x + w * 0.6f, y - h * 0.9f);
        glVertex2f(x + w * 0.6f, y - h * 0.9f); glVertex2f(x + w * 0.4f, y - h * 0.9f);
        glVertex2f(x + w * 0.4f, y - h * 0.9f); glVertex2f(x + w * 0.4f, y - h);
        break;

    case '?':
        // Corrected question mark - Top curve
        glVertex2f(x + w * 0.3f, y - h * 0.3f); glVertex2f(x + w * 0.3f, y);
        glVertex2f(x + w * 0.3f, y); glVertex2f(x + w * 0.7f, y);
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w * 0.7f, y - h * 0.3f);
        glVertex2f(x + w * 0.7f, y - h * 0.3f); glVertex2f(x + w * 0.5f, y - h * 0.5f);

        // Stem
        glVertex2f(x + w * 0.5f, y - h * 0.5f); glVertex2f(x + w * 0.5f, y - h * 0.7f);

        // Bottom dot
        glVertex2f(x + w * 0.4f, y - h); glVertex2f(x + w * 0.6f, y - h);
        glVertex2f(x + w * 0.6f, y - h); glVertex2f(x + w * 0.6f, y - h * 0.9f);
        glVertex2f(x + w * 0.6f, y - h * 0.9f); glVertex2f(x + w * 0.4f, y - h * 0.9f);
        glVertex2f(x + w * 0.4f, y - h * 0.9f); glVertex2f(x + w * 0.4f, y - h);
        break;

    case '-':
        // Corrected hyphen
        glVertex2f(x, y - h * 0.5f); glVertex2f(x + w, y - h * 0.5f);       // Middle horizontal
        break;

    case '+':
        // Corrected plus
        glVertex2f(x, y - h * 0.5f); glVertex2f(x + w, y - h * 0.5f);       // Horizontal
        glVertex2f(x + w * 0.5f, y); glVertex2f(x + w * 0.5f, y - h);       // Vertical
        break;

    case '=':
        // Corrected equals
        glVertex2f(x, y - h * 0.6f); glVertex2f(x + w, y - h * 0.6f);       // Upper line
        glVertex2f(x, y - h * 0.4f); glVertex2f(x + w, y - h * 0.4f);       // Lower line
        break;

    case '/':
        // Corrected forward slash
        glVertex2f(x, y - h); glVertex2f(x + w, y);                   // Diagonal
        break;

    case '\\':
        // Corrected backslash
        glVertex2f(x, y); glVertex2f(x + w, y - h);                   // Diagonal
        break;

    case '(':
        // Corrected left parenthesis
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w * 0.3f, y - h * 0.5f);  // Bottom curve
        glVertex2f(x + w * 0.3f, y - h * 0.5f); glVertex2f(x + w * 0.7f, y);    // Top curve
        break;

    case ')':
        // Corrected right parenthesis
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x + w * 0.7f, y - h * 0.5f);  // Bottom curve
        glVertex2f(x + w * 0.7f, y - h * 0.5f); glVertex2f(x + w * 0.3f, y);    // Top curve
        break;

    case '[':
        // Corrected left bracket
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w * 0.3f, y - h);     // Bottom horizontal
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x + w * 0.3f, y);       // Vertical
        glVertex2f(x + w * 0.3f, y); glVertex2f(x + w * 0.7f, y);         // Top horizontal
        break;

    case ']':
        // Corrected right bracket
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x + w * 0.7f, y - h);     // Bottom horizontal
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w * 0.7f, y);       // Vertical
        glVertex2f(x + w * 0.7f, y); glVertex2f(x + w * 0.3f, y);         // Top horizontal
        break;

    case '{':
        // Corrected left brace
        glVertex2f(x + w * 0.7f, y - h); glVertex2f(x + w * 0.5f, y - h);     // Bottom horizontal
        glVertex2f(x + w * 0.5f, y - h); glVertex2f(x + w * 0.5f, y - h * 0.6f); // Bottom vertical
        glVertex2f(x + w * 0.5f, y - h * 0.6f); glVertex2f(x + w * 0.3f, y - h * 0.5f); // Middle bottom curve
        glVertex2f(x + w * 0.3f, y - h * 0.5f); glVertex2f(x + w * 0.5f, y - h * 0.4f); // Middle top curve
        glVertex2f(x + w * 0.5f, y - h * 0.4f); glVertex2f(x + w * 0.5f, y);   // Top vertical
        glVertex2f(x + w * 0.5f, y); glVertex2f(x + w * 0.7f, y);         // Top horizontal
        break;

    case '}':
        // Corrected right brace
        glVertex2f(x + w * 0.3f, y - h); glVertex2f(x + w * 0.5f, y - h);     // Bottom horizontal
        glVertex2f(x + w * 0.5f, y - h); glVertex2f(x + w * 0.5f, y - h * 0.6f); // Bottom vertical
        glVertex2f(x + w * 0.5f, y - h * 0.6f); glVertex2f(x + w * 0.7f, y - h * 0.5f); // Middle bottom curve
        glVertex2f(x + w * 0.7f, y - h * 0.5f); glVertex2f(x + w * 0.5f, y - h * 0.4f); // Middle top curve
        glVertex2f(x + w * 0.5f, y - h * 0.4f); glVertex2f(x + w * 0.5f, y);   // Top vertical
        glVertex2f(x + w * 0.5f, y); glVertex2f(x + w * 0.3f, y);         // Top horizontal
        break;

    case '"':
        // Corrected double quote
        // Left quote
        glVertex2f(x + w * 0.3f, y - h * 0.3f); glVertex2f(x + w * 0.3f, y);
        // Right quote
        glVertex2f(x + w * 0.7f, y - h * 0.3f); glVertex2f(x + w * 0.7f, y);
        break;

    case '\'':
        // Corrected single quote
        glVertex2f(x + w * 0.5f, y - h * 0.3f); glVertex2f(x + w * 0.5f, y);
        break;

    case '`':
        // Corrected backtick
        glVertex2f(x + w * 0.3f, y); glVertex2f(x + w * 0.5f, y - h * 0.3f);
        break;

    case '~':
        // Corrected tilde
        glVertex2f(x, y - h * 0.5f); glVertex2f(x + w * 0.25f, y - h * 0.3f);
        glVertex2f(x + w * 0.25f, y - h * 0.3f); glVertex2f(x + w * 0.75f, y - h * 0.7f);
        glVertex2f(x + w * 0.75f, y - h * 0.7f); glVertex2f(x + w, y - h * 0.5f);
        break;

        // Add space character (empty)
    case ' ':
        // No lines for space
        break;

        // Default fallback - draw a simple box
    default:
        glVertex2f(x, y); glVertex2f(x + w, y);
        glVertex2f(x + w, y); glVertex2f(x + w, y - h);
        glVertex2f(x + w, y - h); glVertex2f(x, y - h);
        glVertex2f(x, y - h); glVertex2f(x, y);
        glVertex2f(x, y); glVertex2f(x + w, y - h); // X mark
        glVertex2f(x, y - h); glVertex2f(x + w, y);
        break;
    }

    glEnd();
}

// Function to draw a string of characters
void drawString(const std::string& text, float x, float y) {
    float currentX = x;
    for (char c : text) {
        drawCharacter(c, currentX, y);
        currentX += LETTER_WIDTH + LETTER_SPACING;
    }
}

// Draw the full alphabet demo
void drawAlphabetDemo() {
    // Draw uppercase letters
    float yPos = textStartY;
    drawString("ABCDEFGHIJKLM", textStartX, yPos);

    yPos += LINE_SPACING;
    drawString("NOPQRSTUVWXYZ", textStartX, yPos);

    // Draw digits
    yPos += LINE_SPACING;
    drawString("0123456789", textStartX, yPos);

    // Draw some common punctuation
    yPos += LINE_SPACING;
    drawString(".,:;!?-+=", textStartX, yPos);

    // Draw some more symbols
    yPos += LINE_SPACING;
    drawString("/\\()[]{}\"'`~", textStartX, yPos);

    // Draw a sample sentence
    yPos += LINE_SPACING * 2;
    drawString("HELLO WORLD!", textStartX, yPos);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Error: Failed to initialize GLFW\n";
        return -1;
    }

    // Create a windowed mode window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Line Drawing Alphabet", nullptr, nullptr);
    if (!window) {
        std::cerr << "Error: Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Dark background color
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the alphabet demo
        drawAlphabetDemo();

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();

        // Close on ESC key press
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
