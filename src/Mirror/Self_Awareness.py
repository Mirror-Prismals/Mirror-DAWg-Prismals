import pygame
import sys
import mss
import ctypes
import numpy as np
import cv2

num_bits = 2  # Change this value to adjust the number of colors

def calculate_brightness(img):
    # Convert to grayscale
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    # Calculate average brightness
    return np.mean(gray)

def auto_adjust_brightness(img, target_brightness=128):
    current_brightness = calculate_brightness(img)
    
    # Determine the adjustment range based on current brightness
    if current_brightness > 200:  # Very bright scene (e.g., looking at the sun)
        min_factor = 0.1
        max_factor = 0.7
    elif current_brightness > 100:  # Bright scene
        min_factor = 0.5
        max_factor = 1.1 
    elif current_brightness > 120:  # Bright scene
        min_factor = 0.5
        max_factor = .8 
    elif current_brightness < 50:  # Very dark scene
        min_factor = 1.5
        max_factor = 4.0
    else:  # Normal scene
        min_factor = 0.5
        max_factor = 1.3
    
    # Calculate brightness factor
    brightness_factor = target_brightness / current_brightness
    
    # Limit the adjustment factor to the determined range
    brightness_factor = max(min(brightness_factor, max_factor), min_factor)
    
    return brightness_factor, current_brightness

def process_image(img, invert_colors):
    # Auto-adjust brightness
    brightness_factor, current_brightness = auto_adjust_brightness(img)
    img = cv2.convertScaleAbs(img, alpha=brightness_factor, beta=0)

    # Reduce the number of colors (color quantization)
    levels = 2 ** num_bits
    img = (img // (256 // levels)) * (256 // levels)

    # Invert colors if needed
    if invert_colors:
        img = cv2.bitwise_not(img)

    return img, brightness_factor, current_brightness

def main():
    pygame.init()

    # Monitor index (0 for primary monitor)
    monitor_index = 0  # Change this if you want to select a different monitor

    # Use mss to get monitor dimensions
    with mss.mss() as sct:
        monitors = sct.monitors  # mss monitors are 1-indexed
        if monitor_index + 1 >= len(monitors):
            print(f"Monitor index {monitor_index} out of range.")
            sys.exit()
        monitor = monitors[monitor_index + 1]
        screen_width = monitor['width']
        screen_height = monitor['height']
        x = monitor['left']
        y = monitor['top']

    # Set up the display
    screen = pygame.display.set_mode((screen_width, screen_height), pygame.NOFRAME)
    pygame.display.set_caption("Real-Time Palette Converter")

    # Move window to desired monitor
    hwnd = pygame.display.get_wm_info()['window']

    # For Windows
    user32 = ctypes.windll.user32
    SWP_NOSIZE = 0x0001
    SWP_NOZORDER = 0x0004

    # Move the window
    user32.SetWindowPos(hwnd, None, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER)

    # Make window always on top
    WS_EX_TOPMOST = 0x00000008
    GWL_EXSTYLE = -20

    current_style = user32.GetWindowLongPtrW(hwnd, GWL_EXSTYLE)
    user32.SetWindowLongPtrW(hwnd, GWL_EXSTYLE, current_style | WS_EX_TOPMOST)

    # Create a clock to manage the frame rate
    clock = pygame.time.Clock()

    # Set up font for debug info
    font = pygame.font.Font(None, 36)

    invert_colors = False  # Option to invert colors, starting with normal colors

    with mss.mss() as sct:
        while True:
            # Handle events
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()
                elif event.type == pygame.KEYDOWN:
                    if event.key == pygame.K_ESCAPE:
                        pygame.quit()
                        sys.exit()
                    elif event.key == pygame.K_i:
                        invert_colors = not invert_colors  # Toggle inversion with 'i' key

            # Capture the screen
            sct_img = sct.grab(monitor)
            img = np.array(sct_img)  # BGRA format

            # Remove alpha channel and convert to BGR
            img = img[:, :, :3]

            # Process the image
            img, brightness_factor, current_brightness = process_image(img, invert_colors)

            # Convert image to RGB format for Pygame
            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)

            # Create a Surface from the image
            surface = pygame.surfarray.make_surface(img.swapaxes(0, 1))

            # Blit the image to the screen
            screen.blit(surface, (0, 0))

            # Display debug info
            debug_info = f"Brightness Factor: {brightness_factor:.2f}"
            debug_surface = font.render(debug_info, True, (255, 255, 255))
            screen.blit(debug_surface, (10, 10))

            debug_info2 = f"Current Brightness: {current_brightness:.2f}"
            debug_surface2 = font.render(debug_info2, True, (255, 255, 255))
            screen.blit(debug_surface2, (10, 50))

            pygame.display.flip()

            # Limit the frame rate to 30 FPS
            clock.tick(30)

    pygame.quit()

if __name__ == "__main__":
    main()
