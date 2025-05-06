import pygame
import math
import numpy as np
from pygame.math import Vector3
import pygame.sndarray

# Initialize Pygame
pygame.init()
width, height = 800, 600
screen = pygame.display.set_mode((width, height))
clock = pygame.time.Clock()

# Phasor properties
frequency = 440  # Hz
amplitude = 0.5
phi = 360  # Full rotation by default
current_phase = 0

# View properties
rotation_x = 0

# Pause state
paused = False

# Input state
input_mode = None
input_value = ""

def hsv_to_rgb(h, s, v):
    h = h % 360
    c = v * s
    x = c * (1 - abs((h / 60) % 2 - 1))
    m = v - c
    if h < 60:
        r, g, b = c, x, 0
    elif h < 120:
        r, g, b = x, c, 0
    elif h < 180:
        r, g, b = 0, c, x
    elif h < 240:
        r, g, b = 0, x, c
    elif h < 300:
        r, g, b = x, 0, c
    else:
        r, g, b = c, 0, x
    return int((r + m) * 255), int((g + m) * 255), int((b + m) * 255)

def rotate_point(point, rotation_x):
    y = point.y * math.cos(rotation_x) - point.z * math.sin(rotation_x)
    z = point.y * math.sin(rotation_x) + point.z * math.cos(rotation_x)
    return Vector3(point.x, y, z)

def project_point(point):
    factor = 200 / (point.z + 400)
    x = point.x * factor + width // 2
    y = -point.y * factor + height // 2
    return (int(x), int(y))

def draw_phasor(current_phase):
    angle = math.radians(current_phase)
    max_frequency = 13395
    normalized_frequency = min(frequency / max_frequency, 1)
    
    x = math.cos(angle) * amplitude * 100
    y = math.sin(angle) * amplitude * 100
    z = normalized_frequency * 200
    
    phasor_tip = rotate_point(Vector3(x, y, z), rotation_x)
    
    # Draw cylinder
    num_segments = 18
    for i in range(num_segments):
        angle1 = i * 2 * math.pi / num_segments
        angle2 = (i + 1) * 2 * math.pi / num_segments
        p1_bottom = rotate_point(Vector3(math.cos(angle1) * 100, math.sin(angle1) * 100, 0), rotation_x)
        p2_bottom = rotate_point(Vector3(math.cos(angle2) * 100, math.sin(angle2) * 100, 0), rotation_x)
        p1_top = rotate_point(Vector3(math.cos(angle1) * 100, math.sin(angle1) * 100, 200), rotation_x)
        p2_top = rotate_point(Vector3(math.cos(angle2) * 100, math.sin(angle2) * 100, 200), rotation_x)
        
        pygame.draw.line(screen, (100, 100, 100), project_point(p1_bottom), project_point(p2_bottom))
        pygame.draw.line(screen, (100, 100, 100), project_point(p1_top), project_point(p2_top))
        pygame.draw.line(screen, (100, 100, 100), project_point(p1_bottom), project_point(p1_top))
        
        # Draw hue reference on top of cylinder
        hue_color = hsv_to_rgb(i * 360 / num_segments, 1, 1)
        pygame.draw.line(screen, hue_color, project_point(p1_top), project_point(p2_top), 3)

    
def draw_phasor(display_phase):
    angle = math.radians(current_phase)  # Use current_phase for position
    max_frequency = 13395
    normalized_frequency = min(frequency / max_frequency, 1)
    
    x = math.cos(angle) * amplitude * 100
    y = math.sin(angle) * amplitude * 100
    z = normalized_frequency * 200
    
    phasor_tip = rotate_point(Vector3(x, y, z), rotation_x)
    
    # Draw cylinder
    num_segments = 18
    for i in range(num_segments):
        angle1 = i * 2 * math.pi / num_segments
        angle2 = (i + 1) * 2 * math.pi / num_segments
        p1_bottom = rotate_point(Vector3(math.cos(angle1) * 100, math.sin(angle1) * 100, 0), rotation_x)
        p2_bottom = rotate_point(Vector3(math.cos(angle2) * 100, math.sin(angle2) * 100, 0), rotation_x)
        p1_top = rotate_point(Vector3(math.cos(angle1) * 100, math.sin(angle1) * 100, 200), rotation_x)
        p2_top = rotate_point(Vector3(math.cos(angle2) * 100, math.sin(angle2) * 100, 200), rotation_x)
        
        pygame.draw.line(screen, (100, 100, 100), project_point(p1_bottom), project_point(p2_bottom))
        pygame.draw.line(screen, (100, 100, 100), project_point(p1_top), project_point(p2_top))
        pygame.draw.line(screen, (100, 100, 100), project_point(p1_bottom), project_point(p1_top))
        
        # Draw hue reference on top of cylinder, incorporating phi offset
        hue_color = hsv_to_rgb((i * 360 / num_segments + phi) % 360, 1, 1)
        pygame.draw.line(screen, hue_color, project_point(p1_top), project_point(p2_top), 3)
        
    # Draw phasor    
    start_point = project_point(Vector3(0, 0, z))
    end_point = project_point(phasor_tip)
    color = hsv_to_rgb(display_phase, amplitude, normalized_frequency)  # Use display_phase for color
    pygame.draw.line(screen, color, start_point, end_point, 3)

    # Display color rectangle
    pygame.draw.rect(screen, color, (width - 60, 10, 50, 50))

    # Display hex code
    font = pygame.font.Font(None, 24)
    hex_code = '#{:02x}{:02x}{:02x}'.format(*color)
    text_surface = font.render(hex_code, True, (0, 0, 0))
    screen.blit(text_surface, (width - 75, 70))

    # Display data
    text_color = (0, 0, 0)
    text_surface = font.render(f"Phi (phase offset) (p) or (q/e): {phi:.2f}°", True, text_color)
    screen.blit(text_surface, (10, 10))
    text_surface = font.render(f"Frequency (f) or (up/down): {frequency:.2f} Hz", True, text_color)
    screen.blit(text_surface, (10, 40))
    text_surface = font.render(f"Amplitude (m) or (left/right): {amplitude:.2f}", True, text_color)
    screen.blit(text_surface, (10, 70))
    text_surface = font.render(f"Current Phase (c): {current_phase:.2f}°", True, text_color)
    screen.blit(text_surface, (10, 100))
    text_surface = font.render("Rotate (w/s)", True, text_color)
    screen.blit(text_surface, (10, 160))
    text_surface = font.render("Pause (space)", True, text_color)
    screen.blit(text_surface, (10, 130))
    text_surface = font.render("Paused" if paused else "Running", True, text_color)
    screen.blit(text_surface, (10, 190))
    text_surface = font.render(f"Rotation Speed: 1/{SPEED_SCALE/10}x", True, text_color)
    screen.blit(text_surface, (10, 250))
    text_surface = font.render(f"Display Phase: {display_phase:.2f}°", True, text_color)
    screen.blit(text_surface, (10, 280))
    if input_mode:
        text_surface = font.render(f"Enter {input_mode}: {input_value}", True, text_color)
        screen.blit(text_surface, (10, 220))

def generate_sine_wave(freq, duration=0.1, volume=1.0):
    sample_rate = 44100
    t = np.linspace(0, duration, int(sample_rate * duration), False)
    wave = np.sin(2 * np.pi * freq * t) * volume
    return (wave * 32767).astype(np.int16)
# Update the current phase display to show the offset phase

# Create sound object
sample_rate = 44100
buffer = np.zeros((int(sample_rate * 0.1), 2), dtype=np.int16)
sound = pygame.sndarray.make_sound(buffer)

# Main game loop
running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.KEYDOWN:
            if input_mode:
                if event.key == pygame.K_RETURN:
                    try:
                        if input_mode == "frequency":
                            frequency = max(float(input_value), 1)
                        elif input_mode == "amplitude":
                            amplitude = max(min(float(input_value), 1), 0)
                        elif input_mode == "phi":
                            phi = max(float(input_value), 1)
                        elif input_mode == "phase":
                            current_phase = float(input_value) % 360
                    except ValueError:
                        print(f"Invalid input for {input_mode}")
                    input_mode = None
                    input_value = ""
                elif event.key == pygame.K_BACKSPACE:
                    input_value = input_value[:-1]
                else:
                    input_value += event.unicode
            else:
                if event.key == pygame.K_UP:
                    frequency = min(frequency * 1.11235812213455, 13395)
                elif event.key == pygame.K_DOWN:
                    frequency = max(frequency / 1.11235812213455, 1)
                elif event.key == pygame.K_RIGHT:
                    amplitude = min(amplitude * 1.1, 1.0)
                elif event.key == pygame.K_LEFT:
                    amplitude = max(amplitude / 1.1, 0.01)
                elif event.key == pygame.K_q:
                    phi = min(phi + 10, 360)
                elif event.key == pygame.K_e:
                    phi = max(phi - 10, 1)
                elif event.key == pygame.K_SPACE:
                    paused = not paused
                elif event.key == pygame.K_f:
                    input_mode = "frequency"
                elif event.key == pygame.K_m:
                    input_mode = "amplitude"
                elif event.key == pygame.K_p:
                    input_mode = "phi"
                elif event.key == pygame.K_c:
                    input_mode = "phase"

    # Rotate view
    keys = pygame.key.get_pressed()
    if keys[pygame.K_w]:
        rotation_x += 0.60
    if keys[pygame.K_s]:
        rotation_x -= 0.60

    screen.fill((255, 255, 255))
    
    # Update and draw phasor
    SPEED_SCALE = 8 * 10  # 8 times slower than the original speed

    if not paused:
        current_phase = (current_phase + frequency / SPEED_SCALE) % 360
        display_phase = (current_phase + phi) % 360  # Apply phi as an offset
    else:
        display_phase = (current_phase + phi) % 360  # Apply phi as an offset even when paused

    draw_phasor(display_phase)

    # Update and play sound
    if not paused:
        new_buffer = generate_sine_wave(frequency, 0.1, amplitude)
        stereo_buffer = np.column_stack((new_buffer, new_buffer))
        pygame.sndarray.samples(sound)[:] = stereo_buffer
        sound.play()
    else:
        sound.stop()

    pygame.display.flip()
    clock.tick(4.1415926535897932384626433832795)

pygame.quit()
