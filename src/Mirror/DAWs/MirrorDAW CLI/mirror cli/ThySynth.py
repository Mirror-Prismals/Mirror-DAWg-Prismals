import pygame.midi
from pythonosc import dispatcher
from pythonosc import osc_server
import threading
import time

# Constants for OSC
OSC_ADDRESS = "127.0.0.1"  # Must match the address used in mirrordaw.py
OSC_PORT = 5005            # Must match the port used in mirrordaw.py

# Initialize Pygame MIDI
pygame.midi.init()

# Open a MIDI output port
midi_out = pygame.midi.Output(0)  # 0 is usually the default MIDI output port

# Handler for incoming OSC messages
def midi_note_handler(address, channel, note, velocity):
    print(f"Received MIDI Note - Channel: {channel}, Note: {note}, Velocity: {velocity}")
    # Note On message
    midi_out.note_on(note, velocity, channel)
    # Note Off message after a delay
    threading.Timer(0.5, midi_out.note_off, args=[note, velocity, channel]).start()

# OSC server setup in a separate thread
def start_osc_server():
    disp = dispatcher.Dispatcher()
    disp.map("/midi/note", midi_note_handler)

    server = osc_server.ThreadingOSCUDPServer((OSC_ADDRESS, OSC_PORT), disp)
    print(f"Synthesizer is running and listening on {server.server_address}")
    server.serve_forever()

def main():
    osc_thread = threading.Thread(target=start_osc_server)
    osc_thread.daemon = True  # This ensures the thread will close when the program exits
    osc_thread.start()

    try:
        # Keep the main thread alive to maintain the OSC server
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Shutting down synthesizer...")
    finally:
        # Clean up
        midi_out.close()
        pygame.midi.quit()

if __name__ == "__main__":
    main()
