import cv2
import numpy as np
import soundfile as sf
import tkinter as tk
from tkinter import filedialog, messagebox
import threading
import os

def read_metadata(video_path):
    metadata_file = os.path.splitext(video_path)[0] + "_metadata.txt"
    if os.path.exists(metadata_file):
        params = {}
        with open(metadata_file, 'r') as f:
            for line in f:
                key, value = line.strip().split('=')
                params[key] = int(value)
        return params
    else:
        return None

def read_video_extract_samples(video_path, strip_width, samples_per_frame):
    # Open the video file
    cap = cv2.VideoCapture(video_path)

    if not cap.isOpened():
        print("Error opening video file.")
        return None

    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))

    print(f"Total frames: {total_frames}")
    print(f"Samples per frame: {samples_per_frame}")

    samples = []

    frame_count = 0
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break

        # Ensure the frame is in RGB format
        frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        frame_width = frame.shape[1]
        # Recalculate samples per frame based on frame width and strip width
        samples_in_frame = frame_width // strip_width

        # For each strip along the horizontal axis
        for i in range(samples_in_frame):
            x = i * strip_width + strip_width // 2
            y = frame.shape[0] // 2  # Middle row

            # Get the color at (x, y)
            R, G, B = frame[y, x]

            # Combine RGB to get 24-bit integer
            sample_int = (R << 16) | (G << 8) | B

            # Convert to signed 24-bit integer
            if sample_int & 0x800000:
                sample_int -= 0x1000000  # Sign extension for negative values

            # Store the integer sample directly
            samples.append(sample_int)

        frame_count += 1
        if frame_count % 100 == 0:
            print(f"Processed {frame_count}/{total_frames} frames")

    cap.release()
    samples = np.array(samples, dtype=np.int32)

    # Normalize samples to [-1, 1]
    max_amplitude = 2**23 - 1
    samples = samples / max_amplitude

    return samples

def start_decoding(video_path, output_audio_path):
    # Read metadata
    params = read_metadata(video_path)
    if params:
        sample_rate = params.get('sample_rate', None)
        strip_width = params.get('strip_width', None)
        samples_per_frame = params.get('samples_per_frame', None)
    else:
        # Prompt user to input parameters
        def prompt_params():
            sample_rate_input = sample_rate_var.get()
            strip_width_input = strip_width_var.get()
            if not sample_rate_input or not strip_width_input:
                messagebox.showwarning("Warning", "Sample rate and strip width are required.")
                return
            try:
                sample_rate = int(sample_rate_input)
                strip_width = int(strip_width_input)
                decoding_thread = threading.Thread(
                    target=decode_samples,
                    args=(video_path, output_audio_path, sample_rate, strip_width)
                )
                decoding_thread.start()
                param_window.destroy()
            except ValueError:
                messagebox.showerror("Error", "Invalid sample rate or strip width entered.")
                return

        # Create a new window for parameter input
        param_window = tk.Toplevel(root)
        param_window.title("Enter Parameters")

        tk.Label(param_window, text="Sample Rate (e.g., 44100):").grid(row=0, column=0)
        sample_rate_var = tk.StringVar()
        tk.Entry(param_window, textvariable=sample_rate_var).grid(row=0, column=1)

        tk.Label(param_window, text="Strip Width (e.g., 8):").grid(row=1, column=0)
        strip_width_var = tk.StringVar()
        tk.Entry(param_window, textvariable=strip_width_var).grid(row=1, column=1)

        tk.Button(param_window, text="OK", command=prompt_params).grid(row=2, column=0, columnspan=2)

        return
    # If parameters are available, start decoding
    decoding_thread = threading.Thread(
        target=decode_samples,
        args=(video_path, output_audio_path, sample_rate, strip_width, samples_per_frame)
    )
    decoding_thread.start()

def decode_samples(video_path, output_audio_path, sample_rate, strip_width, samples_per_frame):
    samples = read_video_extract_samples(video_path, strip_width=strip_width, samples_per_frame=samples_per_frame)
    if samples is not None:
        print(f"Writing audio file to {output_audio_path}")
        try:
            sf.write(output_audio_path, samples, sample_rate)
            print("Audio reconstruction completed.")
            # Schedule the success message in the main thread
            root.after(0, lambda: messagebox.showinfo("Success", f"Audio has been reconstructed and saved to {output_audio_path}"))
        except Exception as e:
            print(f"Failed to write audio file.\n{e}")
            # Schedule the error message in the main thread
            root.after(0, lambda: messagebox.showerror("Error", f"Failed to write audio file.\n{e}"))
    else:
        print("Failed to reconstruct audio.")
        # Schedule the error message in the main thread
        root.after(0, lambda: messagebox.showerror("Error", "Failed to reconstruct audio."))

def select_video_file():
    video_path = filedialog.askopenfilename(
        title="Select a Video File",
        filetypes=[("Video Files", "*.mp4 *.avi *.mov *.mkv"), ("All Files", "*.*")]
    )
    if video_path:
        output_audio_path = filedialog.asksaveasfilename(
            title="Save Audio As",
            defaultextension=".wav",
            filetypes=[("WAV Audio", "*.wav")]
        )
        if output_audio_path:
            # Start decoding process
            start_decoding(video_path, output_audio_path)
        else:
            messagebox.showwarning("Warning", "No output audio file selected.")
    else:
        messagebox.showwarning("Warning", "No video file selected.")

# Initialize Tkinter root
root = tk.Tk()
root.title("Video to Audio Decoder")
root.geometry("400x200")

# Add a button to select video file
select_button = tk.Button(root, text="Select Video File", command=select_video_file, font=("Helvetica", 16))
select_button.pack(expand=True)

# Run the Tkinter event loop
root.mainloop()
