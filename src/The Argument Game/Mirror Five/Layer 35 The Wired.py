import os
import tkinter as tk
from tkinter import ttk

def hex_to_rgb(hex_color):
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

def format_color(color):
    if color.startswith('#'):
        rgb_color = hex_to_rgb(color)
        return f'#{rgb_color[0]:02x}{rgb_color[1]:02x}{rgb_color[2]:02x}'
    return color

def print_clipped_tree(startpath):
    tree_structure = []
    for root, dirs, files in os.walk(startpath):
        level = root.replace(startpath, '').count(os.sep)
        indent = ' ' * 4 * level
        tree_structure.append('{}{}'.format(indent, ''.join([word[0] for word in os.path.basename(root).split()])))
        subindent = ' ' * 4 * (level + 1)
        for f in files:
            tree_structure.append('{}{}'.format(subindent, ''.join([word[0] for word in f.split()])))
    return tree_structure

def create_grass_pattern(canvas, width, height, grass_color):
    grass_color = format_color(grass_color)
    ascii_grass = [
        "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^",
        " ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^",
        "  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^",
        "   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^",
        "    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^",
        "     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^",
        "      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
    ]
    for y in range(0, height, 20):
        for i, line in enumerate(ascii_grass):
            x = (y // 20 + i) * 10 % width
            canvas.create_text(x, y + i * 10, text=line, fill=grass_color, font=("Courier", 10), anchor="nw")

def display_tree_in_tkinter(tree_structure, bg_color, text_color, grass_color, water_color):
    root = tk.Tk()
    root.title("Directory Tree")
    root.attributes('-fullscreen', True)
    
    bg_color = format_color(bg_color)
    text_color = format_color(text_color)
    grass_color = format_color(grass_color)
    water_color = format_color(water_color)

    # Main frame
    main_frame = tk.Frame(root, bg='brown')
    main_frame.pack(fill=tk.BOTH, expand=1)

    # Side frames for the "riverbank"
    riverbank_width = 150  # Increased width to 3 times
    left_canvas = tk.Canvas(main_frame, bg='brown', width=riverbank_width, height=root.winfo_screenheight(), highlightthickness=0)
    left_canvas.pack(side=tk.LEFT, fill=tk.Y)
    right_canvas = tk.Canvas(main_frame, bg='brown', width=riverbank_width, height=root.winfo_screenheight(), highlightthickness=0)
    right_canvas.pack(side=tk.RIGHT, fill=tk.Y)

    create_grass_pattern(left_canvas, riverbank_width, root.winfo_screenheight(), grass_color)
    create_grass_pattern(right_canvas, riverbank_width, root.winfo_screenheight(), grass_color)

    # Canvas and scrollbar for the tree display
    canvas = tk.Canvas(main_frame, bg=water_color)
    scrollbar = ttk.Scrollbar(main_frame, orient=tk.VERTICAL, command=canvas.yview)
    scrollable_frame = ttk.Frame(canvas, style="Custom.TFrame")

    scrollable_frame.bind(
        "<Configure>",
        lambda e: canvas.configure(
            scrollregion=canvas.bbox("all")
        )
    )

    canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
    canvas.configure(yscrollcommand=scrollbar.set)

    style = ttk.Style()
    style.configure("Custom.TLabel", background=water_color, foreground=text_color, font=("Courier", 14))
    style.configure("Custom.TFrame", background=water_color)

    for line in tree_structure:
        label = ttk.Label(scrollable_frame, text=line, style="Custom.TLabel")
        label.pack(anchor='w')

    canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=1, padx=(riverbank_width, 0))
    scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

    # Mousewheel scrolling
    def on_mousewheel(event):
        canvas.yview_scroll(int(-1*(event.delta/120)), "units")

    root.bind_all("<MouseWheel>", on_mousewheel)

    # Adding interval entry for auto-scrolling speed control
    entry_label = tk.Label(main_frame, text="Enter interval (Krate) in milliseconds:", bg='brown', fg='white')
    entry_label.pack(pady=10)
    
    interval_entry = tk.Entry(main_frame)
    interval_entry.pack(pady=10)
    interval_entry.insert(0, "100")  # Default value
    
    # Auto-scrolling
    def auto_scroll():
        if scroll_interval < 0:
            canvas.yview_scroll(-1, 'units')
        else:
            canvas.yview_scroll(1, 'units')
        root.after(abs(scroll_interval), auto_scroll)
    
    # Update interval function
    def update_interval(event):
        global scroll_interval
        try:
            scroll_interval = int(interval_entry.get())
            if scroll_interval == 0:
                scroll_interval = 100
        except ValueError:
            interval_entry.delete(0, tk.END)
            interval_entry.insert(0, "100")
            scroll_interval = 100
    
    interval_entry.bind("<Return>", update_interval)
    
    scroll_interval = 100  # Initial scroll interval
    root.after(scroll_interval, auto_scroll)

    root.bind("<Escape>", lambda e: root.destroy())

    root.mainloop()

if __name__ == "__main__":
    base_dir = os.path.dirname(os.path.abspath(__file__))  # Get the directory of the current script
    tree_structure = print_clipped_tree(base_dir)
    
    bg_color = "#0000FF"  # Replace with your desired background color
    text_color = "brown"  # Replace with your desired text color
    grass_color = "green"  # Replace with your desired grass color
    water_color = "#00FFFF"  # Replace with your desired water color

    display_tree_in_tkinter(tree_structure, bg_color, text_color, grass_color, water_color)
