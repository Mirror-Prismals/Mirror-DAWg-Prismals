import tkinter as tk
import pygame

# Function to play the startup sound
def play_startup_sound():
    pygame.mixer.init()
    pygame.mixer.music.load("startup.wav")  # Make sure to place your .wav file in the same directory
    pygame.mixer.music.play()

# Function to force the window to stay fullscreen
def force_fullscreen(event=None):
    root.attributes('-fullscreen', True)

# Function to launch the Title Card
def launch_title_card():
    global root
    # Create the main window
    root = tk.Tk()
    root.title("MirrorOS")
    root.attributes('-fullscreen', True)  # Fullscreen window
    root.configure(bg="#000000")  # Black background

    # Canvas to draw the logo
    canvas = tk.Canvas(root, width=root.winfo_screenwidth(), height=root.winfo_screenheight(), bg="#000000", highlightthickness=0)
    canvas.pack(fill="both", expand=True)

    # Colors in Hex (for reference)
    colors = [
        "#FF0000", "#FF0080", "#FF8080", "#FF8000", "#00FF80", "#80FF80",
        "#00FF00", "#00FFFF", "#8000FF", "#FF80FF", "#0000FF", "#0080FF",
        "#8080FF", "#FF00FF", "#80FF00", "#80FFFF", "#FFFF00", "#FFFF80"
    ]

    # Draw a logo with shapes (Circles + Lines) in different colors
    width, height = root.winfo_screenwidth(), root.winfo_screenheight()

    # Create the circular logo using different colors
    circle_radius = 100
    for i, color in enumerate(colors):
        x = (i % 6) * (circle_radius * 2) + width // 4
        y = (i // 6) * (circle_radius * 2) + height // 4
        canvas.create_oval(x, y, x + circle_radius * 2, y + circle_radius * 2, fill=color, outline=color)

    # Add some intersecting lines for a geometric logo effect
    for i in range(6):
        x = (i % 6) * (circle_radius * 2) + width // 4 + circle_radius
        y = (i // 6) * (circle_radius * 2) + height // 4 + circle_radius
        canvas.create_line(0, height, x, y, fill=colors[i], width=4)
        canvas.create_line(width, 0, x, y, fill=colors[-(i+1)], width=4)

    # Play startup sound
    play_startup_sound()

    # Bind the force_fullscreen function to keep window in fullscreen when resized or losing focus
    root.bind("<Configure>", force_fullscreen)

    # Keep the title card visible for 5 seconds before closing
    root.after(5000, root.destroy)

    # Start the Tkinter main loop
    root.mainloop()
