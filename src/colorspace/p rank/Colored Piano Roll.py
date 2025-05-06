import pygame
import sys
import mido
import pygame.midi
from tkinter import Tk, filedialog, simpledialog

# Initialize Pygame and Tkinter
pygame.init()
pygame.midi.init()
root = Tk()
root.withdraw()  # Hide the root window

# Constants
CELL_WIDTH = 20   # Width of each time step
CELL_HEIGHT = 20  # Height of each note row

NUM_TIME_STEPS = 100  # Number of time steps (adjust as needed)

LEFT_MARGIN = 100  # Space to display note labels
BOTTOM_MARGIN = 50  # Space for buttons

WINDOW_WIDTH = 800  # Width of the application window
WINDOW_HEIGHT = 600  # Height of the application window

screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT))
pygame.display.set_caption("Interactive Colored Piano Roll")

# Define group to color mapping
group_colors = {
    "Cs": (255, 0, 0),    # Red
    "Ds": (255, 128, 0),    # Orange
    "Es": (255, 255, 0),    # Yellow
    "Fs": (0, 255, 0),  # Green
    "Gs": (0, 255, 255),  # Cyan
    "As": (0, 0, 255),  # Blue
    "Bs": (255, 0, 255),  # Magenta
}

# Define group to note mapping
group_notes = {
    "Cs": ["Cb", "C", "C#"],
    "Ds": ["Db", "D", "D#"],
    "Es": ["Eb", "E", "E#"],
    "Fs": ["Fb", "F", "F#"],
    "Gs": ["Gb", "G", "G#"],
    "As": ["Ab", "A", "A#"],
    "Bs": ["Bb", "B", "B#"],
}

# List of all note names per octave (including enharmonic equivalents)
notes_order = ["Cb", "C", "C#", "Db", "D", "D#", "Eb", "E", "E#", "Fb", "F",
               "F#", "Gb", "G", "G#", "Ab", "A", "A#", "Bb", "B", "B#"]

# MIDI note number to note names mapping (there may be multiple note names per MIDI note)
midi_note_to_note_names = {}

# Note name to MIDI note number mapping
note_name_to_midi_note = {}

# Generate note names and mappings
note_names = []
note_to_row = {}
row_to_note_name = {}
row = 0

# MIDI note range (adjust as needed)
MIDI_NOTE_START = 21  # A0
MIDI_NOTE_END = 108   # C8

# Build mappings
for midi_note in range(MIDI_NOTE_START, MIDI_NOTE_END + 1):
    octave = (midi_note // 12) - 1
    semitone = midi_note % 12

    # All possible note names for this semitone
    possible_names = []
    for note_name in notes_order:
        # Map note name to semitone
        note_base_semitone = {
            'Cb': 0, 'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3,
            'E': 4, 'E#': 4, 'Fb': 5, 'F': 5, 'F#': 6, 'Gb': 6, 'G': 7,
            'G#': 8, 'Ab': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11, 'B#': 11,
        }[note_name]

        if semitone == note_base_semitone:
            full_note_name = f"{note_name}{octave}"
            possible_names.append(full_note_name)
            # Map note name to MIDI note number
            note_name_to_midi_note[full_note_name] = midi_note

    # Map MIDI note number to all possible note names
    midi_note_to_note_names[midi_note] = possible_names

# Build the grid rows (assign unique rows to each note name)
for midi_note in range(MIDI_NOTE_START, MIDI_NOTE_END + 1):
    possible_names = midi_note_to_note_names[midi_note]
    for note_name in possible_names:
        note_names.append(note_name)
        note_to_row[note_name] = row
        row_to_note_name[row] = note_name
        row += 1

# Data structure to store notes placed by the user
notes_to_draw = []

# Font for drawing note labels
font = pygame.font.SysFont(None, 16)

# Function to get note name from row index
def get_note_name_from_row(row_index):
    if 0 <= row_index < len(row_to_note_name):
        return row_to_note_name[row_index]
    else:
        return None

# Function to get row index from y position
def get_row_from_y(y, offset_y):
    return (y - offset_y) // CELL_HEIGHT

# Function to get time step from x position
def get_time_from_x(x, offset_x):
    return (x - offset_x - LEFT_MARGIN) // CELL_WIDTH

# Variables for dragging to adjust note length and adding notes
dragging = False
current_notes = []  # List of notes being created during dragging
delete_mode = False  # Indicates whether we are deleting notes

# Key signature mapping (for enharmonic spelling)
key_signatures = {
    # Major keys
    "C Major": [],
    "G Major": ["F#"],
    "D Major": ["F#", "C#"],
    "A Major": ["F#", "C#", "G#"],
    "E Major": ["F#", "C#", "G#", "D#"],
    "B Major": ["F#", "C#", "G#", "D#", "A#"],
    "F# Major": ["F#", "C#", "G#", "D#", "A#", "E#"],
    "C# Major": ["F#", "C#", "G#", "D#", "A#", "E#", "B#"],
    "F Major": ["Bb"],
    "Bb Major": ["Bb", "Eb"],
    "Eb Major": ["Bb", "Eb", "Ab"],
    "Ab Major": ["Bb", "Eb", "Ab", "Db"],
    "Db Major": ["Bb", "Eb", "Ab", "Db", "Gb"],
    "Gb Major": ["Bb", "Eb", "Ab", "Db", "Gb", "Cb"],
    "Cb Major": ["Bb", "Eb", "Ab", "Db", "Gb", "Cb", "Fb"],
    # Minor keys
    "A Minor": [],
    "E Minor": ["F#"],
    "B Minor": ["F#", "C#"],
    "F# Minor": ["F#", "C#", "G#"],
    "C# Minor": ["F#", "C#", "G#", "D#"],
    "G# Minor": ["F#", "C#", "G#", "D#", "A#"],
    "D# Minor": ["F#", "C#", "G#", "D#", "A#", "E#"],
    "A# Minor": ["F#", "C#", "G#", "D#", "A#", "E#", "B#"],
    "D Minor": ["Bb"],
    "G Minor": ["Bb", "Eb"],
    "C Minor": ["Bb", "Eb", "Ab"],
    "F Minor": ["Bb", "Eb", "Ab", "Db"],
    "Bb Minor": ["Bb", "Eb", "Ab", "Db", "Gb"],
    "Eb Minor": ["Bb", "Eb", "Ab", "Db", "Gb", "Cb"],
    "Ab Minor": ["Bb", "Eb", "Ab", "Db", "Gb", "Cb", "Fb"],
}

selected_key = "C Major"

# Function to decide which note name to use for a MIDI note number based on key signature
def select_note_name(midi_note):
    possible_names = midi_note_to_note_names.get(midi_note, [])
    accidentals = key_signatures[selected_key]

    # Prioritize note names based on key signature
    for name in possible_names:
        note = name[:-1]  # Remove octave number
        if note in accidentals:
            return name
    for name in possible_names:
        note = name[:-1]
        if '#' in note or 'b' in note:
            continue
        else:
            return name
    # If none match, default to first possible name
    return possible_names[0] if possible_names else None

# Function to import MIDI file
def import_midi_file():
    global notes_to_draw, NUM_TIME_STEPS, selected_key, grid_surface_width, grid_surface_height

    # Open file dialog to select MIDI file
    midi_file_path = filedialog.askopenfilename(title="Select MIDI file", filetypes=[("MIDI files", "*.mid *.midi")])
    if not midi_file_path:
        return

    # Ask the user to select the key
    key_options = list(key_signatures.keys())
    selected_key_option = simpledialog.askstring("Select Key", f"Available keys:\n{', '.join(key_options)}\nEnter the key (e.g., 'G Major' or 'E Minor'):")
    if selected_key_option in key_signatures:
        global selected_key
        selected_key = selected_key_option
    else:
        pygame.messagebox.showerror("Invalid Key", "The key you entered is not recognized.")
        return

    # Load the MIDI file
    mid = mido.MidiFile(midi_file_path)
    notes_to_draw.clear()

    # Tempo handling
    tempo = 500000  # Default tempo (microseconds per beat)
    ticks_per_beat = mid.ticks_per_beat

    # Time variables
    abs_time = 0  # Absolute time in ticks

    # Parse MIDI messages
    for track in mid.tracks:
        abs_time = 0
        note_durations = {}  # Keep track of note durations
        for msg in track:
            abs_time += msg.time
            if msg.type == 'set_tempo':
                tempo = msg.tempo  # Update tempo if a tempo change occurs
            elif msg.type == 'note_on' and msg.velocity > 0:
                midi_note = msg.note
                start_time = mido.tick2second(abs_time, ticks_per_beat, tempo)
                # Start a new note
                note_durations[(track, midi_note)] = (start_time, msg.velocity)
            elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
                midi_note = msg.note
                key = (track, midi_note)
                if key in note_durations:
                    start_time, velocity = note_durations[key]
                    end_time = mido.tick2second(abs_time, ticks_per_beat, tempo)
                    duration = end_time - start_time
                    # Select note name based on key signature
                    note_name = select_note_name(midi_note)
                    if note_name:
                        # Add note to notes_to_draw
                        notes_to_draw.append({
                            "note": note_name,
                            "midi_note": midi_note,
                            "time": start_time,
                            "duration": duration,
                            "velocity": velocity
                        })
                    del note_durations[key]

    # Adjust NUM_TIME_STEPS based on the last note's time
    if notes_to_draw:
        max_time = max(n['time'] + n['duration'] for n in notes_to_draw)
        NUM_TIME_STEPS = int(max(NUM_TIME_STEPS, max_time * 10) + 50)  # Adjust as needed
        grid_surface_width = LEFT_MARGIN + NUM_TIME_STEPS * CELL_WIDTH

# Initialize scrolling variables
offset_x = 0
offset_y = 0
scroll_speed = 20  # Pixels per scroll event

# Create a surface for the grid
grid_surface_width = LEFT_MARGIN + NUM_TIME_STEPS * CELL_WIDTH
grid_surface_height = len(note_names) * CELL_HEIGHT + BOTTOM_MARGIN
grid_surface = pygame.Surface((grid_surface_width, grid_surface_height))

# Playback variables
is_playing = False
play_start_time = 0
playhead_position = 0
play_button_rect = pygame.Rect(100, WINDOW_HEIGHT - 40, 80, 30)
play_button_text = None

# MIDI output
midi_out = pygame.midi.Output(pygame.midi.get_default_output_id())

# Main loop
running = True
clock = pygame.time.Clock()

# Menu options
menu_font = pygame.font.SysFont(None, 24)
import_button_rect = pygame.Rect(10, WINDOW_HEIGHT - 40, 80, 30)
import_button_text = menu_font.render("Import", True, (255, 255, 255))

def play_notes():
    global is_playing, play_start_time, playhead_position, note_events
    is_playing = True
    play_start_time = pygame.time.get_ticks() / 1000.0  # In seconds
    playhead_position = 0.0

    # Sort notes by start time
    notes_sorted = sorted(notes_to_draw, key=lambda n: n['time'])

    # Schedule note events
    note_events = []
    for note in notes_sorted:
        note_start_time = note['time']
        note_end_time = note_start_time + note['duration']
        midi_note = note['midi_note']  # Use original MIDI note number
        velocity = note.get('velocity', 100)
        note_events.append(('note_on', note_start_time, midi_note, velocity))
        note_events.append(('note_off', note_end_time, midi_note, 0))

    # Sort events by time
    note_events.sort(key=lambda e: e[1])

# Schedule of note events during playback
note_events = []

while running:
    current_time = pygame.time.get_ticks() / 1000.0  # In seconds

    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

        elif event.type == pygame.MOUSEBUTTONDOWN:
            x, y = event.pos
            if import_button_rect.collidepoint(x, y):
                import_midi_file()
            elif play_button_rect.collidepoint(x, y):
                if not is_playing:
                    play_notes()
                    play_button_text = menu_font.render("Stop", True, (255, 255, 255))
                else:
                    is_playing = False
                    play_button_text = menu_font.render("Play", True, (255, 255, 255))
                    # Send note_off for all active notes
                    for note in range(21, 109):
                        midi_out.note_off(note, 0)
            elif x >= LEFT_MARGIN:
                grid_x = x + offset_x
                grid_y = y + offset_y
                time = get_time_from_x(grid_x, 0)
                time_sec = time / 10.0  # Adjust as needed
                row = get_row_from_y(grid_y, 0)
                note_name = get_note_name_from_row(row)
                if note_name:
                    midi_note = note_name_to_midi_note[note_name]
                    if event.button == 1 and not is_playing:  # Left click to add notes
                        dragging = True
                        start_time = time_sec
                        current_notes = []
                    elif event.button == 3 and not is_playing:  # Right click to delete notes
                        delete_mode = True
            else:
                # Clicked outside the grid
                pass

        elif event.type == pygame.MOUSEBUTTONUP:
            if event.button == 1 and dragging:
                dragging = False
                current_notes = []
            elif event.button == 3 and delete_mode:
                delete_mode = False

        elif event.type == pygame.MOUSEMOTION:
            if event.buttons[0] or event.buttons[2]:  # Check if any mouse button is pressed
                x, y = event.pos  # Get the current mouse position
                if x >= LEFT_MARGIN:
                    grid_x = x + offset_x
                    grid_y = y + offset_y
                    time = get_time_from_x(grid_x, 0)
                    time_sec = time / 10.0  # Adjust as needed
                    row = get_row_from_y(grid_y, 0)
                    note_name = get_note_name_from_row(row)
                    if note_name:
                        midi_note = note_name_to_midi_note[note_name]
                        if dragging and not is_playing:
                            # Check if note already exists in current_notes
                            exists = any(n for n in current_notes if n["note"] == note_name)
                            if not exists:
                                # Create a new note
                                new_note = {
                                    "note": note_name,
                                    "midi_note": midi_note,
                                    "time": start_time,
                                    "duration": max(0.1, time_sec - start_time),
                                    "velocity": 100
                                }
                                current_notes.append(new_note)
                                notes_to_draw.append(new_note)
                            else:
                                # Update duration of the note
                                for n in current_notes:
                                    if n["note"] == note_name:
                                        n["duration"] = max(0.1, time_sec - start_time)
                        elif delete_mode and not is_playing:
                            # Delete note at this position and time
                            notes_to_draw = [
                                n for n in notes_to_draw
                                if not (n["note"] == note_name and n["time"] <= time_sec < n["time"] + n["duration"])
                            ]
                else:
                    # Mouse is outside the grid
                    pass

        elif event.type == pygame.MOUSEWHEEL:
            # Vertical scrolling
            if pygame.key.get_mods() & pygame.KMOD_SHIFT:
                # Horizontal scrolling when Shift is held
                offset_x -= event.y * scroll_speed
                offset_x = max(0, min(offset_x, grid_surface_width - WINDOW_WIDTH))
            else:
                offset_y -= event.y * scroll_speed
                offset_y = max(0, min(offset_y, grid_surface_height - WINDOW_HEIGHT))

    # Playback handling
    if is_playing:
        elapsed_time = current_time - play_start_time
        playhead_position = elapsed_time * 10  # Adjust as needed

        # Process note events
        while note_events and elapsed_time >= note_events[0][1]:
            event_type, event_time, midi_note, velocity = note_events.pop(0)
            if event_type == 'note_on':
                midi_out.note_on(midi_note, velocity)
            elif event_type == 'note_off':
                midi_out.note_off(midi_note, velocity)

        # Stop playback if no more events
        if not note_events:
            is_playing = False
            play_button_text = menu_font.render("Play", True, (255, 255, 255))
            # Send note_off for all active notes
            for note in range(21, 109):
                midi_out.note_off(note, 0)

    # Clear the screen
    screen.fill((0, 0, 0))

    # Clear the grid surface
    grid_surface.fill((0, 0, 0))

    # Draw grid lines on the grid surface
    # Vertical lines (time steps)
    for i in range(NUM_TIME_STEPS + 1):
        x = LEFT_MARGIN + i * CELL_WIDTH
        pygame.draw.line(grid_surface, (50, 50, 50), (x, 0), (x, grid_surface_height - BOTTOM_MARGIN))
    # Horizontal lines (notes)
    for i in range(len(note_names) + 1):
        y = i * CELL_HEIGHT
        pygame.draw.line(grid_surface, (50, 50, 50), (LEFT_MARGIN, y), (grid_surface_width, y))

    # Draw note labels on the grid surface
    for row in range(len(note_names)):
        note_name = note_names[row]
        y = row * CELL_HEIGHT
        text_surface = font.render(note_name, True, (200, 200, 200))
        grid_surface.blit(text_surface, (5, y + (CELL_HEIGHT - font.get_height()) // 2))

    # Draw notes on the grid surface
    for note in notes_to_draw:
        note_name = note["note"]
        midi_note = note["midi_note"]
        time = note["time"] * 10  # Convert to grid units
        duration = note["duration"] * 10
        row = note_to_row[note_name]
        color = None
        # Determine the color group
        for group, notes in group_notes.items():
            if note_name[:-1] in notes:
                color = group_colors[group]
                break
        if color is None:
            color = (128, 128, 128)  # Default color if not matched
        x = LEFT_MARGIN + time * CELL_WIDTH
        y = row * CELL_HEIGHT
        width = duration * CELL_WIDTH
        height = CELL_HEIGHT
        pygame.draw.rect(grid_surface, color, (x, y, width, height))
        # Draw note label on the note rectangle
        note_text_surface = font.render(note_name, True, (0, 0, 0))  # Black text
        text_rect = note_text_surface.get_rect(center=(x + width / 2, y + height / 2))
        grid_surface.blit(note_text_surface, text_rect)

    # Draw playhead
    if is_playing:
        playhead_x = LEFT_MARGIN + playhead_position * CELL_WIDTH
        pygame.draw.line(grid_surface, (255, 255, 255), (playhead_x, 0), (playhead_x, grid_surface_height - BOTTOM_MARGIN), 2)

    # Blit the grid surface onto the main screen with offsets
    screen.blit(grid_surface, (-offset_x, -offset_y))

    # Draw import button (fixed position)
    pygame.draw.rect(screen, (100, 100, 100), import_button_rect)
    screen.blit(import_button_text, (import_button_rect.x + 10, import_button_rect.y + 5))

    # Draw play button
    if is_playing:
        play_button_text = menu_font.render("Stop", True, (255, 255, 255))
    else:
        play_button_text = menu_font.render("Play", True, (255, 255, 255))
    pygame.draw.rect(screen, (100, 100, 100), play_button_rect)
    screen.blit(play_button_text, (play_button_rect.x + 15, play_button_rect.y + 5))

    pygame.display.flip()
    clock.tick(60)

# Clean up MIDI output
midi_out.close()
pygame.midi.quit()
pygame.quit()
