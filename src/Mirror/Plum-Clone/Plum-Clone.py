import cv2
import os
import pygame
from datetime import datetime
import pytz
from astral import LocationInfo
from astral.sun import sun
import sys
import time
import threading

# Import pywin32 modules for Windows
import win32gui
import win32con

# Initialize Pygame
pygame.init()

# Screen dimensions
WIDTH, HEIGHT = pygame.display.Info().current_w, pygame.display.Info().current_h
screen = pygame.display.set_mode((WIDTH, HEIGHT), pygame.FULLSCREEN)
pygame.display.set_caption("House Simulation")

# Function to set the Pygame window to always stay on top (Windows-specific)
def set_always_on_top():
    hwnd = pygame.display.get_wm_info()['window']  # Get the Pygame window handle
    win32gui.SetWindowPos(
        hwnd,
        win32con.HWND_TOPMOST,
        0,
        0,
        0,
        0,
        win32con.SWP_NOMOVE | win32con.SWP_NOSIZE | win32con.SWP_NOACTIVATE
    )

# Function to modify window styles to prevent minimization
def modify_window_styles():
    hwnd = pygame.display.get_wm_info()['window']
    # Get current window style
    style = win32gui.GetWindowLong(hwnd, win32con.GWL_STYLE)
    # Remove the minimize box and system menu
    style &= ~win32con.WS_MINIMIZEBOX
    style &= ~win32con.WS_SYSMENU
    win32gui.SetWindowLong(hwnd, win32con.GWL_STYLE, style)
    # Optionally, remove maximize box as well
    style &= ~win32con.WS_MAXIMIZEBOX
    win32gui.SetWindowLong(hwnd, win32con.GWL_STYLE, style)

# Function to continuously enforce the always-on-top property
def enforce_always_on_top():
    while True:
        set_always_on_top()
        time.sleep(6)  # Enforce every 6 seconds

# Function to monitor and restore the window if it gets minimized
def prevent_minimize():
    hwnd = pygame.display.get_wm_info()['window']
    while True:
        # Check if the window is minimized
        if win32gui.IsIconic(hwnd):
            # Restore the window
            win32gui.ShowWindow(hwnd, win32con.SW_RESTORE)
            # Re-apply always on top
            set_always_on_top()
        time.sleep(6)  # Check every 6 seconds

# Modify window styles to prevent minimization
modify_window_styles()

# Start a thread to enforce the always-on-top property
topmost_thread = threading.Thread(target=enforce_always_on_top, daemon=True)
topmost_thread.start()

# Start a thread to prevent window minimization
prevent_minimize_thread = threading.Thread(target=prevent_minimize, daemon=True)
prevent_minimize_thread.start()

# Directory structure
BASE_DIR = "house"
# MODIFIED: Added 'sub_basement' below 'basement'
FLOORS = ["sub_basement", "basement", "ground", "upstairs", "bedroom", "internet"]

# Set your hardcoded location (latitude and longitude)
LOCATION = LocationInfo("My Location", "USA", "US/Eastern", 0.0000, -0.0000)  # Replace with your location's coordinates

# Get Eastern Timezone
EASTERN_TZ = pytz.timezone("US/Eastern")

def is_nighttime():
    """Determine if it's currently nighttime based on sunset and sunrise times."""
    now = datetime.now(EASTERN_TZ)
    s = sun(LOCATION.observer, date=now.date(), tzinfo=EASTERN_TZ)
    return now < s['sunrise'] or now > s['sunset']

# Load videos for day and night
videos = {floor: {"day": [], "night": []} for floor in FLOORS}
subfolder_videos = {floor: {"day": {}, "night": {}} for floor in FLOORS}

def load_videos():
    """Load videos dynamically from the filesystem, including internet/room folders."""
    for floor in FLOORS:
        floor_path = os.path.join(BASE_DIR, floor)
        for period in ["day", "night"]:
            period_path = os.path.join(floor_path, period)
            if os.path.exists(period_path):
                # Load videos directly in the main folder for the period
                videos[floor][period] = sorted([
                    os.path.join(period_path, vid) for vid in os.listdir(period_path) if vid.endswith(".mp4")
                ])
                
                # Load subfolders as entities, including their videos
                subfolder_videos[floor][period] = {
                    subfolder: {
                        "folder_path": os.path.join(period_path, subfolder),  # Subfolder itself
                        "videos": sorted([
                            os.path.join(period_path, subfolder, vid) 
                            for vid in os.listdir(os.path.join(period_path, subfolder)) if vid.endswith(".mp4")
                        ])
                    }
                    for subfolder in os.listdir(period_path) if os.path.isdir(os.path.join(period_path, subfolder))
                }

# Load videos initially
load_videos()

# Ensure we crash if night videos are missing during nighttime
def validate_night_videos():
    if is_nighttime() and all(not videos[floor]["night"] for floor in FLOORS):
        print("Error: No night videos available, but it is currently nighttime!")
        pygame.quit()
        sys.exit(1)

validate_night_videos()

# Current state
# MODIFIED: Set starting floor to 'basement' which is index 1
current_floor = 1  # Index of the current floor (1 corresponds to 'basement')
current_room = {floor: 0 for floor in FLOORS}  # Track the current room for each floor
current_subfolder = None  # Track subfolder navigation
FPS = 30  # Frames per second

# Internet and sub_basement floor-specific state
current_internet_room = 0  # Track the current room on the internet floor
current_sub_basement_room = 0  # Track the current room on the sub_basement floor
clock = pygame.time.Clock()

def play_video(video_path):
    """Play video using OpenCV with a movable, resizable, filled black box overlay (movable in the code)."""
    cap = cv2.VideoCapture(video_path)
    
    # Define the box position and size (this can be changed programmatically)
    box_x, box_y, box_width, box_height = 1600, 950, 400, 100  # Set initial position and size
    
    # Define the box color (black in this case, but can be changed)
    box_color = (0, 0, 0)  # RGB for black

    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break

        # Get original frame dimensions
        frame_height, frame_width = frame.shape[:2]

        # Calculate scaling factor to fit the video within the screen
        scale_width = WIDTH / frame_width
        scale_height = HEIGHT / frame_height
        scale = min(scale_width, scale_height)

        # Resize frame to fit within the screen while maintaining aspect ratio
        new_width = int(frame_width * scale)
        new_height = int(frame_height * scale)
        resized_frame = cv2.resize(frame, (new_width, new_height), interpolation=cv2.INTER_LINEAR)

        # Calculate offsets to center the video
        x_offset = (WIDTH - new_width) // 2 
        y_offset = (HEIGHT - new_height) // 2

        # Convert frame to RGB for Pygame
        resized_frame = cv2.cvtColor(resized_frame, cv2.COLOR_BGR2RGB)
        frame_surface = pygame.surfarray.make_surface(resized_frame.swapaxes(0, 1))

        # Fill the screen with the video
        screen.fill((0, 0, 0))
        screen.blit(frame_surface, (x_offset, y_offset))

        # Fill the box with a solid color
        pygame.draw.rect(screen, box_color, (box_x, box_y, box_width, box_height))  # Filled box

        # Update the display
        pygame.display.update()

        # Handle Pygame events
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                cap.release()
                pygame.quit()
                sys.exit()

            # Handle keyboard input for video controls
            if event.type == pygame.KEYDOWN:
                if event.key in (pygame.K_w, pygame.K_a, pygame.K_s, pygame.K_d, 
                                 pygame.K_UP, pygame.K_DOWN, pygame.K_LEFT, pygame.K_RIGHT):
                    cap.release()
                    return event.key

        # Control the frame rate
        clock.tick(FPS)

    cap.release()

# Function to print timezone, sunrise, sunset, and current status
def print_time_info():
    """Print current sunrise, sunset, and day/night status every 30 seconds."""
    while True:
        now = datetime.now(EASTERN_TZ)
        s = sun(LOCATION.observer, date=now.date(), tzinfo=EASTERN_TZ)
        current_period = "Night" if is_nighttime() else "Day"
        print(f"[{now.strftime('%Y-%m-%d %H:%M:%S %Z')}] Sunrise: {s['sunrise']} | Sunset: {s['sunset']} | Time Period: {current_period}")
        time.sleep(30)

# Start a thread to print time info every 30 seconds
time_thread = threading.Thread(target=print_time_info, daemon=True)
time_thread.start()

# Main game loop
while True:
    time_period = "night" if is_nighttime() else "day"

    # Reload videos dynamically if new files are added
    load_videos()

    # Crash if no night videos are available during nighttime
    validate_night_videos()

    floor_name = FLOORS[current_floor]

    if floor_name in ["internet", "sub_basement"]:  # MODIFIED: Include 'sub_basement'
        # Handle internet and sub_basement floor navigation
        room_names = list(subfolder_videos[floor_name][time_period].keys())
        
        if room_names:  # Ensure rooms exist
            if floor_name == "internet":
                current_room_name = room_names[current_internet_room]
            elif floor_name == "sub_basement":
                current_room_name = room_names[current_sub_basement_room]
            
            room_videos = subfolder_videos[floor_name][time_period][current_room_name]["videos"]
            
            # Ensure the current room has videos
            if floor_name == "internet":
                if current_room["internet"] < len(room_videos):
                    current_video = room_videos[current_room["internet"]]
                else:
                    print(f"No videos in room '{current_room_name}' for time period '{time_period}'.")
                    break
            elif floor_name == "sub_basement":
                if current_room["sub_basement"] < len(room_videos):
                    current_video = room_videos[current_room["sub_basement"]]
                else:
                    print(f"No videos in room '{current_room_name}' for time period '{time_period}'.")
                    break
        else:
            print(f"No rooms found in the {floor_name} floor.")
            break

    else:
        if current_subfolder is not None:
            subfolder_keys = list(subfolder_videos[floor_name][time_period].keys())
            if current_subfolder < len(subfolder_keys):
                subfolder_name = subfolder_keys[current_subfolder]
                if current_room[floor_name] < len(subfolder_videos[floor_name][time_period][subfolder_name]["videos"]):
                    current_video = subfolder_videos[floor_name][time_period][subfolder_name]["videos"][current_room[floor_name]]
                else:
                    print("Invalid video index in subfolder.")
                    break
            else:
                print("Invalid subfolder index.")
                break
        else:
            if current_room[floor_name] < len(videos[floor_name][time_period]):
                current_video = videos[floor_name][time_period][current_room[floor_name]]
            else:
                print("Invalid video index in main room.")
                break

    # Play the current video
    key = play_video(current_video)

    # Handle navigation (same logic as before, with modifications for 'sub_basement')
    if key == pygame.K_UP:
        if current_floor < len(FLOORS) - 1:
            current_floor += 1
            current_subfolder = None
    elif key == pygame.K_DOWN:
        if current_floor > 0:
            current_floor -= 1
            current_subfolder = None
    elif key == pygame.K_LEFT:
        if floor_name == "internet":
            if current_internet_room > 0:
                current_internet_room -= 1
                current_room["internet"] = 0
        elif floor_name == "sub_basement":
            if current_sub_basement_room > 0:
                current_sub_basement_room -= 1
                current_room["sub_basement"] = 0
        elif current_subfolder is None and current_room[floor_name] > 0:
            current_room[floor_name] -= 1
    elif key == pygame.K_RIGHT:
        if floor_name == "internet":
            if current_internet_room < len(room_names) - 1:
                current_internet_room += 1
                current_room["internet"] = 0
        elif floor_name == "sub_basement":
            if current_sub_basement_room < len(room_names) - 1:
                current_sub_basement_room += 1
                current_room["sub_basement"] = 0
        elif current_subfolder is None and current_room[floor_name] < len(videos[floor_name][time_period]) - 1:
            current_room[floor_name] += 1
    elif key == pygame.K_w:
        if floor_name in ["internet", "sub_basement"]:
            if floor_name == "internet":
                if current_room["internet"] > 0:
                    current_room["internet"] -= 1
            elif floor_name == "sub_basement":
                if current_room["sub_basement"] > 0:
                    current_room["sub_basement"] -= 1
        elif current_subfolder is not None and current_room[floor_name] > 0:
            current_room[floor_name] -= 1
    elif key == pygame.K_s:
        if floor_name in ["internet", "sub_basement"]:
            if floor_name == "internet":
                if current_room["internet"] < len(room_videos) - 1:
                    current_room["internet"] += 1
            elif floor_name == "sub_basement":
                if current_room["sub_basement"] < len(room_videos) - 1:
                    current_room["sub_basement"] += 1
        elif current_subfolder is not None and current_room[floor_name] < len(subfolder_videos[floor_name][time_period][subfolder_name]["videos"]) - 1:
            current_room[floor_name] += 1
    elif key == pygame.K_a:
        if current_subfolder is None and subfolder_videos[floor_name][time_period]:
            current_subfolder = 0
            current_room[floor_name] = 0
    elif key == pygame.K_d:
        if current_subfolder is not None:
            current_subfolder = None
