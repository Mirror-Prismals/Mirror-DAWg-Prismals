import numpy as np
import soundfile as sf
from moviepy.editor import VideoClip
from PIL import Image, ImageDraw
import os
import tkinter as tk
from tkinter import filedialog, messagebox
import threading

def audio_to_hex(audio_path):
    """
    Reads an audio file and converts its samples to 24-bit hex codes.
    """
    try:
        # Read audio file
        audio_data, sample_rate = sf.read(audio_path)

        # If stereo, take the mean to convert to mono
        if len(audio_data.shape) == 2:
            audio_data = audio_data.mean(axis=1)

        duration = len(audio_data) / sample_rate

        # Normalize audio data to range [-1, 1]
        if np.max(np.abs(audio_data)) != 0:
            audio_data = audio_data / np.max(np.abs(audio_data))

        # Convert to 24-bit signed integers
        max_amplitude = 2**23 - 1
        samples_int = (audio_data * max_amplitude).astype(np.int32)

        # Convert integer samples to hex codes
        hex_codes = [format(sample & 0xFFFFFF, '06x') for sample in samples_int]

        return hex_codes, sample_rate, duration
    except Exception as e:
        messagebox.showerror("Error", f"Failed to process audio file.\n{e}")
        return None, None, None

def generate_color_strip(hex_codes):
    """
    Converts hex codes to RGB tuples.
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

def make_frame(t, colors, samples_per_frame, frame_number, resolution, strip_width, max_blocks):
    """
    Generates a single video frame at time t.
    """
    start_idx = frame_number * samples_per_frame
    end_idx = start_idx + max_blocks

    # Get the samples for the current frame
    current_strip = colors[start_idx:end_idx]

    # If not enough samples, pad with black
    if len(current_strip) < max_blocks:
        padding = [(0, 0, 0)] * (max_blocks - len(current_strip))
        current_strip += padding

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

def generate_video_stream(audio_path, colors, sample_rate, duration, output_video, frame_rate=60, resolution=(3840, 2160), strip_width=8):
    """
    Generates a video from colors synchronized with the audio using a scrolling color strip.
    """
    try:
        # Calculate the number of color blocks that fit in the frame width
        frame_width, frame_height = resolution
        max_blocks = frame_width // strip_width

        # Calculate samples per frame
        samples_per_frame = max_blocks

        # Total number of frames needed
        total_frames = int(np.ceil(len(colors) / samples_per_frame))

        print("Creating video clip with streaming frames...")

        # Define a VideoClip with a frame generator
        def frame_generator(t):
            frame_number = int(t * frame_rate)
            return make_frame(t, colors, samples_per_frame, frame_number, resolution, strip_width, max_blocks)

        # Create the video clip
        clip = VideoClip(make_frame=frame_generator, duration=duration)

        # Save parameters to metadata file
        base_name, ext = os.path.splitext(output_video)
        metadata_file = f"{base_name}_metadata.txt"
        with open(metadata_file, 'w') as f:
            f.write(f"sample_rate={int(sample_rate)}\n")
            f.write(f"strip_width={strip_width}\n")
            f.write(f"frame_rate={frame_rate}\n")
            f.write(f"samples_per_frame={samples_per_frame}\n")

        # Write the video file using a lossless codec
        print(f"Writing the video file to {output_video}...")
        clip.write_videofile(
            output_video,
            codec='libx264rgb',
            audio_codec='none',
            fps=frame_rate,
            preset='ultrafast',
            ffmpeg_params=['-pix_fmt', 'rgb24', '-crf', '0']
        )

        print("Video creation completed.")
        messagebox.showinfo("Success", f"Video has been saved as {output_video}")
    except Exception as e:
        messagebox.showerror("Error", f"Failed to generate video.\n{e}")

def start_processing(audio_path, output_video, frame_rate, resolution, strip_width):
    hex_codes, sample_rate, duration = audio_to_hex(audio_path)
    if hex_codes:
        print("Converting hex codes to colors...")
        colors = generate_color_strip(hex_codes)
        print("Starting video generation...")
        generate_video_stream(audio_path, colors, sample_rate, duration, output_video, frame_rate, resolution, strip_width)

def select_audio_file():
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
            resolution = (3840, 2160)  # Use 4K resolution
            strip_width = 8  # Width of each color block in pixels

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
