import tkinter as tk
import numpy as np
import pyaudio

# Define constants
BACKGROUND_COLOR = "#eaf376"
FLASH_COLOR = "#111111"
DURATION = 1  # Duration in seconds
SAMPLE_RATE = 44100
IMAGE_ROTATE_INTERVAL = 2000  # 2 seconds

# Image paths
image_paths = [
    r"#000000FF#FFFFFFFF"
]

# Function to generate a sine wave
def generate_sine_wave(frequency, duration, sample_rate):
    t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
    wave = 0.5 * np.sin(2 * np.pi * frequency * t)
    return wave.astype(np.float32)

# Function to play the sine wave
def play_tone(frequency):
    p = pyaudio.PyAudio()

    stream = p.open(format=pyaudio.paFloat32,
                    channels=1,
                    rate=SAMPLE_RATE,
                    output=True)

    wave = generate_sine_wave(frequency, DURATION, SAMPLE_RATE)
    
    # Flash the screen while playing
    for window in windows:
        window.config(background=FLASH_COLOR)
        window.update()
    stream.write(wave.tobytes())
    
    stream.stop_stream()
    stream.close()
    p.terminate()
    
    # Revert to original background color
    for window in windows:
        window.config(background=BACKGROUND_COLOR)
        window.update()

# Function to handle key press events to play different tones
def on_key_press(event):
    if event.char == '8':
        play_tone(0)  # Soft white noise
    elif event.char == '1':
        play_tone(100)  # 100 Hz
    elif event.char == '2':
        play_tone(440)  # 440 Hz
    elif event.char == '3':
        play_tone(777)  # 777 Hz

# Function to create a new window and display an image
def create_image_window(image_path):
    window = tk.Toplevel(root)
    window.attributes('-fullscreen', True)
    window.configure(background=BACKGROUND_COLOR)
    img = tk.PhotoImage(file=image_path)
    label = tk.Label(window, image=img)
    label.image = img  # Keep a reference to avoid garbage collection
    label.pack()
    return window

# Create the main window
root = tk.Tk()
root.attributes('-fullscreen', True)
root.configure(background=BACKGROUND_COLOR)
root.title("Main")

# Create a list to hold the window references
windows = [root]

# Create and display image windows
for path in image_paths:
    windows.append(create_image_window(path))

# Bind key press events to the main window
root.bind('<Key>', on_key_press)

# Run the tkinter main loop
root.mainloop()
