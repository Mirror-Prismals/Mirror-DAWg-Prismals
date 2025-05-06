from PIL import Image, ImageDraw
import math
import os

def create_god_rays(output_path, width, height, center, rays_color, num_rays, intensity):
    """Creates an image with god rays emanating from a central point."""
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))  # Transparent background
    draw = ImageDraw.Draw(img)

    cx, cy = center
    max_radius = math.sqrt(width**2 + height**2) / 2

    for i in range(num_rays):
        angle = (2 * math.pi / num_rays) * i
        ray_length = max_radius * (0.8 + 0.2 * (i % 2))  # Alternate ray lengths for variation
        end_x = cx + ray_length * math.cos(angle)
        end_y = cy + ray_length * math.sin(angle)

        overlay = Image.new('RGBA', img.size, (0, 0, 0, 0))
        overlay_draw = ImageDraw.Draw(overlay)

        # Draw a ray with fading transparency
        for thickness in range(1, 10):
            ray_alpha = max(0, int(255 * intensity * (1 - thickness / 10)))
            ray_color = (*rays_color[:3], ray_alpha)
            overlay_draw.line([(cx, cy), (end_x, end_y)], fill=ray_color, width=thickness)

        img = Image.alpha_composite(img, overlay)

    img.save(output_path)
    print(f"Saved god rays image at {output_path}")

def create_sun_and_moon_god_rays(output_dir, width=4096, height=4096):
    """Creates god rays for the sun and moon."""
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Sun god rays
    sun_center = (width // 2, height // 3)
    sun_rays_color = (255, 100, 0)  # Bright yellow
    sun_output_path = os.path.join(output_dir, "god_rays.png")
    create_god_rays(sun_output_path, width, height, sun_center, sun_rays_color, 100, 0.7)

    # Moon god rays (moon glow)
    moon_center = (width // 2, 2 * height // 3)
    moon_rays_color = (173, 216, 230)  # Soft pale blue
    moon_output_path = os.path.join(output_dir, "moon_glow.png")
    create_god_rays(moon_output_path, width, height, moon_center, moon_rays_color, 60, 0.5)

if __name__ == "__main__":
    output_directory = "god_rays_images"
    create_sun_and_moon_god_rays(output_directory)
    print("God rays images generated successfully.")
