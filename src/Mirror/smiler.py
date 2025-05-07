import tkinter as tk
import multiprocessing
import math
import time

def window_process(index, delay):
    phase_offset = index * 0.4
    root = tk.Tk()
    canvas = tk.Canvas(root, width=200, height=200, bg="green")
    canvas.pack(fill="both", expand=True)

    def animate():
        t = time.time() + phase_offset
        x = 700 + 300 * math.cos(t)
        y = 300 + 150 * math.sin(t * 1.2)
        z = (math.sin(t * 0.8) + 1) / 2
        scale = 100 + int(z * 100)

        root.geometry(f"{scale}x{scale}+{int(x)}+{int(y)}")
        root.after(30, animate)

    root.after(int(delay * 1000), animate)
    root.mainloop()

if __name__ == "__main__":
    multiprocessing.set_start_method("spawn")

    processes = []
    for i in range(14):
        p = multiprocessing.Process(target=window_process, args=(i, i * 0.1))
        p.start()
        processes.append(p)

    for p in processes:
        p.join()
