import tkinter as tk
from tkinter import messagebox
import random
import pygame
import numpy as np
import time
import json

# Chord and Progression Classes
class Chord:
    def __init__(self, name, notes):
        self.name = name
        self.notes = notes

class Progression:
    def __init__(self, chords=None):
        if chords is None:
            chords = []
        self.chords = chords

    def add_chord(self, chord):
        self.chords.append(chord)

    def remove_chord(self, index):
        if 0 <= index < len(self.chords):
            del self.chords[index]

    def clear(self):
        self.chords = []

    def randomize(self, num_chords):
        self.chords = [random.choice(all_chords) for _ in range(num_chords)]

    def save(self, filename):
        chords_data = [{'name': chord.name, 'notes': chord.notes} for chord in self.chords]
        with open(filename, 'w') as f:
            json.dump(chords_data, f)

# List of all possible chords
all_chords = [
    Chord("C", [60, 64, 67]),
    Chord("Dm", [62, 65, 69]),
    Chord("Em", [64, 67, 71]),
    Chord("F", [65, 69, 72]),
    Chord("G", [67, 71, 74]),
    Chord("Am", [69, 72, 76]),
    # Inverted chords
    Chord("Cm", [60, 63, 67]),
    Chord("D", [62, 66, 69]),
    Chord("E", [64, 68, 71]),
    Chord("Fm", [65, 68, 72]),
    Chord("Gm", [67, 70, 74]),
    Chord("A", [69, 73, 76]),
    # Major 7ths
    Chord("Cmaj7", [60, 64, 67, 71]),
    Chord("DmMaj7", [62, 65, 69, 73]),
    Chord("EmMaj7", [64, 67, 71, 75]),
    Chord("Fmaj7", [65, 69, 72, 76]),
    Chord("Gmaj7", [67, 71, 74, 78]),
    Chord("AmMaj7", [69, 72, 76, 80]),
    # Minor 7ths
    Chord("Cmin7", [60, 63, 67, 70]),
    Chord("Dm7", [62, 65, 69, 72]),
    Chord("Em7", [64, 67, 71, 74]),
    Chord("Fmin7", [65, 68, 72, 75]),
    Chord("Gmin7", [67, 70, 74, 77]),
    Chord("Am7", [69, 72, 76, 79]),
    # 9ths
    Chord("C9", [60, 64, 67, 71, 74]),
    Chord("Dm9", [62, 65, 69, 72, 76]),
    Chord("Em9", [64, 67, 71, 74, 78]),
    Chord("F9", [65, 69, 72, 76, 79]),
    Chord("G9", [67, 71, 74, 78, 81]),
    Chord("Am9", [69, 72, 76, 79, 83]),
    # 13ths
    Chord("C13", [60, 64, 67, 71, 74, 77, 81]),
    Chord("Dm13", [62, 65, 69, 72, 76, 79, 83]),
    Chord("Em13", [64, 67, 71, 74, 78, 81, 85]),
    Chord("F13", [65, 69, 72, 76, 79, 83, 86]),
    Chord("G13", [67, 71, 74, 78, 81, 85, 88]),
    Chord("Am13", [69, 72, 76, 79, 83, 86, 90]),
]

# Pygame Audio Playback Functions
def play_chord(notes, duration=1.0):
    pygame.mixer.init()
    sample_rate = 44100
    pygame.mixer.set_num_channels(8)

    sounds = []
    for note in notes:
        frequency = 440.0 * (2.0 ** ((note - 69) / 12.0))
        sound = pygame.sndarray.make_sound(generate_tone(frequency, duration, sample_rate))
        sounds.append(sound)

    for sound in sounds:
        sound.play(-1)

    time.sleep(duration)
    
    for sound in sounds:
        sound.stop()
    
    pygame.mixer.quit()

def generate_tone(frequency, duration, sample_rate):
    n_samples = int(round(duration * sample_rate))
    t = np.linspace(0, duration, n_samples, False)
    wave = 0.5 * 32767 * np.sin(2 * np.pi * frequency * t)
    wave = wave.astype(np.int16)
    return np.column_stack((wave, wave))

# Tkinter GUI Application
class ChordVisualizerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Chord Visualizer")
        self.progression = Progression()

        self.chord_frame = tk.Frame(self.root)
        self.chord_frame.pack(pady=20)

        self.control_frame = tk.Frame(self.root)
        self.control_frame.pack(pady=20)

        self.add_control_buttons()
        self.update_chord_display()

    def add_control_buttons(self):
        tk.Button(self.control_frame, text="Add Random Chord", command=self.add_random_chord).pack(side=tk.LEFT, padx=10)
        tk.Button(self.control_frame, text="Clear Progression", command=self.clear_progression).pack(side=tk.LEFT, padx=10)
        tk.Button(self.control_frame, text="Play Progression", command=self.play_progression).pack(side=tk.LEFT, padx=10)
        tk.Button(self.control_frame, text="Save Progression", command=self.save_progression).pack(side=tk.LEFT, padx=10)

    def update_chord_display(self):
        for widget in self.chord_frame.winfo_children():
            widget.destroy()

        for chord in self.progression.chords:
            tk.Label(self.chord_frame, text=chord.name).pack(side=tk.LEFT, padx=10)

    def add_random_chord(self):
        self.progression.add_chord(random.choice(all_chords))
        self.update_chord_display()

    def clear_progression(self):
        self.progression.clear()
        self.update_chord_display()

    def play_progression(self):
        for chord in self.progression.chords:
            play_chord(chord.notes)

    def save_progression(self):
        filename = "saved_progression.json"
        self.progression.save(filename)
        messagebox.showinfo("Saved", f"Progression saved to {filename}")

if __name__ == "__main__":
    root = tk.Tk()
    app = ChordVisualizerApp(root)
    root.mainloop()
