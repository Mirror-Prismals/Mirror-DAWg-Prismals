from ursina import *
from ursina.prefabs.first_person_controller import FirstPersonController
import threading
import time

# Import your DAW functions from mirror.py
from mirror import (
    list_tracks,
    record_track,
    play_audio_tracks,
    play_mida_tracks,
    mix_tracks,
    record_mida_track,
    parse_mida_data
)

# Import the DAW GUI toggle function from daw.py
from daw import toggle_daw_gui

class Voxel(Button):
    def __init__(self, position=(0,0,0)):
        super().__init__(
            parent=scene,
            position=position,
            model='cube',
            origin_y=0.5,
            texture='brick',
            color=color.white,
            highlight_color=color.lime,
        )
    def on_click(self):
        print("Voxel clicked!")

class DAWController(Entity):
    def __init__(self):
        super().__init__()
        self.bpm = 120

    def play_all(self):
        files = list_tracks()
        audio_tracks = [f for f in files if f.endswith('.wav')]
        mida_tracks = [f for f in files if f.endswith('.txt')]

        threads = []
        if audio_tracks:
            audio_thread = threading.Thread(target=play_audio_tracks, args=(audio_tracks,))
            audio_thread.start()
            threads.append(audio_thread)

        if mida_tracks:
            mida_thread = threading.Thread(target=play_mida_tracks, args=(mida_tracks, self.bpm))
            mida_thread.start()
            threads.append(mida_thread)
        print("Playback started.")

    def record_new_track(self, track_number=1, duration=5):
        def rec_thread():
            record_track(track_number, duration)
            print("Recording finished!")
        threading.Thread(target=rec_thread).start()

    def record_mida(self, track_number=2, mida_data="C4 C4 E4 G4"):
        record_mida_track(track_number, mida_data)

    def mix(self, output='mixed_output.wav'):
        def mix_thread():
            mix_tracks(output_filename=output)
        threading.Thread(target=mix_thread).start()

def input(key):
    if key == 'p':
        print("Playing tracks...")
        daw.play_all()
    if key == 'r':
        print("Recording a new audio track...")
        daw.record_new_track(track_number=1, duration=5)
    if key == 'm':
        print("Recording a Mida track...")
        daw.record_mida(track_number=2, mida_data="C4 E4 G4 . C5 E5 G5 . C6 E6 G6")
    if key == 'o':
        print("Mixing tracks...")
        daw.mix()
    if key == 'e':
        # Toggle the DAW GUI on/off
        toggle_daw_gui(player)

app = Ursina()

for z in range(8):
    for x in range(8):
        voxel = Voxel(position=(x,0,z))

player = FirstPersonController()
daw = DAWController()

app.run()
