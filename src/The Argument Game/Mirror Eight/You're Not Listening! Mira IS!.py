import tkinter as tk
from tkinter import messagebox
import pygame

# Initialize Pygame mixer
pygame.mixer.init()

# Dictionary mapping hex codes to audio files
hex_to_audio = {
    "#33CC33": "path/to/your/audio/file1.mp3",
    "#330000": "path/to/your/audio/file2.mp3",
    "#00CC00": "path/to/your/audio/file3.mp3",
    "#0000CC": "path/to/your/audio/file4.mp3",
    "#308000": "path/to/your/audio/file5.mp3",
    "#00C080": "path/to/your/audio/file6.mp3",
    "#000080": "path/to/your/audio/file7.mp3"
}

def play_audio_for_hex(hex_code):
    audio_file = hex_to_audio.get(hex_code.upper())
    if audio_file:
        pygame.mixer.music.load(audio_file)
        pygame.mixer.music.play()
    else:
        messagebox.showerror("Error", "No audio file associated with this hex code.")

def on_hex_code_enter(event):
    hex_code = hex_entry.get()
    if hex_code.startswith("#") and len(hex_code) == 7:
        play_audio_for_hex(hex_code)
    else:
        messagebox.showerror("Error", "Please enter a valid hex code.")

# Create the main window
root = tk.Tk()
root.title("Hex Color Display")
root.geometry("600x400")

# Hex code entry
hex_entry = tk.Entry(root, font=("Arial", 24), width=10)
hex_entry.grid(row=0, column=0, padx=10, pady=10)
hex_entry.bind("<Return>", on_hex_code_enter)

# Display button
display_button = tk.Button(root, text="Display", command=lambda: on_hex_code_enter(None))
display_button.grid(row=0, column=1, padx=10, pady=10)

# Color display area
color_display = tk.Label(root, width=40, height=20, bg="white", font=("Arial", 24))
color_display.grid(row=1, column=0, columnspan=2, padx=10, pady=10)

def update_color_display():
    hex_code = hex_entry.get()
    if hex_code.startswith("#") and len(hex_code) == 7:
        color_display.config(bg=hex_code)
    else:
        messagebox.showerror("Error", "Please enter a valid hex code.")

# Bind the display button to the color update function
display_button.config(command=update_color_display)

# Run the Tkinter event loop
root.mainloop()
