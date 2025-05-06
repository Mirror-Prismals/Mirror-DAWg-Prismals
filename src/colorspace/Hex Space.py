import tkinter as tk
from tkinter import simpledialog, messagebox, ttk
import json
import os
import math

class HexSpaceApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Hex Space")
        self.root.attributes("-fullscreen", True)
        self.screen_width = self.root.winfo_screenwidth()
        self.screen_height = self.root.winfo_screenheight()
        
        self.tab_control = ttk.Notebook(self.root)
        
        self.canvas_tab = ttk.Frame(self.tab_control)
        self.saved_colors_tab = ttk.Frame(self.tab_control)
        
        self.tab_control.add(self.canvas_tab, text='Hex Space')
        self.tab_control.add(self.saved_colors_tab, text='Saved Colors')
        self.tab_control.pack(expand=1, fill="both")
        
        self.canvas = tk.Canvas(self.canvas_tab, width=self.screen_width, height=self.screen_height - 50, bg="white")
        self.canvas.pack()
        
        self.colors = {}
        self.hex_coords = {}
        self.load_colors()
        self.hex_size = 20  # Reduced size to fit more hexagons
        self.zoom_level = 1.0

        self.canvas.bind("<Button-1>", self.on_click)
        self.root.bind("<KeyPress-plus>", self.zoom_in)
        self.root.bind("<KeyPress-minus>", self.zoom_out)
        self.root.bind("<Escape>", self.exit_fullscreen)

        self.create_search_ui()
        self.create_saved_colors_ui()

    def create_search_ui(self):
        search_frame = tk.Frame(self.canvas_tab)
        search_frame.pack(side=tk.BOTTOM, fill=tk.X)
        search_label = tk.Label(search_frame, text="Search:")
        search_label.pack(side=tk.LEFT, padx=10)
        self.search_entry = tk.Entry(search_frame)
        self.search_entry.pack(side=tk.LEFT, padx=10)
        search_button = tk.Button(search_frame, text="Go", command=self.search_color)
        search_button.pack(side=tk.LEFT, padx=10)
    def create_saved_colors_ui(self):
        self.saved_colors_text = tk.Text(self.saved_colors_tab, wrap=tk.NONE)
        self.saved_colors_text.pack(expand=1, fill="both")
        self.update_saved_colors_ui()

    def update_saved_colors_ui(self):
        self.saved_colors_text.delete(1.0, tk.END)
        json_data = json.dumps(self.colors, indent=4)
        self.saved_colors_text.insert(tk.END, json_data)

    def draw_hexagons(self, hex_color):
        self.canvas.delete("all")
        r, g, b = self.hex_to_rgb(hex_color)
        neighbors = self.get_closest_colors(r, g, b)

        for idx, (neighbor_color, (dx, dy)) in enumerate(neighbors):
            x = dx
            y = dy
            self.draw_hexagon(x, y, neighbor_color)

    def draw_hexagon(self, x, y, color):
        size = self.hex_size * self.zoom_level
        w = size * math.sqrt(3)
        h = size * 2
        center_x = self.screen_width / 2
        center_y = (self.screen_height - 50) / 2
        px = center_x + x * w * 1.75
        py = center_y + y * h * 1.5
        points = [
            (px + size * math.cos(math.radians(angle)), py + size * math.sin(math.radians(angle)))
            for angle in range(0, 360, 60)
        ]
        self.canvas.create_polygon(points, outline='black', fill=color, tags=color)
        self.hex_coords[(x, y)] = color

    def load_colors(self):
        if os.path.exists("colors.json"):
            with open("colors.json", "r") as f:
                self.colors = json.load(f)
        else:
            self.colors = {}

    def save_colors(self):
        with open("colors.json", "w") as f:
            json.dump(self.colors, f, indent=4)
        self.update_saved_colors_ui()

    def rgb_to_hex(self, r, g, b):
        return "#{:02x}{:02x}{:02x}".format(r, g, b)

    def hex_to_rgb(self, hex_color):
        hex_color = hex_color.lstrip('#')
        return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

    def on_click(self, event):
        x, y = self.canvas.canvasx(event.x), self.canvas.canvasy(event.y)
        item = self.canvas.find_closest(x, y)
        hex_color = self.canvas.gettags(item)[0]
        self.add_tag(hex_color)

    def add_tag(self, hex_color):
        name = simpledialog.askstring("Name", f"Enter name for color {hex_color}:")
        if name:
            tags = simpledialog.askstring("Tags", "Enter tags (comma-separated):")
            if tags:
                tags_list = [tag.strip() for tag in tags.split(",")]
                self.colors[hex_color] = {"name": name, "tags": tags_list}
                self.save_colors()
                messagebox.showinfo("Success", f"Color {hex_color} named '{name}' with tags {tags_list}.")

    def zoom_in(self, event):
        self.zoom_level *= 1.1
        self.draw_hexagons()

    def zoom_out(self, event):
        self.zoom_level /= 1.1
        self.draw_hexagons()

    def search_color(self):
        search_hex = self.search_entry.get().strip()
        if len(search_hex) == 6 and all(c in '0123456789ABCDEFabcdef' for c in search_hex):
            search_hex = f"#{search_hex}"
        if search_hex.startswith("#") and len(search_hex) == 7:
            self.draw_hexagons(search_hex)
        else:
            messagebox.showerror("Error", "Invalid hex color code.")

    def get_closest_colors(self, r, g, b):
        steps = list(range(-8, 9))
        neighbors = []
        for dx in steps:
            for dy in steps:
                for dz in steps:
                    if abs(dx) + abs(dy) + abs(dz) <= 8:
                        nr, ng, nb = (r + dx * 8) % 256, (g + dy * 8) % 256, (b + dz * 8) % 256
                        if 0 <= nr <= 255 and 0 <= ng <= 255 and 0 <= nb <= 255:
                            neighbor_color = self.rgb_to_hex(nr, ng, nb)
                            neighbors.append((neighbor_color, (dx, dy)))
        return neighbors

    def highlight_color(self, hex_color):
        for item in self.canvas.find_withtag(hex_color):
            self.canvas.itemconfig(item, outline="red", width=3)

    def exit_fullscreen(self, event):
        self.root.attributes("-fullscreen", False)
        self.root.geometry(f"{self.screen_width}x{self.screen_height}")

if __name__ == "__main__":
    root = tk.Tk()
    app = HexSpaceApp(root)
    root.mainloop()
