#!/usr/bin/env python3

import argparse
import os
import sys
import sounddevice as sd
import soundfile as sf
import numpy as np
from pydub import AudioSegment

# Constants
TRACKS_DIR = "tracks"
SAMPLE_RATE = 44100  # Standard CD-quality sample rate
CHANNELS = 2         # Stereo recording

def ensure_tracks_dir():
    if not os.path.exists(TRACKS_DIR):
        os.makedirs(TRACKS_DIR)

def list_tracks():
    files = sorted([f for f in os.listdir(TRACKS_DIR) if f.endswith(".wav")])
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

def play_tracks():
    files = list_tracks()
    if not files:
        print("No tracks to play.")
        return

    print("Playing all tracks...")
    try:
        combined = None
        for file in files:
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
        sd.play(combined, SAMPLE_RATE)
        sd.wait()
    except Exception as e:
        print(f"An error occurred during playback: {e}")

def mix_tracks(output_filename="mixed_output.wav", volumes=None):
    files = list_tracks()
    if not files:
        print("No tracks to mix.")
        return

    print("Mixing tracks...")
    try:
        combined = None
        for idx, file in enumerate(files):
            track = AudioSegment.from_wav(os.path.join(TRACKS_DIR, file))
            # Apply volume adjustment if specified
            if volumes and idx < len(volumes):
                track += volumes[idx]  # PyDub uses dB for volume adjustment
            if combined is None:
                combined = track
            else:
                combined = combined.overlay(track)
        combined.export(output_filename, format="wav")
        print(f"Exported mixed track as {output_filename}")
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

def main():
    parser = argparse.ArgumentParser(description="MirrorDaw CLI")
    subparsers = parser.add_subparsers(dest='command')

    # Record command
    record_parser = subparsers.add_parser('record', help='Record a new track')
    record_parser.add_argument('track_number', type=int, help='Track number to record')
    record_parser.add_argument('duration', type=int, help='Duration in seconds')

    # Play command
    play_parser = subparsers.add_parser('play', help='Play all recorded tracks')

    # Mix command
    mix_parser = subparsers.add_parser('mix', help='Mix and export tracks')
    mix_parser.add_argument('--output', default='mixed_output.wav', help='Output filename')
    mix_parser.add_argument('--volumes', nargs='*', type=int, help='Volume adjustments in dB for each track')

    # List command
    list_parser = subparsers.add_parser('list', help='List all recorded tracks')

    # Delete command
    delete_parser = subparsers.add_parser('delete', help='Delete all recorded tracks')

    args = parser.parse_args()

    if args.command == 'record':
        record_track(args.track_number, args.duration)
    elif args.command == 'play':
        play_tracks()
    elif args.command == 'mix':
        mix_tracks(output_filename=args.output, volumes=args.volumes)
    elif args.command == 'list':
        tracks = list_tracks()
        if tracks:
            print("Recorded Tracks:")
            for track in tracks:
                print(f"- {track}")
        else:
            print("No tracks recorded yet.")
    elif args.command == 'delete':
        delete_tracks()
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
