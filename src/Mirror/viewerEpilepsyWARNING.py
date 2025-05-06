import tkinter as tk
from tkinter import filedialog, simpledialog, messagebox
import re
Hello! Please be careful, this script makes bright flashing lights
# ---------- Settings ----------
FPS = 24
FRAME_DELAY = int(1000 / FPS)  # in milliseconds

# ---------- Helper Functions ----------
def pair_to_hex(pair):
    """
    Converts a pair of numbers (strings) to a hex color.
    Uses first value as R, second as G, and sets B to the average.
    Returns a string like "#RRGGBB" or None if conversion fails.
    """
    try:
        a, b = pair
        # Convert to float then int and take modulo 256
        r = int(float(a)) % 256
        g = int(float(b)) % 256
        b_val = int((r + g) / 2) % 256
        return f"#{r:02x}{g:02x}{b_val:02x}"
    except Exception as e:
        print(f"Error converting pair {pair}: {e}")
        return None

def parse_coordinate_string(coord_string):
    """
    Given a string containing coordinate pairs like:
       (0.101,596.383) (276.263,603.156) ...
    Returns a list of tuples [(num1, num2), ...] as strings.
    """
    # Regex to extract numbers from things like (num1,num2)
    pattern = r"\(\s*([-\d\.]+)\s*,\s*([-\d\.]+)\s*\)"
    matches = re.findall(pattern, coord_string)
    return matches

def group_into_frames(pairs):
    """
    Groups every two pairs into a frame. Each frame will have two colors.
    If there's an incomplete group at the end, it is skipped.
    Returns a list of tuples: [(color1, color2), ...]
    """
    frames = []
    # Need groups of 2 pairs => 4 numbers total per frame
    for i in range(0, len(pairs) - 1, 2):
        # Get two consecutive pairs
        pair1 = pairs[i]
        pair2 = pairs[i+1]
        color1 = pair_to_hex(pair1)
        color2 = pair_to_hex(pair2)
        if color1 is None or color2 is None:
            print(f"Skipping frame with problematic pairs: {pair1} or {pair2}")
            continue
        frames.append((color1, color2))
    return frames

# ---------- Animation Class ----------
class ColorAnimation:
    def __init__(self, master, frames):
        self.master = master
        self.frames = frames
        self.index = 0
        
        # Create a full-window canvas
        self.canvas = tk.Canvas(master, width=800, height=600)
        self.canvas.pack(fill=tk.BOTH, expand=True)
        
        # Create two rectangles that cover left and right halves
        self.rect_left = self.canvas.create_rectangle(0, 0, 400, 600, fill="", outline="")
        self.rect_right = self.canvas.create_rectangle(400, 0, 800, 600, fill="", outline="")
        
        self.animate()
    
    def animate(self):
        if not self.frames:
            messagebox.showerror("Error", "No valid frames to animate!")
            return
        
        # Get the current frame of colors
        color1, color2 = self.frames[self.index]
        # Update rectangles
        self.canvas.itemconfig(self.rect_left, fill=color1)
        self.canvas.itemconfig(self.rect_right, fill=color2)
        
        # Advance to next frame (wrapping around)
        self.index = (self.index + 1) % len(self.frames)
        
        # Schedule next frame
        self.master.after(FRAME_DELAY, self.animate)

# ---------- Main Program ----------
def main():
    root = tk.Tk()
    root.title("Hex Color Animation")
    
    # Hide the main window while loading file
    root.withdraw()
    
    # Ask the user to select an external text file
    file_path = filedialog.askopenfilename(title="Select a text file", filetypes=[("Text Files", "*.txt"), ("All Files", "*.*")])
    if not file_path:
        messagebox.showinfo("Cancelled", "No file was selected. Exiting.")
        return
    
    try:
        with open(file_path, 'r') as f:
            content = f.read()
    except Exception as e:
        messagebox.showerror("Error", f"Could not read file: {e}")
        return
    
    # Ask the user what the file is using a popup dialog
    file_desc = simpledialog.askstring("File Description", "What is this file?")
    if file_desc is None:
        file_desc = "No description provided."
    print(f"User described the file as: {file_desc}")
    
    # Parse the coordinate string from the file
    pairs = parse_coordinate_string(content)
    if not pairs:
        messagebox.showerror("Error", "No valid coordinate pairs found in the file.")
        return
    
    # Group pairs into frames (two pairs per frame)
    frames = group_into_frames(pairs)
    if not frames:
        messagebox.showerror("Error", "No valid frames were generated from the data.")
        return
    
    # Now show the main window and start animation
    root.deiconify()
    anim = ColorAnimation(root, frames)
    root.mainloop()

if __name__ == "__main__":
    main()
