"""
This script converts an audio file into a silent video and then reconstructs the audio from the video.
It works by mapping each audio sample value to a hex color code and creating a visual representation of the audio data.
This is a clever and cool way to encode audio information visually.

Below is an extensive commentary and lesson about how the script works, provided in the form of Python comments.
"""

import numpy as np
import cv2
import soundfile as sf
from moviepy.editor import VideoClip
from PIL import Image, ImageDraw
import os
import tkinter as tk
from tkinter import filedialog, messagebox
import threading

# --- Function Definitions ---

def audio_to_hex(audio_path):
    """
    Reads an audio file and converts its samples to 24-bit hex codes.

    Parameters:
    - audio_path: Path to the input audio file.

    Returns:
    - hex_codes: List of hex color codes corresponding to audio samples.
    - sample_rate: The sample rate of the audio file.
    - duration: Duration of the audio file in seconds.
    """
    try:
        # Read audio file using soundfile library.
        audio_data, sample_rate = sf.read(audio_path)

        # Check if audio is stereo (2 channels).
        if len(audio_data.shape) == 2:
            # Convert stereo to mono by taking the mean of the two channels.
            audio_data = audio_data.mean(axis=1)

        # Calculate duration of the audio.
        duration = len(audio_data) / sample_rate

        # Normalize audio data to range [-1, 1].
        if np.max(np.abs(audio_data)) != 0:
            audio_data = audio_data / np.max(np.abs(audio_data))

        # Convert normalized audio samples to 24-bit signed integers.
        max_amplitude = 2**23 - 1  # Maximum value for 24-bit audio.
        samples_int = (audio_data * max_amplitude).astype(np.int32)

        # Convert integer samples to hex codes (6-digit hex strings).
        hex_codes = [format(sample & 0xFFFFFF, '06x') for sample in samples_int]

        return hex_codes, sample_rate, duration
    except Exception as e:
        # Show an error message if processing fails.
        messagebox.showerror("Error", f"Failed to process audio file.\n{e}")
        return None, None, None

def generate_color_strip(hex_codes):
    """
    Converts hex codes to RGB tuples to create a color strip.

    Parameters:
    - hex_codes: List of hex color codes.

    Returns:
    - colors: List of RGB tuples.
    """
    colors = []
    for idx, hex_code in enumerate(hex_codes):
        try:
            # Convert hex code to an RGB tuple.
            color = tuple(int(hex_code[i:i+2], 16) for i in (0, 2, 4))
            colors.append(color)
        except:
            # Fallback to black color in case of error.
            colors.append((0, 0, 0))
        # Print progress every 100,000 samples.
        if (idx + 1) % 100000 == 0:
            print(f"Converted {idx + 1} / {len(hex_codes)} samples to colors")
    return colors

def make_frame(t, colors, samples_per_frame, frame_number, resolution, strip_width, max_blocks):
    """
    Generates a single video frame at time t.

    Parameters:
    - t: Current time in seconds.
    - colors: List of RGB color tuples.
    - samples_per_frame: Number of samples to display per frame.
    - frame_number: Current frame number.
    - resolution: Resolution of the video frame.
    - strip_width: Width of each color strip in pixels.
    - max_blocks: Maximum number of color blocks per frame.

    Returns:
    - A NumPy array representing the image frame.
    """
    # Calculate start and end indices for the current frame's samples.
    start_idx = frame_number * samples_per_frame
    end_idx = start_idx + max_blocks

    # Get the samples for the current frame.
    current_strip = colors[start_idx:end_idx]

    # If not enough samples, pad with black colors.
    if len(current_strip) < max_blocks:
        padding = [(0, 0, 0)] * (max_blocks - len(current_strip))
        current_strip += padding

    # Create a new image for the frame.
    img = Image.new('RGB', resolution, color=(0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Draw rectangles for each color strip.
    for i, color in enumerate(current_strip):
        x0 = i * strip_width
        y0 = 0
        x1 = x0 + strip_width
        y1 = resolution[1]
        draw.rectangle([x0, y0, x1, y1], fill=color)

    # Convert the PIL image to a NumPy array.
    return np.array(img)

def generate_video_stream(audio_path, colors, sample_rate, duration, output_video, frame_rate=60, resolution=(3840, 2160), strip_width=8):
    """
    Generates a video from colors synchronized with the audio using a color strip.

    Parameters:
    - audio_path: Path to the input audio file.
    - colors: List of RGB color tuples.
    - sample_rate: The sample rate of the audio file.
    - duration: Duration of the audio file in seconds.
    - output_video: Path to the output video file.
    - frame_rate: Frame rate of the output video.
    - resolution: Resolution of the output video.
    - strip_width: Width of each color strip in pixels.
    """
    try:
        # Calculate frame dimensions and maximum number of color blocks per frame.
        frame_width, frame_height = resolution
        max_blocks = frame_width // strip_width

        # Calculate the number of samples displayed per frame.
        samples_per_frame = max_blocks

        # Calculate the total number of frames needed for the video.
        total_frames = int(np.ceil(len(colors) / samples_per_frame))

        print("Creating video clip with streaming frames...")

        # Define a function to generate frames for the video clip.
        def frame_generator(t):
            # Calculate the current frame number based on time and frame rate.
            frame_number = int(t * frame_rate)
            return make_frame(t, colors, samples_per_frame, frame_number, resolution, strip_width, max_blocks)

        # Create the video clip using the frame generator function.
        clip = VideoClip(make_frame=frame_generator, duration=duration)

        # Save parameters to a metadata file for later use in decoding.
        base_name, ext = os.path.splitext(output_video)
        metadata_file = f"{base_name}_metadata.txt"
        with open(metadata_file, 'w') as f:
            f.write(f"sample_rate={int(sample_rate)}\n")
            f.write(f"strip_width={strip_width}\n")
            f.write(f"samples_per_frame={samples_per_frame}\n")

        # Write the video file using a lossless codec to preserve color information.
        print(f"Writing the video file to {output_video}...")
        clip.write_videofile(
            output_video,
            codec='libx264rgb',         # Use RGB color space to avoid color compression.
            audio_codec='none',         # No audio in the output video.
            fps=frame_rate,
            preset='ultrafast',         # Faster encoding.
            ffmpeg_params=['-pix_fmt', 'rgb24', '-crf', '0']  # Lossless compression settings.
        )

        print("Video creation completed.")
        messagebox.showinfo("Success", f"Video has been saved as {output_video}")
    except Exception as e:
        # Show an error message if video generation fails.
        messagebox.showerror("Error", f"Failed to generate video.\n{e}")

def start_encoding(audio_path, output_video, frame_rate, resolution, strip_width):
    """
    Starts the encoding process by converting audio to hex codes, then to colors, and finally to video.

    Parameters:
    - audio_path: Path to the input audio file.
    - output_video: Path to the output video file.
    - frame_rate: Frame rate of the output video.
    - resolution: Resolution of the output video.
    - strip_width: Width of each color strip in pixels.
    """
    # Convert audio samples to hex codes.
    hex_codes, sample_rate, duration = audio_to_hex(audio_path)
    if hex_codes:
        print("Converting hex codes to colors...")
        # Convert hex codes to RGB colors.
        colors = generate_color_strip(hex_codes)
        print("Starting video generation...")
        # Generate the video stream from the colors.
        generate_video_stream(audio_path, colors, sample_rate, duration, output_video, frame_rate, resolution, strip_width)

def read_metadata(video_path):
    """
    Reads metadata from a metadata file associated with the video file.

    Parameters:
    - video_path: Path to the input video file.

    Returns:
    - params: Dictionary containing metadata parameters.
    """
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
    """
    Reads the video file and extracts audio samples by analyzing the color strips.

    Parameters:
    - video_path: Path to the input video file.
    - strip_width: Width of each color strip in pixels.
    - samples_per_frame: Number of samples per video frame.

    Returns:
    - samples: NumPy array of reconstructed audio samples.
    """
    # Open the video file using OpenCV.
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

        # Convert the frame from BGR (OpenCV default) to RGB color space.
        frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        frame_width = frame.shape[1]
        # Recalculate samples per frame based on frame width and strip width.
        samples_in_frame = frame_width // strip_width

        # For each color strip along the horizontal axis.
        for i in range(samples_in_frame):
            x = i * strip_width + strip_width // 2  # Center of the strip.
            y = frame.shape[0] // 2  # Middle row of the frame.

            # Get the color at position (x, y).
            R, G, B = frame[y, x]

            # Combine RGB values to get a 24-bit integer sample.
            sample_int = (R << 16) | (G << 8) | B

            # Convert to signed 24-bit integer.
            if sample_int & 0x800000:
                sample_int -= 0x1000000  # Adjust for negative values.

            # Append the integer sample to the list.
            samples.append(sample_int)

        frame_count += 1
        if frame_count % 100 == 0:
            print(f"Processed {frame_count}/{total_frames} frames")

    cap.release()

    # Convert the samples list to a NumPy array.
    samples = np.array(samples, dtype=np.int32)

    # Normalize samples to range [-1, 1] based on 24-bit audio.
    max_amplitude = 2**23 - 1
    samples = samples / max_amplitude

    return samples

def start_decoding(video_path, output_audio_path):
    """
    Starts the decoding process by extracting samples from the video and reconstructing the audio.

    Parameters:
    - video_path: Path to the input video file.
    - output_audio_path: Path to the output audio file.
    """
    # Read metadata to get parameters needed for decoding.
    params = read_metadata(video_path)
    if params:
        sample_rate = params.get('sample_rate', None)
        strip_width = params.get('strip_width', None)
        samples_per_frame = params.get('samples_per_frame', None)
    else:
        # If metadata is not available, prompt the user to input parameters.
        def prompt_params():
            sample_rate_input = sample_rate_var.get()
            strip_width_input = strip_width_var.get()
            if not sample_rate_input or not strip_width_input:
                messagebox.showwarning("Warning", "Sample rate and strip width are required.")
                return
            try:
                sample_rate = int(sample_rate_input)
                strip_width = int(strip_width_input)
                # Start decoding in a separate thread.
                decoding_thread = threading.Thread(
                    target=decode_samples,
                    args=(video_path, output_audio_path, sample_rate, strip_width)
                )
                decoding_thread.start()
                param_window.destroy()
            except ValueError:
                messagebox.showerror("Error", "Invalid sample rate or strip width entered.")
                return

        # Create a new window for parameter input.
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

    # If parameters are available, start decoding in a separate thread.
    decoding_thread = threading.Thread(
        target=decode_samples,
        args=(video_path, output_audio_path, sample_rate, strip_width, samples_per_frame)
    )
    decoding_thread.start()

def decode_samples(video_path, output_audio_path, sample_rate, strip_width, samples_per_frame):
    """
    Decodes samples from the video and writes the reconstructed audio to a file.

    Parameters:
    - video_path: Path to the input video file.
    - output_audio_path: Path to the output audio file.
    - sample_rate: Sample rate for the output audio.
    - strip_width: Width of each color strip in pixels.
    - samples_per_frame: Number of samples per video frame.
    """
    # Extract samples from the video.
    samples = read_video_extract_samples(video_path, strip_width=strip_width, samples_per_frame=samples_per_frame)
    if samples is not None:
        print(f"Writing audio file to {output_audio_path}")
        try:
            # Write the samples to an audio file using the original sample rate.
            sf.write(output_audio_path, samples, sample_rate)
            print("Audio reconstruction completed.")
            # Notify the user upon success.
            root.after(0, lambda: messagebox.showinfo("Success", f"Audio has been reconstructed and saved to {output_audio_path}"))
        except Exception as e:
            print(f"Failed to write audio file.\n{e}")
            # Show an error message if writing fails.
            root.after(0, lambda: messagebox.showerror("Error", f"Failed to write audio file.\n{e}"))
    else:
        print("Failed to reconstruct audio.")
        # Show an error message if reconstruction fails.
        root.after(0, lambda: messagebox.showerror("Error", "Failed to reconstruct audio."))

def select_encode():
    """
    Handles the encoding process when the user selects to encode audio to video.
    """
    # Prompt the user to select an audio file.
    audio_path = filedialog.askopenfilename(
        title="Select an Audio File",
        filetypes=[("Audio Files", "*.wav *.flac *.mp3 *.aiff *.aac"), ("All Files", "*.*")]
    )
    if audio_path:
        # Prompt the user to select a location to save the output video.
        output_video = filedialog.asksaveasfilename(
            title="Save Video As",
            defaultextension=".mp4",
            filetypes=[("MP4 Video", "*.mp4")]
        )
        if output_video:
            # Set default encoding parameters.
            frame_rate = 60  # Default frame rate.
            resolution = (3840, 2160)  # 4K resolution.
            strip_width = 8  # Width of each color block.

            # Start encoding in a separate thread to keep the GUI responsive.
            encoding_thread = threading.Thread(
                target=start_encoding,
                args=(audio_path, output_video, frame_rate, resolution, strip_width)
            )
            encoding_thread.start()
        else:
            messagebox.showwarning("Warning", "No output video file selected.")
    else:
        messagebox.showwarning("Warning", "No audio file selected.")

def select_decode():
    """
    Handles the decoding process when the user selects to decode video to audio.
    """
    # Prompt the user to select a video file.
    video_path = filedialog.askopenfilename(
        title="Select a Video File",
        filetypes=[("Video Files", "*.mp4 *.avi *.mov *.mkv"), ("All Files", "*.*")]
    )
    if video_path:
        # Prompt the user to select a location to save the output audio.
        output_audio_path = filedialog.asksaveasfilename(
            title="Save Audio As",
            defaultextension=".wav",
            filetypes=[("WAV Audio", "*.wav")]
        )
        if output_audio_path:
            # Start the decoding process.
            start_decoding(video_path, output_audio_path)
        else:
            messagebox.showwarning("Warning", "No output audio file selected.")
    else:
        messagebox.showwarning("Warning", "No video file selected.")

# --- GUI Initialization ---

# Initialize the Tkinter root window.
root = tk.Tk()
root.title("Audio-Video Encoder/Decoder")
root.geometry("400x200")

# Add buttons to select encoding or decoding.
encode_button = tk.Button(root, text="Encode Audio to Video", command=select_encode, font=("Helvetica", 16))
encode_button.pack(expand=True, pady=10)

decode_button = tk.Button(root, text="Decode Video to Audio", command=select_decode, font=("Helvetica", 16))
decode_button.pack(expand=True, pady=10)

# Run the Tkinter event loop.
root.mainloop()
