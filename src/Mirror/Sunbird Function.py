import tkinter as tk
import math

NUM_WINDOWS = 14
WINDOW_SIZE = 100
trail = [(0, 0)] * NUM_WINDOWS
windows = []

# Create all the windows
for i in range(NUM_WINDOWS):
    win = tk.Toplevel()
    win.title(f"Sunbird {i}")
    win.configure(bg="darkgreen")
    win.geometry(f"{WINDOW_SIZE}x{WINDOW_SIZE}+0+0")
    windows.append(win)

# Root hidden (we don't need a main Tk window)
root = tk.Tk()
root.withdraw()

t = 0

def animate():
    global t
    x = 1000 + 600 * math.cos(t)
    y = 300 + 150 * math.sin(t * 1.2)
    trail.insert(0, (x, y))
    if len(trail) > NUM_WINDOWS:
        trail.pop()

    for i, (wx, wy) in enumerate(trail):
        windows[i].geometry(f"{WINDOW_SIZE}x{WINDOW_SIZE}+{int(wx)}+{int(wy)}")

    t += 0.05
    root.after(30, animate)

animate()
root.mainloop()
