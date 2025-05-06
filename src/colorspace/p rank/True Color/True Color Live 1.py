
import numpy as np
import sounddevice as sd
import pygame
import colorsys
import threading
import queue
import sys

# Parameters
SAMPLE_RATE = 44100  # Hz
CHANNELS = 1  # Mono audio
FPS = 60  # Frames per second for visualization
STRIP_WIDTH = 2  # Width of each color block in pixels
RESOLUTION = (1920, 1080)  # Width x Height in pixels

# Initialize a queue to communicate between the audio callback and main thread
audio_queue = queue.Queue()

def sample_to_color(sample):
    """
    Maps an audio sample to an RGB color based on its amplitude.
    
    Parameters:
        sample (float): Audio sample in the range [-1.0, 1.0].
    
    Returns:
        tuple: Corresponding RGB color as (R, G, B).
    """
    # Normalize sample from [-1, 1] to [0, 1]
    normalized = (sample + 1) / 2  # [0, 1]
    
    # Apply a non-linear transformation to enhance color distribution
    normalized = normalized ** 3  # Adjust the exponent as needed
    
    # Map to hue (0.0 to 1.0)
    hue = normalized  # Maps from red through the color spectrum back to red
    
    # Convert HSV to RGB
    r, g, b = colorsys.hsv_to_rgb(hue, 1, 1)  # Full saturation and brightness
    
    return (int(r * 255), int(g * 255), int(b * 255))

def audio_callback(indata, frames, time, status):
    """
    This callback is called for each audio block.
    
    Parameters:
        indata (numpy.ndarray): Incoming audio data.
        frames (int): Number of frames.
        time (CData): Time information.
        status (CallbackFlags): Status flags.
    """
    if status:
        print(status, file=sys.stderr)
    audio_queue.put(indata[:, 0].copy())

def audio_thread():
    """
    Thread to handle audio input.
    """
    with sd.InputStream(samplerate=SAMPLE_RATE,
                        channels=CHANNELS,
                        callback=audio_callback,
                        blocksize=1024):
        sd.sleep(int(1e9))  # Keep the stream open indefinitely

def main():
    # Start the audio capturing thread
    threading.Thread(target=audio_thread, daemon=True).start()

    # Initialize Pygame
    pygame.init()
    screen = pygame.display.set_mode(RESOLUTION)
    pygame.display.set_caption("Real-Time Audio Visualizer")
    clock = pygame.time.Clock()

    # Calculate how many color blocks fit in the frame width
    max_blocks = RESOLUTION[0] // STRIP_WIDTH

    # Initialize the color strip
    color_strip = []

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        # Process all available audio samples in the queue
        while not audio_queue.empty():
            samples = audio_queue.get()
            for sample in samples:
                color = sample_to_color(sample)
                color_strip.append(color)
                if len(color_strip) > max_blocks:
                    color_strip.pop(0)  # Remove the oldest sample to maintain strip length

        # Draw the color strip
        screen.fill((0, 0, 0))  # Clear screen with black
        for i, color in enumerate(color_strip):
            pygame.draw.rect(screen, color, (i * STRIP_WIDTH, 0, STRIP_WIDTH, RESOLUTION[1]))

        pygame.display.flip()
        clock.tick(FPS)

    pygame.quit()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nExiting...")
        pygame.quit()
        sys.exit()
