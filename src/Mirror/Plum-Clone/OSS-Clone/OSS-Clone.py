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

# Import OSC modules
from pythonosc import udp_client

# Initialize Pygame
pygame.init()

# Screen dimensions
WIDTH, HEIGHT = pygame.display.Info().current_w, pygame.display.Info().current_h
screen = pygame.display.set_mode((WIDTH, HEIGHT), pygame.FULLSCREEN)
pygame.display.set_caption("House Simulation")

# OSC Client Setup
OSC_IP = "127.0.0.1"  # Assuming both scripts run on the same machine
OSC_PORT = 5005        # Port number for OSC communication
osc_client = udp_client.SimpleUDPClient(OSC_IP, OSC_PORT)

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
FLOORS = ["sub_basement", "basement", "ground_floor", "upstairs", "bedroom", "internet"]

# Define switches tied to specific videos of specific folders of specific floors
# Each switch is defined by a tuple (floor, folder_name, video_index): switch_id
switches = {
    ('sub_basement', 'frog level', 0): 'switch1',  # Example: second video in ritual_room folder of sub_basement
    ('basement', 'storage', 1): 'switch2',         # Example: second video in storage folder of basement
}

# Passageways updated to check conditions for video-level switches
passageways = {
    ('internet', 'enter', 0): {  # First video in enter folder of internet floor
        'destination': 'secret1',  # Name of the secret folder within 'internet'
        'required_switches': ['switch1'],  # Requires one switch to be activated
    },
    ('internet', 'enter', 2): {  # Third video in enter folder of internet floor
        'destination': 'secret1',  # Name of the secret folder within 'internet'
        'required_switches': ['switch1'],  # Requires one switch to be activated
    },
    ('internet', 'b not', 0): {  # Fifth video in enter folder of internet floor
        'destination': 'secret1',  # Name of the secret folder within 'internet'
        'required_switches': ['switch1'],  # Requires one switch to be activated
    },
}

# Initialize switch states
switch_states = {switch_id: False for switch_id in switches.values()}

# State for secret navigation
secret_navigation_stack = []  # Stack to handle nested secret folders

# Set your hardcoded location (latitude and longitude)
LOCATION = LocationInfo("My Location", "USA", "US/Eastern", 00.0000, -00.0000)  # Replace with your location's coordinates

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

def load_videos(base_dir=BASE_DIR):
    """Load videos and images dynamically from the filesystem, including internet/room folders and secret folders."""
    allowed_extensions = (".mp4", ".jpg", ".jpeg", ".png", ".gif")  # Add more extensions if needed
    
    for floor in FLOORS:
        floor_path = os.path.join(base_dir, floor)
        for period in ["day", "night"]:
            period_path = os.path.join(floor_path, period)
            if os.path.exists(period_path):
                # Load videos and images directly in the main folder for the period
                videos[floor][period] = sorted([
                    os.path.join(period_path, file) for file in os.listdir(period_path) if file.endswith(allowed_extensions)
                ])
                
                # Load subfolders as entities, including their videos and images
                subfolder_videos[floor][period] = {
                    subfolder: {
                        "folder_path": os.path.join(period_path, subfolder),  # Subfolder itself
                        "videos": sorted([
                            os.path.join(period_path, subfolder, file) 
                            for file in os.listdir(os.path.join(period_path, subfolder)) if file.endswith(allowed_extensions)
                        ])
                    }
                    for subfolder in os.listdir(period_path) if os.path.isdir(os.path.join(period_path, subfolder))
                }
    
    # Additionally, load videos and images from secret folders if any
    for secret_path in secret_navigation_stack:
        if os.path.exists(secret_path):
            secret_videos = sorted([
                os.path.join(secret_path, file) for file in os.listdir(secret_path) if file.endswith(allowed_extensions)
            ])
            # Currently, secret_videos are handled in the main loop when secret_navigation_stack is used


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
current_floor = 2  # Index of the current floor (0: sub_basement, 1: basement, etc.)
current_room = {floor: 0 for floor in FLOORS}  # Track the current room for each floor
current_room["secret"] = {}  # Initialize 'secret' key to handle multiple secret folders
current_subfolder = None  # Track subfolder navigation
FPS = 30  # Frames per second

# Internet and sub_basement floor-specific state
current_internet_room = 0  # Track the current room on the internet floor
current_sub_basement_room = 0  # Track the current room on the sub_basement floor
clock = pygame.time.Clock()

def play_video(video_path):
    """Play video using OpenCV with a movable, resizable, filled black box overlay."""
    print(f"Attempting to play video: {video_path}")  # Debug statement
    cap = cv2.VideoCapture(video_path)
    
    if not cap.isOpened():
        print(f"Error: Cannot open video {video_path}")
        return None

    # Define the box position and size
    box_x, box_y, box_width, box_height = 1600, 1950, 400, 100
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

        # Fill the screen with black
        screen.fill((0, 0, 0))
        screen.blit(frame_surface, (x_offset, y_offset))

        # Draw the black box overlay
        pygame.draw.rect(screen, box_color, (box_x, box_y, box_width, box_height))

        pygame.display.update()

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                cap.release()
                pygame.quit()
                sys.exit()

            if event.type == pygame.KEYDOWN:
                if event.key in (pygame.K_w, pygame.K_a, pygame.K_s, pygame.K_d, 
                                pygame.K_UP, pygame.K_DOWN, pygame.K_LEFT, pygame.K_RIGHT,
                                pygame.K_p, pygame.K_h):
                    cap.release()
                    return event.key

        clock.tick(FPS)

    cap.release()
    return None  # Ensure function returns even if video ends naturally

# Function to print timezone, sunrise, sunset, and current status
def print_time_info():
    """Print current sunrise, sunset, and day/night status every 30 seconds."""
    while True:
        now = datetime.now(EASTERN_TZ)
        s = sun(LOCATION.observer, date=now.date(), tzinfo=EASTERN_TZ)
        current_period = "Night" if is_nighttime() else "Day"
        print(f"[{now.strftime('%Y-%m-%d %H:%M:%S %Z')}] Sunrise: {s['sunrise']} | Sunset: {s['sunset']} | Time Period: {current_period}")
        time.sleep(30)

# Start a thread to print time info
time_thread = threading.Thread(target=print_time_info, daemon=True)
time_thread.start()

# Function to flash the screen with a specific color for visual feedback
def flash_screen(color, duration=0.5):
    """Flash the screen with a specific color for a given duration."""
    flash_surface = pygame.Surface((WIDTH, HEIGHT))
    flash_surface.set_alpha(128)  # Semi-transparent
    flash_surface.fill(color)
    screen.blit(flash_surface, (0, 0))
    pygame.display.update()
    pygame.time.delay(int(duration * 1000))

# Function to send current room via OSC every 15 seconds
def send_room_updates():
    while True:
        room_info = get_current_room_info()
        if room_info:
            osc_client.send_message("/current_room", room_info)
            print(f"Sent OSC message: /current_room {room_info}")
        time.sleep(15)

def get_current_room_info():
    """Determine the current room based on the current state."""
    if secret_navigation_stack:
        current_secret_path = secret_navigation_stack[-1]
        # Extract folder name from path
        room_name = os.path.basename(current_secret_path)
        return f"secret:{room_name}"
    else:
        floor_name = FLOORS[current_floor]
        if floor_name in ["internet", "sub_basement"]:
            room_index = current_internet_room if floor_name == "internet" else current_sub_basement_room
            room_name = list(subfolder_videos[floor_name][time_period].keys())[room_index] if subfolder_videos[floor_name][time_period] else "main"
            return f"{floor_name}:{room_name}"
        else:
            if current_subfolder is not None:
                subfolder_name = list(subfolder_videos[floor_name][time_period].keys())[current_subfolder]
                return f"{floor_name}:{subfolder_name}"
            else:
                return f"{floor_name}:main"

# Start a thread to send room updates
osc_sender_thread = threading.Thread(target=send_room_updates, daemon=True)
osc_sender_thread.start()

# Main game loop
while True:
    time_period = "night" if is_nighttime() else "day"

    if secret_navigation_stack:
        # Handle secret folder navigation
        current_secret_path = secret_navigation_stack[-1]
        secret_videos = sorted([
            os.path.join(current_secret_path, vid) 
            for vid in os.listdir(current_secret_path) 
            if vid.endswith(".mp4")
        ])

        if not secret_videos:
            print(f"No videos found in secret folder: {current_secret_path}")
            # Optionally, you might want to pop the empty secret folder
            secret_navigation_stack.pop()
            continue  # Skip to the next iteration

        # Initialize secret room index if not already
        if current_secret_path not in current_room["secret"]:
            current_room["secret"][current_secret_path] = 0

        current_video_index = current_room["secret"][current_secret_path]
        if current_video_index < len(secret_videos):
            current_video = secret_videos[current_video_index]
            print(f"Playing secret video: {current_video}")  # Debug statement
        else:
            print("Invalid video index in secret folder.")
            # Reset index or handle as needed
            current_room["secret"][current_secret_path] = 0
            current_video = secret_videos[0]
            print(f"Resetting to first video in secret folder: {current_video}")  # Debug statement
    else:
        # Handle regular floor navigation
        current_base_dir = BASE_DIR
        load_videos(current_base_dir)
        validate_night_videos()

        floor_name = FLOORS[current_floor]
        print(f"Current floor: {floor_name}, Time period: {time_period}")  # Debug statement

        if floor_name in ["internet", "sub_basement"]:
            room_names = list(subfolder_videos[floor_name][time_period].keys())
            if room_names:
                if floor_name == "internet":
                    current_room_name = room_names[current_internet_room]
                else:  # sub_basement
                    current_room_name = room_names[current_sub_basement_room]
                
                room_videos = subfolder_videos[floor_name][time_period][current_room_name]["videos"]
                
                if floor_name == "internet":
                    if current_room["internet"] < len(room_videos):
                        current_video = room_videos[current_room["internet"]]
                        print(f"Playing internet room video: {current_video}")  # Debug statement
                    else:
                        print(f"No videos in room '{current_room_name}' for time period '{time_period}'.")
                        current_room["internet"] = 0  # Reset to first video
                        if room_videos:
                            current_video = room_videos[0]
                            print(f"Resetting to first video in internet room: {current_video}")  # Debug statement
                        else:
                            print(f"No videos available in internet room '{current_room_name}'.")
                            continue  # Skip to next iteration
                else:  # sub_basement
                    if current_room["sub_basement"] < len(room_videos):
                        current_video = room_videos[current_room["sub_basement"]]
                        print(f"Playing sub-basement room video: {current_video}")  # Debug statement
                    else:
                        print(f"No videos in room '{current_room_name}' for time period '{time_period}'.")
                        current_room["sub_basement"] = 0  # Reset to first video
                        if room_videos:
                            current_video = room_videos[0]
                            print(f"Resetting to first video in sub-basement room: {current_video}")  # Debug statement
                        else:
                            print(f"No videos available in sub-basement room '{current_room_name}'.")
                            continue  # Skip to next iteration
            else:
                print(f"No rooms found in the {floor_name} floor.")
                continue  # Skip to next iteration
        else:
            if current_subfolder is not None:
                subfolder_keys = list(subfolder_videos[floor_name][time_period].keys())
                if current_subfolder < len(subfolder_keys):
                    subfolder_name = subfolder_keys[current_subfolder]
                    if current_room[floor_name] < len(subfolder_videos[floor_name][time_period][subfolder_name]["videos"]):
                        current_video = subfolder_videos[floor_name][time_period][subfolder_name]["videos"][current_room[floor_name]]
                        print(f"Playing subfolder video: {current_video}")  # Debug statement
                    else:
                        print("Invalid video index in subfolder.")
                        current_room[floor_name] = 0  # Reset to first video
                        if subfolder_videos[floor_name][time_period][subfolder_name]["videos"]:
                            current_video = subfolder_videos[floor_name][time_period][subfolder_name]["videos"][0]
                            print(f"Resetting to first video in subfolder: {current_video}")  # Debug statement
                        else:
                            print(f"No videos available in subfolder '{subfolder_name}'.")
                            continue  # Skip to next iteration
                else:
                    print("Invalid subfolder index.")
                    current_subfolder = None  # Exit subfolder navigation
                    continue  # Skip to next iteration
            else:
                if current_room[floor_name] < len(videos[floor_name][time_period]):
                    current_video = videos[floor_name][time_period][current_room[floor_name]]
                    print(f"Playing main room video: {current_video}")  # Debug statement
                else:
                    print("Invalid video index in main room.")
                    current_room[floor_name] = 0  # Reset to first video
                    if videos[floor_name][time_period]:
                        current_video = videos[floor_name][time_period][0]
                        print(f"Resetting to first video in main room: {current_video}")  # Debug statement
                    else:
                        print(f"No videos available in main room of floor '{floor_name}'.")
                        continue  # Skip to next iteration

    # Play the current video
    key = play_video(current_video)

    # Handle navigation and special keys
    if key == pygame.K_h:
        # Get current location details
        if secret_navigation_stack:
            # Currently in a secret folder
            current_secret_path = secret_navigation_stack[-1]
            secret_videos = sorted([
                os.path.join(current_secret_path, vid) for vid in os.listdir(current_secret_path) if vid.endswith(".mp4")
            ])
            if not secret_videos:
                print(f"No videos in secret folder: {current_secret_path}")
            else:
                video_index = current_room["secret"].get(current_secret_path, 0)
                location_key = ('secret', 'secret_room', video_index)  # Adjusted key for secret rooms
        else:
            if floor_name in ["internet", "sub_basement"]:
                room_names = list(subfolder_videos[floor_name][time_period].keys())
                current_room_name = room_names[current_internet_room if floor_name == "internet" else current_sub_basement_room]
                video_index = current_room["internet" if floor_name == "internet" else "sub_basement"]
                location_key = (floor_name, current_room_name, video_index)
            else:
                if current_subfolder is not None:
                    current_room_name = list(subfolder_videos[floor_name][time_period].keys())[current_subfolder]
                else:
                    current_room_name = "main"
                video_index = current_room[floor_name]
                location_key = (floor_name, current_room_name, video_index)

        # Check for switch at current location
        switch_id = switches.get(location_key)
        
        if switch_id:
            # Toggle the switch state
            switch_states[switch_id] = not switch_states[switch_id]
            # Visual feedback
            flash_screen((0, 255, 0) if switch_states[switch_id] else (255, 0, 0))
            print(f"Switch {switch_id} turned {'ON' if switch_states[switch_id] else 'OFF'}")
        else:
            # No switch at current location
            flash_screen((255, 0, 0))
            print("No switch at current location.")

    elif key == pygame.K_p:
        if secret_navigation_stack:
            # Exit secret navigation
            exited_folder = secret_navigation_stack.pop()
            print(f"Exiting secret folder: {exited_folder}")
            if exited_folder in current_room["secret"]:
                del current_room["secret"][exited_folder]
        else:
            # Determine current location key
            if floor_name in ["internet", "sub_basement"]:
                room_names = list(subfolder_videos[floor_name][time_period].keys())
                if room_names:
                    current_room_name = room_names[current_internet_room if floor_name == "internet" else current_sub_basement_room]
                    video_index = current_room["internet" if floor_name == "internet" else "sub_basement"]
                    location_key = (floor_name, current_room_name, video_index)
                else:
                    location_key = (floor_name, 'main', current_room[floor_name])
            else:
                if current_subfolder is not None:
                    subfolder_name = list(subfolder_videos[floor_name][time_period].keys())[current_subfolder]
                    location_key = (floor_name, subfolder_name, current_room[floor_name])
                else:
                    location_key = (floor_name, 'main', current_room[floor_name])

            # Check if a passageway exists at the current location
            passageway = passageways.get(location_key)
            if passageway:
                if all(switch_states[switch_id] for switch_id in passageway['required_switches']):
                    full_path = os.path.join(BASE_DIR, passageway['destination'])
                    if os.path.exists(full_path):
                        secret_navigation_stack.append(full_path)
                        # Initialize the secret folder index
                        if full_path not in current_room["secret"]:
                            current_room["secret"][full_path] = 0

                        # Load videos from the secret folder
                        secret_videos = sorted([
                            os.path.join(full_path, vid) for vid in os.listdir(full_path) if vid.endswith(".mp4")
                        ])
                        if secret_videos:
                            # Set the current video to the first in the secret folder
                            current_video = secret_videos[current_room["secret"][full_path]]
                            print(f"Entering secret folder: {full_path}, playing first video.")
                        else:
                            print(f"Secret folder {full_path} is empty.")
                    else:
                        print(f"Secret folder {full_path} does not exist.")
                else:
                    print("Cannot enter secret passageway: required switches not activated.")
                    flash_screen((255, 255, 0))  # Optional: Flash yellow to indicate a warning
            else:
                print(f"No passageway at current location: {location_key}")
                flash_screen((255, 255, 0))  # Optional: Flash yellow to indicate a warning

    else:
        # Handle navigation
        if key == pygame.K_UP:
            if current_floor < len(FLOORS) - 1:
                current_floor += 1
                current_subfolder = None
                print(f"Moved up to floor: {FLOORS[current_floor]}")
        elif key == pygame.K_DOWN:
            if current_floor > 0:
                current_floor -= 1
                current_subfolder = None
                print(f"Moved down to floor: {FLOORS[current_floor]}")
        elif key == pygame.K_LEFT:
            if floor_name == "internet":
                if current_internet_room > 0:
                    current_internet_room -= 1
                    current_room["internet"] = 0
                    print(f"Moved to previous internet room: {current_internet_room}")
            elif floor_name == "sub_basement":
                if current_sub_basement_room > 0:
                    current_sub_basement_room -= 1
                    current_room["sub_basement"] = 0
                    print(f"Moved to previous sub-basement room: {current_sub_basement_room}")
            elif current_subfolder is None and current_room[floor_name] > 0:
                current_room[floor_name] -= 1
                print(f"Moved to previous room in floor '{floor_name}': {current_room[floor_name]}")
        elif key == pygame.K_RIGHT:
            if floor_name == "internet":
                room_names = list(subfolder_videos[floor_name][time_period].keys())
                if current_internet_room < len(room_names) - 1:
                    current_internet_room += 1
                    current_room["internet"] = 0
                    print(f"Moved to next internet room: {current_internet_room}")
            elif floor_name == "sub_basement":
                room_names = list(subfolder_videos[floor_name][time_period].keys())
                if current_sub_basement_room < len(room_names) - 1:
                    current_sub_basement_room += 1
                    current_room["sub_basement"] = 0
                    print(f"Moved to next sub-basement room: {current_sub_basement_room}")
            elif current_subfolder is None:
                if current_room[floor_name] < len(videos[floor_name][time_period]) - 1:
                    current_room[floor_name] += 1
                    print(f"Moved to next room in floor '{floor_name}': {current_room[floor_name]}")
        elif key == pygame.K_w:
            if secret_navigation_stack:
                # Handle 'w' in secret folder: move to previous video
                current_secret_path = secret_navigation_stack[-1]
                if current_room["secret"][current_secret_path] > 0:
                    current_room["secret"][current_secret_path] -= 1
                    print(f"Moved to previous video in secret folder: {current_room['secret'][current_secret_path]}")
                else:
                    print("Already at the first video in the secret folder.")
                    flash_screen((0, 0, 255))  # Optional: Flash blue to indicate no previous video
            elif floor_name in ["internet", "sub_basement"]:
                if floor_name == "internet":
                    if current_room["internet"] > 0:
                        current_room["internet"] -= 1
                        print(f"Moved up in internet floor: {current_room['internet']}")
                elif floor_name == "sub_basement":
                    if current_room["sub_basement"] > 0:
                        current_room["sub_basement"] -= 1
                        print(f"Moved up in sub-basement floor: {current_room['sub_basement']}")
            elif current_subfolder is not None and current_room[floor_name] > 0:
                current_room[floor_name] -= 1
                print(f"Moved up in subfolder '{current_subfolder}' of floor '{floor_name}': {current_room[floor_name]}")
        elif key == pygame.K_s:
            if secret_navigation_stack:
                # Handle 's' in secret folder: move to next video
                current_secret_path = secret_navigation_stack[-1]
                if current_room["secret"][current_secret_path] < len(secret_videos) - 1:
                    current_room["secret"][current_secret_path] += 1
                    print(f"Moved to next video in secret folder: {current_room['secret'][current_secret_path]}")
                else:
                    print("Already at the last video in the secret folder.")
                    flash_screen((0, 0, 255))  # Optional: Flash blue to indicate no next video
            elif floor_name in ["internet", "sub_basement"]:
                room_videos = subfolder_videos[floor_name][time_period].get(
                    list(subfolder_videos[floor_name][time_period].keys())[current_internet_room if floor_name == "internet" else current_sub_basement_room],
                    {"videos": []}
                )["videos"]
                if floor_name == "internet":
                    if current_room["internet"] < len(room_videos) - 1:
                        current_room["internet"] += 1
                        print(f"Moved down in internet floor: {current_room['internet']}")
                elif floor_name == "sub_basement":
                    if current_room["sub_basement"] < len(room_videos) - 1:
                        current_room["sub_basement"] += 1
                        print(f"Moved down in sub-basement floor: {current_room['sub_basement']}")
            elif current_subfolder is not None:
                subfolder_name = list(subfolder_videos[floor_name][time_period].keys())[current_subfolder]
                room_videos = subfolder_videos[floor_name][time_period][subfolder_name]["videos"]
                if current_room[floor_name] < len(room_videos) - 1:
                    current_room[floor_name] += 1
                    print(f"Moved down in subfolder '{subfolder_name}' of floor '{floor_name}': {current_room[floor_name]}")
        elif key == pygame.K_a:
            if current_subfolder is None and subfolder_videos[floor_name][time_period]:
                current_subfolder = 0
                current_room[floor_name] = 0
                print(f"Entered subfolder navigation on floor '{floor_name}'.")
        elif key == pygame.K_d:
            if current_subfolder is not None:
                print(f"Exited subfolder navigation on floor '{floor_name}'.")
                current_subfolder = None
