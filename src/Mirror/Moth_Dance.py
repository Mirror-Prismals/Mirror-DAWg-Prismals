import tkinter as tk
from tkinter import ttk
import colorsys

def hex_to_rgb(hex_color):
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

def rgb_to_hex(rgb):
    return '#{:02x}{:02x}{:02x}'.format(*rgb)

def invert_color(hex_color):
    rgb = hex_to_rgb(hex_color)
    return rgb_to_hex(tuple(255 - c for c in rgb))

def add_full_red(hex_color):
    r, g, b = hex_to_rgb(hex_color)
    return rgb_to_hex((255, g, b))

def color_distance(color1, color2):
    rgb1 = hex_to_rgb(color1)
    rgb2 = hex_to_rgb(color2)  # Fixed: changed hex_to_hex to hex_to_rgb
    return sum((a - b) ** 2 for a, b in zip(rgb1, rgb2)) ** 0.5

class MothDanceApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Moth Dance Color Theorem")
        self.geometry("800x600")
        
        self.create_widgets()
    
    def create_widgets(self):
        ttk.Label(self, text="Starting Color:").pack(pady=5)
        self.color_entry = ttk.Entry(self)
        self.color_entry.pack(pady=5)
        self.color_entry.insert(0, "#008080")
        
        ttk.Button(self, text="Explore Moth Dance", command=self.explore_moth_dance).pack(pady=10)
        
        self.result_frame = ttk.Frame(self)
        self.result_frame.pack(pady=20, expand=True, fill='both')
    
    def explore_moth_dance(self):
        start_color = self.color_entry.get()
        
        for widget in self.result_frame.winfo_children():
            widget.destroy()
        
        self.display_sequence("Just Intonation-like Moth Dance", self.ji_sequence(start_color))
        self.display_sequence("12-tone Equal Temperament-like Moth Dance", self.et_sequence(start_color))
    
    def ji_sequence(self, start_color):
        sequence = [start_color]
        sequence.append(add_full_red(sequence[-1]))
        sequence.append(invert_color(sequence[-1]))
        sequence.append(invert_color(sequence[-1]))
        return sequence
    
    def et_sequence(self, start_color):
        sequence = [start_color]
        sequence.append(add_full_red(sequence[-1]))
        sequence.append(invert_color(sequence[-1]))
        sequence.append(start_color)
        return sequence
    
    def display_sequence(self, title, sequence):
        frame = ttk.LabelFrame(self.result_frame, text=title)
        frame.pack(pady=10, padx=10, fill='x')
        
        for i, color in enumerate(sequence):
            label = ttk.Label(frame, text=f"Step {i+1}: {color}", background=color, foreground=self.contrast_color(color))
            label.pack(pady=5, padx=5, fill='x')
        
        distance = color_distance(sequence[0], sequence[-1])
        ttk.Label(frame, text=f"Distance from start to end: {distance:.2f}").pack(pady=5)
    
    def contrast_color(self, hex_color):
        r, g, b = [x/255.0 for x in hex_to_rgb(hex_color)]
        luminance = 0.299 * r + 0.587 * g + 0.114 * b
        return "#000000" if luminance > 0.5 else "#FFFFFF"

if __name__ == "__main__":
    app = MothDanceApp()
    app.mainloop()
