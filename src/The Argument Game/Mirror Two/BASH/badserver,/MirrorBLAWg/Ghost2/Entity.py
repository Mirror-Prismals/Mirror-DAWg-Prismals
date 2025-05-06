class Entity:
    def __init__(self, world, x=0, y=0, width=64, height=64):
        self.world = world
        self.x = x
        self.y = y
        self.width = width
        self.height = height

class Player(Entity):
    def __init__(self, world, x=0, y=0, width=64, height=64):
        super().__init__(world, x, y, width, height)

class Camera(Entity):
    def __init__(self, world, x=0, y=0, width=64, height=64):
        super().__init__(world, x, y, width, height)

# Creating a sample world instance to test the classes
class World:
    pass

world = World()
player = Player(world)
camera = Camera(world)

# Testing the classes
print("Player Position: ({}, {})".format(player.x, player.y))
print("Camera Position: ({}, {})".format(camera.x, camera.y))
