import tkinter as tk
from pythonosc import dispatcher
from pythonosc import osc_server
import threading

# Constants for OSC
OSC_ADDRESS = "127.0.0.1"
OSC_PORT = 5005  # Must match the port used in the DAW

# GUI class for displaying OSC messages
class OscGui:
    def __init__(self, root):
        self.root = root
        self.root.title("OSC MIDI Note Receiver")

        # Create a text widget to display incoming messages
        self.text_area = tk.Text(root, height=15, width=50)
        self.text_area.pack()
        self.text_area.insert(tk.END, "Waiting for OSC messages...\n")

    def display_message(self, message):
        self.text_area.insert(tk.END, message + "\n")
        self.text_area.see(tk.END)  # Auto-scroll to the bottom

# Handler for incoming OSC messages
def midi_note_handler(address, *args):
    gui = midi_note_handler.gui
    if len(args) == 3:
        channel, note, velocity = args
        message = f"Received MIDI Note - Channel: {channel}, Note: {note}, Velocity: {velocity}"
        print(message)
        gui.display_message(message)
    else:
        print("Received unexpected OSC message with arguments:", args)

# OSC server setup in a separate thread
def start_osc_server(gui):
    disp = dispatcher.Dispatcher()
    midi_note_handler.gui = gui  # Attach GUI to handler
    disp.map("/midi/note", midi_note_handler)

    server = osc_server.ThreadingOSCUDPServer((OSC_ADDRESS, OSC_PORT), disp)
    print(f"Serving on {server.server_address}")
    server.serve_forever()

# Initialize the GUI and start the OSC server thread
def main():
    root = tk.Tk()
    gui = OscGui(root)

    osc_thread = threading.Thread(target=start_osc_server, args=(gui,))
    osc_thread.daemon = True  # This ensures the thread will close when the GUI is closed
    osc_thread.start()

    root.mainloop()

if __name__ == "__main__":
    main()
