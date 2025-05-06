import tkinter as tk
import importlib.util
import random

class BadServer:
    def __init__(self):
        pass

    def start(self):
        # Check if 'oss' is defined in dawgserver.py
        try:
            if self.is_oss_defined():
                # If 'oss' is defined, initialize DawgServer
                self.initialize_dawgserver()
            else:
                # If 'oss' is not defined, display a warning message
                self.display_warning()
        except ModuleNotFoundError:
            # If dawgserver module is not found, display a warning message
            self.display_warning()

    def is_oss_defined(self):
        # Check if 'oss' is defined in dawgserver.py
        spec = importlib.util.find_spec("dawgserver.dawgserver")
        if spec is not None:
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            return hasattr(module, 'oss')
        return False

    def initialize_dawgserver(self):
        # Start oss, which initializes DawgServer
        oss()

    def display_warning(self):
        # Display a warning message
        error_root = tk.Tk()
        error_root.attributes('-fullscreen', True)
        error_root.configure(bg="yellow")

        # Background widget
        bg_label = tk.Label(error_root, text="Warning: DawgServer plug depth not found", fg="orange", font=("Arial", 16), justify="center", bg="yellow")
        bg_label.place(relx=0.5, rely=0.5, anchor=tk.CENTER)

        error_root.mainloop()

    def plug_depth(self):
        # Create a new label widget
        depth_label = tk.Label(error_root, text="Depth Label", fg="blue", font=("Arial", 16), justify="center", bg="white")
        depth_label.place(relx=0.5, rely=0.6, anchor=tk.CENTER)

class Press:
    def __init__(self, error_root):
        self.error_root = error_root
        self.error_root.configure(bg="blue")  # Set background color to blue
        self.error_root.attributes('-fullscreen', True)  # Set fullscreen
        self.current_level_label = tk.Label(self.error_root, text="", fg="green", font=("Georgia", 16), justify="center", bg="blue")  # Set background color to blue
        self.current_level_label.place(relx=0.5, rely=0.3, anchor=tk.CENTER)  # Adjusted placement for current level
        self.plug_depth_label = tk.Label(self.error_root, text="Plug Level: 1", fg="white", font=("Comic Sans Ms", 16), justify="center", bg="blue")
        self.plug_depth_label.place(relx=0.5, rely=0.7, anchor=tk.CENTER)  # Placement for plug depth
        self.error_root.bind("<space>", self.display_confirmation)
    
    def display_confirmation(self, event):
        # Generate a random number between 1 and 100
        level = random.randint(1, 100)
        
        # Display the current level in the label
        self.current_level_label.config(text=f"Current Width: {level}", bg="blue", fg="white")  # Adjust text color for visibility




if __name__ == "__main__":
    error_root = tk.Tk()
    error_root.attributes('-fullscreen', True)
    space = Press(error_root)

    # Bind key events here before starting the main loop
    error_root.bind("<space>", space.display_confirmation)

    error_root.mainloop()

    # Test
    bad_server = BadServer()
    bad_server.start()
