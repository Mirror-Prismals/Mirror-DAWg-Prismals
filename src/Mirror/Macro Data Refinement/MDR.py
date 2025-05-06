import pygame
import random
import time
import pickle
import math

# ================================
# CONFIGURATION & INITIAL GRID DIMENSIONS
# ================================
INITIAL_GRID_WIDTH = 14  # initial grid columns
INITIAL_GRID_HEIGHT = 14  # initial grid rows
SIM_SPEED_DEFAULT = 500   # ms per transition (auto mode)
FORECAST_WINDOW = 10      # moving-average forecast window
AUTO_LOG_INTERVAL = 30000 # ms

# ================================
# Global grid dimension variable (editable at runtime)
# ================================
current_grid_width = INITIAL_GRID_WIDTH
current_grid_height = INITIAL_GRID_HEIGHT

def total_states():
    return current_grid_width * current_grid_height

# ================================
# State and Transition Generation
# ================================
def generate_states(num):
    # Create a flat list of state labels: "0", "1", ..., "num-1"
    return [str(i) for i in range(num)]

def generate_transitions(states_list):
    num = len(states_list)
    trans = {}
    for s in states_list:
        weights = [random.random() for _ in range(num)]
        total = sum(weights)
        normalized = [w / total for w in weights]
        trans[s] = {states_list[i]: normalized[i] for i in range(num)}
    return trans

states = generate_states(total_states())
transitions = generate_transitions(states)

# ================================
# Simulation Global Variables
# ================================
simulation_speed = SIM_SPEED_DEFAULT
paused = False
state_sequence = [states[0]]
current_state = states[0]
last_update_time = 0
simulation_start_time = 0
simulation_mode = "auto"  # "auto" or "step"

# View modes: "plot", "histogram", "analysis", "editor", "replay", "forecast", "settings"
view_mode = "plot"
debug_mode = False
zoom_level = 1.0
# New: target zoom for easing
target_camera_zoom = 1.0
flash_timer = 0
max_plot_steps = 100
log_display_count = 10
transition_log = [f"0: {states[0]}"]
control_scroll_offset = 0
replay_index = None

last_auto_log_save = 0

multi_sim_mode = False
sim2_history = [states[0]]
sim3_history = [states[0]]
sim2_transitions = generate_transitions(states)
sim3_transitions = generate_transitions(states)
for s in states:
    sim2_transitions[s][s] *= 1.1
    sim3_transitions[s][s] *= 0.9

help_overlay_enabled = False

# ================================
# Transition Editor Variables (T view)
# ================================
selected_cells = set()    # Set of (row, col) tuples (indices in the grid)
is_selecting = False      # True while right-click dragging to select
selection_start = None
selection_rect = None

# Group dragging is not used to adjust individual values; in T view you simply select cells.
animation_active = False
animation_start_time = 0
animation_duration = 1000  # ms
animation_initial_bbox = None
animation_target_bin = None

# For panning & zooming in T view:
camera_offset = [0, 0]  # x,y offset (in pixels) of the grid within T view
camera_zoom = 1.0       # zoom factor for T view

# For panning via middle-mouse drag:
pan_active = False
pan_start = (0, 0)
pan_start_offset = (0, 0)

# -------------------------
# NEW: Touch & Pinch Globals
# -------------------------
touches = {}         # Maps finger_id to current (x, y) in pixels
prev_touches = {}    # Stores previous positions for pan calculations

# ================================
# Bin Drop Zone Definitions (for T view)
# ================================
BIN_HEIGHT = 50
BIN_GAP = 10
bins = [pygame.Rect(0, 0, 0, 0) for _ in range(4)]
bin_labels = ["Bin1", "Bin2", "Bin3", "Bin4"]

# ================================
# Per-Cell Floating Offsets (Random Floating Effect)
# ================================
cell_offsets = {}  # Key: (r, c), Value: dict with "start", "target", "start_time", "end_time"
CELL_OFFSET_INTERVAL = 5000  # ms

def get_cell_offset(r, c, current_time):
    global cell_offsets
    if (r, c) not in cell_offsets:
        cell_offsets[(r, c)] = {
            "start": (0, 0),
            "target": (random.uniform(-3, 3), random.uniform(-3, 3)),
            "start_time": current_time,
            "end_time": current_time + CELL_OFFSET_INTERVAL
        }
    data = cell_offsets[(r, c)]
    if current_time >= data["end_time"]:
        data["start"] = data["target"]
        data["target"] = (random.uniform(-3, 3), random.uniform(-3, 3))
        data["start_time"] = data["end_time"]
        data["end_time"] = data["start_time"] + CELL_OFFSET_INTERVAL
    t = (current_time - data["start_time"]) / (data["end_time"] - data["start_time"])
    offset_x = data["start"][0] + (data["target"][0] - data["start"][0]) * t
    offset_y = data["start"][1] + (data["target"][1] - data["start"][1]) * t
    return (offset_x, offset_y)

# ================================
# Pygame Initialization & Fullscreen Setup
# ================================
pygame.init()
screen = pygame.display.set_mode((0, 0), pygame.FULLSCREEN)
window_width, window_height = screen.get_size()
control_panel_width = 250
plot_width = window_width - control_panel_width
pygame.display.set_caption("Dynamic State Network Simulation")
last_update_time = pygame.time.get_ticks()
simulation_start_time = last_update_time
last_auto_log_save = last_update_time

margin_left = 50
margin_right = 20
margin_top = 50
margin_bottom = 50

# ================================
# Colors, Fonts, and Sound Setup (Black & White)
# ================================
background_color   = (0, 0, 0)
plot_bg_color      = (0, 0, 0)
control_bg_color   = (0, 0, 0)
line_color         = (255, 255, 255)
grid_color         = (255, 255, 255)
text_color         = (255, 255, 255)
error_flash_color  = (255, 255, 255)
# For bins, use white fill with black border.
button_color       = (255, 255, 255)
button_hover_color = (255, 255, 255)
multi_color_1      = (255, 255, 255)
multi_color_2      = (255, 255, 255)
font = pygame.font.SysFont("Arial", 16)

# ================================
# (Sound initialization omitted)
# ================================
sound_files = {}
def play_state_sound(state):
    if sound_files.get(state):
        sound_files[state].play()

# ================================
# Utility & Simulation Functions
# ================================
def update_simulation():
    global current_state, last_update_time, flash_timer
    next_states = list(transitions[current_state].keys())
    probabilities = list(transitions[current_state].values())
    next_state = random.choices(next_states, weights=probabilities)[0]
    state_sequence.append(next_state)
    current_state = next_state
    play_state_sound(current_state)
    transition_log.append(f"{len(state_sequence)-1}: {current_state}")
    if current_state == "error":
        flash_effect(300)

def update_multi_simulations():
    global sim2_history, sim3_history
    curr2 = sim2_history[-1]
    ns2 = random.choices(list(sim2_transitions[curr2].keys()), weights=list(sim2_transitions[curr2].values()))[0]
    sim2_history.append(ns2)
    curr3 = sim3_history[-1]
    ns3 = random.choices(list(sim3_transitions[curr3].keys()), weights=list(sim3_transitions[curr3].values()))[0]
    sim3_history.append(ns3)

def manual_override(new_state):
    global current_state
    if new_state in states:
        state_sequence.append(new_state)
        current_state = new_state
        play_state_sound(current_state)
        transition_log.append(f"{len(state_sequence)-1}: {current_state}")
        if current_state == "error":
            flash_effect(300)

def flash_effect(duration_ms):
    global flash_timer
    flash_timer = duration_ms

def get_state_counts(seq):
    counts = {s: 0 for s in states}
    for s in seq:
        counts[s] += 1
    return counts

def get_simulation_time():
    return (pygame.time.get_ticks() - simulation_start_time) / 1000.0

def export_log():
    try:
        with open("simulation_log.txt", "w") as f:
            f.write("State Evolution Log\n")
            f.write("Elapsed Time: {:.1f} seconds\n".format(get_simulation_time()))
            f.write("Transitions: {}\n".format(len(state_sequence)-1))
            f.write("\n".join(state_sequence))
        print("Simulation log exported successfully.")
    except Exception as e:
        print("Error exporting log:", e)

def auto_save_log():
    try:
        with open("auto_sim_log.txt", "a") as f:
            f.write("Auto Save at {:.1f} seconds\n".format(get_simulation_time()))
            f.write("Transitions: {}\n".format(len(state_sequence)-1))
            f.write("\n".join(state_sequence) + "\n\n")
        print("Auto log saved.")
    except Exception as e:
        print("Error auto saving log:", e)

def reset_simulation():
    global state_sequence, current_state, simulation_start_time, transition_log, replay_index
    state_sequence = [states[0]]
    current_state = states[0]
    simulation_start_time = pygame.time.get_ticks()
    transition_log = [f"0: {states[0]}"]
    replay_index = None

def clear_log():
    global transition_log
    transition_log = []

def modify_transition_weight(curr_state, next_state, delta):
    transitions[curr_state][next_state] = max(0.0, transitions[curr_state][next_state] + delta)

def compute_dwell_stats():
    stats = {s: [] for s in states}
    if not state_sequence:
        return stats
    curr = state_sequence[0]
    count = 1
    for s in state_sequence[1:]:
        if s == curr:
            count += 1
        else:
            stats[curr].append(count)
            curr = s
            count = 1
    stats[curr].append(count)
    result = {}
    for s, runs in stats.items():
        avg = sum(runs) / len(runs) if runs else 0
        result[s] = (avg, max(runs) if runs else 0)
    return result

def save_simulation_state():
    global current_grid_width, current_grid_height
    data = {
        "state_sequence": state_sequence,
        "current_state": current_state,
        "transitions": transitions,
        "simulation_speed": simulation_speed,
        "zoom_level": zoom_level,
        "simulation_mode": simulation_mode,
        "transition_log": transition_log,
        "grid_width": current_grid_width,
        "grid_height": current_grid_height
    }
    try:
        with open("sim_state.pkl", "wb") as f:
            pickle.dump(data, f)
        print("Simulation state saved successfully.")
    except Exception as e:
        print("Error saving simulation state:", e)

def load_simulation_state():
    global state_sequence, current_state, transitions, simulation_speed, zoom_level, simulation_mode, transition_log, current_grid_width, current_grid_height, states
    try:
        with open("sim_state.pkl", "rb") as f:
            data = pickle.load(f)
        state_sequence = data.get("state_sequence", [states[0]])
        current_state = data.get("current_state", states[0])
        transitions = data.get("transitions", transitions)
        simulation_speed = data.get("simulation_speed", simulation_speed)
        zoom_level = data.get("zoom_level", zoom_level)
        simulation_mode = data.get("simulation_mode", simulation_mode)
        transition_log = data.get("transition_log", [f"0: {states[0]}"])
        current_grid_width = data.get("grid_width", current_grid_width)
        current_grid_height = data.get("grid_height", current_grid_height)
        states = generate_states(current_grid_width * current_grid_height)
        print("Simulation state loaded successfully.")
    except Exception as e:
        print("Error loading simulation state:", e)

# ================================
# Drawing Functions
# ================================
def draw_plot(surface, sequence):
    # Determine the portion of the sequence to display.
    if view_mode == "replay" and replay_index is not None:
        display_sequence = sequence[:replay_index+1]
    else:
        display_sequence = sequence[-max_plot_steps:]
    plot_area_width = plot_width - margin_left - margin_right
    plot_area_height = window_height - margin_top - margin_bottom
    state_to_num = {s: i for i, s in enumerate(states)}
    y_max = max(state_to_num.values())
    y_range = y_max - min(state_to_num.values()) or 1
    x_scale = (plot_area_width / (max_plot_steps - 1)) * zoom_level
    y_scale = plot_area_height / y_range

    # Create points for the state line.
    points = []
    for i, s in enumerate(display_sequence):
        x = margin_left + i * x_scale
        y = margin_top + (y_max - state_to_num[s]) * y_scale
        points.append((x, y))
    # Draw the background.
    pygame.draw.rect(surface, plot_bg_color, (0, 0, plot_width, window_height))
    # Draw the state line.
    if len(points) > 1:
        step_points = []
        for i in range(len(points)-1):
            cp = points[i]
            np = points[i+1]
            step_points.append((cp[0], cp[1]))
            step_points.append((np[0], cp[1]))
        step_points.append(points[-1])
        pygame.draw.lines(surface, line_color, False, step_points, 2)

    # If in forecast view, compute and draw the forecast trace.
    if view_mode == "forecast":
        total_steps = len(sequence)
        visible_count = len(display_sequence)
        offset = total_steps - visible_count
        # We can only compute forecast when enough history is available.
        if total_steps >= FORECAST_WINDOW:
            forecast_trace_points = []
            start_index = max(offset, FORECAST_WINDOW - 1)
            for i in range(start_index, total_steps):
                # Compute moving average forecast over last FORECAST_WINDOW states.
                window_states = sequence[i-FORECAST_WINDOW+1:i+1]
                vals = [state_to_num[s] for s in window_states]
                avg_val = sum(vals) / len(vals)
                # Map the index to the display coordinate.
                x = margin_left + (i - offset) * x_scale
                y = margin_top + (y_max - avg_val) * y_scale
                forecast_trace_points.append((x, y))
            if len(forecast_trace_points) > 1:
                pygame.draw.lines(surface, (200, 200, 200), False, forecast_trace_points, 2)

def draw_histogram(surface, sequence):
    counts = {s: 0 for s in states}
    for s in sequence:
        counts[s] += 1
    bar_area_left = margin_left
    bar_area_right = plot_width - margin_right
    bar_area_top = margin_top
    bar_area_bottom = window_height - margin_bottom
    bar_area_width = bar_area_right - bar_area_left
    bar_area_height = bar_area_bottom - bar_area_top
    pygame.draw.rect(surface, plot_bg_color, (0, 0, plot_width, window_height))
    num = len(states)
    bar_width = bar_area_width / (num * 2)
    max_count = max(counts.values()) if counts.values() else 1
    for i, s in enumerate(states):
        count = counts[s]
        bar_height = (count / max_count) * bar_area_height if max_count > 0 else 0
        x = bar_area_left + (2*i+1)*bar_width
        y = bar_area_bottom - bar_height
        pygame.draw.rect(surface, line_color, (x, y, bar_width, bar_height))
        label = font.render(f"{s} ({count})", True, text_color)
        surface.blit(label, (x, bar_area_bottom + 5))

def draw_analysis(surface):
    pygame.draw.rect(surface, plot_bg_color, (0, 0, plot_width, window_height))
    stats = compute_dwell_stats()
    y_offset = margin_top
    header = font.render("Dwell Time Analysis", True, text_color)
    surface.blit(header, (margin_left, y_offset))
    y_offset += font.get_linesize() + 5
    for s in states:
        avg, mx = stats[s]
        avg_time = avg * simulation_speed / 1000.0
        mx_time = mx * simulation_speed / 1000.0
        line = f"{s}: Avg {avg:.1f} ({avg_time:.2f}s), Max {mx} ({mx_time:.2f}s)"
        label = font.render(line, True, text_color)
        surface.blit(label, (margin_left, y_offset))
        y_offset += font.get_linesize() + 3

def draw_transition_editor(surface):
    grid_width = (window_width - 2*margin_left) * camera_zoom
    grid_height = (window_height - 2*margin_top - BIN_HEIGHT - 2*BIN_GAP) * camera_zoom
    cell_width = grid_width / current_grid_width
    cell_height = grid_height / current_grid_height
    editor_area = pygame.Rect(margin_left, margin_top, grid_width, grid_height)
    pygame.draw.rect(surface, plot_bg_color, (0, 0, window_width, window_height))
    current_time_ms = pygame.time.get_ticks()
    mouse_x, mouse_y = pygame.mouse.get_pos()
    for r in range(current_grid_height):
        for c in range(current_grid_width):
            idx = r * current_grid_width + c
            base_x = editor_area.x + c * cell_width + camera_offset[0]
            base_y = editor_area.y + r * cell_height + camera_offset[1]
            float_offset = get_cell_offset(r, c, current_time_ms)
            total_x = base_x + float_offset[0]
            total_y = base_y + float_offset[1]
            cell_center = (total_x + cell_width/2, total_y + cell_height/2)
            dist = math.hypot(mouse_x - cell_center[0], mouse_y - cell_center[1])
            base_font_size = 16
            max_font_size = 40
            max_effect_distance = 100
            factor = max(0, (max_effect_distance - dist) / max_effect_distance)
            font_size = base_font_size + factor * (max_font_size - base_font_size)
            cell_font = pygame.font.SysFont("Arial", int(font_size * camera_zoom))
            disp_val = int(round(transitions[states[idx]][states[idx]]))
            text_surface = cell_font.render(str(disp_val), True, text_color)
            surface.blit(text_surface, (total_x + 5, total_y + cell_height/2 - cell_font.get_linesize()/2))
    if selected_cells:
        xs, ys = [], []
        for (r, c) in selected_cells:
            x = editor_area.x + c * cell_width + camera_offset[0]
            y = editor_area.y + r * cell_height + camera_offset[1]
            xs.extend([x, x+cell_width])
            ys.extend([y, y+cell_height])
        if xs and ys:
            bbox = pygame.Rect(min(xs), min(ys), max(xs)-min(xs), max(ys)-min(ys))
            pygame.draw.rect(surface, (255,255,255), bbox, 2)
    if is_selecting and selection_rect is not None:
        sel_rect = selection_rect.copy()
        sel_rect.normalize()
        overlay = pygame.Surface((sel_rect.width, sel_rect.height), pygame.SRCALPHA)
        overlay.fill((255,255,255,100))
        surface.blit(overlay, (sel_rect.x, sel_rect.y))
    bin_area_y = window_height - BIN_HEIGHT - BIN_GAP
    total_bin_width = (window_width - 2*margin_left - 3*BIN_GAP)
    bin_w = total_bin_width / 4
    start_x = margin_left
    for i in range(4):
        bin_rect = pygame.Rect(start_x + BIN_GAP + i*(bin_w+BIN_GAP), bin_area_y, bin_w, BIN_HEIGHT)
        pygame.draw.rect(surface, (255,255,255), bin_rect)
        pygame.draw.rect(surface, (0,0,0), bin_rect, 2)
        label = font.render(bin_labels[i], True, (0,0,0))
        surface.blit(label, (bin_rect.x+5, bin_rect.y+5))
        bins[i] = bin_rect
    global animation_active, animation_start_time, animation_duration, animation_initial_bbox, animation_target_bin
    if animation_active:
        t = (pygame.time.get_ticks() - animation_start_time) / animation_duration
        if t > 1:
            t = 1
            for (r, c) in selected_cells:
                idx = r * current_grid_width + c
                if animation_target_bin == 0:
                    new_val = random.uniform(1,9)
                elif animation_target_bin == 1:
                    new_val = random.uniform(3,8)
                elif animation_target_bin == 2:
                    new_val = random.uniform(2,6)
                elif animation_target_bin == 3:
                    new_val = random.choice([random.uniform(1,3), random.uniform(7,9)])
                transitions[states[idx]][states[idx]] = new_val
            animation_active = False
            selected_cells.clear()
        else:
            start_rect = animation_initial_bbox
            target_bin_rect = bins[animation_target_bin]
            target_rect = pygame.Rect(
                target_bin_rect.centerx - 10,
                target_bin_rect.centery - 10,
                20,
                20
            )
            interp_rect = start_rect.copy()
            interp_rect.x = start_rect.x + (target_rect.x - start_rect.x) * t
            interp_rect.y = start_rect.y + (target_rect.y - start_rect.y) * t
            interp_rect.width = start_rect.width + (target_rect.width - start_rect.width) * t
            interp_rect.height = start_rect.height + (target_rect.height - start_rect.height) * t
            pygame.draw.rect(surface, (255,255,255), interp_rect, 2)
    return

def draw_settings_view(surface):
    global current_grid_width, current_grid_height, states, transitions
    pygame.draw.rect(surface, plot_bg_color, (0, 0, plot_width, window_height))
    y_offset = margin_top
    header = font.render("Settings", True, text_color)
    surface.blit(header, (margin_left, y_offset))
    y_offset += font.get_linesize() + 10
    settings = [
        ("Speed (ms/step)", simulation_speed),
        ("Zoom Level", zoom_level),
        ("Grid Width", current_grid_width),
        ("Grid Height", current_grid_height)
    ]
    button_width = 30
    button_height = 25
    buttons = []
    for i, (label_text, value) in enumerate(settings):
        line = f"{label_text}: {value}"
        label = font.render(line, True, text_color)
        surface.blit(label, (margin_left, y_offset))
        plus_rect = pygame.Rect(plot_width - margin_right - button_width*2 - 10, y_offset, button_width, button_height)
        minus_rect = pygame.Rect(plot_width - margin_right - button_width, y_offset, button_width, button_height)
        pygame.draw.rect(surface, button_color, plus_rect)
        pygame.draw.rect(surface, button_color, minus_rect)
        plus_label = font.render("+", True, text_color)
        minus_label = font.render("-", True, text_color)
        surface.blit(plus_label, (plus_rect.x+8, plus_rect.y+3))
        surface.blit(minus_label, (minus_rect.x+8, minus_rect.y+3))
        param_key = label_text.lower().replace(" ", "_")
        buttons.append({"rect": plus_rect, "param": param_key, "action": "plus"})
        buttons.append({"rect": minus_rect, "param": param_key, "action": "minus"})
        y_offset += font.get_linesize() + 20
    help_text = font.render("Click buttons to adjust parameters. Grid change resets T view.", True, text_color)
    surface.blit(help_text, (margin_left, y_offset))
    return buttons

def draw_replay_view(surface, sequence):
    draw_plot(surface, sequence)
    if replay_index is not None:
        pointer_x = margin_left + (replay_index if replay_index < max_plot_steps else max_plot_steps-1) * ((plot_width - margin_left - margin_right)/(max_plot_steps-1)) * zoom_level
        pygame.draw.line(surface, (255,255,255), (pointer_x, margin_top), (pointer_x, window_height - margin_bottom), 2)

def draw_log(surface):
    log_area_width = plot_width - margin_left - margin_right
    log_area_height = 100
    log_x = margin_left
    log_y = window_height - log_area_height - 10
    pygame.draw.rect(surface, (0,0,0), (log_x, log_y, log_area_width, log_area_height))
    recent = transition_log[-log_display_count:]
    for i, entry in enumerate(recent):
        label = font.render(entry, True, text_color)
        surface.blit(label, (log_x+5, log_y+5 + i*font.get_linesize()))

def draw_scroll_buttons(surface):
    button_width = control_panel_width - 20
    button_height = 30
    up_rect = pygame.Rect(plot_width+10, window_height - (button_height*2+10), button_width, button_height)
    down_rect = pygame.Rect(plot_width+10, window_height - (button_height+5), button_width, button_height)
    mouse_pos = pygame.mouse.get_pos()
    up_color = button_hover_color if up_rect.collidepoint(mouse_pos) else button_color
    down_color = button_hover_color if down_rect.collidepoint(mouse_pos) else button_color
    pygame.draw.rect(surface, up_color, up_rect)
    pygame.draw.rect(surface, down_color, down_rect)
    label_up = font.render("Scroll Up", True, text_color)
    label_down = font.render("Scroll Down", True, text_color)
    surface.blit(label_up, (up_rect.x+10, up_rect.y+5))
    surface.blit(label_down, (down_rect.x+10, down_rect.y+5))
    return up_rect, down_rect

def get_control_panel_info_lines():
    info_lines = [
        "Controls:",
        "Space: Pause/Resume",
        "R: Reset Sim",
        "Up/Down: Speed +/-",
        "1-4: Manual Override",
        "Z: Zoom In, X: Zoom Out",
        "D: Toggle Debug",
        "E: Export Log",
        "P: Plot View",
        "H: Histogram View",
        "A: Analysis View",
        "T: Transition Editor",
        "Y: Replay View",
        "F: Forecast View",
        "F2: Settings View",
        "M: Multi-Sim Mode",
        "S: Toggle Auto/Step",
        "N: Next Step (Step Mode)",
        "C: Clear Log",
        "O: Save Sim, L: Load Sim",
        "F1: Toggle Help",
        "",
        f"Paused: {paused}",
        f"Sim Mode: {simulation_mode}",
        f"View Mode: {view_mode}",
        f"Speed: {simulation_speed} ms/step",
        f"Current: {current_state}",
        f"Zoom: {zoom_level:.2f}",
        f"Time: {get_simulation_time():.1f}s"
    ]
    if view_mode=="replay" and replay_index is not None:
        info_lines.append(f"Replay: Step {replay_index} of {len(state_sequence)-1}")
    if multi_sim_mode:
        info_lines.append("Multi-Sim: ON")
    info_lines.append("")
    info_lines.append("State Counts:")
    counts = get_state_counts(state_sequence)
    for s in states:
        info_lines.append(f" {s}: {counts[s]}")
    if debug_mode:
        info_lines.append("")
        info_lines.append("Debug Info:")
        info_lines.append(f"Total Transitions: {len(state_sequence)-1}")
        info_lines.append(f"History Length: {len(state_sequence)}")
    return info_lines

def draw_control_panel(surface):
    global control_scroll_offset
    pygame.draw.rect(surface, control_bg_color, (plot_width, 0, control_panel_width, window_height))
    lines = get_control_panel_info_lines()
    line_height = font.get_linesize()
    top_margin = 20
    max_visible = (window_height - top_margin - 70) // line_height
    if len(lines) > max_visible:
        control_scroll_offset = min(control_scroll_offset, len(lines)-max_visible)
    else:
        control_scroll_offset = 0
    visible = lines[control_scroll_offset:control_scroll_offset+max_visible]
    y_offset = top_margin
    for line in visible:
        label = font.render(line, True, text_color)
        surface.blit(label, (plot_width+10, y_offset))
        y_offset += line_height
    return draw_scroll_buttons(surface)

def draw_help_overlay(surface):
    overlay = pygame.Surface((window_width, window_height))
    overlay.set_alpha(200)
    overlay.fill((0,0,0))
    surface.blit(overlay, (0,0))
    help_lines = [
        "Help - Key Bindings:",
        "Space: Pause/Resume",
        "R: Reset Simulation",
        "Up/Down: Speed +/-",
        "1-4: Manual Override",
        "Z/X: Zoom In/Out",
        "D: Toggle Debug",
        "E: Export Log",
        "P: Plot View",
        "H: Histogram View",
        "A: Analysis View",
        "T: Transition Editor",
        "Y: Replay View (Left/Right Arrows)",
        "F: Forecast View",
        "F2: Settings View",
        "M: Multi-Sim Mode",
        "S: Toggle Auto/Step",
        "N: Next Step (Step Mode)",
        "C: Clear Log",
        "O: Save Sim, L: Load Sim",
        "F1: Toggle Help"
    ]
    y = 50
    for line in help_lines:
        label = font.render(line, True, text_color)
        surface.blit(label, (50, y))
        y += font.get_linesize()+5

def update_window_title():
    title = f"State: {current_state} | Speed: {simulation_speed} ms | Time: {get_simulation_time():.1f}s"
    pygame.display.set_caption(title)

# ================================
# MAIN LOOP
# ================================
running = True
clock = pygame.time.Clock()
while running:
    current_time = pygame.time.get_ticks()
    camera_zoom += (target_camera_zoom - camera_zoom) * 0.1
    if current_time - last_auto_log_save >= AUTO_LOG_INTERVAL:
        auto_save_log()
        last_auto_log_save = current_time

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

        if view_mode == "editor":
            if event.type == pygame.FINGERDOWN:
                pos = (event.x * window_width, event.y * window_height)
                touches[event.finger_id] = pos
                prev_touches[event.finger_id] = pos
            elif event.type == pygame.FINGERMOTION:
                new_pos = (event.x * window_width, event.y * window_height)
                old_pos = touches.get(event.finger_id, new_pos)
                touches[event.finger_id] = new_pos
                if len(touches) == 2:
                    ids = list(touches.keys())
                    v1 = (touches[ids[0]][0] - prev_touches[ids[0]][0], touches[ids[0]][1] - prev_touches[ids[0]][1])
                    v2 = (touches[ids[1]][0] - prev_touches[ids[1]][0], touches[ids[1]][1] - prev_touches[ids[1]][1])
                    prev_center = ((prev_touches[ids[0]][0] + prev_touches[ids[1]][0]) / 2,
                                   (prev_touches[ids[0]][1] + prev_touches[ids[1]][1]) / 2)
                    new_center = ((touches[ids[0]][0] + touches[ids[1]][0]) / 2,
                                  (touches[ids[0]][1] + touches[ids[1]][1]) / 2)
                    v1_center = (prev_touches[ids[0]][0] - prev_center[0], prev_touches[ids[0]][1] - prev_center[1])
                    v2_center = (prev_touches[ids[1]][0] - prev_center[0], prev_touches[ids[1]][1] - prev_center[1])
                    dot1 = v1[0]*v1_center[0] + v1[1]*v1_center[1]
                    dot2 = v2[0]*v2_center[0] + v2[1]*v2_center[1]
                    if (dot1 > 0 and dot2 > 0) or (dot1 < 0 and dot2 < 0):
                        delta = (new_center[0] - prev_center[0], new_center[1] - prev_center[1])
                        camera_offset[0] += delta[0]
                        camera_offset[1] += delta[1]
                    else:
                        old_distance = math.dist(prev_touches[ids[0]], prev_touches[ids[1]])
                        new_distance = math.dist(touches[ids[0]], touches[ids[1]])
                        if old_distance != 0:
                            zoom_ratio = new_distance / old_distance
                            target_camera_zoom *= zoom_ratio
                prev_touches[event.finger_id] = new_pos
            elif event.type == pygame.FINGERUP:
                if event.finger_id in touches:
                    del touches[event.finger_id]
                if event.finger_id in prev_touches:
                    del prev_touches[event.finger_id]

        if event.type == pygame.MOUSEWHEEL and view_mode=="editor":
            zoom_factor = 1.1 if event.y > 0 else 0.9
            target_camera_zoom *= zoom_factor
            mouse_pos = pygame.mouse.get_pos()
            camera_offset[0] = (camera_offset[0] - mouse_pos[0]) * zoom_factor + mouse_pos[0]
            camera_offset[1] = (camera_offset[1] - mouse_pos[1]) * zoom_factor + mouse_pos[1]

        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE:
                pygame.display.set_mode((900,600))
            elif event.key == pygame.K_SPACE:
                paused = not paused
            elif event.key == pygame.K_r:
                reset_simulation()
            elif event.key == pygame.K_UP:
                simulation_speed = max(50, simulation_speed - 50)
            elif event.key == pygame.K_DOWN:
                simulation_speed += 50
            elif event.key == pygame.K_d:
                debug_mode = not debug_mode
            elif event.key == pygame.K_z:
                zoom_level = min(3.0, zoom_level + 0.1)
            elif event.key == pygame.K_x:
                zoom_level = max(0.5, zoom_level - 0.1)
            elif event.key == pygame.K_1:
                manual_override(states[0])
            elif event.key == pygame.K_2:
                manual_override(states[1])
            elif event.key == pygame.K_3:
                manual_override(states[2])
            elif event.key == pygame.K_4:
                manual_override(states[3])
            elif event.key == pygame.K_e:
                export_log()
            elif event.key == pygame.K_p:
                view_mode = "plot"
            elif event.key == pygame.K_h:
                view_mode = "histogram"
            elif event.key == pygame.K_a:
                view_mode = "analysis"
            elif event.key == pygame.K_t:
                view_mode = "editor"
                selected_cells = set()
            elif event.key == pygame.K_y:
                view_mode = "replay"
                replay_index = len(state_sequence)-1
            elif event.key == pygame.K_f:
                view_mode = "forecast"
            elif event.key == pygame.K_F2:
                view_mode = "settings"
            elif event.key == pygame.K_m:
                multi_sim_mode = not multi_sim_mode
            elif event.key == pygame.K_s:
                simulation_mode = "step" if simulation_mode=="auto" else "auto"
            elif event.key == pygame.K_n:
                if simulation_mode=="step" and not paused:
                    update_simulation()
                    last_update_time = current_time
            elif event.key == pygame.K_c:
                clear_log()
            elif event.key == pygame.K_o:
                save_simulation_state()
            elif event.key == pygame.K_l:
                load_simulation_state()
            elif event.key == pygame.K_F1:
                help_overlay_enabled = not help_overlay_enabled
            elif view_mode=="replay":
                if event.key == pygame.K_LEFT:
                    replay_index = max(0, replay_index-1)
                elif event.key == pygame.K_RIGHT:
                    replay_index = min(len(state_sequence)-1, replay_index+1)
        if view_mode != "editor":
            if event.type == pygame.MOUSEBUTTONDOWN:
                mouse_pos = event.pos
                if mouse_pos[0] >= plot_width:
                    button_width = control_panel_width - 20
                    button_height = 30
                    up_rect = pygame.Rect(plot_width+10, window_height - (button_height*2+10), button_width, button_height)
                    down_rect = pygame.Rect(plot_width+10, window_height - (button_height+5), button_width, button_height)
                    info_lines = get_control_panel_info_lines()
                    line_height = font.get_linesize()
                    top_margin = 20
                    max_visible = (window_height - top_margin - 70) // line_height
                    max_scroll = max(0, len(info_lines)-max_visible)
                    if up_rect.collidepoint(mouse_pos):
                        control_scroll_offset = max(0, control_scroll_offset-1)
                    elif down_rect.collidepoint(mouse_pos):
                        control_scroll_offset = min(max_scroll, control_scroll_offset+1)
        if view_mode=="editor":
            editor_area = pygame.Rect(margin_left, margin_top, window_width - 2*margin_left, window_height - BIN_HEIGHT - 2*BIN_GAP - margin_top)
            if event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 2:
                    pan_active = True
                    pan_start = event.pos
                    pan_start_offset = camera_offset.copy()
                if event.button == 3 and editor_area.collidepoint(event.pos):
                    is_selecting = True
                    selection_start = event.pos
                    selection_rect = pygame.Rect(event.pos[0], event.pos[1], 0, 0)
                    selected_cells = set()
            if event.type == pygame.MOUSEMOTION:
                if pan_active:
                    dx = event.pos[0] - pan_start[0]
                    dy = event.pos[1] - pan_start[1]
                    camera_offset[0] = pan_start_offset[0] + dx
                    camera_offset[1] = pan_start_offset[1] + dy
                if is_selecting and selection_start is not None:
                    x0, y0 = selection_start
                    x1, y1 = event.pos
                    selection_rect = pygame.Rect(min(x0,x1), min(y0,y1), abs(x1-x0), abs(y1-y0))
                    cell_width = (window_width - 2*margin_left) / current_grid_width * camera_zoom
                    cell_height = (window_height - BIN_HEIGHT - 2*BIN_GAP - margin_top) / current_grid_height * camera_zoom
                    selected_cells = set()
                    for r in range(current_grid_height):
                        for c in range(current_grid_width):
                            cell_x = margin_left + c*cell_width + camera_offset[0]
                            cell_y = margin_top + r*cell_height + camera_offset[1]
                            cell_rect = pygame.Rect(cell_x, cell_y, cell_width, cell_height)
                            if selection_rect.colliderect(cell_rect):
                                selected_cells.add((r, c))
            if event.type == pygame.MOUSEBUTTONUP:
                if pan_active and event.button == 2:
                    pan_active = False
                if is_selecting and event.button == 3:
                    is_selecting = False
            if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                for i, bin_rect in enumerate(bins):
                    if bin_rect.collidepoint(event.pos) and selected_cells:
                        animation_active = True
                        animation_start_time = pygame.time.get_ticks()
                        animation_duration = 1000
                        animation_target_bin = i
                        cell_width = (window_width - 2*margin_left) / current_grid_width * camera_zoom
                        cell_height = (window_height - BIN_HEIGHT - 2*BIN_GAP - margin_top) / current_grid_height * camera_zoom
                        xs, ys = [], []
                        for (r, c) in selected_cells:
                            cell_x = margin_left + c*cell_width + camera_offset[0]
                            cell_y = margin_top + r*cell_height + camera_offset[1]
                            xs.extend([cell_x, cell_x+cell_width])
                            ys.extend([cell_y, cell_y+cell_height])
                        if xs and ys:
                            animation_initial_bbox = pygame.Rect(min(xs), min(ys), max(xs)-min(xs), max(ys)-min(ys))
    if simulation_mode=="auto" and not paused and view_mode!="replay":
        if current_time - last_update_time >= simulation_speed:
            update_simulation()
            last_update_time = current_time
            if multi_sim_mode:
                update_multi_simulations()

    update_window_title()
    screen.fill(background_color)
    if view_mode=="plot":
        draw_plot(screen, state_sequence)
        if multi_sim_mode:
            def draw_multi(sim_history, color):
                disp = sim_history[-max_plot_steps:]
                plot_area_width = plot_width - margin_left - margin_right
                plot_area_height = window_height - margin_top - margin_bottom
                state_to_num = {s: i for i, s in enumerate(states)}
                y_max = max(state_to_num.values())
                x_scale = (plot_area_width / (max_plot_steps - 1)) * zoom_level
                y_scale = plot_area_height / (y_max - min(state_to_num.values()) or 1)
                pts = []
                for i, s in enumerate(disp):
                    x = margin_left + i*x_scale
                    y = margin_top + (y_max - state_to_num[s])*y_scale
                    pts.append((x,y))
                if len(pts)>1:
                    sp = []
                    for i in range(len(pts)-1):
                        sp.append((pts[i][0], pts[i][1]))
                        sp.append((pts[i+1][0], pts[i][1]))
                    sp.append(pts[-1])
                    pygame.draw.lines(screen, color, False, sp, 2)
            draw_multi(sim2_history, multi_color_1)
            draw_multi(sim3_history, multi_color_2)
    elif view_mode=="histogram":
        draw_histogram(screen, state_sequence)
    elif view_mode=="analysis":
        draw_analysis(screen)
    elif view_mode=="editor":
        draw_transition_editor(screen)
    elif view_mode=="replay":
        draw_replay_view(screen, state_sequence)
    elif view_mode=="forecast":
        draw_plot(screen, state_sequence)
    elif view_mode=="settings":
        draw_settings_view(screen)
    if view_mode != "editor":
        draw_log(screen)
        draw_control_panel(screen)
    else:
        label = font.render("Transition Editor", True, text_color)
        screen.blit(label, (window_width - control_panel_width - 150, 10))
    if animation_active:
        t = (pygame.time.get_ticks() - animation_start_time) / animation_duration
        if t > 1:
            t = 1
            for (r, c) in selected_cells:
                idx = r * current_grid_width + c
                if animation_target_bin == 0:
                    new_val = random.uniform(1,9)
                elif animation_target_bin == 1:
                    new_val = random.uniform(3,8)
                elif animation_target_bin == 2:
                    new_val = random.uniform(2,6)
                elif animation_target_bin == 3:
                    new_val = random.choice([random.uniform(1,3), random.uniform(7,9)])
                transitions[states[idx]][states[idx]] = new_val
            animation_active = False
            selected_cells.clear()
        else:
            start_rect = animation_initial_bbox
            target_bin_rect = bins[animation_target_bin]
            target_rect = pygame.Rect(
                target_bin_rect.centerx - 10,
                target_bin_rect.centery - 10,
                20,
                20
            )
            interp_rect = start_rect.copy()
            interp_rect.x = start_rect.x + (target_rect.x - start_rect.x) * t
            interp_rect.y = start_rect.y + (target_rect.y - start_rect.y) * t
            interp_rect.width = start_rect.width + (target_rect.width - start_rect.width) * t
            interp_rect.height = start_rect.height + (target_rect.height - start_rect.height) * t
            pygame.draw.rect(screen, (255,255,255), interp_rect, 2)
    if flash_timer > 0:
        flash_surf = pygame.Surface((plot_width, window_height))
        flash_surf.set_alpha(100)
        flash_surf.fill(error_flash_color)
        screen.blit(flash_surf, (0,0))
        flash_timer -= clock.get_time()
    if help_overlay_enabled:
        draw_help_overlay(screen)
    pygame.display.flip()
    clock.tick(60)
pygame.quit()
