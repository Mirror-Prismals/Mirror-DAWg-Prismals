## mainDashboard.py

import numpy as np
import sounddevice as sd
import pygame
import colorsys
import threading
import queue
import sys
import time
import os
import tkinter as tk
import pygame
import random
import traceback

# Parameters for the visualizer
SAMPLE_RATE = 44100
CHANNELS = 1
FPS = 60
STRIP_WIDTH = 2
SCROLL_SPEED = 1

# Threshold for dBA level (adjust as needed)
DBA_THRESHOLD = -3 # Example threshold in decibels

# Timer duration in seconds
TIMER_DURATION = 24  # Adjust as needed

# Initialize a queue to communicate between the audio callback and main thread
audio_queue = queue.Queue()

# Flags to control the audio stream and state
audio_stream_active = True
audio_playing = True
threshold_exceeded = False  # Indicates if the dBA threshold has been exceeded
game_started = False        # Indicates if the Frog Level game has started

# Text from Mita
scrolling_text = [
    "Dear Mira and Player,",
    "The Argument Game was created for both of you.",
    "It is a challenge, a test, and a journey...",
    "Sincerely, Mita"
]

# Color mappings
color_mappings = [
    ("#FF0000", "Red"),
    ("#FF0080", "Pink"),
    ("#FF8080", "Infrared"),
    ("#FF8000", "Orange"),
    ("#00FF80", "Mint"),
    ("#80FF80", "Camo"),
    ("#00FF00", "Lime"),
    ("#00FFFF", "Cyan"),
    ("#8000FF", "Violet"),
    ("#FF80FF", "Ultraviolet"),
    ("#0000FF", "Blue"),
    ("#0080FF", "Azure"),
    ("#8080FF", "Indigo"),
    ("#FF00FF", "Magenta"),
    ("#80FF00", "Chartreuse"),
    ("#80FFFF", "Aqua"),
    ("#FFFF00", "Yellow"),
    ("#FFFF80", "Mustard"),
    ("#000000", "Null"),
    ("#808080", "Nill"),
    ("#FFFFFF", "Mill"),
    ("#FF4700", "Tangerine"),
    # Outside
    ("#4B6712", "Outside 1"),
    ("#708E2F", "Outside 2"),
    ("#84A1D1", "Outside 3"),
    # Skin
    ("#2D221E", "Skin 1"),
    ("#A57E6E", "Skin 2"),
    ("#FFCEB4", "Skin 3"),
    ("#695046", "Skin 4"),
    # Nill/Null/Na
    ("#FFFFFF", "Nill/Null/Na 1"),
    ("#808080", "Nill/Null/Na 2"),
    ("#000000", "Nill/Null/Na 3"),
    # Red
    ("#FF0000", "Red 1"),
    ("#800000", "Red 2"),
    ("#200000", "Red 3"),
    # Lime
    ("#00FF00", "Lime 1"),
    ("#008500", "Lime 2"),
    ("#002000", "Lime 3"),
    # Blue
    ("#0000FF", "Blue 1"),
    ("#000080", "Blue 2"),
    ("#000020", "Blue 3"),
    # Cyan
    ("#00FFFF", "Cyan 1"),
    ("#008080", "Cyan 2"),
    ("#002020", "Cyan 3"),
    # Magenta
    ("#FF00FF", "Magenta 1"),
    ("#800080", "Magenta 2"),
    ("#200020", "Magenta 3"),
    # Yellow
    ("#FFFF00", "Yellow 1"),
    ("#808000", "Yellow 2"),
    ("#202000", "Yellow 3"),
    # Orange
    ("#FF7F00", "Orange 1"),
    ("#804000", "Orange 2"),
    ("#201000", "Orange 3"),
    # Pink
    ("#FF0080", "Pink 1"),
    ("#800040", "Pink 2"),
    ("#200010", "Pink 3"),
    # Thalo
    ("#00FF80", "Thalo 1"),
    ("#008040", "Thalo 2"),
    ("#002010", "Thalo 3"),
    # Violet
    ("#8000FF", "Violet 1"),
    ("#400080", "Violet 2"),
    ("#100020", "Violet 3"),
    # Chartreuse
    ("#80FF00", "Chartreuse 1"),
    ("#408000", "Chartreuse 2"),
    ("#102000", "Chartreuse 3"),
    # Cerulean
    ("#0080FF", "Cerulean 1"),
    ("#004080", "Cerulean 2"),
    ("#001020", "Cerulean 3"),
    # Infrared
    ("#FF8080", "Infrared 1"),
    ("#804040", "Infrared 2"),
    ("#201010", "Infrared 3"),
    # Camo
    ("#80FF80", "Camo 1"),
    ("#408040", "Camo 2"),
    ("#102010", "Camo 3"),
    # Indigo
    ("#8080FF", "Indigo 1"),
    ("#404080", "Indigo 2"),
    ("#101020", "Indigo 3"),
    # Aqua
    ("#80FFFF", "Aqua 1"),
    ("#408080", "Aqua 2"),
    ("#102032", "Aqua 3"),
    # Mustard
    ("#FFFF80", "Mustard 1"),
    ("#808040", "Mustard 2"),
    ("#202010", "Mustard 3"),
    # Ultraviolet
    ("#FF80FF", "Ultraviolet 1"),
    ("#804080", "Ultraviolet 2"),
    ("#201020", "Ultraviolet 3"),
]

def sample_to_color(sample, color_scheme, specified_colors=None):
    """Convert an audio sample to an RGB color using the specified color scheme."""
    if color_scheme == "original":
        # Original behavior using colorsys
        normalized = (sample + 1) / 2  # Normalize [-1, 1] to [0, 1]
        normalized = normalized ** 3    # Apply nonlinear transformation
        hue = normalized                # Map to hue [0.0, 1.0]
        r, g, b = colorsys.hsv_to_rgb(hue, 1, 1)  # Full saturation and brightness
        return int(r * 255), int(g * 255), int(b * 255)
    elif color_scheme == "specified" and specified_colors is not None:
        # Use the specified color list
        normalized = (sample + 1) / 2  # Normalize [-1, 1] to [0, 1]
        index = int(normalized * (len(specified_colors) - 1))
        color_hex = specified_colors[index]
        color_rgb = pygame.Color(color_hex)
        return color_rgb.r, color_rgb.g, color_rgb.b
    else:
        # Fallback to white if colors are not specified
        return 255, 255, 255

def calculate_dba(audio_samples):
    """Calculate the approximate dBA level from audio samples."""
    # Compute RMS value
    rms = np.sqrt(np.mean(np.square(audio_samples)))
    # Avoid log of zero
    if rms == 0:
        rms = 1e-16
    # Convert to decibels
    db = 20 * np.log10(rms)
    return db

def audio_callback(indata, frames, time_info, status):
    """Callback function for audio input."""
    if status:
        print(status, file=sys.stderr)
    audio_queue.put(indata[:, 0].copy())

def audio_thread():
    """Thread to handle audio input."""
    global audio_stream_active
    with sd.InputStream(samplerate=SAMPLE_RATE,
                        channels=CHANNELS,
                        callback=audio_callback,
                        blocksize=1024):
        while audio_stream_active:
            sd.sleep(100)

def stop_audio_stream():
    """Function to stop the audio stream."""
    global audio_stream_active
    audio_stream_active = False

def cleanup_resources():
    """Function to clean up all resources before transitioning."""
    if pygame.mixer.get_init():
        pygame.mixer.music.stop()
    stop_audio_stream()
    pygame.quit()

def select_monitor():
    """Function to ask the user which monitor to display the dashboard on."""
    selected_monitor = 0  # Default to main monitor
    import tkinter as tk
    root = tk.Tk()
    root.title("Select Monitor")
    root.geometry("300x100")
    root.eval('tk::PlaceWindow . center')  # Center the window on the screen

    def set_monitor(monitor_index):
        nonlocal selected_monitor
        selected_monitor = monitor_index
        root.destroy()

    label = tk.Label(root, text="Select monitor to display the dashboard:")
    label.pack(pady=10)

    button_main = tk.Button(root, text="Main Monitor", command=lambda: set_monitor(0))
    button_second = tk.Button(root, text="Second Monitor", command=lambda: set_monitor(1))
    button_main.pack(side="left", padx=40, pady=10)
    button_second.pack(side="right", padx=40, pady=10)

    root.mainloop()
    return selected_monitor

def launch_main_hub():
    try:
        global audio_playing, threshold_exceeded, game_started

        # Ask the user to select the monitor
        monitor_index = select_monitor()

        # Set the environment variable to prevent window from minimizing on focus loss
        os.environ['SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS'] = '0'

        # Set the environment variable for the display before initializing Pygame
        os.environ['SDL_VIDEO_FULLSCREEN_DISPLAY'] = str(monitor_index)

        # Initialize Pygame
        pygame.init()
        pygame.display.init()

        # Get the display information
        display_info = pygame.display.Info()
        width, height = display_info.current_w, display_info.current_h
        RESOLUTION = (width, height)

        # Set up the display mode with NOFRAME flag to prevent hiding when unfocused
        screen = pygame.display.set_mode(RESOLUTION, pygame.NOFRAME)
        pygame.display.set_caption("Main Hub Visualizer")
        clock = pygame.time.Clock()

        # Start the audio capturing thread
        audio_thread_handle = threading.Thread(target=audio_thread, daemon=True)
        audio_thread_handle.start()

        # Load and play the background music after window initialization
        pygame.mixer.init()
        if os.path.isfile("output.wav"):
            pygame.mixer.music.load("output.wav")  # Ensure this file exists
            pygame.mixer.music.play()
        else:
            print("Error: 'output.wav' not found.")

        # Start the timer
        game_start_time = time.time() + TIMER_DURATION

        # Font settings
        font = pygame.font.SysFont("Helvetica", 36)
        text_color_static = (0, 0, 0)     # White text for visibility
        text_color_scrolling = (255, 255, 255)  # White text for the scrolling message
        
        # Static text at the top
        static_text_surface = font.render("Mira is functioning on main", True, text_color_static)

        # Scroll variables
        scroll_y = RESOLUTION[1]  # Start the text off-screen at the bottom

        # Calculate how many color blocks fit in the frame width
        STRIP_WIDTH = 2
        max_blocks = RESOLUTION[0] // STRIP_WIDTH
        color_strip = []

        # Colors specified for when threshold is exceeded
        specified_colors = [color[0] for color in color_mappings]

        # Game variables
        current_hex = None
        option1 = None
        option2 = None
        correct_answer = None
        score = 0
        special_condition_met = False  # New variable to track the special condition

        # Load sounds
        if os.path.isfile("right.wav"):
            right_sound = pygame.mixer.Sound("right.wav")
        else:
            print("Error: 'right.wav' not found.")
            right_sound = None

        if os.path.isfile("wrong.mp3"):
            wrong_sound = pygame.mixer.Sound("wrong.mp3")
        else:
            print("Error: 'wrong.mp3 not found.")
            wrong_sound = None

        # Set volume levels (optional)
        if right_sound:
            right_sound.set_volume(0.5)
        if wrong_sound:
            wrong_sound.set_volume(0.5)

        def next_question():
            nonlocal current_hex, option1, option2, correct_answer
            # Select a random correct color
            correct_color = random.choice(color_mappings)
            # Select a random incorrect color
            incorrect_color = random.choice(color_mappings)
            while incorrect_color == correct_color:
                incorrect_color = random.choice(color_mappings)
            # Randomly assign options
            if random.choice([True, False]):
                option1 = correct_color[1]
                option2 = incorrect_color[1]
            else:
                option1 = incorrect_color[1]
                option2 = correct_color[1]
            # Set the hex code
            current_hex = correct_color[0]
            # Store the correct answer
            correct_answer = correct_color[1]

        running = True
        while running:
            current_time = time.time()

            # Check if timer has expired and threshold not exceeded
            if not threshold_exceeded and not game_started and current_time >= game_start_time:
                game_started = True
                # Initialize game variables
                next_question()

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                if event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE:
                        running = False
                    # Additional key handling can be added here
                if event.type == pygame.MOUSEBUTTONDOWN:
                    if game_started:
                        mouse_pos = event.pos
                        # Check if the user clicked on one of the option buttons
                        # Define button rectangles
                        button_width = 200
                        button_height = 50
                        button1_rect = pygame.Rect((RESOLUTION[0] // 2 - button_width - 10, RESOLUTION[1] // 2), (button_width, button_height))
                        button2_rect = pygame.Rect((RESOLUTION[0] // 2 + 10, RESOLUTION[1] // 2), (button_width, button_height))
                        selected_option = None
                        if button1_rect.collidepoint(mouse_pos):
                            selected_option = option1
                        elif button2_rect.collidepoint(mouse_pos):
                            selected_option = option2

                        if selected_option is not None:
                            # Normalize strings for comparison
                            selected_option_normalized = selected_option.strip().lower()
                            correct_answer_normalized = correct_answer.strip().lower()

                            if selected_option_normalized == correct_answer_normalized:
                                # Correct answer
                                score += 1
                                if right_sound:
                                    right_sound.play()
                            else:
                                # Incorrect answer
                                score = 0
                                if wrong_sound:
                                    wrong_sound.play()
                                
                                # Check for the special condition
                                if selected_option_normalized == "cyan 2" and current_hex.lower() != "#008080":
                                    special_condition_met = True

                            # Get next question
                            next_question()

            # Process audio samples
            dba = None  # Initialize dba variable
            while not audio_queue.empty():
                samples = audio_queue.get()
                # Calculate dBA level
                dba = calculate_dba(samples)
                print(f"dBA Level: {dba:.2f} dB")  # Print dBA level to terminal

                # Check if dBA exceeds threshold
                if dba > DBA_THRESHOLD and not threshold_exceeded:
                    threshold_exceeded = True  # Set the flag
                    # Stop playback if it's still playing
                    if audio_playing and pygame.mixer.get_init() and pygame.mixer.music.get_busy():
                        pygame.mixer.music.stop()
                        audio_playing = False
                    text_color_static = (0, 0, 0)  # Black color in RGB
                    static_text_surface = font.render("M:3/", True, text_color_static)

                    # Clear the color strip to avoid mixing colors
                    color_strip = []
                    # Since threshold is exceeded, stop the game if it's running
                    game_started = False

                # Determine the color scheme
                if threshold_exceeded:
                    color_scheme = "specified"
                else:
                    color_scheme = "original"

                for sample in samples:
                    if color_scheme == "specified":
                        color = sample_to_color(sample, color_scheme, specified_colors)
                    else:
                        color = sample_to_color(sample, color_scheme)
                    color_strip.append(color)
                    if len(color_strip) > max_blocks:
                        color_strip.pop(0)

            # Draw the color strip (audio visualizer)
            screen.fill((0, 0, 0))  # Clear screen with black
            for i, color in enumerate(color_strip):
                pygame.draw.rect(screen, color, (i * STRIP_WIDTH, 0, STRIP_WIDTH, RESOLUTION[1]))

            # Draw the static text at the top
            screen.blit(static_text_surface, (50, 50))

            # Draw the special condition text if met
            if special_condition_met:
                special_text = font.render("wait, you can make the whole room Cyan 2?", True, (0, 128, 128))  # Yellow text
                screen.blit(special_text, (50, 150))  # Position it below "Mira is functioning on main"

            # Draw the game interface if the game has started
            if game_started:
                # Display the score
                score_surface = font.render(f"Score: {score}", True, text_color_static)
                screen.blit(score_surface, (50, 100))

                # Display the hex code
                hex_surface = font.render(current_hex, True, (255, 255, 255))
                screen.blit(hex_surface, (RESOLUTION[0] // 2 - hex_surface.get_width() // 2, RESOLUTION[1] // 2 - 100))

                # Draw option buttons
                button_width = 200
                button_height = 50
                button1_rect = pygame.Rect((RESOLUTION[0] // 2 - button_width - 10, RESOLUTION[1] // 2), (button_width, button_height))
                button2_rect = pygame.Rect((RESOLUTION[0] // 2 + 10, RESOLUTION[1] // 2), (button_width, button_height))

                pygame.draw.rect(screen, (200, 200, 200), button1_rect)
                pygame.draw.rect(screen, (200, 200, 200), button2_rect)

                option1_surface = font.render(option1, True, (0, 0, 0))
                option2_surface = font.render(option2, True, (0, 0, 0))

                screen.blit(option1_surface, (button1_rect.centerx - option1_surface.get_width() // 2, button1_rect.centery - option1_surface.get_height() // 2))
                screen.blit(option2_surface, (button2_rect.centerx - option2_surface.get_width() // 2, button2_rect.centery - option2_surface.get_height() // 2))
            else:
                # Draw the scrolling text (only if scrolling text is active)
                if scroll_y is not None:
                    for i, line in enumerate(scrolling_text):
                        text_surface = font.render(line, True, text_color_scrolling)
                        screen.blit(text_surface, (50, scroll_y + i * 50))

                    # Move the text upwards
                    scroll_y -= SCROLL_SPEED
                    if scroll_y + len(scrolling_text) * 50 < 0:
                        scroll_y = None  # Stop scrolling the text

            pygame.display.flip()
            clock.tick(FPS)

    except Exception as e:
        print("An error occurred:")
        traceback.print_exc()
    finally:
        # Cleanup before exiting
        cleanup_resources()

if __name__ == "__main__":
    launch_main_hub()
