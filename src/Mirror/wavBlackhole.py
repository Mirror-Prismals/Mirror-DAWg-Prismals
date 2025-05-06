import os
from tkinter import Tk, Label
from tkinterdnd2 import TkinterDnD, DND_FILES
from pydub import AudioSegment
from pathlib import Path

# Function to handle the file conversion
def convert_to_wav(mp3_file):
    try:
        downloads_folder = Path.home() / "Downloads"
        if not downloads_folder.exists():
            downloads_folder.mkdir(parents=True, exist_ok=True)

        # Load MP3 and export as WAV
        sound = AudioSegment.from_mp3(mp3_file)
        wav_file = downloads_folder / (os.path.basename(mp3_file).replace(".mp3", ".wav"))
        sound.export(wav_file, format="wav")
        print(f"Converted and saved: {wav_file}")
    except Exception as e:
        print(f"Error: {e}")

# Setup GUI
def setup_gui():
    root = TkinterDnD.Tk()  # TkinterDnD for drag-and-drop
    root.title("MP3 to WAV Converter")
    root.geometry("400x200")
    root.resizable(False, False)

    label = Label(root, text="Drag your .mp3 files here", bg="lightblue", font=("Arial", 14), relief="solid", borderwidth=1)
    label.pack(expand=True, fill="both", padx=10, pady=10)

    def handle_drop(event):
        files = root.tk.splitlist(event.data)
        for file in files:
            if file.endswith(".mp3"):
                convert_to_wav(file)
            else:
                print(f"Skipped non-MP3 file: {file}")

    root.drop_target_register(DND_FILES)
    root.dnd_bind("<<Drop>>", handle_drop)

    root.mainloop()

if __name__ == "__main__":
    setup_gui()
