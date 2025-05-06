import tkinter as tk

# Function to create a labeled color block
def create_color_block(frame, color, text):
    block = tk.Frame(frame, bg=color, width=512, height=512)
    block.pack(padx=10, pady=5, side=tk.LEFT)
    label = tk.Label(frame, text=text, bg='white', fg='black')
    label.pack(padx=10, pady=5, side=tk.LEFT)

# Create the main window
root = tk.Tk()
root.title("Color Display")
root.attributes('-fullscreen', True)
# Create a frame for the color blocks
frame = tk.Frame(root)
frame.pack(pady=20)

# Colors and labels
colors = {
    "Hot": "#118866",
    "Warm": "#0a5555",
    "Luke": "#996999",
    "Cold": "#470000"
}

# Create color blocks
for label, color in colors.items():
    create_color_block(frame, color, f"{label} ({color})")

# Start the Tkinter event loop
root.mainloop()
