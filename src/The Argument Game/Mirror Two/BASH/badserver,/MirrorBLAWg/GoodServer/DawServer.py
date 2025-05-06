import tkinter as tk
from tkinter import messagebox

class DawServer:
    def __init__(self, root):
        self.root = root

        self.root.title("503 Service Temporarily Unavailable")
        root.attributes('-fullscreen', True)
        
        self.root.configure(bg="cyan")  # Set background to black
        self.status_label = tk.Label(root, text="~jynx", fg="green")
        self.status_label.pack(pady=20)

        self.start_button = tk.Button(root, text="You Are Being Recorded")
