from ursina import *
import threading
import time
import sounddevice as sd

# Import mirror.py DAW functions
from mirror import (
    list_tracks,
    record_track,
    play_audio_tracks,
    play_mida_tracks,
    mix_tracks,
    record_mida_track,
    delete_tracks
)

daw_gui = None
showing_daw_gui = False

# BPM for Mida playback and recording
bpm = 120

# Current volume setting from the slider (0.0 to 1.0)
# We'll apply this volume when mixing tracks.
current_volume = 0.5

# Flag to track if something is currently playing (for Stop)
playing_threads = []

def play_all():
    """Play all audio and Mida tracks simultaneously in background threads."""
    files = list_tracks()
    if not files:
        print("No tracks to play.")
        return

    audio_tracks = [f for f in files if f.endswith('.wav')]
    mida_tracks = [f for f in files if f.endswith('.txt')]

    # Stop any ongoing playback first, just in case
    sd.stop()
    global playing_threads
    playing_threads.clear()

    # Start audio and Mida playback concurrently
    if audio_tracks:
        audio_thread = threading.Thread(target=play_audio_tracks, args=(audio_tracks,))
        audio_thread.start()
        playing_threads.append(audio_thread)

    if mida_tracks:
        mida_thread = threading.Thread(target=play_mida_tracks, args=(mida_tracks, bpm))
        mida_thread.start()
        playing_threads.append(mida_thread)

    print("Playback started.")

def stop_all():
    """Stop ongoing audio playback. Mida threads can't be stopped mid-play easily, 
    but stopping audio at least halts sounddevice output."""
    sd.stop()
    # If you wanted to stop Mida threads gracefully, you’d need additional logic.
    print("Playback stopped (audio).")

def record_audio_track():
    """Record a demonstration audio track (track #1 for 5 seconds)."""
    def rec_thread():
        record_track(1, 5)
    threading.Thread(target=rec_thread).start()

def record_mida_demo():
    """Record a demonstration Mida track (track #2 with a sample Mida sequence)."""
    record_mida_track(2, "C4 E4 G4 . C5 E5 G5 . C6 E6 G6")

def mix_all():
    """Mix all recorded tracks using the current volume setting."""
    # Convert current_volume to dB offset. 
    # At 0.5 = 0 dB, at 1.0 = +10 dB, at 0.0 = -10 dB.
    db_change = (current_volume - 0.5) * 20
    volumes = [int(db_change)]  # Apply the same volume change to all tracks for simplicity

    def mix_thread():
        mix_tracks(output_filename="mixed_output.wav", volumes=volumes)
    threading.Thread(target=mix_thread).start()

def list_all_tracks():
    tracks = list_tracks()
    if tracks:
        print("Recorded Tracks:")
        for track in tracks:
            if track.endswith('.wav'):
                print(f"- {track} (Audio)")
            elif track.endswith('.txt'):
                print(f"- {track} (Mida)")
    else:
        print("No tracks recorded yet.")

def delete_all_tracks():
    print("Deleting all tracks...")
    # The delete_tracks function will prompt in the console
    delete_tracks()

def setup_daw_gui():
    global daw_gui, showing_daw_gui, current_volume

    # Callback functions for the buttons
    def on_play_click():
        play_all()

    def on_stop_click():
        stop_all()

    def on_record_audio_click():
        record_audio_track()

    def on_record_mida_click():
        record_mida_demo()

    def on_mix_click():
        mix_all()

    def on_list_click():
        list_all_tracks()

    def on_delete_click():
        delete_all_tracks()

    # Volume slider callback
    def on_volume_change():
        # This simply updates the global current_volume variable
        # Changes apply when we do mixing next time
        nonlocal volume_slider
        current_volume = volume_slider.value
        print(f"Volume set to: {current_volume}")

    # Create the DAW GUI window panel with full suite of controls
    # This includes: Play, Stop, Record Audio, Record Mida, Mix, List, Delete, and Volume slider.
    volume_slider = Slider(min=0, max=1, step=0.1, default=0.5, text='Volume', on_value_changed=on_volume_change)

    daw_gui = WindowPanel(
        title='DAW Controls',
        content=(
            Button(text='Play', on_click=on_play_click),
            Button(text='Stop', on_click=on_stop_click),
            Button(text='Record Audio', on_click=on_record_audio_click),
            Button(text='Record Mida', on_click=on_record_mida_click),
            Button(text='Mix', on_click=on_mix_click),
            Button(text='List', on_click=on_list_click),
            Button(text='Delete', on_click=on_delete_click),
            volume_slider
        ),
        enabled=False
    )
    showing_daw_gui = False

def toggle_daw_gui(player):
    global daw_gui, showing_daw_gui
    if daw_gui is None:
        setup_daw_gui()

    showing_daw_gui = not showing_daw_gui
    if showing_daw_gui:
        # Show DAW GUI
        daw_gui.enabled = True
        player.enabled = False
        mouse.locked = False
    else:
        # Hide DAW GUI
        daw_gui.enabled = False
        player.enabled = True
        mouse.locked = True
