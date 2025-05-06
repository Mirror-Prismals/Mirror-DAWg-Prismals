from ursina import *
from ursina.prefabs.first_person_controller import FirstPersonController
from math import sin, cos, radians
from daw import toggle_daw_gui
# Constants
TIME_MULTIPLIER = 1  # Real-time seconds multiplied (higher = faster cycle)
current_time = 0.0  # Start time (6 AM)
FULL_DAY_DEGREES = 360
SUNRISE_ANGLE = -90

# Sprint and Jump Mechanics
SPRINT_SPEED = 30       # Speed while sprinting
WALK_SPEED = 10         # Normal walk speed
JUMP_FORCE_BASE = 20   # Base jump force
JUMP_FORCE_INCREMENT = 10  # Additional force per combo jump
MAX_JUMP_COMBO = 3      # Maximum combo jumps
JUMP_COMBO_TIME = 0.9   # Time window for combo jump
HORIZONTAL_BOOST = 0.1  # Speed boost multiplier for combo jumps
GRAVITY = -0.05         # Gravity applied per frame

# Combo jump variables
jump_combo = 0
combo_timer = 0.0
is_sprinting = False  # Track sprinting state

# Skybox textures
sky_textures = [
    'assets/morning_sky.jpg',
    'assets/afternoon_sky.jpg',
    'assets/evening_sky.jpg',
    'assets/night_sky.jpg',
]

# Player velocity
player_velocity = Vec3(0, 0, 0)  # Initial velocity vector


# Calculate positions of the sun and moon
def calculate_sun_and_moon_positions(current_time):
    orbital_radius = 1000
    day_fraction = (current_time - 6) / 24
    sun_angle = day_fraction * FULL_DAY_DEGREES + SUNRISE_ANGLE
    moon_angle = (day_fraction + 0.5) * FULL_DAY_DEGREES + SUNRISE_ANGLE

    sun_position = Vec3(
        cos(radians(sun_angle)) * orbital_radius,
        sin(radians(sun_angle)) * orbital_radius,
        0
    )
    moon_position = Vec3(
        cos(radians(moon_angle)) * orbital_radius,
        sin(radians(moon_angle)) * orbital_radius,
        0
    )

    return sun_position, moon_position


# Update the skybox blending
def update_skybox():
    global current_time

    current_time += (time.dt * TIME_MULTIPLIER) / 3600
    if current_time >= 24:
        current_time -= 24

    time_period = int((current_time // 6) % 4)
    blend_factor = (current_time % 6) / 6.0

    blend_factors = Vec4(0, 0, 0, 0)
    blend_factors[time_period] = 1.0 - blend_factor
    blend_factors[(time_period + 1) % 4] = blend_factor
    sky.set_shader_input("blend_factors", blend_factors)


# Update the positions of the sun and moon
def update_sun_and_moon():
    sun_position, moon_position = calculate_sun_and_moon_positions(current_time)
    sun.position = sun_position
    moon.position = moon_position
# Constants for jump smoothness
JUMP_ACCELERATION = 50  # Rate of upward acceleration
jumping = False  # Track if the player is in a jump phase

def update_clock_display():
    """Update the in-game clock display."""
    clock_display.text = f"In-Game Time: {int(current_time):02d}:{int((current_time % 1) * 60):02d}"

def input(key):
    global jump_combo, combo_timer, is_sprinting, player_velocity, jumping
    if key == 'e':
        toggle_daw_gui(player)
    if key == 'space':
        # Check ground proximity with raycast
        hit_info = raycast(player.position + Vec3(0, 0.5, 0), Vec3(0, -1, 0), ignore=(player,))
        if hit_info.hit and hit_info.distance <= 1.1:
            jump_combo += 1
            combo_timer = JUMP_COMBO_TIME  # Reset the combo timer

            # Start jump with smooth acceleration
            jumping = True
            player_velocity.y = 0  # Reset vertical velocity

            # Apply horizontal boost for combo jumps
            horizontal_boost = player.forward * HORIZONTAL_BOOST * jump_combo
            player.position += horizontal_boost

    if key == 'shift':
        is_sprinting = True
        player.speed = SPRINT_SPEED

    if key == 'shift up':
        is_sprinting = False
        player.speed = WALK_SPEED


def update():
    global combo_timer, player_velocity, jump_combo, jumping

    update_skybox()
    update_sun_and_moon()
    update_clock_display()
    # Smooth upward acceleration for jumping
    if jumping:
        target_jump_force = JUMP_FORCE_BASE + (jump_combo - 1) * JUMP_FORCE_INCREMENT
        if player_velocity.y < target_jump_force:
            player_velocity.y += JUMP_ACCELERATION * time.dt
        else:
            jumping = False  # Stop accelerating once target force is reached

    # Apply gravity if not jumping
    if not jumping:
        player_velocity.y += GRAVITY * time.dt

    # Apply velocity to player
    player.position += player_velocity * time.dt

    # Raycast for ground detection
    hit_info = raycast(player.position + Vec3(0, 0.5, 0), Vec3(0, -1, 0), ignore=(player,))
    if hit_info.hit and hit_info.distance <= 1.1:
        if not jumping:
            # Correct player position and reset combo
            player.position.y = max(player.position.y, hit_info.world_point.y + 0.1)
            player_velocity.y = 0
            jump_combo = 0
            combo_timer = 0
    else:
        jumping = True

    # Handle combo timing
    if combo_timer > 0:
        combo_timer -= time.dt
    else:
        jump_combo = 0
def main():
    app = Ursina()

    # Load the terrain
    terrain = Entity(
        model='assets/terrain.obj',
        texture='assets/terrain_texture.png',
        scale=(1, 1, 1,),
        position=(0, 0, -0),
        collider='mesh'
    )

    # Add the skybox
    global sky
    sky = Entity(
        model='sphere',
        scale=(10000, 10000, 10000),
        double_sided=True,
        shader=Shader(language=Shader.GLSL, vertex='''  
            #version 140
            uniform mat4 p3d_ModelViewProjectionMatrix;
            in vec4 vertex;
            in vec2 uv;
            out vec2 texcoord;
            void main() {
                gl_Position = p3d_ModelViewProjectionMatrix * vertex;
                texcoord = uv;
            }
        ''', fragment='''  
            #version 140
            uniform sampler2D texture1;
            uniform sampler2D texture2;
            uniform sampler2D texture3;
            uniform sampler2D texture4;
            uniform vec4 blend_factors;
            in vec2 texcoord;
            out vec4 fragColor;
            void main() {
                vec4 color1 = texture(texture1, texcoord) * blend_factors.x;
                vec4 color2 = texture(texture2, texcoord) * blend_factors.y;
                vec4 color3 = texture(texture3, texcoord) * blend_factors.z;
                vec4 color4 = texture(texture4, texcoord) * blend_factors.w;
                fragColor = color1 + color2 + color3 + color4;
            }
        ''')
    )
    sky.set_shader_input("texture1", load_texture(sky_textures[0]))
    sky.set_shader_input("texture2", load_texture(sky_textures[1]))
    sky.set_shader_input("texture3", load_texture(sky_textures[2]))
    sky.set_shader_input("texture4", load_texture(sky_textures[3]))
    sky.set_shader_input("blend_factors", Vec4(1.0, 0.0, 0.0, 0.0))

    # Add the sun
    global sun
    sun = Entity(model='sphere', color=color.white, scale=50)

    # Add the moon
    global moon
    moon = Entity(model='sphere', color=color.yellow, scale=40)
    # Add the clock display
    global clock_display
    clock_display = Text(
        text=f"In-Game Time: {int(current_time):02d}:{int((current_time % 1) * 60):02d}",
        position=(-0.8, 0.45),  # Adjust position as needed
        scale=1.5,
        background=True
    )
    # Initialize the player
    global player
    player = FirstPersonController()
    player.speed = WALK_SPEED  # Set the default walking speed
    player.position = terrain.position + Vec3(0, 200, 0)
    player.gravity = 0.8

    app.run()


if __name__ == "__main__":
    main()
