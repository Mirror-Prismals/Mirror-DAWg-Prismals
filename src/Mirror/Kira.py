import tkinter as tk
import math

# Parameters
tail_length = 20
spawn_interval = 50  # in ms
radius_x = 300
radius_y = 150
center_x = 800
center_y = 450
base_size = 200

windows = []
t = 0

def spawn_next():
    global t
    angle = t
    x = center_x + radius_x * math.cos(angle)
    y = center_y + radius_y * math.sin(angle * 1.1)
    scale = 0.5 + 0.5 * math.sin(angle + math.pi / 2)
    width = int(base_size * scale)
    height = int((base_size - 40) * scale)

    win = tk.Toplevel(root)
    win.configure(bg="green")
    win.geometry(f"{width}x{height}+{int(x)}+{int(y)}")

    windows.append(win)
    if len(windows) > tail_length:
        old = windows.pop(0)
        old.destroy()

    t += 0.3
    root.after(spawn_interval, spawn_next)

root = tk.Tk()
root.withdraw()  # Hide the root window
root.after(0, spawn_next)
root.mainloop()
