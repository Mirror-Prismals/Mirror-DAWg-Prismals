import tkinter as tk
from tkinter import messagebox
from random import randint


# Function to generate a random HEX color
def random_hex(length):
    return "#" + "".join([f"{randint(0, 255):02X}" for _ in range(length // 2)])


# Normalize HEX input by ensuring it starts with a single '#'
def normalize_hex_input(hex_code):
    return f"#{hex_code.lstrip('#')}"


# Function to convert 8-digit HEX (transparent) to 6-digit HEX (opaque)
def hex_to_opaque(hex_with_alpha, bg_color):
    try:
        hex_with_alpha = normalize_hex_input(hex_with_alpha)
        bg_color = normalize_hex_input(bg_color)
        
        if len(hex_with_alpha) != 9:  # Includes '#'
            raise ValueError("HEX code must be 8 characters long (RRGGBBAA).")
        
        # Extract RGBA components
        r = int(hex_with_alpha[1:3], 16)
        g = int(hex_with_alpha[3:5], 16)
        b = int(hex_with_alpha[5:7], 16)
        a = int(hex_with_alpha[7:9], 16)  # Alpha (0–255)

        # Extract background RGB components
        bg_r = int(bg_color[1:3], 16)
        bg_g = int(bg_color[3:5], 16)
        bg_b = int(bg_color[5:7], 16)

        # Normalize alpha and blend with background
        alpha = a / 255
        r_new = round(r * alpha + bg_r * (1 - alpha))
        g_new = round(g * alpha + bg_g * (1 - alpha))
        b_new = round(b * alpha + bg_b * (1 - alpha))

        return f"#{r_new:02X}{g_new:02X}{b_new:02X}"
    except ValueError as e:
        messagebox.showerror("Input Error", f"Invalid HEX code: {e}")
        return ""


# Function to reverse: Convert 6-digit HEX to 8-digit HEX (transparent)
def opaque_to_transparent(hex_color, bg_color):
    try:
        hex_color = normalize_hex_input(hex_color)
        bg_color = normalize_hex_input(bg_color)

        if len(hex_color) != 7:  # Includes '#'
            raise ValueError("HEX code must be 6 characters long (RRGGBB).")
        
        # Extract RGB components
        r = int(hex_color[1:3], 16)
        g = int(hex_color[3:5], 16)
        b = int(hex_color[5:7], 16)

        # Extract background RGB components
        bg_r = int(bg_color[1:3], 16)
        bg_g = int(bg_color[3:5], 16)
        bg_b = int(bg_color[5:7], 16)

        # Calculate alpha and reverse-blend color
        alpha = 128  # Fixed transparency for demo (50% opacity)
        alpha_norm = alpha / 255

        r_orig = round((r - bg_r * (1 - alpha_norm)) / alpha_norm)
        g_orig = round((g - bg_g * (1 - alpha_norm)) / alpha_norm)
        b_orig = round((b - bg_b * (1 - alpha_norm)) / alpha_norm)

        # Clamp values to valid range (0–255)
        r_orig = max(0, min(255, r_orig))
        g_orig = max(0, min(255, g_orig))
        b_orig = max(0, min(255, b_orig))

        return f"#{r_orig:02X}{g_orig:02X}{b_orig:02X}{alpha:02X}"
    except ValueError as e:
        messagebox.showerror("Input Error", f"Invalid HEX code: {e}")
        return ""


# Function to convert from 8-digit HEX to 6-digit HEX
def convert_8_to_6():
    hex_code = hex_8_entry.get()
    bg_color = bg_color_entry.get()
    opaque_hex = hex_to_opaque(hex_code, bg_color)
    if opaque_hex:
        hex_6_result_label.config(text=f"Opaque HEX: {opaque_hex}", fg="green")
        hex_6_color_label.config(bg=opaque_hex)


# Function to convert from 6-digit HEX to 8-digit HEX
def convert_6_to_8():
    hex_code = hex_6_entry.get()
    bg_color = bg_color_entry.get()
    transparent_hex = opaque_to_transparent(hex_code, bg_color)
    if transparent_hex:
        hex_8_result_label.config(text=f"Transparent HEX: {transparent_hex}", fg="blue")
        hex_8_color_label.config(bg="#" + hex_code.lstrip('#'))


# Function to randomize inputs
def randomize_8_digit():
    hex_8_entry.delete(0, tk.END)
    hex_8_entry.insert(0, random_hex(8))


def randomize_6_digit():
    hex_6_entry.delete(0, tk.END)
    hex_6_entry.insert(0, random_hex(6))


# GUI Setup
root = tk.Tk()
root.title("Bidirectional HEX Converter with Custom Background")

# Background Color
bg_frame = tk.LabelFrame(root, text="Background Color")
bg_frame.pack(padx=10, pady=10, fill="both")

tk.Label(bg_frame, text="Background HEX (#RRGGBB):").grid(row=0, column=0, padx=5, pady=5)
bg_color_entry = tk.Entry(bg_frame, width=10)
bg_color_entry.insert(0, "#FFFFFF")  # Default to white
bg_color_entry.grid(row=0, column=1, padx=5, pady=5)

# Frame for 8-digit HEX (transparent) to 6-digit HEX (opaque)
frame_8_to_6 = tk.LabelFrame(root, text="8-digit Transparent HEX → 6-digit Opaque HEX")
frame_8_to_6.pack(padx=10, pady=10, fill="both")

tk.Label(frame_8_to_6, text="Transparent HEX (#RRGGBBAA):").grid(row=0, column=0, padx=5, pady=5)
hex_8_entry = tk.Entry(frame_8_to_6, width=15)
hex_8_entry.grid(row=0, column=1, padx=5, pady=5)

randomize_8_button = tk.Button(frame_8_to_6, text="Randomize", command=randomize_8_digit)
randomize_8_button.grid(row=0, column=2, padx=5, pady=5)

convert_8_button = tk.Button(frame_8_to_6, text="Convert to Opaque", command=convert_8_to_6)
convert_8_button.grid(row=0, column=3, padx=5, pady=5)

hex_6_result_label = tk.Label(frame_8_to_6, text="Opaque HEX: ", fg="green")
hex_6_result_label.grid(row=1, column=0, columnspan=4, pady=5)

hex_6_color_label = tk.Label(frame_8_to_6, width=20, height=2, bg="white", relief="solid")
hex_6_color_label.grid(row=2, column=0, columnspan=4, pady=5)

# Frame for 6-digit HEX (opaque) to 8-digit HEX (transparent)
frame_6_to_8 = tk.LabelFrame(root, text="6-digit Opaque HEX → 8-digit Transparent HEX")
frame_6_to_8.pack(padx=10, pady=10, fill="both")

tk.Label(frame_6_to_8, text="Opaque HEX (#RRGGBB):").grid(row=0, column=0, padx=5, pady=5)
hex_6_entry = tk.Entry(frame_6_to_8, width=15)
hex_6_entry.grid(row=0, column=1, padx=5, pady=5)

randomize_6_button = tk.Button(frame_6_to_8, text="Randomize", command=randomize_6_digit)
randomize_6_button.grid(row=0, column=2, padx=5, pady=5)

convert_6_button = tk.Button(frame_6_to_8, text="Convert to Transparent", command=convert_6_to_8)
convert_6_button.grid(row=0, column=3, padx=5, pady=5)

hex_8_result_label = tk.Label(frame_6_to_8, text="Transparent HEX: ", fg="blue")
hex_8_result_label.grid(row=1, column=0, columnspan=4, pady=5)

hex_8_color_label = tk.Label(frame_6_to_8, width=20, height=2, bg="white", relief="solid")
hex_8_color_label.grid(row=2, column=0, columnspan=4, pady=5)

# Run the GUI
root.mainloop()
