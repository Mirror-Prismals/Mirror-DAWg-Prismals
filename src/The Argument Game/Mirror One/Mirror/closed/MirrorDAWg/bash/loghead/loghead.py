import tkinter as tk

class LogHead:
    def __init__(self, root):
        self.root = root
        self.text_widget = None

        self.create_loghead()

    def create_loghead(self):
        # Create a text widget for displaying log messages
        self.text_widget = tk.Text(self.root, bg="black", fg="green", wrap="word")
        self.text_widget.pack(expand=True, fill="both")

    def display_debug(self, debug_message):
        # Insert the debug message into the text widget
        self.text_widget.insert("end", debug_message + "\n")
        # Scroll to the end to show the latest message
        self.text_widget.see("end")
