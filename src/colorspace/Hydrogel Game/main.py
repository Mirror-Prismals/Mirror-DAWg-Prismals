import pygame
import sys
import numpy as np
from OpenGL.GL import *
from OpenGL.GLU import *
from camera import Camera
from world import World

# Initialize Pygame
pygame.init()

# Constants
FPS = 48
MOUSE_SENSITIVITY = 0.1

# Set up display
screen = pygame.display.set_mode((0, 0), pygame.OPENGL | pygame.DOUBLEBUF | pygame.FULLSCREEN)
pygame.display.set_caption("4D Hydrogel Game")
clock = pygame.time.Clock()

# Initialize camera and world
camera = Camera(position=(0, 0, 5))
world = World()

# OpenGL settings
glEnable(GL_DEPTH_TEST)
glMatrixMode(GL_PROJECTION)
gluPerspective(45, (screen.get_width() / screen.get_height()), 0.1, 50.0)
glMatrixMode(GL_MODELVIEW)

# Hide mouse cursor and capture it
pygame.mouse.set_visible(False)
pygame.event.set_grab(True)

# Game loop
running = True
while running:
    delta_time = clock.tick(FPS) / 1000.0
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.MOUSEMOTION:
            camera.handle_mouse(event.rel[0], event.rel[1])
    
    # Handle input
    camera.handle_input(delta_time)

    # Update world
    world.update()

    # Render
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glLoadIdentity()
    gluLookAt(
        camera.position[0], camera.position[1], camera.position[2],
        camera.position[0] + camera.get_front()[0], camera.position[1] + camera.get_front()[1], camera.position[2] + camera.get_front()[2],
        0, 1, 0
    )

    world.draw()

    # Display update
    pygame.display.flip()

pygame.quit()
sys.exit()
