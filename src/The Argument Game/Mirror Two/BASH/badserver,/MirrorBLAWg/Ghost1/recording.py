import tkinter as tk

class RecordingWindow:
    def __init__(self, root):
        self.root = root
        self.root.title("Recording")
        self.root.attributes('-fullscreen', True)  # Set window to fullscreen
        self.root.configure(bg="black")

        self.label = tk.Label(self.root, text="Recording Window", bg="black", fg="white", font=("Helvetica", 24))
        self.label.pack(pady=20)

        self.exit_button = tk.Button(self.root, text="Exit", command=self.exit_fullscreen, bg="red", fg="white")
        self.exit_button.pack(anchor="ne", padx=10, pady=10)

    def exit_fullscreen(self):
        self.root.attributes('-fullscreen', False)
        self.root.quit()

if __name__ == "__main__":
    root = tk.Tk()
    app = RecordingWindow(root)
    root.mainloop()
