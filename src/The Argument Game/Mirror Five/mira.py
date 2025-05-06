import os
import tkinter as tk
from PIL import ImageColor
import pygame

# Function to convert hex to RGB
def hex_to_rgb(hex_color):
    return ImageColor.getcolor(hex_color, "RGB")

# Function to move the window to the second monitor and exit fullscreen
def move_to_second_monitor(event=None):
    root.update_idletasks()
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()
    second_monitor_x_offset = screen_width
    root.geometry(f"+{second_monitor_x_offset}+0")
    root.attributes("-fullscreen", False)  # Exit fullscreen

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
    text = canvas.create_text(text_x, text_y, text="hello", font=("Arial", 20))

    # Ensure the canvas can detect mouse events
    canvas.bind("<Motion>", lambda event: event)

    # Bind the mouse enter event to the button
    def on_enter(event):
        print("Mouse entered button area")
        move_to_second_monitor(event)

    # Tag the button and text for event binding
    canvas.tag_bind(button, "<Enter>", on_enter)
    canvas.tag_bind(text, "<Enter>", on_enter)

    # Run the Tkinter main loop
    root.mainloop()
