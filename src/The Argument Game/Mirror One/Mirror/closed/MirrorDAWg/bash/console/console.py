import tkinter as tk
class Console:
    def __init__(self, root, dawg_server):
        self.root = root
        self.dawg_server = dawg_server
        self.text_widget = None

        self.create_console()

    def create_console(self):
        # Create a text widget for the console
        self.text_widget = tk.Text(self.root, bg="black", fg="white", wrap="word")
        self.text_widget.pack(expand=True, fill="both")

        # Display initial message
        self.display_message("Server is listening on port 5555")

        # Start animation
        self.animate_dots()

        # Bind Enter key press event
        self.text_widget.bind("<Return>", self.process_input)

    def process_input(self, event):
        # Get the input text from the console
        input_text = self.text_widget.get("end-1c linestart", "end-1c lineend")

        # Process the input command
        if input_text.lower() == "m":
            self.display_message("mirror")
        else:
            self.display_message("Invalid command")

        # Clear the input field
        self.text_widget.delete("end-1c linestart", "end")

    def animate_dots(self):
        # Animation logic for displaying dots
        self.display_message("...")

        # Repeat animation
        self.root.after(1000, self.animate_dots)

    def display_message(self, message):
        # Insert message into the console
        self.text_widget.insert("end", message + "\n")
        self.text_widget.see("end")

# Initialize GUI
root = tk.Tk()
root.title("Mock Console")
root.geometry("400x300")

# Initialize DawgServer (not shown here)
dawg_server = None

# Initialize Console
console = Console(root, dawg_server)

root.mainloop()
