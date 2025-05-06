import tkinter as tk
import random
import math

def hex_to_rgb_and_alpha(hex_color):
    hex_color = hex_color.lstrip('#')
    if len(hex_color) != 8:
        raise ValueError("Input should be an 8-digit hex code.")
    
    r = int(hex_color[0:2], 16)
    g = int(hex_color[2:4], 16)
    b = int(hex_color[4:6], 16)
    a = int(hex_color[6:8], 16)
    return (r, g, b, a)

def create_oval_with_alpha(canvas, x1, y1, x2, y2, fill):
    r, g, b, a = hex_to_rgb_and_alpha(fill)
    color = f'#{r:02x}{g:02x}{b:02x}'
    alpha = a / 255.0
    
    # Create the oval with background color
    oval = canvas.create_oval(x1, y1, x2, y2, fill=color, outline="")
    
    # Apply transparency using built-in stipple patterns
    if alpha > 0.75:
        stipple_pattern = "gray25"
    elif alpha > 0.5:
        stipple_pattern = "gray50"
    elif alpha > 0.25:
        stipple_pattern = "gray75"
    else:
        stipple_pattern = ""
    
    canvas.itemconfig(oval, stipple=stipple_pattern)
    
    return oval

class SampleGalaxyApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Sample Galaxy")
        
        self.canvas = tk.Canvas(root, width=800, height=600, bg="black")
        self.canvas.pack()
        
        self.angle_offset = 60  # Initial angle offset for the spiral arms
        self.time = 0  # Time counter for slow modulation
        self.rotation_counter = 0  # Counter for rotation updates
        self.color_update_interval = 1  # Update colors every 10 rotations

        self.create_galaxy_view()
        self.animate_rotation()

    def create_galaxy_view(self):
        # Clear canvas
        self.canvas.delete("all")

        # Draw spiral galaxy
        self.draw_spiral_galaxy(angle_offset=self.angle_offset)

    def draw_spiral_galaxy(self, angle_offset):
        num_arms = 5  # Number of arms in the spiral galaxy
        num_stars_per_arm = 40  # Number of stars per arm
        arm_length = 270  # Length of each arm
        center_x, center_y = 400, 300  # Center of the galaxy
        flatten_factor = 0.5  # Flatten factor for the side view
        arm_thickness = 2  # Thickness of the arms

        for arm in range(num_arms):
            for i in range(num_stars_per_arm):
                angle = (i / num_stars_per_arm) * 2 * math.pi  # Angle around the center

                # Calculate the distance from the center for each star
                distance_from_center = arm_length * (1 + i / num_stars_per_arm) - 188

                # Slow modulation over time
                distance_modulation = math.sin(self.time + i * 0.01) * 1  # Slower modulation
                distance_from_center += distance_modulation

                # Calculate the position of the star with adjusted angle offset
                x_base = center_x + distance_from_center * math.cos(angle - arm * 2 * math.pi / num_arms - angle_offset / 2)
                y_base = center_y + distance_from_center * math.sin(angle - arm * 2 * math.pi / num_arms - angle_offset / 2) * flatten_factor

                # Spread stars within the thickness of the arm
                for _ in range(int(arm_thickness / 2)):  # Adjust the multiplier for more/less density
                    x = x_base + random.uniform(-arm_thickness, arm_thickness)
                    y = y_base + random.uniform(-arm_thickness, arm_thickness) * flatten_factor

                    # Draw the star with random color and transparency
                    color = "#{:02x}{:02x}{:02x}{:02x}".format(
                        random.randint(0, 21),
                        random.randint(0, 21),
                        random.randint(0, 255),
                        random.randint(0, 40)  # Random transparency
                    )
                    size = random.uniform(1, 10)  # Random size for stars
                    create_oval_with_alpha(self.canvas, x - size, y - size, x + size, y + size, fill=color)

    def animate_rotation(self):
        # Animate the rotation of the galaxy
        self.angle_offset += 0.02  # Increment the angle offset for rotation
        self.time += 0.05  # Increment time for modulation

        # Increment rotation counter
        self.rotation_counter += 1

        # Update colors and redraw galaxy every 'color_update_interval' rotations
        if self.rotation_counter >= self.color_update_interval:
            self.create_galaxy_view()  # Redraw the galaxy with updated colors
            self.rotation_counter = 0  # Reset rotation counter

        self.root.after(50, self.animate_rotation)  # Repeat the rotation animation every 50 milliseconds

    def magnetize_star_system(self, system):
        # Placeholder for magnetizing a star system to a point on one of the arms
        pass

if __name__ == "__main__":
    root = tk.Tk()
    app = SampleGalaxyApp(root)
    root.mainloop()
