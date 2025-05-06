#This will be expnded later :V
class IcoColor:
       def __init__(self, r, g, b):
           self.r = min(max(r, 0), 165)
           self.g = min(max(g, 0), 165)
           self.b = min(max(b, 0), 165)
       
       @classmethod
       def from_rgb(cls, r, g, b):
           def rgb_to_ico(value):
               return min(max(165 - (256 - value), 0), 165)
           return cls(rgb_to_ico(r), rgb_to_ico(g), rgb_to_ico(b))
       
       def to_rgb(self):
           def ico_to_rgb(value):
               return 256 - (165 - value)
           return (ico_to_rgb(self.r), ico_to_rgb(self.g), ico_to_rgb(self.b))
       
       def __str__(self):
           return f"Ico({self.r}, {self.g}, {self.b})"

   # Usage example
   rgb_color = (222, 200, 190)
   ico_color = IcoColor.from_rgb(*rgb_color)
   print(f"RGB {rgb_color} converts to {ico_color}")
   print(f"Converting back to RGB: {ico_color.to_rgb()}")
