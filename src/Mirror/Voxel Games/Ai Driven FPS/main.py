# main.py
from ursina import *
from ursina.prefabs.first_person_controller import FirstPersonController

# We import our "AI Controller" (and anything else) from mirror.py
from mirror import AIFPSController, sample_script

# We import a minimal DAW/GUI toggler from daw.py
from daw import toggle_daw_gui

def input(key):
    """Handle global input. For example, press 'e' to toggle the DAW/GUI."""
    if key == 'e':
        toggle_daw_gui(player)

app = Ursina()

# Create a ground of voxels (blocks).
for z in range(10):
    for x in range(10):
        Entity(
            model='cube',
            color=color.white,
            collider='box',
            position=(x,0,z),
            texture='white_cube'
        )

# The human player's FPS controller
player = FirstPersonController()

# Create a basic AI FPS Controller bot
bot = AIFPSController(position=(5,1,5))
# Load a simple, hardcoded script from mirror.py
bot.load_script(sample_script)

app.run()
