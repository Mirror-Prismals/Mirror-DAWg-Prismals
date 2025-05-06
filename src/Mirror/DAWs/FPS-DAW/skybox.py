from PIL import Image, ImageDraw
import os

def create_gradient(width, height, top_color, bottom_color):
    """Creates a vertical gradient image."""
    base = Image.new('RGB', (width, height), top_color)
    top_r, top_g, top_b = top_color
    bottom_r, bottom_g, bottom_b = bottom_color

    for y in range(height):
        blend = y / height
        r = int((1 - blend) * top_r + blend * bottom_r)
        g = int((1 - blend) * top_g + blend * bottom_g)
        b = int((1 - blend) * top_b + blend * bottom_b)
        ImageDraw.Draw(base).line([(0, y), (width, y)], fill=(r, g, b))

    return base

def create_skybox_images(output_dir, width=1024, height=1024):
    """Creates skybox images for morning, afternoon, evening, and night."""
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    sky_colors = {
        "morning": ((80, 80, 255), (0, 0, 255)),  # Soft orange to white
        "afternoon": ((135, 206, 235), (255, 255, 255)),  # Sky blue to white
        "evening": ((255, 100, 50), (0, 128, 150)),  # Deep orange to soft orange
        "night": ((10, 10, 50), (0, 0, 0)),  # Deep blue to black
    }

    for time_of_day, (top_color, bottom_color) in sky_colors.items():
        gradient = create_gradient(width, height, top_color, bottom_color)
        file_path = os.path.join(output_dir, f"{time_of_day}_sky.jpg")
        gradient.save(file_path)
        print(f"Saved {file_path}")

if __name__ == "__main__":
    output_directory = "skybox_images"
    create_skybox_images(output_directory)
    print("Skybox images generated successfully.")
