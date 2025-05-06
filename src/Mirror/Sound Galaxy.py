import os
import tkinter as tk
from tkinter import Label, filedialog
import subprocess  # For opening the file browser on Windows
import numpy as np
from sklearn.cluster import KMeans
import random
import sounddevice as sd
import soundfile as sf


# Function to generate colors (Mock Data for Now)
def generate_colors(audio_files):
    # Random color generation for each file
    np.random.seed(42)  # Ensure consistent colors
    return {file: [tuple(np.random.randint(0, 256, 3)) for _ in range(6)] for file in audio_files}


# Galaxy Layout Generator
def generate_galaxy_layout(colors):
    clusters = {}
    kmeans = KMeans(n_clusters=6)  # Group sounds by color palettes
    dominant_colors = [np.mean(colors[file], axis=0) for file in colors]
    cluster_labels = kmeans.fit_predict(dominant_colors)

    for label, file in zip(cluster_labels, colors.keys()):
        if label not in clusters:
            clusters[label] = []
        clusters[label].append(file)

    # Assign spread-out 2D positions
    positions = {}
    cluster_centers = [(random.randint(300, 700), random.randint(300, 500)) for _ in range(6)]
    for cluster_idx, files in clusters.items():
        center_x, center_y = cluster_centers[cluster_idx]
        for file in files:
            positions[file] = (
                center_x + random.randint(-100, 100),
                center_y + random.randint(-100, 100),
            )
    return positions, clusters


# GUI Application Class
class SoundGalaxyApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Sound Galaxy Explorer")
        self.geometry("1000x700")

        # Ask for the directory dynamically
        self.directory = filedialog.askdirectory(title="Select Folder with Samples")
        if not self.directory:
            print("No folder selected. Exiting.")
            self.destroy()
            return

        self.audio_files = [f for f in os.listdir(self.directory) if f.endswith(".wav")]
        if not self.audio_files:
            print("No .wav files found in the selected directory. Exiting.")
            self.destroy()
            return

        self.colors = generate_colors(self.audio_files)
        self.positions, self.clusters = generate_galaxy_layout(self.colors)
        self.scale = 1.0
        self.offset_x, self.offset_y = 0, 0
        self.dragging = False
        self.start_x, self.start_y = 0, 0

        self.canvas = tk.Canvas(self, bg="black", width=1000, height=700)
        self.canvas.pack(fill="both", expand=True)

        # Status label to display currently playing sound
        self.status_label = Label(self, text="Welcome to the Sound Galaxy!", font=("Arial", 14), bg="black", fg="white")
        self.status_label.pack(side="bottom", fill="x")

        # Mouse bindings for pan (right click), open path (middle click), and play sound (left click)
        self.canvas.bind("<Button-2>", self.open_file_path)  # Middle click to open file path
        self.canvas.bind("<Button-3>", self.start_pan)  # Right click to start panning
        self.canvas.bind("<B3-Motion>", self.do_pan)  # Hold right click to pan
        self.canvas.bind("<ButtonRelease-3>", self.stop_pan)  # Release right click to stop panning
        self.canvas.bind("<MouseWheel>", self.zoom)  # Scroll to zoom

        # Key binding for stopping playback
        self.bind("<space>", self.stop_playback)

        self.after(5000, self.show_press_space_message)  # Transition message after 5 seconds

        self.draw_galaxy()

    def draw_galaxy(self):
        self.canvas.delete("all")
        for file, position in self.positions.items():
            x, y = self.apply_transform(*position)
            color = "#{:02x}{:02x}{:02x}".format(*map(int, np.mean(self.colors[file], axis=0)))
            self.canvas.create_oval(
                x - 10, y - 10, x + 10, y + 10, fill=color, outline=color, tags=file
            )
            self.canvas.tag_bind(file, "<Button-1>", lambda e, f=file: self.play_sound(f))  # Left click to play sound
            self.canvas.tag_bind(file, "<Button-2>", lambda e, f=file: self.open_file_path(f))  # Middle click to open path
            self.canvas.create_text(
                x, y + 20, text=f"Sound {self.audio_files.index(file)}", fill="white", font=("Arial", 8)
            )

    def apply_transform(self, x, y):
        """Apply zoom and pan transformations."""
        x = (x + self.offset_x) * self.scale
        y = (y + self.offset_y) * self.scale
        return x, y

    def start_pan(self, event):
        """Start panning the canvas with right click."""
        self.dragging = True
        self.start_x, self.start_y = event.x, event.y

    def do_pan(self, event):
        """Handle panning motion."""
        if self.dragging:
            dx, dy = event.x - self.start_x, event.y - self.start_y
            self.offset_x += dx / self.scale
            self.offset_y += dy / self.scale
            self.start_x, self.start_y = event.x, event.y
            self.draw_galaxy()

    def stop_pan(self, event):
        """Stop panning."""
        self.dragging = False

    def zoom(self, event):
        """Handle zooming in and out."""
        scale_factor = 1.1 if event.delta > 0 else 0.9
        self.scale *= scale_factor
        self.draw_galaxy()

    def play_sound(self, file):
        """Lazy load and play the selected sound, update status label."""
        file_path = os.path.join(self.directory, file)
        try:
            data, samplerate = sf.read(file_path)  # Load only when clicked
            sd.play(data, samplerate)
            self.status_label.config(text=f"Now playing: {file}")
        except Exception as e:
            self.status_label.config(text=f"Error playing {file}: {e}")

    def stop_playback(self, event=None):
        """Stop audio playback."""
        sd.stop()
        self.status_label.config(text="Playback stopped.")

    def show_press_space_message(self):
        """Update the status label to indicate SPACE can stop playback."""
        self.status_label.config(text="Press SPACE to stop playback.")

    def open_file_path(self, event):
        """Open the file's location in the system file browser."""
        # Find the closest object to the click location
        clicked_id = self.canvas.find_closest(event.x, event.y)
        tags = self.canvas.gettags(clicked_id)
        if tags:
            file = tags[0]
            file_path = os.path.join(self.directory, file)
            try:
                subprocess.Popen(f'explorer /select,"{file_path}"')
            except Exception as e:
                self.status_label.config(text=f"Error opening file path: {e}")


# Main Application Loop
if __name__ == "__main__":
    app = SoundGalaxyApp()
    app.mainloop()
