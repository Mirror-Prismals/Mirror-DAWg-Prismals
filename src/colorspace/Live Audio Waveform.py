import pygame
import numpy as np
import pyaudio

# Initialize Pygame
pygame.init()

# Set up the display
WINDOW_WIDTH = 1600
WINDOW_HEIGHT = 800
window = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
pygame.display.set_caption("Live Audio Waveform")

# Set up PyAudio
CHUNK = 777
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 36666

p = pyaudio.PyAudio()
stream = p.open(format=FORMAT,
                channels=CHANNELS,
                rate=RATE,
                input=True,
                frames_per_buffer=CHUNK)

# Function to get audio data
def get_audio_data():
    data = stream.read(CHUNK, exception_on_overflow=False)
    numpy_data = np.frombuffer(data, dtype=np.int16)
    normalized_data = np.interp(numpy_data, (-32768, 32767), (0, WINDOW_HEIGHT))
    return normalized_data

# Main loop
running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    # Get the normalized audio data
    normalized_data = get_audio_data()

    # Clear the window
    window.fill((0, 0, 0))

    # Calculate the scaling factor
    scaling_factor = WINDOW_WIDTH / (CHUNK * 0.5)

    # Draw the waveform
    for i in range(len(normalized_data) - 1):
        x1 = i * scaling_factor
        x2 = (i + 1) * scaling_factor
        pygame.draw.line(window, (255, 255, 255), (x1, normalized_data[i]), (x2, normalized_data[i + 1]))

    # Update the display
    pygame.display.flip()

# Clean up
stream.stop_stream()
stream.close()
p.terminate()
pygame.quit()
