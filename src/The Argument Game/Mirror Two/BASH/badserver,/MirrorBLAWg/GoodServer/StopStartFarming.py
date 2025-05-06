import tkinter as tk
from tkinter import messagebox

class MockServerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Mirror Three Server")
        root.attributes('-fullscreen', True)

        self.root.configure(bg="black")  # Set background to black
        self.status_label = tk.Label(root, text="Server Idle", fg="black")
        self.status_label.pack(pady=20)

        self.start_button = tk.Button(root, text="Start Farming", command=self.start_farming)
        self.start_button.pack(side="left", padx=20)

        self.stop_button = tk.Button(root, text="Stop Farming", command=self.stop_farming, state="disabled")
        self.stop_button.pack(side="right", padx=20)

    def start_farming(self):
        self.status_label.config(text="Farming", fg="green")
        self.start_button.config(state="disabled")
        self.stop_button.config(state="normal")
        messagebox.showinfo("Server", "Farming started successfully!")

    def stop_farming(self):
        self.status_label.config(text="Server Idle", fg="black")
        self.start_button.config(state="normal")
        self.stop_button.config(state="disabled")
        messagebox.showinfo("Server", "Farming stopped successfully!")

if __name__ == "__main__":
    root = tk.Tk()
    app = MockServerApp(root)
    root.mainloop()
