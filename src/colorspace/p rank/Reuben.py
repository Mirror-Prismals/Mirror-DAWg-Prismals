import pygame
import numpy as np
import sys
import sounddevice as sd
from scipy.signal import butter, lfilter

# Constants
hbar = 1.0545718e-34      # Reduced Planck's constant (J·s)
rho = 1.21                # Density of air (kg/m³)
c = 343                   # Speed of sound in air (m/s)
pi = np.pi
k_B = 1.380649e-23        # Boltzmann's constant (J/K)
T = 300                 # Temperature in Kelvin

# Simulation Parameters
grid_size_x = 200         # Number of grid points in X dimension
grid_size_y = 200         # Number of grid points in Y dimension
space_length = .25        # Length of the grid in meters
dx = space_length / grid_size_x  # Spatial resolution (m)
dt = dx / (2 * c)         # Time step based on CFL condition
time_steps = 100000       # Maximum number of time steps

# Initialize Pressure Fields
# Define as (grid_size_y, grid_size_x) to align with (height, width)
p_previous = np.zeros((grid_size_y, grid_size_x))
p_current = np.zeros((grid_size_y, grid_size_x))
p_next = np.zeros((grid_size_y, grid_size_x))

# Source Parameters
source_position = (grid_size_y // 2, grid_size_x // 2)  # (y, x)
source_frequency = 1000    # Frequency in Hz (1 kHz)
source_angular_freq = 2 * pi * source_frequency
source_amplitude = 1e-5     # Initial pressure amplitude (Pa)

# Initialize Pygame
# Initialize Pygame
pygame.init()
infoObject = pygame.display.Info()
window_width, window_height = infoObject.current_w, infoObject.current_h

# Create a borderless window instead of exclusive fullscreen
screen = pygame.display.set_mode((window_width, window_height), pygame.NOFRAME)
pygame.display.set_caption("2D Pressure Field Simulation with Live Sound")
clock = pygame.time.Clock()

# Adjustable Parameters for Scale and Position
visualizer_scale = 8  # Controls the size of each grid cell (pixels)

# Position the visualizer as needed
# For a borderless window, you might not need to adjust positions relative to the screen
visualizer_position = (155, -300)  # Example for 1600x900 resolution

# Audio Parameters
fs = 44100                 # Sampling frequency (Hz)
block_size = 1024          # Block size for audio stream

# Shared variable to store the latest dBA level
current_dBA = 0.0

# A-weighting filter design
def a_weighting_filter(fs):
    # A-weighting filter coefficients from IEC/CD 1672.
    f1 = 20.598997
    f2 = 107.65265
    f3 = 737.86223
    f4 = 12194.217
    A1000 = 1.9997

    # Define the A-weighting filter based on standard coefficients
    # Using a Butterworth bandpass filter as an approximation
    b, a = butter(4, [f1, f4], btype='band', fs=fs)
    return b, a

b, a = a_weighting_filter(fs)

# Function to apply A-weighting
def apply_a_weighting(signal):
    return lfilter(b, a, signal)

# Callback function to process audio blocks
def audio_callback(indata, frames, time, status):
    global current_dBA
    if status:
        print(status, file=sys.stderr)
    # Convert to mono by averaging channels
    mono_data = np.mean(indata, axis=1)
    # Apply A-weighting filter
    weighted_data = apply_a_weighting(mono_data)
    # Calculate RMS of the weighted signal
    rms = np.sqrt(np.mean(weighted_data**2))
    # Reference pressure (0 dB SPL)
    p_ref = 20e-6  # 20 micropascals
    # Calculate dBA
    dBA = 20 * np.log10(rms / p_ref + 1e-12)  # Added epsilon to avoid log(0)
    current_dBA = dBA

# Start audio stream
stream = sd.InputStream(callback=audio_callback, channels=2, samplerate=fs, blocksize=block_size)
stream.start()

# Function to map dBA to source amplitude
def dBA_to_amplitude(dBA, min_amp=1e-5, max_amp=1e-3):
    # Clamp dBA values to a reasonable range
    dBA = np.clip(dBA, 30, 100)
    # Map dBA to amplitude logarithmically for better sensitivity
    amplitude = min_amp * 10 ** ((dBA - 30) / 20)
    amplitude = np.clip(amplitude, min_amp, max_amp)
    return amplitude

# Function to map pressure to color
def pressure_to_color(p):
    # Define a threshold for visualization
    max_pressure = 1e-4
    p_normalized = np.clip(p / max_pressure, -1, 1)
    # Map to color: red for compression, blue for rarefaction
    colors = np.zeros((grid_size_y, grid_size_x, 3), dtype=np.uint8)
    colors[..., 0] = np.clip((p_normalized * 255), 0, 255).astype(np.uint8)   # Red channel
    colors[..., 2] = np.clip((-p_normalized * 255), 0, 255).astype(np.uint8)  # Blue channel
    return colors

# Function to calculate phonon count at each grid point
def calculate_phonons(p):
    omega = source_angular_freq
    E_phonon = hbar * omega
    E = (p ** 2) / (2 * rho * c ** 2)
    N = E / E_phonon
    return N

# Function to draw the pressure field with manual scale and position
def draw_pressure_field(p_field):
    colors = pressure_to_color(p_field)
    # Transpose the array to match Pygame's (width, height, 3) expectation
    colors = np.transpose(colors, (1, 0, 2))  # Now shape is (width, height, 3)
    # Create a Pygame-compatible surface using surfarray
    grid_surface = pygame.surfarray.make_surface(colors)
    # Scale the surface based on visualizer_scale
    scaled_width = grid_size_x * visualizer_scale
    scaled_height = grid_size_y * visualizer_scale
    scaled_surface = pygame.transform.scale(grid_surface, (scaled_width, scaled_height))
    # Set the absolute position of the visualizer
    x_pos, y_pos = visualizer_position
    # Blit the scaled grid onto the screen at the specified position
    screen.fill((0, 0, 0))  # Clear screen with black
    screen.blit(scaled_surface, (x_pos, y_pos))

    # Optional: Draw crosshair at the center for debugging
    # Uncomment these lines to verify centering
    # center_x = x_pos + scaled_width // 2
    # center_y = y_pos + scaled_height // 2
    # pygame.draw.line(screen, (255, 255, 255), (center_x, 0), (center_x, window_height), 1)  # Vertical line
    # pygame.draw.line(screen, (255, 255, 255), (0, center_y), (window_width, center_y), 1)  # Horizontal line

# Function to handle fullscreen toggle
def toggle_fullscreen(current_mode):
    if current_mode == pygame.FULLSCREEN:
        pygame.display.set_mode((window_width, window_height))
        return False
    else:
        pygame.display.set_mode((window_width, window_height), pygame.FULLSCREEN)
        return True

# Main Simulation Loop
running = True
t = 0
phonon_at_source = []
fullscreen = True  # Start in fullscreen mode

while running and t < time_steps:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
            break
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE:
                running = False  # Exit simulation
            elif event.key == pygame.K_f:
                fullscreen = toggle_fullscreen(fullscreen)  # Toggle fullscreen

    if not running:
        break

    # Map current dBA to source amplitude
    source_amplitude = dBA_to_amplitude(current_dBA)

    # Update the source with the mapped amplitude
    p_current[source_position] += source_amplitude * np.sin(source_angular_freq * t * dt)

    # Compute the next pressure field using the 2D wave equation (FDTD)
    # Update interior points
    p_next[1:-1, 1:-1] = (
        2 * p_current[1:-1, 1:-1] - p_previous[1:-1, 1:-1] +
        (c ** 2) * (dt ** 2) / (dx ** 2) * (
            p_current[2:, 1:-1] + p_current[:-2, 1:-1] +
            p_current[1:-1, 2:] + p_current[1:-1, :-2] -
            4 * p_current[1:-1, 1:-1]
        )
    )

    # Apply damping to simulate energy loss
    damping = 0.995
    p_next *= damping

    # Update previous and current pressure fields
    p_previous, p_current = p_current, p_next.copy()

    # Calculate phonon counts
    phonons = calculate_phonons(p_current)
    phonon_at_source.append(phonons[source_position])

    # Draw the pressure field
    draw_pressure_field(p_current)

    # Update the display
    pygame.display.flip()

    # Control the simulation speed
    clock.tick(60)  # Limit to 60 FPS

    # Increment time step
    t += 1

# After simulation, save phonon counts to a file
with open("phonon_counts.txt", "w") as file:
    for count in phonon_at_source:
        file.write(f"{count}\n")

print("Phonon counts saved to phonon_counts.txt")

# Stop and close the audio stream
stream.stop()
stream.close()

# Quit Pygame gracefully
pygame.quit()
