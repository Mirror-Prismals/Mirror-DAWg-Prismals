import pygame
import numpy as np
import random
import sys

# -----------------------------
# Define our 21 states with name, RGB color, and weight tuple.
# -----------------------------
states = [
    {"name": "Red",         "color": (255, 0, 0),     "weight": (1, 0, 0)},
    {"name": "Pink",        "color": (255, 0, 128),   "weight": (2, 0, 1)},
    {"name": "Infrared",    "color": (255, 128, 128), "weight": (2, 1, 1)},
    {"name": "Orange",      "color": (255, 128, 0),   "weight": (2, 1, 0)},
    {"name": "Thalo",       "color": (0, 255, 128),   "weight": (0, 2, 1)},
    {"name": "Camo",        "color": (128, 255, 128), "weight": (1, 2, 1)},
    {"name": "Lime",        "color": (0, 255, 0),     "weight": (0, 1, 0)},
    {"name": "Cyan",        "color": (0, 255, 255),   "weight": (0, 1, 1)},
    {"name": "Violet",      "color": (128, 0, 255),   "weight": (1, 0, 2)},
    {"name": "Ultraviolet", "color": (255, 128, 255), "weight": (2, 1, 2)},
    {"name": "Blue",        "color": (0, 0, 255),     "weight": (0, 0, 1)},
    {"name": "Cerulean",    "color": (0, 128, 255),   "weight": (0, 1, 2)},
    {"name": "Indigo",      "color": (128, 128, 255), "weight": (1, 1, 2)},
    {"name": "Magenta",     "color": (255, 0, 255),   "weight": (1, 0, 1)},
    {"name": "Chartreuse",  "color": (128, 255, 0),   "weight": (1, 2, 0)},
    {"name": "Aqua",        "color": (128, 255, 255), "weight": (1, 2, 2)},
    {"name": "Yellow",      "color": (255, 255, 0),   "weight": (1, 1, 0)},
    {"name": "Mustard",     "color": (255, 255, 128), "weight": (2, 2, 1)},
    # Special states:
    {"name": "Null",        "color": (0, 0, 0),       "weight": (0, 0, 0)},   # Dead: remains unchanged
    {"name": "Nill",        "color": (255, 255, 255), "weight": (2, 2, 2)},   # White: chooses randomly among non-dead neighbors
    {"name": "N/A",         "color": (128, 128, 128), "weight": (1, 1, 1)}    # Grey: uses twist on computed vector
]

num_states = len(states)

# Build NumPy arrays for quick lookups.
weights = np.array([s["weight"] for s in states], dtype=np.int32)   # Shape: (21, 3)
colors  = np.array([s["color"]  for s in states], dtype=np.uint8)    # Shape: (21, 3)

# Special state indices.
NULL_IDX = 18
NILL_IDX = 19
NA_IDX   = 20

# Build a lookup table: maps weight tuple (a,b,c) to a state index.
lookup = -np.ones((3, 3, 3), dtype=np.int32)
for idx, s in enumerate(states):
    a, b, c = s["weight"]
    lookup[a, b, c] = idx

# -----------------------------
# Vectorized update of the grid using NumPy.
# -----------------------------
def update_grid(grid):
    rows, cols = grid.shape
    cell_weights = weights[grid]  # Shape: (rows, cols, 3)
    a_arr = cell_weights[..., 0]
    b_arr = cell_weights[..., 1]
    c_arr = cell_weights[..., 2]
    
    # Define the 3x3 neighborhood offsets.
    offsets = [(-1, -1), (-1, 0), (-1, 1),
               (0, -1),  (0, 0),  (0, 1),
               (1, -1),  (1, 0),  (1, 1)]
    
    coeffs = []
    for di, dj in offsets:
        hor = np.where(dj == -1, a_arr, np.where(dj == 0, b_arr, c_arr))
        ver = np.where(di == -1, a_arr, np.where(di == 0, b_arr, c_arr))
        coeffs.append(hor * ver)
    coeffs = np.array(coeffs)  # Shape: (9, rows, cols)
    total_filter = np.sum(coeffs, axis=0)
    
    weighted_sum = np.zeros((rows, cols, 3), dtype=np.float32)
    for coeff, (di, dj) in zip(coeffs, offsets):
        rolled = np.roll(np.roll(grid, shift=di, axis=0), shift=dj, axis=1)
        neighbor_weights = weights[rolled]  # Shape: (rows, cols, 3)
        weighted_sum += coeff[..., np.newaxis] * neighbor_weights
    
    new_components = np.where(
        total_filter[..., np.newaxis] != 0,
        np.rint(weighted_sum / total_filter[..., np.newaxis]),
        cell_weights
    ).astype(np.int32)
    
    new_state_default = lookup[new_components[..., 0], new_components[..., 1], new_components[..., 2]]
    new_grid = new_state_default.copy()
    
    # Special handling.
    null_mask = (grid == NULL_IDX)
    new_grid[null_mask] = grid[null_mask]
    
    na_mask = (grid == NA_IDX)
    if np.any(na_mask):
        twist = (new_components[na_mask] + 1) % 3
        new_grid[na_mask] = lookup[twist[:, 0], twist[:, 1], twist[:, 2]]
    
    nill_indices = np.argwhere(grid == NILL_IDX)
    for y, x in nill_indices:
        choices = []
        for di, dj in offsets:
            ny = (y + di) % rows
            nx = (x + dj) % cols
            if grid[ny, nx] != NULL_IDX:
                choices.append(grid[ny, nx])
        if choices:
            new_grid[y, x] = random.choice(choices)
        else:
            new_grid[y, x] = grid[y, x]
    
    return new_grid

# -----------------------------
# 2.5D Rendering: Use isometric projection.
# Each cell's "height" is derived from the sum of its weight tuple.
# -----------------------------
# Precompute a height value for each state.
height_map = np.array([sum(s["weight"]) for s in states], dtype=np.int32)

# Isometric tile parameters.
TILE_WIDTH = 32
TILE_HEIGHT = 16
HEIGHT_SCALE = 3  # pixels per weight unit

# Grid dimensions for the automata (keep it low-res for a 2.5D view).
GRID_COLS = 100
GRID_ROWS = 100

def main():
    pygame.init()
    screen = pygame.display.set_mode((0, 0), pygame.FULLSCREEN)
    pygame.display.set_caption("2.5D Cellular Automata")
    info = pygame.display.Info()
    screen_width, screen_height = info.current_w, info.current_h
    
    # Create a grid of states.
    grid = np.random.randint(0, num_states, size=(GRID_ROWS, GRID_COLS), dtype=np.int32)
    
    # Compute isometric projection offsets to center the grid.
    iso_width = (GRID_COLS + GRID_ROWS) * (TILE_WIDTH // 2)
    iso_height = (GRID_COLS + GRID_ROWS) * (TILE_HEIGHT // 2)
    offset_x = (screen_width - iso_width) // 2
    offset_y = (screen_height - iso_height) // 2
    
    clock = pygame.time.Clock()
    running = True
    
    while running:
        # Event handling.
        for event in pygame.event.get():
            if event.type == pygame.QUIT or (event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE):
                running = False
        
        # Update the cellular automata grid.
        grid = update_grid(grid)
        
        # Clear screen.
        screen.fill((30, 30, 30))
        
        # Loop over each cell to render the isometric tile.
        for row in range(GRID_ROWS):
            for col in range(GRID_COLS):
                state_idx = grid[row, col]
                color = states[state_idx]["color"]
                
                # Compute base isometric coordinates.
                iso_x = (col - row) * (TILE_WIDTH // 2) + offset_x
                iso_y = (col + row) * (TILE_HEIGHT // 2) + offset_y
                
                # Get height offset from the state's weight sum.
                h_val = height_map[state_idx]
                h_offset = HEIGHT_SCALE * h_val
                
                # Define vertices for the top face (diamond shape).
                top    = (iso_x, iso_y - h_offset)
                right  = (iso_x + TILE_WIDTH // 2, iso_y + TILE_HEIGHT // 2 - h_offset)
                bottom = (iso_x, iso_y + TILE_HEIGHT - h_offset)
                left   = (iso_x - TILE_WIDTH // 2, iso_y + TILE_HEIGHT // 2 - h_offset)
                vertices = [top, right, bottom, left]
                
                # Draw the top face.
                pygame.draw.polygon(screen, color, vertices)
                # Optional: draw a border for clarity.
                pygame.draw.polygon(screen, (0, 0, 0), vertices, 1)
        
        pygame.display.flip()
        clock.tick(15)  # Adjust FPS as needed (isometric drawing is heavier)
    
    pygame.quit()
    sys.exit()

if __name__ == "__main__":
    main()
