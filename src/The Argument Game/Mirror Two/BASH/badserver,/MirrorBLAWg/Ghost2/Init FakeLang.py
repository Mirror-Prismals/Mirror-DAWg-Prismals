
import pygame
import sys

pygame.init()
screen = pygame.display.set_mode((0, 0), pygame.FULLSCREEN)

mouse_region = (4, 3)
mouse_positions = []

def mouse_region_average():
    if len(mouse_positions) == mouse_region[0] * mouse_region[1]:
        avg_x = sum([pos[0] for pos in mouse_positions]) // len(mouse_positions)
        avg_y = sum([pos[1] for pos in mouse_positions]) // len(mouse_positions)
        print("Average mouse position in Mouse Region: ({}, {})".format(avg_x, avg_y))
        mouse_positions.clear()

while True:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit()
        if event.type == pygame.MOUSEMOTION:
            mouse_positions.append(event.pos)
            if len(mouse_positions) > mouse_region[0] * mouse_region[1]:
                mouse_positions.pop(0)
    
    mouse_region_average()
    
    pygame.display.flip()
