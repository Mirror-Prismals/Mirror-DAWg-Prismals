import tkinter as tk
import random

class BloodLevelChannel:
    def __init__(self, name):
        self.name = name
        self.value = 1

    def change_value(self):
        self.value = random.randint(1, 9999)

class BloodLevel:
    def __init__(self, root):
        self.root = root
        self.root.title("KRate")
        self.root.attributes('-fullscreen', True)  # Set window to fullscreen
        self.root.configure(bg="black")

        self.channels = [
            BloodLevelChannel("Channel 1"),
            BloodLevelChannel("Channel 2"),
            BloodLevelChannel("Channel 3"),
            BloodLevelChannel("Channel 4"),
            BloodLevelChannel("Channel 5"),
            BloodLevelChannel("Channel 6"),
            BloodLevelChannel("Channel 7"),
            BloodLevelChannel("Channel 8"),
            BloodLevelChannel("Channel 9"),
            BloodLevelChannel("Channel 10"),
            BloodLevelChannel("Channel 11"),
            BloodLevelChannel("Channel 12"),
            BloodLevelChannel("Channel 13"),
            
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
        self.root.after(10000, self.update_values)

    def exit_fullscreen(self, event):
        self.root.attributes('-fullscreen', False)

if __name__ == "__main__":
    root = tk.Tk()
    blood_level = BloodLevel(root)
    root.mainloop()
