import os
import tkinter as tk
from PIL import ImageColor
import pygame

# Function to convert hex to RGB
def hex_to_rgb(hex_color):
    return ImageColor.getcolor(hex_color, "RGB")

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

    # Set the background color using RGB values
    rgb_color = hex_to_rgb("#5aFF34")
    root.configure(bg=f"#{rgb_color[0]:02x}{rgb_color[1]:02x}{rgb_color[2]:02x}")

    # Run the Tkinter main loop
    root.mainloop()
