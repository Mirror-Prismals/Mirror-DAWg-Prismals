import os
from pydub import AudioSegment

# Define the roles (stem types)
VALID_ROLES = ["left", "right", "front_fill", "sub"]
INPUT_DIR = "tracks"    # Directory containing input tracks
OUTPUT_DIR = "output_stems"   # Directory to save output stems

# Desired properties
DESIRED_SAMPLE_RATE = 44100
DESIRED_CHANNELS = 2

def normalize_audio(audio):
    """
    Normalize audio to desired sample rate and channels.
    """
    if audio.frame_rate != DESIRED_SAMPLE_RATE:
        audio = audio.set_frame_rate(DESIRED_SAMPLE_RATE)
    if audio.channels != DESIRED_CHANNELS:
        audio = audio.set_channels(DESIRED_CHANNELS)
    return audio

def mix_tracks():
    """
    Sort every track in INPUT_DIR into its role based on filename,
    mix the tracks for each role, and export four stems.
    """
    # Initialize a dictionary to hold lists of tracks for each role
    role_tracks = {role: [] for role in VALID_ROLES}

    # Ensure output directory exists
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)
        print(f"Created output directory '{OUTPUT_DIR}'.")

    # Get a list of files in the input directory, excluding already exported stems
    try:
        files = os.listdir(INPUT_DIR)
    except FileNotFoundError:
        print(f"Input directory '{INPUT_DIR}' does not exist. Please create it and add your tracks.")
        return

    print(f"Found {len(files)} file(s) in '{INPUT_DIR}' directory.")

    # Filter out the stem files to avoid processing them
    input_files = [f for f in files if f.lower().endswith(".wav") and not f.lower().endswith("_stem.wav")]

    print(f"Processing {len(input_files)} input file(s).")

    # Iterate over files and assign to roles
    for file in input_files:
        lower_file = file.lower()
        matched = False
        for role in VALID_ROLES:
            if role in lower_file:
                file_path = os.path.join(INPUT_DIR, file)
                role_tracks[role].append(file_path)
                print(f"Adding '{file}' to role: '{role}'")
                matched = True
                break  # Assuming one role per file
        if not matched:
            print(f"File '{file}' does not match any role. Skipping.")

    # Now, mix the tracks for each role
    for role in VALID_ROLES:
        tracks = role_tracks[role]
        if not tracks:
            print(f"No tracks for role '{role}'. Exporting silence.")
            # Export silence for this role
            silence = AudioSegment.silent(duration=1000)  # 1 second of silence
            output_path = os.path.join(OUTPUT_DIR, f"{role}_stem.wav")
            silence.export(output_path, format="wav")
            print(f"Exported silence for role '{role}' to '{output_path}'")
            continue

        print(f"Mixing {len(tracks)} track(s) for role '{role}'")

        try:
            # Start with the first track
            combined = AudioSegment.from_wav(tracks[0])
            combined = normalize_audio(combined)
            print(f"Loaded '{os.path.basename(tracks[0])}' as base for role '{role}' (Sample Rate: {combined.frame_rate}, Channels: {combined.channels})")

            # Overlay the rest of the tracks
            for track_path in tracks[1:]:
                track = AudioSegment.from_wav(track_path)
                track = normalize_audio(track)
                print(f"Loaded '{os.path.basename(track_path)}' for role '{role}' (Sample Rate: {track.frame_rate}, Channels: {track.channels})")
                combined = combined.overlay(track)
                print(f"Overlayed '{os.path.basename(track_path)}' onto role '{role}' stem")

            # Export the mixed stem
            output_path = os.path.join(OUTPUT_DIR, f"{role}_stem.wav")
            combined.export(output_path, format="wav")
            print(f"Exported mixed stem for role '{role}' to '{output_path}' (Duration: {len(combined)} ms)")

        except Exception as e:
            print(f"Error processing role '{role}': {e}")

    print("Mixing complete.")

# Run the script to mix the tracks
if __name__ == "__main__":
    mix_tracks()
