import tkinter as tk
import time
import math

class RecordingWindow:
    def __init__(self, root):
        self.root = root
        self.root.title("Recording")
        self.root.attributes('-fullscreen', True)  # Set window to fullscreen
        self.root.configure(bg="black")

        self.label = tk.Label(self.root, text="Recording Window", bg="black", fg="white", font=("Helvetica", 24))
        self.label.pack(pady=20)

        self.canvas = tk.Canvas(self.root, width=200, height=200, bg="black", highlightthickness=0)
        self.canvas.pack()

        self.dot = self.canvas.create_oval(90, 90, 110, 110, fill="lime", outline="lime")

        self.exit_button = tk.Button(self.root, text="Exit", command=self.exit_fullscreen, bg="lime", fg="white")
        self.exit_button.pack(anchor="ne", padx=10, pady=10)

        self.update_dot()
        self.create_clock()
        self.update_clock()

    def update_dot(self):
        current_color = self.canvas.itemcget(self.dot, "fill")
        new_color = "green" if current_color == "lime" else "lime"
        self.canvas.itemconfig(self.dot, fill=new_color, outline=new_color)
        self.root.after(500, self.update_dot)  # Update dot every 500 ms (Krate)

    def create_clock(self):
        self.clock_canvas = tk.Canvas(self.root, width=400, height=400, bg="black", highlightthickness=0)
        self.clock_canvas.pack()

        self.center_x = 200
        self.center_y = 200
        self.clock_radius = 150

        self.clock_face = self.clock_canvas.create_oval(
            self.center_x - self.clock_radius, self.center_y - self.clock_radius,
            self.center_x + self.clock_radius, self.center_y + self.clock_radius,
            outline="white"
        )

        self.hour_hand = self.clock_canvas.create_line(
            self.center_x, self.center_y,
            self.center_x, self.center_y - self.clock_radius // 2,
            fill="white", width=4
        )

        self.minute_hand = self.clock_canvas.create_line(
            self.center_x, self.center_y,
            self.center_x, self.center_y - self.clock_radius * 0.75,
            fill="white", width=2
        )

        self.second_hand = self.clock_canvas.create_line(
            self.center_x, self.center_y,
            self.center_x, self.center_y - self.clock_radius,
            fill="red", width=1
        )

    def update_clock(self):
        now = time.localtime()
        hours = now.tm_hour % 12
        minutes = now.tm_min
        seconds = now.tm_sec

        hour_angle = 360 / 12 * hours + 360 / (12 * 60) * minutes
        minute_angle = 360 / 60 * minutes
        second_angle = 360 / 60 * seconds

        self.update_hand(self.hour_hand, hour_angle, self.clock_radius // 2)
        self.update_hand(self.minute_hand, minute_angle, self.clock_radius * 0.75)
        self.update_hand(self.second_hand, second_angle, self.clock_radius)

        self.root.after(1000, self.update_clock)  # Update every second

    def update_hand(self, hand, angle, length):
        angle_rad = math.radians(angle - 90)
        end_x = self.center_x + length * math.cos(angle_rad)
        end_y = self.center_y + length * math.sin(angle_rad)
        self.clock_canvas.coords(hand, self.center_x, self.center_y, end_x, end_y)

    def exit_fullscreen(self):
        self.root.attributes('-fullscreen', False)
        self.root.quit()

if __name__ == "__main__":
    root = tk.Tk()
    app = RecordingWindow(root)
    root.mainloop()
