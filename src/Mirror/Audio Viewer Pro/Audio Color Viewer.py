import os
import tkinter as tk
import numpy as np
import librosa
from sklearn.cluster import KMeans
import hashlib
import sounddevice as sd
import soundfile as sf
from tkinter import messagebox


# Function to generate colors from audio data
def generate_colors(audio_data):
    audio_hash = hashlib.md5(audio_data).hexdigest()
    np.random.seed(int(audio_hash[:8], 16))
    num_colors = 6
    random_colors = [np.random.randint(0, 256, 3) for _ in range(num_colors)]
    hex_colors = ['#' + ''.join(f'{c:02x}' for c in color) for color in random_colors]
    return hex_colors


# Function to process audio file and get colors
def process_audio(file_path):
    try:
        with open(file_path, "rb") as f:
            audio_data = f.read()
        audio, sample_rate = librosa.load(file_path, sr=16000)
        mel_spectrogram = librosa.feature.melspectrogram(y=audio, sr=sample_rate, n_mels=3, fmax=8000)
        kmeans = KMeans(n_clusters=6)
        kmeans.fit(mel_spectrogram.T)
        colors = generate_colors(audio_data)
        return colors
    except Exception as e:
        messagebox.showerror("Error", f"Failed to process file: {file_path}\n{e}")
        return ["#000000"] * 6


# Function to parse the log file and map prompts to file names
def parse_log(log_path):
    mapping = {}
    current_prompt = None

    try:
        with open(log_path, 'r', encoding="utf-8", errors="replace") as file:
            lines = file.readlines()
            for line in lines:
                if "Generating audio for:" in line:
                    current_prompt = line.split("Generating audio for: ")[1].strip()
                elif "Saved audio to" in line:
                    file_path = line.split("Saved audio to ")[1].strip()
                    file_name = os.path.basename(file_path)
                    if current_prompt:
                        mapping[file_name] = current_prompt
                        current_prompt = None
        return mapping
    except Exception as e:
        messagebox.showerror("Error", f"Failed to parse log file: {e}")
        return {}


# Function to play audio
def play_audio(file_path):
    try:
        data, samplerate = sf.read(file_path)
        sd.play(data, samplerate)
    except Exception as e:
        messagebox.showerror("Error", f"Failed to play audio: {file_path}\n{e}")


# Function to stop audio playback
def stop_audio():
    sd.stop()


# GUI Application Class
class AudioColorApp:
    def __init__(self, root, file_to_prompt, directory):
        self.root = root
        self.file_to_prompt = file_to_prompt
        self.audio_files = list(file_to_prompt.keys())
        self.current_index = 0
        self.directory = directory

        self.root.title("Audio Color Viewer")
        self.root.attributes("-fullscreen", True)
        self.root.bind("<Right>", self.next_sample)
        self.root.bind("<Left>", self.prev_sample)
        self.root.bind("<space>", lambda e: stop_audio())
        self.root.bind("<Escape>", lambda e: self.root.destroy())

        self.file_label = tk.Label(root, text="", font=("Arial", 24), bg="black", fg="white")
        self.file_label.pack(pady=20)

        self.prompt_label = tk.Label(root, text="", font=("Arial", 18), bg="black", fg="white", wraplength=800)
        self.prompt_label.pack(pady=10)

        self.color_frames = [tk.Frame(root, width=100, height=100, bg="#000000") for _ in range(6)]
        for frame in self.color_frames:
            frame.pack(side="left", padx=10, pady=10, expand=True)

        self.update_display()

    def update_display(self):
        file_name = self.audio_files[self.current_index]
        colors = process_audio(os.path.join(self.directory, file_name))
        prompt = self.file_to_prompt.get(file_name, "No prompt available")

        self.file_label.config(text=file_name)
        self.prompt_label.config(text=f"Prompt: {prompt}")
        for frame, color in zip(self.color_frames, colors):
            frame.config(bg=color)

        # Autoplay audio
        play_audio(os.path.join(self.directory, file_name))

    def next_sample(self, event):
        if self.current_index < len(self.audio_files) - 1:
            self.current_index += 1
            self.update_display()

    def prev_sample(self, event):
        if self.current_index > 0:
            self.current_index -= 1
            self.update_display()


# Main Application Loop
if __name__ == "__main__":
    directory = r""  # Update with your directory path
    log_path = "generating.txt"  # Update with your log file path

    file_to_prompt = parse_log(log_path)
    if not file_to_prompt:
        print("No prompts or files found. Please check the log file.")
    else:
        root = tk.Tk()
        app = AudioColorApp(root, file_to_prompt, directory)
        root.mainloop()
