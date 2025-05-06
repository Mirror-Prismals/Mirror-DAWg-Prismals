from ursina import *
from ursina.prefabs.first_person_controller import FirstPersonController
import subprocess
import threading

app = Ursina()
window.fullscreen = True
grass_texture = load_texture('assets/tes_block.png')
stone_texture = load_texture('assets/stonemez.png')
biome_texture = load_texture('assets/grassmez.png')
sky_texture = load_texture('assets/skygradient.png')
arm_texture = load_texture('assets/arm4.png')

block_pick = 1
is_flying = False
debug_mode = False
debug_text = None

def update():
    global block_pick, is_flying, debug_mode, debug_text
    if held_keys['left mouse'] or held_keys['right mouse']:
        hand.active()
    else:
        hand.passive()

    if held_keys['1']: block_pick = 1
    if held_keys['2']: block_pick = 2
    if held_keys['3']: block_pick = 3

    if held_keys['p']:
        is_flying = not is_flying
        player.gravity = 0 if is_flying else 1
        player.jump_height = 0 if is_flying else 2
        player.speed = player.flying_speed if is_flying else player.walking_speed

    if is_flying:
        if held_keys['space']:
            player.y += player.flying_speed * time.dt
        if held_keys['shift']:
            player.y -= player.flying_speed * time.dt

    if held_keys['f3']:
        debug_mode = not debug_mode
        if debug_mode:
            debug_text = Text(
                text='',
                position=(-0.5, 0.4),
                origin=(0, 0),
                background=True
            )
        else:
            if debug_text:
                destroy(debug_text)
                debug_text = None

    if debug_mode and debug_text:
        x, y, z = player.position
        orientation = player.rotation_y
        cardinal_direction = ''
        if 45 <= orientation < 135:
            cardinal_direction = 'East'
        elif 135 <= orientation < 225:
            cardinal_direction = 'South'
        elif 225 <= orientation < 315:
            cardinal_direction = 'West'
        else:
            cardinal_direction = 'North'

        debug_text.text = f'Coordinates: ({x:.2f}, {y:.2f}, {z:.2f})\nOrientation: {orientation:.2f}\nDirection: {cardinal_direction}'

def play_chuck_script():
    subprocess.run(["chuck", "dirt_break.ck"])

class Voxel(Button):
    def __init__(self, position=(0,0,0), texture=grass_texture):
        super().__init__(
            parent=scene,
            position=position,
            model='Grass_block',
            origin_y=0.5,
            texture=texture,
            color=color.color(1, 0, random.uniform(0.9, 1), 1,),
            scale=0.5
        )

    def input(self, key):
        if self.hovered:
            if key == 'right mouse down':
                # Call the ChucK script in a new thread
                threading.Thread(target=play_chuck_script).start()
                if block_pick == 1: voxel = Voxel(position=self.position + mouse.normal, texture=grass_texture)
                if block_pick == 2: voxel = Voxel(position=self.position + mouse.normal, texture=stone_texture)
                if block_pick == 3: voxel = Voxel(position=self.position + mouse.normal, texture=biome_texture)

            if key == 'left mouse down':
                # Call the ChucK script in a new thread
                threading.Thread(target=play_chuck_script).start()
                destroy(self)

class Sky(Entity):
    def __init__(self):
        super().__init__(
            parent=scene,
            model='sphere',
            texture=sky_texture,
            scale=1800,
            double_sided=True
        )

class Hand(Entity):
    def __init__(self):
        super().__init__(
            parent=camera.ui,
            model='arm',
            texture=arm_texture,
            scale=0.2,
            rotation=Vec3(330, -10, 0),
            position=Vec2(0.7, -0.6)
        )

    def active(self):
        self.position = Vec2(0.68, -0.58)

    def passive(self):
        self.position = Vec2(0.7, -0.6)

# Create a grid of voxels with a stone ring around the grass
for z in range(-10, 10):
    for x in range(-10, 10):
        if x == -10 or x == 9 or z == -10 or z == 9:
            voxel = Voxel(position=(x, 0, z), texture=stone_texture)
        else:
            voxel = Voxel(position=(x, 0, z), texture=grass_texture)

player = FirstPersonController()
player.walking_speed = 5
player.flying_speed = 10
sky = Sky()
hand = Hand()
app.run()
