import numpy as np
from OpenGL.GL import *
from entities.blawg import Blawg
from entities.wedge import Wedge
from entities.womp import Womp

class World:
    def __init__(self):
        self.entities = [
            Blawg(0, 0, 0),
            Wedge(1, 1, 1),
            Womp(-1, -1, -1)
        ]

    def draw(self):
        for entity in self.entities:
            entity.draw()

    def update(self):
        for entity in self.entities:
            entity.update()
