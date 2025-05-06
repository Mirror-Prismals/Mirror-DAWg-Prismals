from OpenGL.GL import *
import math

class Womp:
    def __init__(self, x, y, z):
        self.position = (x, y, z)

    def draw(self):
        glPushMatrix()
        glTranslatef(*self.position)
        glColor3f(0, 0, 1)
        glBegin(GL_TRIANGLE_FAN)
        glVertex3f(0, 0, 0)  # Center of circle
        num_segments = 36
        for i in range(num_segments + 1):
            angle = i * 2 * math.pi / num_segments
            glVertex3f(math.cos(angle) * 0.5, math.sin(angle) * 0.5, 0)
        glEnd()
        glPopMatrix()

    def update(self):
        pass
