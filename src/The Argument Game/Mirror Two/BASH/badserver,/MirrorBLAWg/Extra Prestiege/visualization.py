import tkinter as tk
from tkinter import messagebox

class MockServerApp:
    def __init__(self, root):
        self.root = root

        self.root.title("503 Service Temporarily Unavailable")
        root.attributes('-fullscreen', True)
        
        self.root.configure(bg="blue")  # Set background to black
        self.status_label = tk.Label(root, text="503 Service Temporarily Unavailable", fg="Blue")
        self.status_label.pack(pady=20)

        self.start_button = tk.Button(root, text='''''', command=self.start_server)
        self.start_button.pack(side="left", padx=20)

        self.stop_button = tk.Button(root, text='''....''', command=self.stop_server, state="disabled")
        self.stop_button.pack(side="right", padx=2000)

    def start_server(self):
        # Code to start the real server will go here
        self.status_label.config(text="Server Status: Running", fg="Teal")
        self.start_button.config(state="disabled")
        self.stop_button.config(state="normal")
       # messagebox.showinfo("Server", "Server started successfully!")

    def stop_server(self):
        # Code to stop the real server will go here
        self.status_label.config(text="Server Status: Stopped", fg="red")
        self.start_button.config(state="normal")
        self.stop_button.config(state="disabled")
        messagebox.showinfo("Server", "Server stopped successfully!")

if __name__ == "__main__":
    root = tk.Tk()
    app = MockServerApp(root)
    root.mainloop()
