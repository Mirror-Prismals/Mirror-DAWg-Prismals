#!/usr/bin/env python3

import argparse
import os
import time
import threading
import sounddevice as sd
import soundfile as sf
import numpy as np
from pydub import AudioSegment
import re
from pythonosc.udp_client import SimpleUDPClient

# Constants
TRACKS_DIR = "tracks"
SAMPLE_RATE = 44100  # Standard CD-quality sample rate
CHANNELS = 2         # Stereo recording
OSC_ADDRESS = "127.0.0.1"  # Localhost
OSC_PORT = 5005  # Default OSC port

# OSC Client Setup
osc_client = SimpleUDPClient(OSC_ADDRESS, OSC_PORT)

def ensure_tracks_dir():
    if not os.path.exists(TRACKS_DIR):
        os.makedirs(TRACKS_DIR)

def list_tracks():
    files = sorted([f for f in os.listdir(TRACKS_DIR) if f.endswith(".wav") or f.endswith(".txt")])
    return files

def record_track(track_number, duration):
    ensure_tracks_dir()
    filename = os.path.join(TRACKS_DIR, f"track_{track_number}.wav")
    print(f"Recording Track {track_number} for {duration} seconds...")
    try:
        recording = sd.rec(int(duration * SAMPLE_RATE), samplerate=SAMPLE_RATE, channels=CHANNELS)
        sd.wait()  # Wait until recording is finished
        sf.write(filename, recording, SAMPLE_RATE)
        print(f"Track {track_number} recorded and saved as {filename}")
    except Exception as e:
        print(f"An error occurred during recording: {e}")

def play_audio_tracks(audio_tracks):
    try:
        combined = None
        for file in audio_tracks:
            data, sr = sf.read(os.path.join(TRACKS_DIR, file))
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
            sd.play(combined, SAMPLE_RATE)
            sd.wait()
    except Exception as e:
        print(f"An error occurred during audio playback: {e}")

def send_osc_note_message(note, velocity=64, channel=1):
    """Send an OSC message representing a MIDI note on/off event and return the message string."""
    # Map note to MIDI number (e.g., "C4" to 60)
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
    note_number = 12 * (int(octave) + 1) + note_map[note_name]  # Calculate MIDI note number

    # Send the OSC message
    osc_client.send_message("/midi/note", [channel, note_number, velocity])
    message = ""# f"Ch={channel},N={note_number},V={velocity}"
    return message

def play_mida_tracks(mida_tracks, bpm):
    # Parse all Mida tracks into event lists
    all_event_lists = []
    for filename in mida_tracks:
        event_list = parse_mida_file(filename)
        all_event_lists.append(event_list)

    if not all_event_lists:
        return

    sixteenth_note_duration = 60 / (bpm * 4)  # BPM to 16th note in seconds

    # Determine the maximum length among all event lists
    max_length = max(len(event_list) for event_list in all_event_lists)

    # Pad shorter event lists with spaces
    for idx, event_list in enumerate(all_event_lists):
        if len(event_list) < max_length:
            event_list.extend([' '] * (max_length - len(event_list)))
        all_event_lists[idx] = event_list

    # Play back the Mida tracks in sync
    for i in range(max_length):
        line_output = []
        osc_messages = []
        for event_list in all_event_lists:
            event = event_list[i]
            output_line = event
            if event not in ['-', '.', ' ']:  # Only send OSC for note events
                notes = event.split("~")  # Handle chords or simultaneous notes
                for note in notes:
                    osc_message = send_osc_note_message(note)
                    output_line += f" {osc_message}"
            line_output.append(output_line)
        print(' '.join(line_output))
        time.sleep(sixteenth_note_duration)

def parse_mida_file(filename):
    try:
        with open(os.path.join(TRACKS_DIR, filename), 'r') as f:
            mida_data = f.read()
            event_list = parse_mida_data(mida_data)
            return event_list
    except Exception as e:
        print(f"An error occurred while parsing Mida file {filename}: {e}")
        return []  # Return empty list if error

def parse_mida_data(mida_data):
    # This function parses the Mida data and returns a list of events
    tokens = re.findall(r'([A-G][#b]?-?\d+|\.\d*|\-\d*|~|\d+)', mida_data)
    event_list = []
    idx = 0
    while idx < len(tokens):
        token = tokens[idx]
        if token == '~':
            # Handle simultaneous notes
            notes = []
            idx += 1
            while idx < len(tokens) and re.match(r'[A-G][#b]?-?\d+', tokens[idx]):
                notes.append(tokens[idx])
                idx += 1
            event_list.append('~'.join(notes))
        elif token.startswith('.'):
            # Rest
            length = int(token[1:]) if token[1:].isdigit() else 1
            for _ in range(length):
                event_list.append('.')
            idx += 1
        elif token.startswith('-'):
            # Sustain
            length = int(token[1:]) if token[1:].isdigit() else 1
            for _ in range(length):
                event_list.append('-')
            idx += 1
        elif re.match(r'[A-G][#b]?-?\d+', token):
            # Note
            note = token
            idx += 1
            # Check if next token is a length modifier
            if idx < len(tokens) and tokens[idx].isdigit():
                length = int(tokens[idx])
                idx += 1
            else:
                length = 1
            event_list.append(note)
            for _ in range(length - 1):
                event_list.append('-')
        else:
            idx += 1  # Skip any unrecognized tokens
    return event_list

def mix_tracks(output_filename="mixed_output.wav", volumes=None):
    files = list_tracks()
    if not files:
        print("No tracks to mix.")
        return

    print("Mixing tracks...")
    try:
        combined = None
        idx_audio = 0  # Index for volumes list
        for file in files:
            if file.endswith('.wav'):
                track = AudioSegment.from_wav(os.path.join(TRACKS_DIR, file))
                # Apply volume adjustment if specified
                if volumes and idx_audio < len(volumes):
                    track += volumes[idx_audio]  # PyDub uses dB for volume adjustment
                    idx_audio += 1
                if combined is None:
                    combined = track
                else:
                    combined = combined.overlay(track)
            # Mida tracks are not included in the audio mix
        if combined:
            combined.export(output_filename, format="wav")
            print(f"Exported mixed track as {output_filename}")
        else:
            print("No audio tracks to mix.")
    except Exception as e:
        print(f"An error occurred during mixing: {e}")

def delete_tracks():
    confirm = input("Are you sure you want to delete all tracks? This cannot be undone. (yes/no): ")
    if confirm.lower() == 'yes':
        for file in list_tracks():
            os.remove(os.path.join(TRACKS_DIR, file))
        print("All tracks have been deleted.")
    else:
        print("Deletion cancelled.")

def record_mida_track(track_number, mida_data):
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

    # Play command
    play_parser = subparsers.add_parser('play', help='Play all recorded tracks')
    play_parser.add_argument('--bpm', type=int, default=120, help='BPM for Mida tracks')

    # Mix command
    mix_parser = subparsers.add_parser('mix', help='Mix and export tracks')
    mix_parser.add_argument('--output', default='mixed_output.wav', help='Output filename')
    mix_parser.add_argument('--volumes', nargs='*', type=int, help='Volume adjustments in dB for each track')

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
        record_track(args.track_number, args.duration)
    elif args.command == 'play':
        bpm = args.bpm
        files = list_tracks()
        if not files:
            print("No tracks to play.")
            return

        print("Playing all tracks...")
        audio_tracks = []
        mida_tracks = []

        for file in files:
            if file.endswith('.wav'):
                audio_tracks.append(file)
            elif file.endswith('.txt'):
                mida_tracks.append(file)

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
        mix_tracks(output_filename=args.output, volumes=args.volumes)
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
            time.sleep(sixteenth_note_duration)
    elif args.command == 'mida_record':
        record_mida_track(args.track_number, args.mida_data)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
