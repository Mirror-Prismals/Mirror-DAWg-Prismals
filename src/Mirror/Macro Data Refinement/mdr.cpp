// main.cpp
// Complete C++ port of the dynamic state network simulation with improved T view behavior and forecast enhancements.
// Uses GLFW for window/input, GLM for math, and stb_easy_font for text rendering.
// Make sure you have GLFW, GLM, and stb_easy_font.h in your include path.
// Compile with: g++ main.cpp -lglfw -lGL -ldl -o simulation

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <set>
#include <random>
#include <chrono>
#include <algorithm>

// Include stb_easy_font for text rendering.
// Download stb_easy_font.h from https://github.com/nothings/stb/blob/master/stb_easy_font.h
#define STB_EASY_FONT_IMPLEMENTATION
#include "stb_easy_font.h"

// --------------------------
// Utility: Pair hash for unordered_map keys.
struct pair_hash {
    std::size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};

// --------------------------
// Global Configuration & Simulation State
// --------------------------
const int INITIAL_GRID_WIDTH = 64;
const int INITIAL_GRID_HEIGHT = 64;
const int SIM_SPEED_DEFAULT = 500;      // milliseconds per step
const int FORECAST_WINDOW = 10;
const int AUTO_LOG_INTERVAL = 30000;      // milliseconds

// Global grid dimensions (editable at runtime)
int current_grid_width = INITIAL_GRID_WIDTH;
int current_grid_height = INITIAL_GRID_HEIGHT;
inline int total_states() { return current_grid_width * current_grid_height; }

// Random generator
std::random_device rd;
std::mt19937 rng(rd());

// Utility: current time in milliseconds using GLFW time.
double getTimeMs() {
    return glfwGetTime() * 1000.0;
}

// --------------------------
// Simulation Data Structures
// --------------------------
std::vector<std::string> states;
std::vector<std::vector<double>> transitions;

int simulation_speed = SIM_SPEED_DEFAULT;
bool paused = false;
std::vector<int> state_sequence; // holds state IDs
int current_state = 0;
double last_update_time = 0;
double simulation_start_time = 0;
std::string simulation_mode = "auto"; // "auto" or "step"

// View modes: "plot", "histogram", "analysis", "editor", "replay", "settings", "help", "record"
// Forecast view toggles with forecast_toggle (0: off, 1: overlay, 2: forecast only).
std::string view_mode = "plot";
bool debug_mode = false;
double zoom_level = 1.0;
double target_camera_zoom = 1.0;
double flash_timer = 0;
int max_plot_steps = 100;
int log_display_count = 10;
std::vector<std::string> transition_log;
double last_auto_log_save = 0;

// Multi-simulation mode
bool multi_sim_mode = false;
std::vector<int> sim2_history;
std::vector<int> sim3_history;
std::vector<std::vector<double>> sim2_transitions;
std::vector<std::vector<double>> sim3_transitions;

// --------------------------
// Transition Editor Variables ("T view")
std::set<std::pair<int, int>> selected_cells; // (row, col)
bool is_selecting = false;
glm::vec2 selection_start(0.0f);
glm::vec4 selection_rect(0.0f, 0.0f, 0.0f, 0.0f);

// Animation for bin drop effect:
bool animation_active = false;
double animation_start_time = 0;
int animation_duration = 1000; // ms
glm::vec4 animation_initial_bbox(0, 0, 0, 0);
int animation_target_bin = -1;

// For panning in editor view:
glm::vec2 camera_offset(0.0f, 0.0f);
double camera_zoom = 1.0;
bool pan_active = false;
glm::vec2 pan_start(0.0f, 0.0f);
glm::vec2 pan_start_offset(0.0f, 0.0f);

// --------------------------
// Bin Drop Zone Definitions (for editor view)
const int BIN_HEIGHT = 50;
const int BIN_GAP = 10;
std::vector<glm::vec4> bins(6, glm::vec4(0, 0, 0, 0));
std::vector<std::string> bin_labels = { "Lhate", "Worror", "Huvalence", "Crostalgia", "Shuilt", "Jempasy" };
const int BIN_CAPACITY = 100000;
std::vector<int> bin_loads(6, 0);

// --------------------------
// Per-Cell Floating Offsets (Random Floating Effect)
struct CellOffset {
    glm::vec2 start;
    glm::vec2 target;
    double start_time;
    double end_time;
};
std::unordered_map<std::pair<int, int>, CellOffset, pair_hash> cell_offsets;
const int CELL_OFFSET_INTERVAL = 5000; // ms

// --------------------------
// Global UI Layout
// --------------------------
int window_width = 1280;
int window_height = 720;
int control_panel_width = 250;
int plot_width = 0; // computed as window_width - control_panel_width
int margin_left = 50, margin_right = 20, margin_top = 50, margin_bottom = 50;

// --------------------------
// Colors (RGB, 0-1)
glm::vec3 background_color(0, 0, 0);
glm::vec3 plot_bg_color(0, 0, 0);
glm::vec3 control_bg_color(0, 0, 0);
glm::vec3 line_color(1, 1, 1);
glm::vec3 grid_color(1, 1, 1);
glm::vec3 text_color(1, 1, 1);
glm::vec3 error_flash_color(1, 1, 1);
glm::vec3 button_color(1, 1, 1);
glm::vec3 button_hover_color(1, 1, 1);
glm::vec3 multi_color_1(1, 1, 1);
glm::vec3 multi_color_2(1, 1, 1);

// --------------------------
// Global mouse position (for editor view)
double g_mouseX = 0.0, g_mouseY = 0.0;

// --------------------------
// Forecast Display & Recording Variables
// --------------------------
int forecast_toggle = 0; // 0: off, 1: overlay, 2: forecast only
bool forecast_recording = false;
// Recorded forecast data stored in memory.
std::vector<std::string> forecast_records;

// --------------------------
// For Record View Copy Feedback
bool record_copy_message = false;
double record_copy_time = 0;

// --------------------------
// Global GLFW window pointer (for clipboard)
GLFWwindow* g_window = nullptr;

// --------------------------
// Text Rendering using stb_easy_font
// --------------------------
void drawText(float x, float y, const char* text, const glm::vec3& color) {
    char buffer[99999];
    int num_quads = stb_easy_font_print(x, y, const_cast<char*>(text), nullptr, buffer, sizeof(buffer));
    glColor3f(color.r, color.g, color.b);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void drawTextScaled(float x, float y, const char* text, const glm::vec3& color, float scale) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);
    drawText(0, 0, text, color);
    glPopMatrix();
}

// --------------------------
// Basic OpenGL Drawing Primitives
// --------------------------
void drawRect(float x, float y, float width, float height, const glm::vec3& color, bool filled = true, float lineWidth = 1.0f) {
    glColor3f(color.r, color.g, color.b);
    if (filled)
        glBegin(GL_QUADS);
    else {
        glLineWidth(lineWidth);
        glBegin(GL_LINE_LOOP);
    }
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void drawLine(float x1, float y1, float x2, float y2, const glm::vec3& color, float lineWidth = 1.0f) {
    glColor3f(color.r, color.g, color.b);
    glLineWidth(lineWidth);
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

// Utility: Draw a filled circle at (cx,cy) with a given radius and color.
void drawCircle(float cx, float cy, float radius, const glm::vec3& color) {
    const int num_segments = 50;
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= num_segments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);
        float dx = radius * cosf(theta);
        float dy = radius * sinf(theta);
        glVertex2f(cx + dx, cy + dy);
    }
    glEnd();
}

// --------------------------
// Record View Drawing Function
// --------------------------
void draw_record_view() {
    drawRect(0, 0, window_width, window_height, plot_bg_color, true);
    float y = margin_top;
    drawText(margin_left, y, "Forecast Recording Output (press J to return):", text_color);
    y += 30;
    if (forecast_records.empty()) {
        drawText(margin_left, y, "No forecast recordings available.", text_color);
    }
    else {
        for (const auto& line : forecast_records) {
            drawText(margin_left, y, line.c_str(), text_color);
            y += 20;
            if (y > window_height - margin_bottom) break;
        }
    }
    // Instruction to copy records
    drawText(margin_left, window_height - margin_bottom - 30, "Press C to copy records to clipboard", text_color);
    // If forecast recording is active, show a blinking indicator
    if (forecast_recording) {
        if (fmod(getTimeMs() / 500.0, 2.0) < 1.0)
            drawCircle(20, 20, 10, glm::vec3(0.0f, 1.0f, 0.0f));
        drawText(margin_left, window_height - margin_bottom - 60, "Recording Active", text_color);
    }
    // Show copy feedback if applicable
    if (record_copy_message && (getTimeMs() - record_copy_time) < 2000) {
        drawText(margin_left, window_height - margin_bottom - 90, "Copied to clipboard!", text_color);
    }
}

// --------------------------
// Simulation Setup Functions
// --------------------------
std::vector<std::string> generate_states(int num) {
    std::vector<std::string> s;
    for (int i = 0; i < num; i++) {
        s.push_back(std::to_string(i));
    }
    return s;
}

std::vector<std::vector<double>> generate_transitions(const std::vector<std::string>& st) {
    int num = st.size();
    std::vector<std::vector<double>> trans(num, std::vector<double>(num, 0.0));
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < num; i++) {
        double total = 0.0;
        std::vector<double> weights(num);
        for (int j = 0; j < num; j++) {
            weights[j] = dist(rng);
            total += weights[j];
        }
        for (int j = 0; j < num; j++) {
            trans[i][j] = weights[j] / total;
        }
    }
    return trans;
}

void initialize_simulation() {
    states = generate_states(total_states());
    transitions = generate_transitions(states);
    state_sequence.clear();
    state_sequence.push_back(0);
    current_state = 0;
    transition_log.clear();
    transition_log.push_back("0: " + states[0]);
    simulation_start_time = getTimeMs();
    last_update_time = getTimeMs();
    last_auto_log_save = getTimeMs();

    // Multi-sim
    sim2_history.clear();
    sim3_history.clear();
    sim2_history.push_back(0);
    sim3_history.push_back(0);
    sim2_transitions = generate_transitions(states);
    sim3_transitions = generate_transitions(states);
    for (int i = 0; i < total_states(); i++) {
        sim2_transitions[i][i] *= 1.1;
        sim3_transitions[i][i] *= 0.9;
    }
}

// --------------------------
// Simulation Update Functions
// --------------------------
void play_state_sound(int state) {
    // Sound functionality not implemented.
}

void update_simulation() {
    int numStates = total_states();
    std::discrete_distribution<int> dist(transitions[current_state].begin(), transitions[current_state].end());
    int next_state = dist(rng);
    state_sequence.push_back(next_state);
    current_state = next_state;
    play_state_sound(current_state);
    transition_log.push_back(std::to_string(state_sequence.size() - 1) + ": " + states[current_state]);
    if (states[current_state] == "error")
        flash_timer = 300;
}

void update_multi_simulations() {
    int curr2 = sim2_history.back();
    std::discrete_distribution<int> dist2(sim2_transitions[curr2].begin(), sim2_transitions[curr2].end());
    int ns2 = dist2(rng);
    sim2_history.push_back(ns2);
    int curr3 = sim3_history.back();
    std::discrete_distribution<int> dist3(sim3_transitions[curr3].begin(), sim3_transitions[curr3].end());
    int ns3 = dist3(rng);
    sim3_history.push_back(ns3);
}

void manual_override(int new_state) {
    if (new_state >= 0 && new_state < total_states()) {
        state_sequence.push_back(new_state);
        current_state = new_state;
        play_state_sound(current_state);
        transition_log.push_back(std::to_string(state_sequence.size() - 1) + ": " + states[current_state]);
        if (states[current_state] == "error")
            flash_timer = 300;
    }
}

void flash_effect(double duration_ms) {
    flash_timer = duration_ms;
}

std::unordered_map<int, int> get_state_counts(const std::vector<int>& seq) {
    std::unordered_map<int, int> counts;
    for (int s : seq) {
        counts[s]++;
    }
    return counts;
}

double get_simulation_time() {
    return (getTimeMs() - simulation_start_time) / 1000.0;
}

void export_log() {
    std::ofstream ofs("simulation_log.txt");
    if (ofs) {
        ofs << "State Evolution Log\n";
        ofs << "Elapsed Time: " << get_simulation_time() << " seconds\n";
        ofs << "Transitions: " << (state_sequence.size() - 1) << "\n";
        for (int s : state_sequence)
            ofs << states[s] << "\n";
        ofs.close();
        std::cout << "Simulation log exported successfully.\n";
    }
    else {
        std::cerr << "Error exporting log\n";
    }
}

void auto_save_log() {
    std::ofstream ofs("auto_sim_log.txt", std::ios::app);
    if (ofs) {
        ofs << "Auto Save at " << get_simulation_time() << " seconds\n";
        ofs << "Transitions: " << (state_sequence.size() - 1) << "\n";
        for (int s : state_sequence)
            ofs << states[s] << "\n";
        ofs << "\n";
        ofs.close();
        std::cout << "Auto log saved.\n";
    }
    else {
        std::cerr << "Error auto saving log\n";
    }
}

void reset_simulation() {
    state_sequence.clear();
    state_sequence.push_back(0);
    current_state = 0;
    simulation_start_time = getTimeMs();
    transition_log.clear();
    transition_log.push_back("0: " + states[0]);
}

void clear_log() {
    transition_log.clear();
}

void modify_transition_weight(int curr_state, int next_state, double delta) {
    transitions[curr_state][next_state] = std::max<double>(0.0, transitions[curr_state][next_state] + delta);
}

std::unordered_map<int, std::pair<double, int>> compute_dwell_stats() {
    std::unordered_map<int, std::vector<int>> stats;
    for (int i = 0; i < total_states(); i++)
        stats[i] = std::vector<int>();
    if (state_sequence.empty()) return {};
    int curr = state_sequence[0];
    int count = 1;
    for (size_t i = 1; i < state_sequence.size(); i++) {
        if (state_sequence[i] == curr) {
            count++;
        }
        else {
            stats[curr].push_back(count);
            curr = state_sequence[i];
            count = 1;
        }
    }
    stats[curr].push_back(count);
    std::unordered_map<int, std::pair<double, int>> result;
    for (auto& p : stats) {
        double avg = 0.0;
        int mx = 0;
        if (!p.second.empty()) {
            for (int v : p.second) {
                avg += v;
                if (v > mx) mx = v;
            }
            avg /= p.second.size();
        }
        result[p.first] = { avg, mx };
    }
    return result;
}

void save_simulation_state() {
    std::ofstream ofs("sim_state.txt");
    if (ofs) {
        ofs << simulation_start_time << "\n";
        ofs << current_state << "\n";
        ofs << simulation_speed << "\n";
        ofs << zoom_level << "\n";
        ofs << simulation_mode << "\n";
        ofs << current_grid_width << " " << current_grid_height << "\n";
        ofs << state_sequence.size() << "\n";
        for (int s : state_sequence)
            ofs << s << " ";
        ofs << "\n";
        for (auto& row : transitions) {
            for (double val : row)
                ofs << val << " ";
            ofs << "\n";
        }
        ofs.close();
        std::cout << "Simulation state saved successfully.\n";
    }
    else {
        std::cerr << "Error saving simulation state.\n";
    }
}

void load_simulation_state() {
    std::ifstream ifs("sim_state.txt");
    if (ifs) {
        ifs >> simulation_start_time;
        ifs >> current_state;
        ifs >> simulation_speed;
        ifs >> zoom_level;
        ifs >> simulation_mode;
        ifs >> current_grid_width >> current_grid_height;
        int seqSize;
        ifs >> seqSize;
        state_sequence.clear();
        for (int i = 0; i < seqSize; i++) {
            int s;
            ifs >> s;
            state_sequence.push_back(s);
        }
        states = generate_states(total_states());
        transitions.resize(total_states());
        for (int i = 0; i < total_states(); i++) {
            transitions[i].resize(total_states());
            for (int j = 0; j < total_states(); j++) {
                ifs >> transitions[i][j];
            }
        }
        std::cout << "Simulation state loaded successfully.\n";
    }
    else {
        std::cerr << "Error loading simulation state.\n";
    }
}

// --------------------------
// Forecast Data Calculation
// --------------------------
std::vector<glm::vec2> compute_forecast_points(float plot_area_width) {
    std::vector<glm::vec2> forecast_points;
    if (state_sequence.size() >= FORECAST_WINDOW) {
        int total_steps = state_sequence.size();
        int display_count = std::min<int>(total_steps, max_plot_steps);
        int offset_val = (total_steps > max_plot_steps) ? total_steps - max_plot_steps : 0;
        float plot_area_height = window_height - margin_top - margin_bottom;
        int y_max = states.size() - 1;
        float x_scale = (plot_area_width / (max_plot_steps - 1));
        float y_scale = (plot_area_height / (y_max > 0 ? y_max : 1));
        int start_idx = std::max(offset_val, FORECAST_WINDOW - 1);
        for (int i = start_idx; i < total_steps; i++) {
            int sum = 0;
            for (int j = i - FORECAST_WINDOW + 1; j <= i; j++) {
                sum += state_sequence[j];
            }
            float avg_val = sum / (float)FORECAST_WINDOW;
            float x = margin_left + (i - offset_val) * x_scale;
            float y = margin_top + (y_max - avg_val) * y_scale;
            forecast_points.push_back(glm::vec2(x, y));
        }
    }
    return forecast_points;
}

// --------------------------
// Drawing Functions
// --------------------------
void draw_plot() {
    if (view_mode == "forecast" && forecast_toggle == 2) {
        drawRect(0, 0, plot_width, window_height, plot_bg_color, true);
    }
    else {
        drawRect(0, 0, plot_width, window_height, plot_bg_color, true);
        int display_count = std::min<int>(state_sequence.size(), max_plot_steps);
        float plot_area_width = plot_width - margin_left - margin_right;
        float plot_area_height = window_height - margin_top - margin_bottom;
        int y_max = states.size() - 1;
        float x_scale = (plot_area_width / (max_plot_steps - 1));
        float y_scale = (plot_area_height / (y_max > 0 ? y_max : 1));
        std::vector<glm::vec2> points;
        int offset_val = (state_sequence.size() > max_plot_steps) ? state_sequence.size() - max_plot_steps : 0;
        for (int i = offset_val; i < state_sequence.size(); i++) {
            float x = margin_left + (i - offset_val) * x_scale;
            float y = margin_top + (y_max - state_sequence[i]) * y_scale;
            points.push_back(glm::vec2(x, y));
        }
        if (points.size() > 1) {
            glColor3f(line_color.r, line_color.g, line_color.b);
            glLineWidth(2.0f);
            glBegin(GL_LINE_STRIP);
            for (size_t i = 0; i < points.size(); i++) {
                glVertex2f(points[i].x, points[i].y);
            }
            glEnd();
        }
    }
    if (state_sequence.size() >= FORECAST_WINDOW) {
        float plot_area_width = plot_width - margin_left - margin_right;
        std::vector<glm::vec2> forecast_points = compute_forecast_points(plot_area_width);
        if (forecast_points.size() > 1) {
            glColor3f(0.8f, 0.8f, 0.8f);
            glLineWidth(2.0f);
            glBegin(GL_LINE_STRIP);
            for (auto& p : forecast_points)
                glVertex2f(p.x, p.y);
            glEnd();
        }
    }
}

void draw_histogram() {
    auto counts = get_state_counts(state_sequence);
    float bar_area_left = margin_left;
    float bar_area_right = plot_width - margin_right;
    float bar_area_top = margin_top;
    float bar_area_bottom = window_height - margin_bottom;
    float bar_area_width = bar_area_right - bar_area_left;
    float bar_area_height = bar_area_bottom - bar_area_top;
    drawRect(0, 0, plot_width, window_height, plot_bg_color, true);
    int num = states.size();
    float bar_width = bar_area_width / (num * 2.0f);
    int max_count = 1;
    for (auto& p : counts)
        if (p.second > max_count) max_count = p.second;
    for (int i = 0; i < num; i++) {
        int count = counts[i];
        float bar_height = (max_count > 0) ? (count / (float)max_count) * bar_area_height : 0;
        float x = bar_area_left + (2 * i + 1) * bar_width;
        float y = bar_area_bottom - bar_height;
        drawRect(x, y, bar_width, bar_height, line_color, true);
        std::stringstream ss;
        ss << states[i] << " (" << count << ")";
        drawText(x, bar_area_bottom + 5, ss.str().c_str(), text_color);
    }
}

void draw_analysis() {
    drawRect(0, 0, plot_width, window_height, plot_bg_color, true);
    auto stats = compute_dwell_stats();
    float y_offset = margin_top;
    drawText(margin_left, y_offset, "Dwell Time Analysis", text_color);
    y_offset += 20;
    for (int i = 0; i < states.size(); i++) {
        auto st = stats[i];
        double avg = st.first;
        int mx = st.second;
        double avg_time = avg * simulation_speed / 1000.0;
        double mx_time = mx * simulation_speed / 1000.0;
        std::stringstream ss;
        ss << states[i] << ": Avg " << avg << " (" << avg_time << "s), Max " << mx << " (" << mx_time << "s)";
        drawText(margin_left, y_offset, ss.str().c_str(), text_color);
        y_offset += 20;
    }
}

void draw_transition_editor() {
    float grid_width = window_width - 2 * margin_left;
    float grid_height = window_height - 2 * margin_top - BIN_HEIGHT - 2 * BIN_GAP;
    float cell_width = grid_width / current_grid_width;
    float cell_height = grid_height / current_grid_height;
    drawRect(0, 0, window_width, window_height, plot_bg_color, true);
    double current_time = getTimeMs();
    float local_mouse_x = (float)(g_mouseX - (margin_left + camera_offset.x)) / zoom_level;
    float local_mouse_y = (float)(g_mouseY - (margin_top + camera_offset.y)) / zoom_level;
    glPushMatrix();
    glTranslatef(margin_left + camera_offset.x, margin_top + camera_offset.y, 0);
    glScalef(zoom_level, zoom_level, 1.0f);
    float effect_radius_screen = 50.0f;
    float effect_radius_local = effect_radius_screen / zoom_level;
    float max_scale = 4.0f;
    float min_scale = 1.0f;
    for (int r = 0; r < current_grid_height; r++) {
        for (int c = 0; c < current_grid_width; c++) {
            int idx = r * current_grid_width + c;
            float base_x = c * cell_width;
            float base_y = r * cell_height;
            float cell_center_x = base_x + cell_width / 2;
            float cell_center_y = base_y + cell_height / 2;
            float dx = cell_center_x - local_mouse_x;
            float dy = cell_center_y - local_mouse_y;
            float dist = sqrt(dx * dx + dy * dy);
            float scaleFactor = 1.0f;
            if (dist < effect_radius_local) {
                scaleFactor = min_scale + (max_scale - min_scale) * (1 - (dist / effect_radius_local));
            }
            std::pair<int, int> key = { r, c };
            if (cell_offsets.find(key) == cell_offsets.end()) {
                cell_offsets[key] = { glm::vec2(0,0),
                    glm::vec2((float)((rand() % 600 - 300) / 100.0f), (float)((rand() % 600 - 300) / 100.0f)),
                    current_time,
                    current_time + CELL_OFFSET_INTERVAL };
            }
            CellOffset& co = cell_offsets[key];
            if (current_time >= co.end_time) {
                co.start = co.target;
                co.target = glm::vec2((float)((rand() % 600 - 300) / 100.0f), (float)((rand() % 600 - 300) / 100.0f));
                co.start_time = co.end_time;
                co.end_time = co.start_time + CELL_OFFSET_INTERVAL;
            }
            float t = (current_time - co.start_time) / (co.end_time - co.start_time);
            glm::vec2 float_offset = co.start + (co.target - co.start) * t;
            float total_x = base_x + float_offset.x;
            float total_y = base_y + float_offset.y;
            std::string text = std::to_string((int)round(transitions[idx][idx]));
            drawTextScaled(total_x + 5, total_y + cell_height / 2, text.c_str(), text_color, scaleFactor);
        }
    }
    glPopMatrix();
    if (is_selecting) {
        glColor4f(1, 1, 1, 0.4f);
        glBegin(GL_QUADS);
        glVertex2f(selection_rect.x, selection_rect.y);
        glVertex2f(selection_rect.x + selection_rect.z, selection_rect.y);
        glVertex2f(selection_rect.x + selection_rect.z, selection_rect.y + selection_rect.w);
        glVertex2f(selection_rect.x, selection_rect.y + selection_rect.w);
        glEnd();
    }
    if (!selected_cells.empty()) {
        float minx = (float)window_width, miny = (float)window_height, maxx = 0, maxy = 0;
        for (auto& p : selected_cells) {
            float cell_x = margin_left + camera_offset.x + p.second * (cell_width * zoom_level);
            float cell_y = margin_top + camera_offset.y + p.first * (cell_height * zoom_level);
            minx = std::min<float>(minx, cell_x);
            miny = std::min<float>(miny, cell_y);
            maxx = std::max<float>(maxx, cell_x + cell_width * zoom_level);
            maxy = std::max<float>(maxy, cell_y + cell_height * zoom_level);
        }
        drawRect(minx, miny, maxx - minx, maxy - miny, glm::vec3(1, 1, 1), false, 2.0f);
    }
    float bin_area_y = window_height - BIN_HEIGHT - BIN_GAP;
    float total_bin_width = (window_width - 2 * margin_left - (5 * BIN_GAP));
    float bin_w = total_bin_width / 6.0f;
    float start_x = margin_left;
    for (int i = 0; i < 6; i++) {
        glm::vec4 bin_rect(start_x + BIN_GAP + i * (bin_w + BIN_GAP), bin_area_y, bin_w, BIN_HEIGHT);
        drawRect(bin_rect.x, bin_rect.y, bin_rect.z, bin_rect.w, glm::vec3(1, 1, 1), false, 2.0f);
        drawText(bin_rect.x + 5, bin_rect.y + 5, bin_labels[i].c_str(), glm::vec3(1, 1, 1));
        bins[i] = bin_rect;
    }
    if (animation_active) {
        double animT = (getTimeMs() - animation_start_time) / animation_duration;
        if (animT >= 1.0) {
            animT = 1.0;
            for (auto& p : selected_cells) {
                int idx = p.first * current_grid_width + p.second;
                double new_val = 0;
                switch (animation_target_bin) {
                case 0:
                    new_val = ((double)rand() / RAND_MAX) * (9 - 1) + 1;
                    break;
                case 1:
                    new_val = ((double)rand() / RAND_MAX) * (8 - 3) + 3;
                    break;
                case 2:
                    new_val = ((double)rand() / RAND_MAX) * (6 - 2) + 2;
                    break;
                case 3:
                    if (rand() % 2 == 0)
                        new_val = ((double)rand() / RAND_MAX) * (3 - 1) + 1;
                    else
                        new_val = ((double)rand() / RAND_MAX) * (9 - 7) + 7;
                    break;
                case 4:
                    new_val = std::max(1.0, transitions[idx][idx] * 0.5);
                    break;
                case 5:
                    new_val = (transitions[idx][idx] + (((double)rand() / RAND_MAX) * (9 - 1) + 1)) / 2.0;
                    break;
                default:
                    new_val = transitions[idx][idx];
                    break;
                }
                transitions[idx][idx] = new_val;
            }
            animation_active = false;
            selected_cells.clear();
        }
        else {
            glm::vec4 start_rect = animation_initial_bbox;
            glm::vec4 target_bin_rect = bins[animation_target_bin];
            glm::vec4 target_rect;
            target_rect.x = target_bin_rect.x + target_bin_rect.z / 2 - 10;
            target_rect.y = target_bin_rect.y + target_bin_rect.w / 2 - 10;
            target_rect.z = 20;
            target_rect.w = 20;
            glm::vec4 interp_rect;
            interp_rect.x = start_rect.x + (target_rect.x - start_rect.x) * animT;
            interp_rect.y = start_rect.y + (target_rect.y - start_rect.y) * animT;
            interp_rect.z = start_rect.z + (target_rect.z - start_rect.z) * animT;
            interp_rect.w = start_rect.w + (target_rect.w - start_rect.w) * animT;
            drawRect(interp_rect.x, interp_rect.y, interp_rect.z, interp_rect.w, glm::vec3(1, 1, 1), false, 2.0f);
        }
    }
}

void draw_settings_view() {
    drawRect(0, 0, plot_width, window_height, plot_bg_color, true);
    float y_offset = margin_top;
    drawText(margin_left, y_offset, "Settings", text_color);
    y_offset += 30;
    std::vector<std::pair<std::string, double>> settings = {
        {"Speed (ms/step)", (double)simulation_speed},
        {"Zoom Level", zoom_level},
        {"Grid Width", (double)current_grid_width},
        {"Grid Height", (double)current_grid_height}
    };
    for (auto& setting : settings) {
        std::stringstream ss;
        ss << setting.first << ": " << setting.second;
        drawText(margin_left, y_offset, ss.str().c_str(), text_color);
        y_offset += 30;
    }
    drawText(margin_left, y_offset, "Click buttons to adjust parameters. Grid change resets T view.", text_color);
}

void draw_replay_view() {
    draw_plot();
}

void draw_log() {
    float log_area_width = plot_width - margin_left - margin_right;
    float log_area_height = 100;
    float log_x = margin_left;
    float log_y = window_height - log_area_height - 10;
    drawRect(log_x, log_y, log_area_width, log_area_height, glm::vec3(0, 0, 0), true);
    int start = std::max<int>(0, transition_log.size() - log_display_count);
    float y = log_y + 5;
    for (size_t i = start; i < transition_log.size(); i++) {
        drawText(log_x + 5, y, transition_log[i].c_str(), text_color);
        y += 20;
    }
}

void draw_control_panel() {
    drawRect(plot_width, 0, control_panel_width, window_height, control_bg_color, true);
    std::vector<std::string> info_lines = {
        "Controls:",
        "Space: Pause/Resume",
        "R: Reset Sim",
        "Up/Down: Speed +/-",
        "1-6: Manual Override",
        "Z: Zoom In, X: Zoom Out",
        "D: Toggle Debug",
        "E: Export Log",
        "P: Plot View",
        "H: Histogram View",
        "A: Analysis View",
        "T: Transition Editor",
        "Y: Replay View",
        "F: Toggle Forecast (cycles Overlay -> Only -> Off)",
        "K: Toggle Forecast Recording",
        "F2: Settings View",
        "M: Multi-Sim Mode",
        "S: Toggle Auto/Step",
        "N: Next Step (Step Mode)",
        "C: Clear Log (or copy records in Record View)",
        "O: Save Sim, L: Load Sim",
        "F1: Toggle Help",
        "J: Toggle Record View",
        "",
        std::string("Paused: ") + (paused ? "true" : "false"),
        "Sim Mode: " + simulation_mode,
        "View Mode: " + view_mode,
        "Speed: " + std::to_string(simulation_speed) + " ms/step",
        "Current: " + states[current_state],
        "Zoom: " + std::to_string(zoom_level),
        "Time: " + std::to_string(get_simulation_time()) + "s"
    };
    float y = 20;
    for (auto& line : info_lines) {
        drawText(plot_width + 10, y, line.c_str(), text_color);
        y += 20;
    }
}

void draw_help_overlay() {
    glColor4f(0, 0, 0, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(window_width, 0);
    glVertex2f(window_width, window_height);
    glVertex2f(0, window_height);
    glEnd();
    std::vector<std::string> help_lines = {
        "Help - Key Bindings:",
        "Space: Pause/Resume",
        "R: Reset Simulation",
        "Up/Down: Speed +/-",
        "1-6: Manual Override",
        "Z/X: Zoom In/Out",
        "D: Toggle Debug",
        "E: Export Log",
        "P: Plot View",
        "H: Histogram View",
        "A: Analysis View",
        "T: Transition Editor",
        "Y: Replay View",
        "F: Toggle Forecast (cycles Overlay -> Only -> Off)",
        "K: Toggle Forecast Recording",
        "F2: Settings View",
        "M: Multi-Sim Mode",
        "S: Toggle Auto/Step",
        "N: Next Step (Step Mode)",
        "C: Clear Log (or copy records in Record View)",
        "O: Save Sim, L: Load Sim",
        "F1: Toggle Help",
        "J: Toggle Record View"
    };
    float y = 50;
    for (auto& line : help_lines) {
        drawText(50, y, line.c_str(), text_color);
        y += 20;
    }
}

void update_window_title(GLFWwindow* window) {
    std::stringstream ss;
    ss << "State: " << states[current_state]
        << " | Speed: " << simulation_speed << " ms"
        << " | Time: " << get_simulation_time() << "s";
    glfwSetWindowTitle(window, ss.str().c_str());
}

// --------------------------
// GLFW Callbacks
// --------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        else if (key == GLFW_KEY_SPACE)
            paused = !paused;
        else if (key == GLFW_KEY_R)
            reset_simulation();
        else if (key == GLFW_KEY_UP)
            simulation_speed = std::max<int>(50, simulation_speed - 50);
        else if (key == GLFW_KEY_DOWN)
            simulation_speed += 50;
        else if (key == GLFW_KEY_D)
            debug_mode = !debug_mode;
        else if (key == GLFW_KEY_Z)
            zoom_level = std::min<double>(3.0, zoom_level + 0.1);
        else if (key == GLFW_KEY_X)
            zoom_level = std::max<double>(0.5, zoom_level - 0.1);
        else if (key == GLFW_KEY_1)
            manual_override(0);
        else if (key == GLFW_KEY_2) {
            if (total_states() > 1)
                manual_override(1);
        }
        else if (key == GLFW_KEY_3) {
            if (total_states() > 2)
                manual_override(2);
        }
        else if (key == GLFW_KEY_4) {
            if (total_states() > 3)
                manual_override(3);
        }
        else if (key == GLFW_KEY_5) {
            if (total_states() > 4)
                manual_override(4);
        }
        else if (key == GLFW_KEY_6) {
            if (total_states() > 5)
                manual_override(5);
        }
        else if (key == GLFW_KEY_E)
            export_log();
        else if (key == GLFW_KEY_P)
            view_mode = "plot";
        else if (key == GLFW_KEY_H)
            view_mode = "histogram";
        else if (key == GLFW_KEY_A)
            view_mode = "analysis";
        else if (key == GLFW_KEY_T) {
            view_mode = "editor";
            selected_cells.clear();
        }
        else if (key == GLFW_KEY_Y)
            view_mode = "replay";
        else if (key == GLFW_KEY_F) {
            if (forecast_toggle == 0) {
                forecast_toggle = 1;
                view_mode = "forecast";
            }
            else if (forecast_toggle == 1) {
                forecast_toggle = 2;
            }
            else if (forecast_toggle == 2) {
                forecast_toggle = 0;
                view_mode = "plot";
            }
        }
        else if (key == GLFW_KEY_K) {
            // Toggle forecast recording.
            forecast_recording = !forecast_recording;
            if (forecast_recording) {
                forecast_records.clear();
                std::cout << "Forecast recording started." << std::endl;
            }
            else {
                std::cout << "Forecast recording stopped." << std::endl;
            }
        }
        else if (key == GLFW_KEY_F2)
            view_mode = "settings";
        else if (key == GLFW_KEY_M)
            multi_sim_mode = !multi_sim_mode;
        else if (key == GLFW_KEY_S)
            simulation_mode = (simulation_mode == "auto" ? "step" : "auto");
        else if (key == GLFW_KEY_N) {
            if (simulation_mode == "step" && !paused) {
                update_simulation();
                last_update_time = getTimeMs();
            }
        }
        else if (key == GLFW_KEY_C) {
            // In record view, copy records to clipboard; otherwise, clear log.
            if (view_mode == "record") {
                std::stringstream ss;
                for (const auto& line : forecast_records) {
                    ss << line << "\n";
                }
                glfwSetClipboardString(g_window, ss.str().c_str());
                record_copy_message = true;
                record_copy_time = getTimeMs();
                std::cout << "Forecast records copied to clipboard." << std::endl;
            }
            else {
                clear_log();
            }
        }
        else if (key == GLFW_KEY_O)
            save_simulation_state();
        else if (key == GLFW_KEY_L)
            load_simulation_state();
        else if (key == GLFW_KEY_F1)
            view_mode = (view_mode == "help" ? "plot" : "help");
        else if (key == GLFW_KEY_J) {
            // Toggle record view.
            if (view_mode != "record")
                view_mode = "record";
            else
                view_mode = "plot";
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (view_mode == "editor") {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                is_selecting = true;
                selection_start = glm::vec2((float)xpos, (float)ypos);
                selection_rect = glm::vec4((float)xpos, (float)ypos, 0, 0);
                selected_cells.clear();
            }
            else if (action == GLFW_RELEASE) {
                is_selecting = false;
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (action == GLFW_PRESS) {
                pan_active = true;
                pan_start = glm::vec2((float)xpos, (float)ypos);
                pan_start_offset = camera_offset;
            }
            else if (action == GLFW_RELEASE) {
                pan_active = false;
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_LEFT) {
            for (int i = 0; i < 6; i++) {
                glm::vec4 b = bins[i];
                if (xpos >= b.x && xpos <= b.x + b.z &&
                    ypos >= b.y && ypos <= b.y + b.w &&
                    !selected_cells.empty()) {
                    if (bin_loads[i] < BIN_CAPACITY) {
                        animation_active = true;
                        animation_start_time = getTimeMs();
                        animation_duration = 1000;
                        animation_target_bin = i;
                        bin_loads[i] += selected_cells.size();
                        float cell_grid_width = (window_width - 2 * margin_left) / current_grid_width;
                        float cell_grid_height = (window_height - 2 * margin_top - BIN_HEIGHT - 2 * BIN_GAP) / current_grid_height;
                        float minx = (float)window_width, miny = (float)window_height, maxx = 0, maxy = 0;
                        for (auto& p : selected_cells) {
                            float cell_x = margin_left + camera_offset.x + p.second * (cell_grid_width * zoom_level);
                            float cell_y = margin_top + camera_offset.y + p.first * (cell_grid_height * zoom_level);
                            minx = std::min<float>(minx, cell_x);
                            miny = std::min<float>(miny, cell_y);
                            maxx = std::max<float>(maxx, cell_x + cell_grid_width * zoom_level);
                            maxy = std::max<float>(maxy, cell_y + cell_grid_height * zoom_level);
                        }
                        animation_initial_bbox = glm::vec4(minx, miny, maxx - minx, maxy - miny);
                    }
                }
            }
            bool allAtCapacity = true;
            for (int load : bin_loads) {
                if (load < BIN_CAPACITY) { allAtCapacity = false; break; }
            }
            if (allAtCapacity) {
                for (int r = 0; r < current_grid_height; r++) {
                    for (int c = 0; c < current_grid_width; c++) {
                        int idx = r * current_grid_width + c;
                        transitions[idx][idx] = 0;
                    }
                }
                std::fill(bin_loads.begin(), bin_loads.end(), 0);
            }
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    g_mouseX = xpos;
    g_mouseY = ypos;
    if (view_mode == "editor") {
        if (pan_active) {
            glm::vec2 curr((float)xpos, (float)ypos);
            glm::vec2 delta = curr - pan_start;
            camera_offset = pan_start_offset + delta;
        }
        if (is_selecting) {
            float x0 = selection_start.x;
            float y0 = selection_start.y;
            float x1 = (float)xpos;
            float y1 = (float)ypos;
            selection_rect.x = std::min<float>(x0, x1);
            selection_rect.y = std::min<float>(y0, y1);
            selection_rect.z = std::abs(x1 - x0);
            selection_rect.w = std::abs(y1 - y0);
            float grid_width = window_width - 2 * margin_left;
            float grid_height = window_height - 2 * margin_top - BIN_HEIGHT - 2 * BIN_GAP;
            float cell_width = grid_width / current_grid_width;
            float cell_height = grid_height / current_grid_height;
            selected_cells.clear();
            for (int r = 0; r < current_grid_height; r++) {
                for (int c = 0; c < current_grid_width; c++) {
                    float cell_x = margin_left + camera_offset.x + c * (cell_width * zoom_level);
                    float cell_y = margin_top + camera_offset.y + r * (cell_height * zoom_level);
                    if (selection_rect.x < cell_x + cell_width * zoom_level &&
                        selection_rect.x + selection_rect.z > cell_x &&
                        selection_rect.y < cell_y + cell_height * zoom_level &&
                        selection_rect.y + selection_rect.w > cell_y) {
                        selected_cells.insert({ r, c });
                    }
                }
            }
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (view_mode == "editor") {
        double zoom_factor = (yoffset > 0 ? 1.1 : 0.9);
        zoom_level *= zoom_factor;
        if (zoom_level < 0.5) zoom_level = 0.5;
        if (zoom_level > 3.0) zoom_level = 3.0;
    }
}

// --------------------------
// Main Function
// --------------------------
int main() {
    if (!glfwInit()) {
        std::cerr << "Error initializing GLFW\n";
        return -1;
    }
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    window_width = mode->width;
    window_height = mode->height;
    plot_width = window_width - control_panel_width;
    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Dynamic State Network Simulation", monitor, NULL);
    if (!window) {
        std::cerr << "Error creating GLFW window\n";
        glfwTerminate();
        return -1;
    }
    g_window = window;
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, window_width, window_height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    initialize_simulation();

    while (!glfwWindowShouldClose(window)) {
        double current_time = getTimeMs();
        camera_zoom += (target_camera_zoom - camera_zoom) * 0.1;
        if (current_time - last_auto_log_save >= AUTO_LOG_INTERVAL) {
            auto_save_log();
            last_auto_log_save = current_time;
        }
        glfwPollEvents();
        if (simulation_mode == "auto" && !paused && view_mode != "replay" && view_mode != "help" && view_mode != "record") {
            if (current_time - last_update_time >= simulation_speed) {
                update_simulation();
                last_update_time = current_time;
                if (multi_sim_mode)
                    update_multi_simulations();
            }
        }
        update_window_title(window);
        glClearColor(background_color.r, background_color.g, background_color.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (view_mode == "record") {
            draw_record_view();
        }
        else if (view_mode == "plot" || view_mode == "forecast") {
            draw_plot();
        }
        else if (view_mode == "histogram") {
            draw_histogram();
        }
        else if (view_mode == "analysis") {
            draw_analysis();
        }
        else if (view_mode == "editor") {
            draw_transition_editor();
        }
        else if (view_mode == "replay") {
            draw_replay_view();
        }
        else if (view_mode == "settings") {
            draw_settings_view();
        }
        else if (view_mode == "help") {
            draw_help_overlay();
        }

        if (view_mode != "editor" && view_mode != "help" && view_mode != "record") {
            draw_log();
            draw_control_panel();
        }
        else if (view_mode == "editor") {
            drawText(window_width - control_panel_width - 150, 10, "Transition Editor", text_color);
        }

        // Record forecast data when recording is active and forecast view is on
        if (forecast_recording && forecast_toggle != 0) {
            float plot_area_width = plot_width - margin_left - margin_right;
            std::vector<glm::vec2> forecast_points = compute_forecast_points(plot_area_width);
            if (!forecast_points.empty()) {
                std::stringstream ss;
                ss << get_simulation_time() << ": ";
                for (auto& p : forecast_points) {
                    ss << "(" << p.x << "," << p.y << ") ";
                }
                forecast_records.push_back(ss.str());
            }
        }
        // Draw blinking indicator when recording is active (and not in record view)
        if (forecast_recording && view_mode != "record") {
            if (fmod(current_time / 500.0, 2.0) < 1.0)
                drawCircle(20, 20, 10, glm::vec3(0.0f, 1.0f, 0.0f));
        }

        if (flash_timer > 0) {
            glColor4f(error_flash_color.r, error_flash_color.g, error_flash_color.b, 0.4f);
            glBegin(GL_QUADS);
            glVertex2f(0, 0);
            glVertex2f(plot_width, 0);
            glVertex2f(plot_width, window_height);
            glVertex2f(0, window_height);
            glEnd();
            flash_timer -= (current_time - last_update_time);
        }
        glfwSwapBuffers(window);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
