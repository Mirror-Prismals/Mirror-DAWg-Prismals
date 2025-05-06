import tkinter as tk
from tkinter import filedialog, messagebox
import librosa
import soundfile as sf
import numpy as np

def select_input_file():
    root = tk.Tk()
    root.withdraw()
    file_path = filedialog.askopenfilename(
        title="Select Input Audio File",
        filetypes=[("Audio Files", "*.wav *.mp3 *.flac *.aac")]
    )
    return file_path

def select_output_file():
    root = tk.Tk()
    root.withdraw()
    file_path = filedialog.asksaveasfilename(
        title="Save Modified Audio As",
        defaultextension=".wav",
        filetypes=[("WAV Files", "*.wav")]
    )
    return file_path

def load_audio(file_path):
    y, sr = librosa.load(file_path, sr=None)
    return y, sr

def detect_beats(y, sr):
    tempo, beat_frames = librosa.beat.beat_track(y=y, sr=sr)
    beat_times = librosa.frames_to_time(beat_frames, sr=sr)
    return beat_times

def switch_beats(y, sr, beat_times):
    beats_per_measure = 4
    if len(beat_times) < beats_per_measure:
        return y
    beat_times = np.append(beat_times, len(y)/sr)
    modified_audio = []
    for i in range(0, len(beat_times) - 1, beats_per_measure):
        measure_end = i + beats_per_measure
        if measure_end > len(beat_times) - 1:
            measure_end = len(beat_times) - 1
        current_measure_beats = beat_times[i:measure_end+1]
        segments = []
        for j in range(len(current_measure_beats)-1):
            start = int(current_measure_beats[j] * sr)
            end = int(current_measure_beats[j+1] * sr)
            segment = y[start:end]
            segments.append(segment)
        if len(segments) >= 4:
            segments[1], segments[3] = segments[3], segments[1]
        for segment in segments:
            modified_audio.append(segment)
    if modified_audio:
        modified_audio = np.concatenate(modified_audio)
    else:
        modified_audio = y
    return modified_audio

def save_audio(y, sr, output_path):
    sf.write(output_path, y, sr)
    print(f"Modified audio saved to {output_path}")

def main():
    input_file = select_input_file()
    if not input_file:
        messagebox.showerror("Error", "No input file selected.")
        return
    output_file = select_output_file()
    if not output_file:
        messagebox.showerror("Error", "No output file selected.")
        return
    try:
        y, sr = load_audio(input_file)
        beat_times = detect_beats(y, sr)
        if len(beat_times) < 4:
            messagebox.showerror("Error", "Not enough beats detected to perform swapping.")
            return
        modified_y = switch_beats(y, sr, beat_times)
        save_audio(modified_y, sr, output_file)
        messagebox.showinfo("Success", "Modified audio has been saved successfully.")
    except Exception as e:
        messagebox.showerror("Error", f"An error occurred: {e}")

if __name__ == "__main__":
    main()
