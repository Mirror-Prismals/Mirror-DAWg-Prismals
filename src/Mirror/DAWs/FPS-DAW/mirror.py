#!/usr/bin/env python3

import argparse
import os
import sounddevice as sd
import soundfile as sf
import numpy as np
from pydub import AudioSegment
import re
from pythonosc.udp_client import SimpleUDPClient
import threading

# Constants
TRACKS_DIR = "tracks"
SAMPLE_RATE = 44100  # Standard CD-quality sample rate
CHANNELS = 2         # Stereo recording
OSC_ADDRESS = "127.0.0.1"  # Localhost
OSC_PORT = 5005  # Default OSC port

VALID_ROLES = ["left", "right", "front_fill", "sub"]  # Allowed roles

# OSC Client Setup
osc_client = SimpleUDPClient(OSC_ADDRESS, OSC_PORT)

def ensure_tracks_dir():
    if not os.path.exists(TRACKS_DIR):
        os.makedirs(TRACKS_DIR)

def list_tracks():
    files = sorted([f for f in os.listdir(TRACKS_DIR) if f.endswith(".wav") or f.endswith(".txt")])
    return files

def apply_role(data, role):
    """
    Apply stereo panning and role assignment to recorded audio.
    data: np.array with shape (samples, 2)  # Stereo data
    role: string, one of ["left", "right", "front_fill", "sub"]

    Returns processed stereo data.
    """
    if role not in VALID_ROLES:
        raise ValueError(f"Invalid role: {role}")

    # Separate the two channels
    left_channel = data[:, 0].copy()
    right_channel = data[:, 1].copy()

    if role == "left":
        # Pan fully to the left (mute right channel)
        data[:, 1] = 0.0
    elif role == "right":
        # Move left channel to the right
        data[:, 1] = data[:, 0]  # Copy left channel to the right channel
        data[:, 0] = 0.0         # Mute the original left channel
    elif role in ["front_fill", "sub"]:
        # Phantom center (average both channels)
        center = (left_channel + right_channel) / 2.0
        data[:, 0] = center
        data[:, 1] = center

    return data

def record_track(track_number, duration, role="left", device=None):
    ensure_tracks_dir()
    if role not in VALID_ROLES:
        print(f"Invalid role '{role}'. Defaulting to 'left'.")
        role = "left"
    filename = os.path.join(TRACKS_DIR, f"track_{track_number}_{role}.wav")
    print(f"Recording Track {track_number} for {duration} seconds as role '{role}'...")
    try:
        recording = sd.rec(int(duration * SAMPLE_RATE), samplerate=SAMPLE_RATE, channels=CHANNELS, device=device)
        sd.wait()
        recording = apply_role(recording, role)
        sf.write(filename, recording, SAMPLE_RATE)
        print(f"Track {track_number} ({role}) recorded and saved as {filename}")
    except Exception as e:
        print(f"An error occurred during recording: {e}")

def reassign_track_role(filename, new_role):
    """Load the given track, reapply the new role, and overwrite the file."""
    if new_role not in VALID_ROLES:
        print(f"Invalid role '{new_role}'. Cannot reassign.")
        return
    path = os.path.join(TRACKS_DIR, filename)
    if not os.path.exists(path):
        print(f"File {filename} does not exist.")
        return
    try:
        data, sr = sf.read(path)
        if data.ndim == 1:
            # Convert mono to stereo
            data = np.column_stack((data, data))
        data = apply_role(data, new_role)
        sf.write(path, data, sr)
        print(f"Reassigned {filename} to role '{new_role}'")
    except Exception as e:
        print(f"Error reassigning role: {e}")

def play_single_track(filename):
    """Play a single track for preview."""
    path = os.path.join(TRACKS_DIR, filename)
    if not os.path.exists(path):
        print(f"File {filename} does not exist.")
        return
    try:
        data, sr = sf.read(path)
        sd.play(data, sr)
        sd.wait()
    except Exception as e:
        print(f"Error playing track {filename}: {e}")

def play_audio_tracks(audio_tracks):
    """Play all audio tracks with proper stereo channel mapping."""
    try:
        combined = None
        for file in audio_tracks:
            data, sr = sf.read(os.path.join(TRACKS_DIR, file))

            # Ensure stereo output
            if data.ndim == 1:
                # If mono, duplicate channel for stereo
                data = np.column_stack((data, data))

            if combined is None:
                combined = data
            else:
                # Pad shorter arrays with zeros
                if data.shape[0] < combined.shape[0]:
                    data = np.pad(data, ((0, combined.shape[0] - data.shape[0]), (0, 0)), 'constant')
                elif data.shape[0] > combined.shape[0]:
                    combined = np.pad(combined, ((0, data.shape[0] - combined.shape[0]), (0, 0)), 'constant')
                combined += data

        if combined is not None:
            sd.play(combined, sr)
            sd.wait()
    except Exception as e:
        print(f"An error occurred during audio playback: {e}")

def send_osc_note_message(note, velocity=64, channel=1):
    """Send an OSC message representing a MIDI note on/off event and return the message string."""
    note_map = {
        "C": 0, "C#": 1, "Db": 1, "D": 2, "D#": 3, "Eb": 3,
        "E": 4, "F": 5, "F#": 6, "Gb": 6, "G": 7, "G#": 8,
        "Ab": 8, "A": 9, "A#": 10, "Bb": 10, "B": 11
    }
    match = re.match(r'([A-G][#b]?)(-?\d+)', note)
    if not match:
        message = f"Invalid note format: {note}"
        print(message)
        return message
    note_name, octave = match.groups()
    note_number = 12 * (int(octave) + 1) + note_map.get(note_name, 0)
    osc_client.send_message("/midi/note", [channel, note_number, velocity])
    return ""

def parse_mida_data(mida_data):
    """Parse Mida notation into a list of events."""
    tokens = re.findall(r'([A-G][#b]?-?\d+|\.|\-|~)', mida_data)
    event_list = []
    for token in tokens:
        if token in VALID_ROLES:
            event_list.append(token)
        else:
            event_list.append(token)
    return event_list

def parse_mida_file(filename):
    """Parse a Mida file into a list of events."""
    try:
        with open(os.path.join(TRACKS_DIR, filename), 'r') as f:
            mida_data = f.read()
            event_list = parse_mida_data(mida_data)
            return event_list
    except Exception as e:
        print(f"An error occurred while parsing Mida file {filename}: {e}")
        return []

def play_mida_tracks(mida_tracks, bpm):
    """Play all Mida tracks in sync based on BPM."""
    all_event_lists = []
    for filename in mida_tracks:
        event_list = parse_mida_file(filename)
        all_event_lists.append(event_list)

    if not all_event_lists:
        return

    sixteenth_note_duration = 60 / (bpm * 4)
    max_length = max(len(ev) for ev in all_event_lists)

    # Pad shorter event lists with spaces
    for idx, ev in enumerate(all_event_lists):
        if len(ev) < max_length:
            all_event_lists[idx].extend([' '] * (max_length - len(ev)))

    for i in range(max_length):
        line_output = []
        for ev in all_event_lists:
            event = ev[i]
            output_line = event
            if event not in ['-', '.', ' ']:
                notes = event.split("~")
                for note in notes:
                    osc_message = send_osc_note_message(note)
                    output_line += f" {osc_message}"
            line_output.append(output_line)
        print(' '.join(line_output))
        sd.sleep(int(sixteenth_note_duration * 1000))

def mix_tracks(volumes, roles):
    """
    Process and mix tracks for each role (front_fill, left, right, sub).
    Export the summed tracks as stems into TRACKS_DIR.
    """
    output_filenames = {
        "front_fill": "front_fill_stem.wav",
        "left": "left_stem.wav",
        "right": "right_stem.wav",
        "sub": "sub_stem.wav",
    }

    # Initialize channels as None for each role
    channels = {role: None for role in roles}
    print(f"Initialized channels for roles: {roles}")

    # Iterate over files in the TRACKS_DIR to sort tracks into roles
    for file in os.listdir(TRACKS_DIR):
        if not file.lower().endswith(".wav"):
            continue

        # Skip output stems to prevent feedback loop
        if file.lower().endswith("_stem.wav") or file.lower().endswith("_mixed.wav"):
            continue

        # Check if the file ends with one of the roles (e.g., track_1_left.wav)
        for role in roles:
            if file.lower().endswith(f"_{role}.wav"):
                file_path = os.path.join(TRACKS_DIR, file)
                print(f"Adding {file} to role: {role}")

                # Load the track
                try:
                    track = AudioSegment.from_wav(file_path)
                except Exception as e:
                    print(f"Error loading {file}: {e}")
                    continue

                print(f"Loaded track {file} with length: {len(track)} ms")

                # Apply volume adjustment based on the passed volumes
                track = track + volumes.get(role, 0)

                # Overlay the track on the corresponding role channel
                if channels[role] is None:
                    channels[role] = track
                else:
                    channels[role] = channels[role].overlay(track)
    OUTPUT_DIR = "masters"
    # Export the summed tracks for each role to new files
    for role, audio in channels.items():
        output_path = os.path.join(OUTPUT_DIR, f"{role}_stem.wav")  # Use role-specific names for the output

        if audio is not None:
            print(f"Exporting {role} stem.")
            try:
                audio.export(output_path, format="wav")
                print(f"Exported {role} stem to {output_path}")
            except Exception as e:
                print(f"Error exporting {role} stem: {e}")
        else:
            print(f"No tracks for role '{role}'. Skipping export.")

def delete_tracks():
    """Delete all recorded tracks after user confirmation."""
    confirm = input("Are you sure you want to delete all tracks? This cannot be undone. (yes/no): ")
    if confirm.lower() == 'yes':
        for file in list_tracks():
            os.remove(os.path.join(TRACKS_DIR, file))
        print("All tracks have been deleted.")
    else:
        print("Deletion cancelled.")

def record_mida_track(track_number, mida_data):
    """Save Mida data to a file."""
    ensure_tracks_dir()
    filename = os.path.join(TRACKS_DIR, f"track_M{track_number}.txt")
    print(f"Recording Mida Track {track_number}...")
    try:
        with open(filename, 'w') as f:
            f.write(mida_data)
        print(f"Mida Track {track_number} saved as {filename}")
    except Exception as e:
        print(f"An error occurred while saving Mida track: {e}")

def main():
    parser = argparse.ArgumentParser(description="MirrorDaw CLI")
    subparsers = parser.add_subparsers(dest='command')

    # Record command
    record_parser = subparsers.add_parser('record', help='Record a new audio track')
    record_parser.add_argument('track_number', type=int, help='Track number to record')
    record_parser.add_argument('duration', type=int, help='Duration in seconds')
    record_parser.add_argument('--role', default='left', help='Role: left, right, front_fill, sub')
    record_parser.add_argument('--device', type=int, default=None, help='Input device index')

    # Play command
    play_parser = subparsers.add_parser('play', help='Play all recorded tracks')
    play_parser.add_argument('--bpm', type=int, default=120, help='BPM for Mida tracks')

    # Mix command
    mix_parser = subparsers.add_parser('mix', help='Mix and export tracks')
    mix_parser.add_argument('--output_dir', default='mixed_outputs', help='Directory to save mixed outputs')
    mix_parser.add_argument('--volumes', nargs='*', type=int, help='Volume adjustments in dB for each role (left, front_fill, sub, right)')
    mix_parser.add_argument('--roles', nargs='*', help='Roles to include in mixing (e.g. left front_fill sub right)')

    # List command
    list_parser = subparsers.add_parser('list', help='List all recorded tracks')

    # Delete command
    delete_parser = subparsers.add_parser('delete', help='Delete all recorded tracks')

    # Mida interpret command
    mida_parser = subparsers.add_parser('mida', help='Interpret Mida notation')
    mida_parser.add_argument('mida_data', type=str, help='Mida data string')
    mida_parser.add_argument('--bpm', type=int, default=120, help='BPM for timing the Mida data')

    # Mida record command
    mida_record_parser = subparsers.add_parser('mida_record', help='Record a new Mida track')
    mida_record_parser.add_argument('track_number', type=int, help='Track number to record')
    mida_record_parser.add_argument('mida_data', type=str, help='Mida data string')

    args = parser.parse_args()

    if args.command == 'record':
        record_track(args.track_number, args.duration, role=args.role, device=args.device)
    elif args.command == 'play':
        bpm = args.bpm
        files = list_tracks()
        if not files:
            print("No tracks to play.")
            return

        print("Playing all tracks...")
        audio_tracks = [f for f in files if f.endswith('.wav')]
        mida_tracks = [f for f in files if f.endswith('.txt')]

        # Start audio and Mida playback concurrently
        threads = []

        if audio_tracks:
            audio_thread = threading.Thread(target=play_audio_tracks, args=(audio_tracks,))
            audio_thread.start()
            threads.append(audio_thread)

        if mida_tracks:
            mida_thread = threading.Thread(target=play_mida_tracks, args=(mida_tracks, bpm))
            mida_thread.start()
            threads.append(mida_thread)

        for t in threads:
            t.join()

    elif args.command == 'mix':
        # Prepare volume adjustments
        if args.volumes:
            if len(args.volumes) != 4:
                print("Please provide exactly four volume adjustments for roles: left, front_fill, sub, right.")
                return
            volumes = {
                "left": args.volumes[0],
                "front_fill": args.volumes[1],
                "sub": args.volumes[2],
                "right": args.volumes[3]
            }
        else:
            volumes = {
                "left": 0,
                "front_fill": 0,
                "sub": 0,
                "right": 0
            }

        # Prepare roles
        if args.roles:
            roles = args.roles
        else:
            roles = VALID_ROLES  # Mix all roles by default

        # Call mix_tracks from mirror.py with correct arguments
        mix_tracks(volumes=volumes, roles=roles)

    elif args.command == 'list':
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

    elif args.command == 'delete':
        delete_tracks()

    elif args.command == 'mida':
        # For testing purposes, interpret Mida data directly
        event_list = parse_mida_data(args.mida_data)
        bpm = args.bpm
        sixteenth_note_duration = 60 / (bpm * 4)
        for event in event_list:
            output_line = event
            if event not in ['-', '.', ' ']:
                notes = event.split("~")
                for note in notes:
                    osc_message = send_osc_note_message(note)
                    output_line += f" {osc_message}"
            print(output_line)
            sd.sleep(int(sixteenth_note_duration * 1000))

    elif args.command == 'mida_record':
        record_mida_track(args.track_number, args.mida_data)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
