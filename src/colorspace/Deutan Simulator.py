import tkinter as tk

def hex_to_rgb(hex_color):
    """Convert hexadecimal color code to RGB."""
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

def rgb_to_hex(rgb_color):
    """Convert RGB color to hexadecimal."""
    return '#%02x%02x%02x' % rgb_color

def adjust_color_for_clickability(rgb_color):
    """Adjust color to ensure it's clickable for simulation."""
    # Check if color is predominantly red or blue
    if rgb_color[0] > 200 and rgb_color[1] < 50 and rgb_color[2] < 50:
        # Swap red and blue components
        return (rgb_color[2], rgb_color[1], rgb_color[0])
    elif rgb_color[0] < 50 and rgb_color[1] < 50 and rgb_color[2] > 200:
        # Swap red and blue components
        return (rgb_color[2], rgb_color[1], rgb_color[0])
    else:
        return rgb_color

def duetan_simulation(hex_color):
    """Simulate Duetanopia by shifting red and green components."""
    # Convert hexadecimal color to RGB
    rgb_color = hex_to_rgb(hex_color)

    # Adjust color for clickability
    rgb_color = adjust_color_for_clickability(rgb_color)

    # Calculate the shift ratio
    ratio = [0, 0]
    if rgb_color[1] != 0:
        ratio[0] = rgb_color[1] / 255.0
    if rgb_color[0] != 255:
        ratio[1] = (255 - rgb_color[0]) / 255.0

    # Adjust red and green components
    new_color1 = (rgb_color[0], int(128 * ratio[0]), int(128 * ratio[1]))
    new_color2 = (int(128 + (127 * ratio[1])), rgb_color[1], int(128 - (128 * ratio[0])))

    # Convert RGB colors back to hexadecimal
    hex_color1 = rgb_to_hex(new_color1)
    hex_color2 = rgb_to_hex(new_color2)

    return hex_color1, hex_color2

def simulate():
    """Callback function to simulate Duetanopia and display results."""
    input_color = entry.get()
    try:
        hex_color1, hex_color2 = duetan_simulation(input_color)
        label_color1.config(bg=hex_color1)
        label_color2.config(bg=hex_color2)
        label_hex1.config(text=hex_color1)
        label_hex2.config(text=hex_color2)
    except ValueError:
        label_color1.config(bg="white")
        label_color2.config(bg="white")
        label_hex1.config(text=input_color)
        label_hex2.config(text=input_color)

def simulate_clicked_color(hex_color):
    """Callback function to simulate Duetanopia based on clicked color."""
    entry.delete(0, tk.END)
    entry.insert(0, hex_color)
    simulate()

def invert_color(color_index):
    """Invert the hexadecimal color code of Color 1 or Color 2 and populate the entry field."""
    hex_color = label_hex1.cget("text") if color_index == 1 else label_hex2.cget("text")
    rgb_color = hex_to_rgb(hex_color)
    inverted_rgb_color = tuple(255 - value for value in rgb_color)
    inverted_hex_color = rgb_to_hex(inverted_rgb_color)
    entry.delete(0, tk.END)
    entry.insert(0, inverted_hex_color)

# GUI Setup
root = tk.Tk()
root.title("Duetan Simulator")

label_input = tk.Label(root, text="Input Hex Color:")
label_input.grid(row=0, column=0)

entry = tk.Entry(root)
entry.grid(row=0, column=1)

button_simulate = tk.Button(root, text="Simulate", command=simulate)
button_simulate.grid(row=0, column=2)

label_color1 = tk.Label(root, text="Color 1", width=10, height=5)
label_color1.grid(row=1, column=0)

label_color2 = tk.Label(root, text="Color 2", width=10, height=5)
label_color2.grid(row=1, column=1)

label_hex1 = tk.Label(root, text="", bg="white")
label_hex1.grid(row=2, column=0)
label_hex1.bind("<Button-1>", lambda event: simulate_clicked_color(label_hex1.cget("text")))

label_hex2 = tk.Label(root, text="", bg="white")
label_hex2.grid(row=2, column=1)
label_hex2.bind("<Button-1>", lambda event: simulate_clicked_color(label_hex2.cget("text")))

button_invert_color1 = tk.Button(root, text="Invert Color 1", command=lambda: invert_color(1))
button_invert_color1.grid(row=3, column=0)

button_invert_color2 = tk.Button(root, text="Invert Color 2", command=lambda: invert_color(2))
button_invert_color2.grid(row=3, column=1)

button_select_color1 = tk.Button(root, text="Select Color 1", command=lambda: simulate_clicked_color(label_hex1.cget("text")))
button_select_color1.grid(row=4, column=0)

button_select_color2 = tk.Button(root, text="Select Color 2", command=lambda: simulate_clicked_color(label_hex2.cget("text")))
button_select_color2.grid(row=4, column=1)

root.mainloop()
