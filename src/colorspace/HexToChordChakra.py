import numpy as np
import simpleaudio as sa
import tkinter as tk
from tkinter import messagebox
import random
import threading

# Frequencies for chakras (out of tune notes)
chakra_frequencies = {
    'A': 432,   # Root Chakra
    'B': 480,   # Sacral Chakra
    'C': 528,   # Solar Plexus Chakra
    'D': 594,   # Heart Chakra
    'E': 672,   # Throat Chakra
    'F': 720,   # Third Eye Chakra
    'G': 768    # Crown Chakra
}

# Define key colors and their corresponding chakra notes
key_colors = {
    'red': 'G',
    'lime': 'C',
    'blue': 'E',
    'yellow': 'C',
    'cyan': 'E',
    'magenta': 'F',
    'maroon': 'G',
    'olive': 'C',
    'green': 'C',
    'purple': 'F',
    'teal': 'E',
    'navy': 'E'
}

# Function to generate a sine wave for a given frequency
def generate_sine_wave(frequency, duration, sample_rate=44100, amplitude=32767):
    t = np.linspace(0, duration, int(sample_rate * duration), False)
    wave = np.sin(frequency * t * 2 * np.pi)
    audio = (wave * amplitude).astype(np.int16)
    return audio

# Function to apply a massive reverb effect
def apply_reverb(audio, sample_rate, decay=1.01, delay=0.05, feedback_loops=50):
    delay_samples = int(sample_rate * delay)
    reverb_audio = np.zeros(len(audio) + delay_samples * feedback_loops)  # Extending for long reverb tail
    reverb_audio[:len(audio)] = audio
    
    max_amplitude = np.iinfo(np.int16).max
    
    for i in range(delay_samples, len(reverb_audio)):
        reverb_audio[i] += decay * reverb_audio[i - delay_samples]
        # Prevent clipping
        if reverb_audio[i] > max_amplitude:
            reverb_audio[i] = max_amplitude
        elif reverb_audio[i] < -max_amplitude:
            reverb_audio[i] = -max_amplitude
    
    return reverb_audio.astype(np.int16)

# Function to play an ambient soundscape
def play_ambient_soundscape(colors, duration=2, decay=1.01):
    sample_rate = 44100
    combined_audio_length = int(sample_rate * (duration + duration * 10))  # Add reverb tail length
    combined_audio = np.zeros(combined_audio_length)

    for color in colors:
        if color in key_colors:
            key_note = key_colors[color]
            frequency = chakra_frequencies[key_note]
            wave = generate_sine_wave(frequency, duration, sample_rate)
            wave_with_reverb = apply_reverb(wave, sample_rate, decay=decay, delay=0.05)
            combined_audio[:len(wave_with_reverb)] += wave_with_reverb

    combined_audio = (combined_audio / len(colors)).astype(np.int16)
    wave_object = sa.WaveObject(combined_audio, 1, 2, sample_rate)
    wave_object.play()

# GUI for color entry and playing soundscape
def create_gui():
    def add_color():
        color_name = color_entry.get().lower()
        if color_name in key_colors:
            color_listbox.insert(tk.END, color_name)
            color_entry.delete(0, tk.END)
        else:
            messagebox.showerror("Invalid Color", f"'{color_name}' is not a valid color name.")

    def random_colors():
        color_listbox.delete(0, tk.END)
        colors = random.sample(list(key_colors.keys()), 3)
        for color in colors:
            color_listbox.insert(tk.END, color)

    def play_soundscape():
        colors = [color_listbox.get(i) for i in range(color_listbox.size())]
        if colors:
            decay = reverb_slider.get() / 100.0  # Convert slider value to decay factor
            threading.Thread(target=play_ambient_soundscape, args=(colors, 2, decay)).start()
        else:
            messagebox.showerror("No Colors", "Please add at least one color.")

    root = tk.Tk()
    root.title("Ambient Soundscape Generator")

    tk.Label(root, text="Enter Color Name:").grid(row=0, column=0, padx=10, pady=10)
    color_entry = tk.Entry(root)
    color_entry.grid(row=0, column=1, padx=10, pady=10)

    add_button = tk.Button(root, text="Add Color", command=add_color)
    add_button.grid(row=0, column=2, padx=10, pady=10)

    random_button = tk.Button(root, text="Random Colors", command=random_colors)
    random_button.grid(row=1, column=0, columnspan=3, padx=10, pady=10)

    color_listbox = tk.Listbox(root)
    color_listbox.grid(row=2, column=0, columnspan=3, padx=10, pady=10)

    tk.Label(root, text="Reverb Decay:").grid(row=3, column=0, padx=10, pady=10)
    reverb_slider = tk.Scale(root, from_=0, to_=100, orient=tk.HORIZONTAL)
    reverb_slider.set(99)  # Default value for massive reverb
    reverb_slider.grid(row=3, column=1, columnspan=2, padx=10, pady=10)

    play_button = tk.Button(root, text="Play Soundscape", command=play_soundscape)
    play_button.grid(row=4, column=0, columnspan=3, padx=10, pady=10)

    root.mainloop()

if __name__ == "__main__":
    create_gui()
