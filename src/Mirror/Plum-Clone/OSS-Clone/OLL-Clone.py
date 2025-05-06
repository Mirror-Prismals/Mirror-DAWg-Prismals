import asyncio
import datetime
import subprocess
import traceback
import nest_asyncio
import os
import re  # Added for regex filtering
from pythonosc import dispatcher
from pythonosc import osc_server
from pythonosc import udp_client
import tkinter as tk
from tkinter import font
from PIL import ImageGrab  # Added for screenshot functionality

# Apply nest_asyncio to allow nested event loops
nest_asyncio.apply()

# Global variables
log_file = None
PROMPTS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "prompts")
response_text = ""  # Variable to store the response text for Tkinter display
current_room = None  # Track the current room
is_secret_room = False  # Flag to indicate if the current room is a secret room

# New Global Variables for Screenshot Functionality
SCREENSHOT_INTERVAL = 60  # Interval in seconds
TEMP_SCREENSHOT_PATH = r""  # Path to save screenshots

# Regex pattern to filter out unwanted messages
UNWANTED_MESSAGE_PATTERN = re.compile(
    r"Sent OSC message: /ai_response failed to get console mode for stdout: The handle is invalid\.|"
    r"failed to get console mode for stderr: The handle is invalid\."
)

# Tkinter setup
root = tk.Tk()
root.configure(bg="black")
# Remove fullscreen and set a specific window size
root.geometry("800x600")  # Width x Height in pixels

def create_log_file():
    """Generate a unique log file name with a timestamp for the session."""
    start_time = datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    return f"chat_log_{start_time}.txt"

def log_message_to_file(name, text):
    """Log the message to a file with a timestamp."""
    global log_file
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    try:
        with open(log_file, "a", encoding="utf-8") as file:
            file.write(f"[{timestamp}] {name}: {text}\n")
    except Exception as e:
        print(f"Error writing to log file: {e}")

def get_prompt_for_room(room_name):
    """
    Given a room name like 'ground:main' or 'secret:roomname', return the corresponding prompt text.
    Returns None if the prompt file does not exist.
    """
    filename = room_name.replace(":", "_") + ".txt"
    prompt_path = os.path.join(PROMPTS_DIR, filename)

    if os.path.isfile(prompt_path):
        try:
            with open(prompt_path, "r", encoding="utf-8") as f:
                prompt = f.read().strip()
            return prompt
        except Exception as e:
            print(f"Error reading prompt file '{prompt_path}': {e}")
            return None
    else:
        print(f"No prompt file found for room '{room_name}'. Expected at '{prompt_path}'")
        return None

async def get_ollama_response_async(message, model_name="llava"):
    """Asynchronously get a response from Ollama."""
    ollama_directory = r"C:\Users\zreba"  # Update this path as needed

    try:
        process = await asyncio.create_subprocess_exec(
            "ollama", "run", model_name, message,
            cwd=ollama_directory,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            creationflags=subprocess.CREATE_NO_WINDOW  # Suppress console window
        )

        try:
            stdout, stderr = await asyncio.wait_for(process.communicate(), timeout=600)
        except asyncio.TimeoutError:
            process.kill()
            await process.communicate()
            print("Ollama request timed out.")
            return "I'm experiencing delays. Please try again later.", False

        if process.returncode == 0:
            return stdout.decode().strip(), True
        else:
            # Optionally, filter out specific warnings
            error_message = stderr.decode()
            if not UNWANTED_MESSAGE_PATTERN.search(error_message):
                print("Error getting Ollama response:", error_message)
            # Return a generic error message to avoid displaying unwanted warnings
            return "I'm not sure how to respond to that.", False

    except Exception as e:
        print(f"An error occurred with Ollama: {e}")
        return "I'm not sure how to respond to that.", False

def update_tkinter_display(response):
    """Update the text displayed in the Tkinter window, filtering out unwanted messages."""
    global response_text
    if UNWANTED_MESSAGE_PATTERN.search(response):
#        print("Filtered out unwanted message from GUI display.")
        return  # Do not update the GUI with this message
    response_text = response
    label.config(text=response_text)
    root.update()

async def handle_current_room(address, *args):
    """Handle incoming /current_room OSC messages."""
    global current_room, is_secret_room  # Declare as global to modify them

    if len(args) == 0:
        print("Received /current_room message with no arguments.")
        return

    new_room = args[0]
#ss    print(f"Received OSC message on {address}: {new_room}")
    log_message_to_file("User", new_room)

    # Determine if the new room is a secret room
    if new_room.startswith("secret:"):
        room_name = new_room.split("secret:")[1]
        is_secret = True
    else:
        room_name = new_room
        is_secret = False

    # Check if the room has changed
    if new_room == current_room:
#        print(f"Already in room '{new_room}'. No action taken.")
        return  # No change in room; do nothing

    # If there was a previous room, send /clear
    if current_room is not None:
        try:
            response_osc_client.send_message("/clear", "")
            print(f"Sent OSC message: /clear for leaving room '{current_room}'")
        except Exception as e:
            print(f"Error sending /clear OSC message: {e}")

    # Update the current room and secret room flag
    current_room = new_room
    is_secret_room = is_secret
    #print(f"Updated current room to '{current_room}', Secret Room: {is_secret_room}")
    print(f"you are in '{current_room}'")
    # Get the prompt for the new room
    prompt = get_prompt_for_room(new_room)

    if prompt:
        # Always use 'hermes' for text prompts
        model = "openhermes"

        ollama_response, success = await get_ollama_response_async(prompt, model_name=model)
        print(f"Ollama ({model}): {ollama_response}")
        log_message_to_file(f"Ollama ({model})", ollama_response)

        if success:
            try:
                response_osc_client.send_message("/ai_response", ollama_response)
                print(f"Sent OSC message: /ai_response {ollama_response}")
            except Exception as e:
                print(f"Error sending /ai_response OSC message: {e}")

            # Update Tkinter display
            update_tkinter_display(ollama_response)
        else:
            print("Ollama failed to respond successfully.")
    else:
        print(f"No prompt associated with room '{new_room}'. Skipping Ollama response.")

def take_screenshot():
    """Take a screenshot of the main monitor and save it to the TEMP_SCREENSHOT_PATH."""
    try:
        screenshot = ImageGrab.grab()
        screenshot.save(TEMP_SCREENSHOT_PATH, "PNG")
        #print(f"Screenshot saved to {TEMP_SCREENSHOT_PATH}")
        print("(refreshing vision, please wait..)")
        return True
    except Exception as e:
        print(f"Failed to take screenshot: {e}")
        return False

async def handle_screenshot():
    """
    Handle the screenshot by sending a describe command to the 'llava' Ollama model.
    """
    if take_screenshot():
        prompt = f"describe the image, do not call it an image, pretend youre really there experiancing it. {TEMP_SCREENSHOT_PATH}"
        # Always use 'llava' for image recognition
        model = "llava"
        ollama_response, success = await get_ollama_response_async(prompt, model_name=model)
        print(f"Ollama ({model}) Response to Screenshot: {ollama_response}")
        log_message_to_file(f"Ollama ({model}) (Screenshot)", ollama_response)

        if success:
            try:
                response_osc_client.send_message("/ai_response", ollama_response)
                print(f"Sent OSC message: /ai_response {ollama_response}")
            except Exception as e:
                print(f"Error sending /ai_response OSC message: {e}")

            # Optionally, update Tkinter display with the screenshot description
            update_tkinter_display(ollama_response)
        else:
            print("Ollama failed to respond to screenshot description.")

async def screenshot_task():
    """Asynchronous task to take screenshots at regular intervals."""
    while True:
        await handle_screenshot()
        await asyncio.sleep(SCREENSHOT_INTERVAL)

async def main():
    global log_file

    # Initialize log file once at startup
    log_file = create_log_file()
    print(f"Starting new session, logging to: {log_file}")

    # OSC Server Setup
    receive_ip = "127.0.0.1"  # Listen on localhost
    receive_port = 5005        # Port number to listen on

    # OSC Client Setup for sending responses back to Script 2
    send_ip = "127.0.0.1"      # Assuming both scripts run on the same machine
    send_port = 5006            # Port number Script 2 is listening on
    global response_osc_client
    response_osc_client = udp_client.SimpleUDPClient(send_ip, send_port)

    # Dispatcher Setup
    osc_dispatcher = dispatcher.Dispatcher()

    # Wrapper for async coroutine
    def wrap_async(coro):
        def wrapper(*args):
            asyncio.ensure_future(coro(*args))
        return wrapper

    # Map the OSC address to the async handler using the wrapper
    osc_dispatcher.map("/current_room", wrap_async(handle_current_room))

    # Create OSC Server
    server = osc_server.AsyncIOOSCUDPServer(
        (receive_ip, receive_port),
        osc_dispatcher,
        asyncio.get_event_loop()
    )
    transport, protocol = await server.create_serve_endpoint()  # Create datagram endpoint

    #print(f"OSC Server is running on {receive_ip}:{receive_port}, sending responses to {send_ip}:{send_port}")
    print("loading simulation...blue green's house..")
    # Start the screenshot task
    asyncio.ensure_future(screenshot_task())
    print("Started terrain loading... please wait...")

    try:
        while True:
            await asyncio.sleep(3600)  # Keep the server running indefinitely
    except asyncio.CancelledError:
        print("OSC server task was cancelled.")
    finally:
        transport.close()

# Tkinter label setup
font_size = 48  # Adjust font size as needed
label_font = font.Font(family="Helvetica", size=font_size, weight="bold")
label = tk.Label(root, text=response_text, fg="white", bg="black", font=label_font, wraplength=780)  # Adjust wraplength as needed
label.pack(expand=True)

def run_asyncio(loop):
    """Run the asyncio event loop briefly to process pending tasks."""
    try:
        loop.call_soon(loop.stop)
        loop.run_forever()
    except Exception as e:
        print(f"Error running asyncio loop: {e}")

    # Schedule the next run of the asyncio loop
    root.after(100, lambda: run_asyncio(loop))

if __name__ == "__main__":
    try:
        # Initialize asyncio event loop
        loop = asyncio.get_event_loop()

        # Start the main asyncio task
        asyncio.ensure_future(main())

        # Schedule the asyncio loop to run periodically
        root.after(100, lambda: run_asyncio(loop))

        # Start Tkinter's mainloop (runs in the main thread)
        root.mainloop()

    except Exception as e:
        print(f"An error occurred in the main event loop: {e}")
        traceback.print_exc()
