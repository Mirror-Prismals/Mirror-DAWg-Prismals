import tkinter as tk
from PIL import Image, ImageTk

# Conversion matrices for colorblindness simulation
protanopia_matrix = [
    [0.567, 0.433, 0],
    [0.558, 0.442, 0],
    [0,     0.242, 0.758]
]

deuteranopia_matrix = [
    [0.625, 0.375, 0],
    [0.7,   0.3,   0],
    [0,     0.3,   0.7]
]

tritanopia_matrix = [
    [0.95,  0.05,  0],
    [0,     0.433, 0.567],
    [0,     0.475, 0.525]
]

def simulate_colorblindness(hex_color, matrix):
    # Remove '#' character if present
    hex_color = hex_color.lstrip('#')
    
    # Convert hexadecimal color to RGB
    rgb_color = tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

    # Apply transformation matrix to simulate colorblindness
    sim_color = [
        sum([rgb_color[j] * matrix[i][j] for j in range(3)]) for i in range(3)
    ]

    # Clip values to [0, 255]
    sim_color = [min(max(0, int(val)), 255) for val in sim_color]

    # Convert back to hexadecimal
    sim_hex_color = '#{:02x}{:02x}{:02x}'.format(*sim_color)
    return sim_hex_color

def update_colors():
    original_hex = original_color_entry.get()
    protan_hex = simulate_colorblindness(original_hex, protanopia_matrix)
    deuteran_hex = simulate_colorblindness(original_hex, deuteranopia_matrix)
    tritan_hex = simulate_colorblindness(original_hex, tritanopia_matrix)

    original_color_label.config(bg=original_hex, text=f"Original: {original_hex}")
    protan_color_label.config(bg=protan_hex, text=f"Protanopia: {protan_hex}")
    deuteran_color_label.config(bg=deuteran_hex, text=f"Deuteranopia: {deuteran_hex}")
    tritan_color_label.config(bg=tritan_hex, text=f"Tritanopia: {tritan_hex}")

# GUI setup
root = tk.Tk()
root.title("Colorblind Simulation")

# Original color entry
original_color_label = tk.Label(root, text="Original Color", padx=20, pady=10)
original_color_label.grid(row=0, column=0)
original_color_entry = tk.Entry(root)
original_color_entry.grid(row=0, column=1)
original_color_entry.insert(0, "#FF0000")  # Default to red

# Colorblind simulation labels
protan_color_label = tk.Label(root, text="Protanopia Simulation", padx=20, pady=10)
protan_color_label.grid(row=1, column=0)
deuteran_color_label = tk.Label(root, text="Deuteranopia Simulation", padx=20, pady=10)
deuteran_color_label.grid(row=2, column=0)
tritan_color_label = tk.Label(root, text="Tritanopia Simulation", padx=20, pady=10)
tritan_color_label.grid(row=3, column=0)

# Update button
update_button = tk.Button(root, text="Update", command=update_colors)
update_button.grid(row=0, column=2, rowspan=4)

update_colors()  # Update colors initially

root.mainloop()
