# daw.py
from ursina import *

# A simple global reference to the DAW/GUI window
daw_gui = None
showing_daw_gui = False

def setup_daw_gui(player):
    """Creates a minimal window panel. 
       In the future, you could reintroduce audio/DAW controls here."""
    global daw_gui

    def test_button_click():
        print("DAW (or game GUI) button clicked!")

    daw_gui = WindowPanel(
        title='DAW/GUI',
        content=(
            Text("Imagine your audio, modding, or game options here."),
            Button(text='Test Action', on_click=test_button_click)
        ),
        enabled=False  # Start hidden
    )

def toggle_daw_gui(player):
    """Toggle the minimal DAW/GUI. Temporarily disable player movement when GUI is active."""
    global daw_gui, showing_daw_gui
    if daw_gui is None:
        setup_daw_gui(player)

    showing_daw_gui = not showing_daw_gui
    if showing_daw_gui:
        # Show DAW GUI, disable player movement
        daw_gui.enabled = True
        player.enabled = False
        mouse.locked = False
    else:
        # Hide DAW GUI, enable player movement
        daw_gui.enabled = False
        player.enabled = True
        mouse.locked = True
