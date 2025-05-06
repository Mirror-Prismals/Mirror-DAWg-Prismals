import os
import tkinter as tk
from PIL import ImageColor
import pygame

# Function to convert hex to RGB
def hex_to_rgb(hex_color):
    return ImageColor.getcolor(hex_color, "RGB")

# Function to move the window to the second monitor
def move_to_second_monitor(event):
    root.update_idletasks()
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()
    # Assuming the second monitor is positioned to the right of the primary monitor
    second_monitor_x_offset = screen_width
    root.geometry(f"+{second_monitor_x_offset}+0")

# Initialize Pygame mixer
pygame.mixer.init()

# Path to the audio file relative to the current working directory
audio_file = "samplegalaxyambientmenu(2).wav"

# Print the current working directory for debugging
print("Current working directory:", os.getcwd())

# List all files in the current working directory for debugging
print("Files in the current directory:", os.listdir(os.getcwd()))

# Check if the audio file exists
if not os.path.exists(audio_file):
    print(f"Error: The file {audio_file} does not exist.")
else:
    # Load and play the audio file
    pygame.mixer.music.load(audio_file)
    pygame.mixer.music.play(-1)  # Loop the music indefinitely

    # Create the main Tkinter window
    root = tk.Tk()
    root.attributes("-fullscreen", True)

    # Set the background color to the specified hex value for orange
    hex_color = "#f44000"
    rgb_color = hex_to_rgb(hex_color)
    root.configure(bg=f"#{rgb_color[0]:02x}{rgb_color[1]:02x}{rgb_color[2]:02x}")

    # Create a Canvas to draw the button
    canvas = tk.Canvas(root, bg=f"#{rgb_color[0]:02x}{rgb_color[1]:02x}{rgb_color[2]:02x}", highlightthickness=0)
    canvas.pack(fill=tk.BOTH, expand=True)

    # Draw the polygon button in the center
    button_width = 100
    button_height = 50
    canvas_width = root.winfo_screenwidth()
    canvas_height = root.winfo_screenheight()
    button_coords = [
        (canvas_width // 2 - button_width // 2, canvas_height // 2 - button_height // 2),
        (canvas_width // 2 + button_width // 2, canvas_height // 2 - button_height // 2),
        (canvas_width // 2 + button_width // 2, canvas_height // 2 + button_height // 2),
        (canvas_width // 2 - button_width // 2, canvas_height // 2 + button_height // 2),
    ]

    button = canvas.create_polygon(button_coords, fill="white", outline="black")

    # Draw the text "hello" in the center of the button
    text_x = canvas_width // 2
    text_y = canvas_height // 2
    canvas.create_text(text_x, text_y, text="hello", font=("Arial", 20))

    # Bind the click event to the button
    def on_click(event):
        if canvas.find_withtag(tk.CURRENT):
            move_to_second_monitor(event)

    canvas.tag_bind(button, "<Button-1>", on_click)

    # Run the Tkinter main loop
    root.mainloop()
