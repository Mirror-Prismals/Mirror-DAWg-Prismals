import tkinter as tk

colors = [
    "5756f1", "48259a", "a9bc85", "318176", "7bf029", "064541", "84807c", "8c18a2",
    "00fff0", "995693", "f0788c", "71e85f", "a78672", "8b5899", "15899b", "0887a5",
    "d37cf0", "7d73ab", "591838", "21ecaf", "c771da", "16cd69", "e7df20", "ca49fa",
    "5acb0d", "1e6392", "a7890f"
]

def hex_to_rgb(hex_color):
    """Converts a hex color string to an RGB tuple."""
    hex_color = hex_color.lstrip("#")
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

def create_color_boxes(root):
    """Creates and places color boxes with hex labels."""
    for index, hex_color in enumerate(colors):
        rgb_color = hex_to_rgb(hex_color)
        color_box = tk.Frame(root, bg=f"#{hex_color}", width=150, height=100) # Adjusted size
        color_box.grid(row=index // 9, column=index % 9, padx=5, pady=(5, 20))  # Arrange in a grid, added padding under
        
        label = tk.Label(root, text=f"#{hex_color}", font=("Arial", 12))
        label.grid(row=(index // 9) + 1, column=index % 9, padx=5, pady=2, sticky="n") # Position label below box

def main():
    root = tk.Tk()
    root.title("Color Display")
    root.attributes('-fullscreen', True)  # Make it fullscreen
    
    create_color_boxes(root)
    
    root.mainloop()

if __name__ == "__main__":
    main()
