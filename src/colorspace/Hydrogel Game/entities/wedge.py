from OpenGL.GL import *

class Wedge:
    def __init__(self, x, y, z):
        self.position = (x, y, z)

    def draw(self):
        glPushMatrix()
        glTranslatef(*self.position)
        glColor3f(0, 1, 0)
        glBegin(GL_TRIANGLES)
        # Simplified icosahedron vertices
        glVertex3f(0, 1, 0)
        glVertex3f(-1, -1, 1)
        glVertex3f(1, -1, 1)
        glVertex3f(0, 1, 0)
        glVertex3f(1, -1, 1)
        glVertex3f(1, -1, -1)
        glVertex3f(0, 1, 0)
        glVertex3f(1, -1, -1)
        glVertex3f(-1, -1, -1)
        glVertex3f(0, 1, 0)
        glVertex3f(-1, -1, -1)
        glVertex3f(-1, -1, 1)
        glEnd()
        glPopMatrix()

    def update(self):
        pass
