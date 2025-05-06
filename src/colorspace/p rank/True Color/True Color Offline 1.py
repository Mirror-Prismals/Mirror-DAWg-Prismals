import numpy as np
import soundfile as sf
from moviepy.editor import VideoClip, AudioFileClip
from PIL import Image, ImageDraw
import os
import sys
import tkinter as tk
from tkinter import filedialog, messagebox
import colorsys
import threading

def audio_to_hex(audio_path):
    """
    Reads an audio file and converts its samples to 24-bit hex codes.

    Parameters:
        audio_path (str): Path to the input audio file.

    Returns:
        hex_codes (list): List of hex codes representing each audio sample.
        sample_rate (int): Sampling rate of the audio file.
        duration (float): Duration of the audio file in seconds.
        audio_data (numpy.ndarray): Normalized audio data.
    """
    try:
        # Read audio file
        audio_data, sample_rate = sf.read(audio_path)

        # If stereo, take the mean to convert to mono
        if len(audio_data.shape) == 2:
            audio_data = audio_data.mean(axis=1)

        duration = len(audio_data) / sample_rate

        # Normalize audio data to range [-1, 1]
        if np.max(np.abs(audio_data)) == 0:
            audio_data = audio_data
        else:
            audio_data = audio_data / np.max(np.abs(audio_data))

        # Convert to 24-bit signed integers
        max_amplitude = 2**23 - 1
        samples_int = (audio_data * max_amplitude).astype(np.int32)

        # Convert integer samples to hex codes
        hex_codes = [format(sample & 0xFFFFFF, '06x') for sample in samples_int]

        return hex_codes, sample_rate, duration, audio_data
    except Exception as e:
        messagebox.showerror("Error", f"Failed to process audio file.\n{e}")
        return None, None, None, None

def generate_color_strip(hex_codes):
    """
    Converts hex codes to RGB tuples.

    Parameters:
        hex_codes (list): List of hex codes.

    Returns:
        colors (list): List of RGB tuples.
    """
    colors = []
    for idx, hex_code in enumerate(hex_codes):
        try:
            color = tuple(int(hex_code[i:i+2], 16) for i in (0, 2, 4))
            colors.append(color)
        except:
            colors.append((0, 0, 0))  # Fallback to black in case of error
        if (idx + 1) % 100000 == 0:
            print(f"Converted {idx + 1} / {len(hex_codes)} samples to colors")
    return colors

def make_frame(t, colors, sample_rate, frame_rate, resolution, strip_width, max_blocks):
    """
    Generates a single video frame at time t.

    Parameters:
        t (float): Current time in seconds.
        colors (list): List of RGB tuples.
        sample_rate (int): Sampling rate of the audio file.
        frame_rate (int): Frames per second for the video.
        resolution (tuple): Resolution of the video (width, height).
        strip_width (int): Width of each color block in pixels.
        max_blocks (int): Number of color blocks per frame.

    Returns:
        frame (numpy.ndarray): The generated frame as an RGB array.
    """
    frame_number = int(t * frame_rate)
    samples_per_frame = sample_rate / frame_rate
    start_idx = int(frame_number * samples_per_frame)
    end_idx = start_idx + max_blocks

    # Get the latest max_blocks samples up to the current frame
    current_strip = colors[max(0, end_idx - max_blocks):end_idx]

    # If not enough samples, pad with black
    if len(current_strip) < max_blocks:
        padding = [(0, 0, 0)] * (max_blocks - len(current_strip))
        current_strip = padding + current_strip

    # Create an image with the current color strip
    img = Image.new('RGB', resolution, color=(0, 0, 0))
    draw = ImageDraw.Draw(img)

    for i, color in enumerate(current_strip):
        x0 = i * strip_width
        y0 = 0
        x1 = x0 + strip_width
        y1 = resolution[1]
        draw.rectangle([x0, y0, x1, y1], fill=color)

    return np.array(img)

def generate_video_stream(colors, sample_rate, duration, output_video, frame_rate=60, resolution=(1920, 1080), strip_width=2):
    """
    Generates a video from colors synchronized with the audio using a scrolling color strip.

    Parameters:
        colors (list): List of RGB tuples representing each audio sample.
        sample_rate (int): Sampling rate of the audio file.
        duration (float): Duration of the audio file in seconds.
        output_video (str): Path to the output video file.
        frame_rate (int): Frames per second for the video.
        resolution (tuple): Resolution of the video (width, height).
        strip_width (int): Width of each color block in pixels.
    """
    try:
        # Calculate the number of color blocks that fit in the frame width
        frame_width, frame_height = resolution
        max_blocks = frame_width // strip_width

        print("Creating video clip with streaming frames...")

        # Define a VideoClip with a frame generator
        def frame_generator(t):
            return make_frame(t, colors, sample_rate, frame_rate, resolution, strip_width, max_blocks)

        # Create the video clip
        clip = VideoClip(make_frame=frame_generator, duration=duration)

        # Load audio
        audio_clip = AudioFileClip(audio_path)

        # Set audio to the video
        clip = clip.set_audio(audio_clip)

        # Write the video file
        print(f"Writing the video file to {output_video}...")
        clip.write_videofile(output_video, codec='libx264', audio_codec='aac', fps=frame_rate)

        print("Video creation completed.")
        messagebox.showinfo("Success", f"Video has been saved as {output_video}")
    except Exception as e:
        messagebox.showerror("Error", f"Failed to generate video.\n{e}")

def start_processing(audio_path, output_video, frame_rate, resolution, strip_width):
    hex_codes, sample_rate, duration, audio_data = audio_to_hex(audio_path)
    if hex_codes:
        print("Converting hex codes to colors...")
        colors = generate_color_strip(hex_codes)
        print("Starting video generation...")
        generate_video_stream(colors, sample_rate, duration, output_video, frame_rate, resolution, strip_width)

def select_audio_file():
    global audio_path
    audio_path = filedialog.askopenfilename(
        title="Select an Audio File",
        filetypes=[("Audio Files", "*.wav *.flac *.mp3 *.aiff *.aac"), ("All Files", "*.*")]
    )
    if audio_path:
        output_video = filedialog.asksaveasfilename(
            title="Save Video As",
            defaultextension=".mp4",
            filetypes=[("MP4 Video", "*.mp4")]
        )
        if output_video:
            # Optional: Allow user to set frame rate and resolution
            # For simplicity, we'll use default values
            frame_rate = 60  # Default frame rate
            resolution = (1920, 1080)  # Default resolution (Full HD)
            strip_width = 2  # Width of each color block in pixels

            # Run processing in a separate thread to keep the GUI responsive
            processing_thread = threading.Thread(
                target=start_processing,
                args=(audio_path, output_video, frame_rate, resolution, strip_width)
            )
            processing_thread.start()
        else:
            messagebox.showwarning("Warning", "No output video file selected.")
    else:
        messagebox.showwarning("Warning", "No audio file selected.")

# Initialize Tkinter root
root = tk.Tk()
root.title("Audio to Video Visualizer")
root.geometry("400x200")

# Add a button to select audio file
select_button = tk.Button(root, text="Select Audio File", command=select_audio_file, font=("Helvetica", 16))
select_button.pack(expand=True)

# Run the Tkinter event loop
root.mainloop()
