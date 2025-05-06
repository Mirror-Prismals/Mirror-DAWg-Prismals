from ursina import *
import numpy as np
import sounddevice as sd
import threading
import os
from scipy.io.wavfile import write

app = Ursina()

# Parameters for sine wave
frequency = 440  # A4 standard pitch
sample_rate = 44100  # Audio sample rate
duration = 1.0  # Duration of the sine wave (1 second)
sine_wave_file = "sine_wave.wav"  # Filename for the pre-generated sine wave
gain_db = -94  # Initial gain in dB
z_press_count = 0  # Counter for consecutive 'z' presses
import soundfile as sf
# Hardcoded colors for each zoom state
color_palette = [
    color.hex('#FF0000'),  # Red
    color.hex('#FF0080'),  # Pink
    color.hex('#FF8080'),  # Infrared
    color.hex('#FF8000'),  # Orange
    color.hex('#00FF80'),  # Thalo
    color.hex('#80FF80'),  # Camo
    color.hex('#00FF00'),  # Lime
    color.hex('#00FFFF'),  # Cyan
    color.hex('#8000FF'),  # Violet
    color.hex('#FF80FF'),  # Ultraviolet
    color.hex('#0000FF'),  # Blue
    color.hex('#0080FF'),  # Cerulean
    color.hex('#8080FF'),  # Indigo
    color.hex('#FF00FF'),  # Magenta
    color.hex('#80FF00'),  # Chartreuse
    color.hex('#80FFFF'),  # Aqua
    color.hex('#FFFF00'),  # Yellow
    color.hex('#FFFF80'),  # Mustard
    color.hex('#000000'),  # Null
    color.hex('#FFFFFF'),  # Nill
    color.hex('#808080'),  # N/A
]
current_palette_index = 0  # Start with the first color in the palette

gem_layers = []  # List to hold all gem layers
max_layers = 8  # Number of layers for each gem


# Generate a sine wave and save it to a file
def generate_sine_wave(frequency, duration, sample_rate, gain_db, filename):
    amplitude = 10 ** (gain_db / 20.0)  # Convert dB to linear scale
    t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
    sine_wave = amplitude * np.sin(2 * np.pi * frequency * t)
    write(filename, sample_rate, (sine_wave * 32767).astype(np.int16))


# Check if the sine wave file exists, and create it if not
def ensure_sine_wave_file():
    if not os.path.exists(sine_wave_file):
        generate_sine_wave(frequency, duration, sample_rate, gain_db, sine_wave_file)


# Play the pre-generated sine wave in a separate thread
def play_sine_wave():
    def play_audio():
        try:
            # Load and play the sine wave
            data, _ = sf.read(sine_wave_file)
            sd.play(data, samplerate=sample_rate)
            sd.wait()
        except Exception as e:
            print(f"Error playing sine wave: {e}")

    threading.Thread(target=play_audio, daemon=True).start()


# Create a new gem layer with progressive shading and age
def create_gem_layer(base_color, i, scale_multiplier=1):
    scale = (2 - (i * 0.2)) * scale_multiplier
    adjusted_color = base_color * (1 - i * 0.1)  # Darken progressively
    layer = Entity(
        model='quad',
        scale=(scale, scale / 2),
        position=(0, 0, -0.01 * (len(gem_layers) + i)),  # Deeper position
        color=adjusted_color,
    )

    # Assign age to the layer, starting at 1
    layer.age = 1
    return layer


# Add gem layers dynamically based on the hardcoded color
def add_gem_layers(base_color):
    for i in range(max_layers):
        gem_layers.append(create_gem_layer(base_color, i))


# Manage entities (delete layers that are too old)
def manage_entities():
    global gem_layers

    # Loop through layers, increment age, and delete if age > 3
    for i, layer in enumerate(gem_layers):
        # Increment the age of the layer
        layer.age += 1

        if layer.age > 4:  # Delete if the age is greater than 3
            destroy(layer)
            gem_layers.pop(i)  # Remove the layer from the list


# Animate zooming in
def zoom_gem():
    global current_palette_index, gain_db, z_press_count

    # Increment the palette index and wrap around
    current_palette_index = (current_palette_index + 1) % len(color_palette)
    current_color = color_palette[current_palette_index]

    # Increment the press count and adjust gain
    z_press_count += 1
    gain_db = min(-94 + 10 * z_press_count, 0)  # Start low, increment by +10 dB, max 0 dB

    # Regenerate the sine wave with the new gain
    generate_sine_wave(frequency, duration, sample_rate, gain_db, sine_wave_file)

    # Animate existing layers to zoom deeper
    for i, layer in enumerate(gem_layers):
        layer.animate_scale((layer.scale_x * 2, layer.scale_y * 2), duration=1, curve=curve.in_out_back)
        layer.animate_position((layer.x, layer.y, layer.z - 0.1), duration=1, curve=curve.in_out_back)

    # Manage entities by deleting older layers
    manage_entities()

    # Add new layers with the updated color
    add_gem_layers(current_color)

    # Play the sine wave
    play_sine_wave()


# Reset the gem (zoom out and reset to the first color)
def zoom_out():
    global current_palette_index, gem_layers, gain_db, z_press_count

    # Reset to the initial palette (Red)
    current_palette_index = 0
    current_color = color_palette[current_palette_index]

    # Delete all existing layers
    for layer in gem_layers:
        destroy(layer)
    gem_layers.clear()

    # Reset gain and press count
    gain_db = -94
    z_press_count = 0

    # Regenerate sine wave with initial gain
    generate_sine_wave(frequency, duration, sample_rate, gain_db, sine_wave_file)

    # Recreate gem layers with the initial color
    add_gem_layers(current_color)


# Input events for zooming
def input(key):
    if key == 'z':  # Zoom in
        zoom_gem()
    elif key == 'x':  # Zoom out and reset
        zoom_out()


# Ensure sine wave file exists before starting
ensure_sine_wave_file()

# Initial gem creation with red color
add_gem_layers(color_palette[current_palette_index])

# Background color
window.color = color.light_gray

app.run()
