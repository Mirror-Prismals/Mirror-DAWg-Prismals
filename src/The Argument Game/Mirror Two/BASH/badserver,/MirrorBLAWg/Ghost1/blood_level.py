import tkinter as tk
import random

class BloodLevelChannel:
    def __init__(self, name):
        self.name = name
        self.value = 1234

    def change_value(self):
        self.value = random.randint(1000, 9999)

class BloodLevel:
    def __init__(self, root):
        self.root = root
        self.root.title("Blood Level")
        self.root.attributes('-fullscreen', True)  # Set window to fullscreen
        self.root.configure(bg="black")

        self.channels = [
            BloodLevelChannel("Channel 1"),
            BloodLevelChannel("Channel 2"),
            BloodLevelChannel("Channel 3"),
            BloodLevelChannel("Channel 4"),
        ]

        self.labels = []
        for i, channel in enumerate(self.channels):
            label = tk.Label(self.root, text=f"{channel.name}: {channel.value}", bg="black", fg="white", font=("Helvetica", 24))
            label.pack(pady=20)
            self.labels.append(label)

        self.update_values()
        self.root.bind("<Escape>", self.exit_fullscreen)

    def update_values(self):
        for i, channel in enumerate(self.channels):
            channel.change_value()
            self.labels[i].config(text=f"{channel.name}: {channel.value}")
        self.root.after(1000, self.update_values)

    def exit_fullscreen(self, event):
        self.root.attributes('-fullscreen', False)

if __name__ == "__main__":
    root = tk.Tk()
    blood_level = BloodLevel(root)
    root.mainloop()
