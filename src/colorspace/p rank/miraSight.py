from PIL import Image, ImageOps, ImageEnhance
import os
import tkinter as tk
from tkinter import filedialog
import uuid  # Import the uuid module

# Define the custom 64-color palette
custom_palette = [

    # Outside
    (75, 103, 18), (112, 142, 47), (132, 161, 209),
            # Skin
    (45, 34, 30), (165,126,110), (255,206,180), (105, 80, 70),
    # Nill/Null/Na
    (255,255,255), (128,128,128), (0,0,0),
    # Red
    (255, 0, 0), (128, 0, 0), (32, 0, 0),
    # Lime
    (0, 255, 0), (0, 133, 0), (0, 32, 0),
    # Blue
    (0, 0, 255), (0, 0, 128), (0, 0, 32),
    # Cyan
    (0, 255, 255), 
    (0, 128, 128), (0, 32, 32),
    # Magenta
    (255, 0, 255), (128, 0, 128), (32, 0, 32),
    # Yellow
    (255, 255, 0), (128, 128, 0), (32, 32, 0),
    # Orange
    (255, 127, 0), (128, 64, 0), (32, 16, 0),
    # Pink
    (255, 0, 128), (128, 0, 64), (32, 0, 16),
    # Thalo
    (0, 255, 128), (0, 128, 64), (0, 32, 16),
    # Violet
    (128, 0, 255), (64, 0, 128), (16, 0, 32),
    # Chartreuse
    (128, 255, 0), (64, 128, 0), (16, 32, 0),
    # Cerulean
    (0, 128, 255), (0, 64, 128), (0, 16, 32),
    # Infrared
    (255, 128, 128), (128, 64, 64), (32, 16, 16),
    # Camo
    (128, 255, 128), (64, 128, 64), (16, 32, 16),
    # Indigo
    (128, 128, 255), (64, 64, 128), (16, 16, 32),
    # Aqua
    (128, 255, 255), 
    (64, 128, 128), (16, 32, 32),
    # Mustard
    (255, 255, 128), (128, 128, 64), (32, 32, 16),
    # Ultraviolet
    (255, 128, 255), (128, 64, 128), (32, 16, 32)
]

def convert_image_to_custom_palette(image, palette):
    # Convert the image to 'P' mode (indexed color) with the custom palette
    image = image.convert('P')
    image.putpalette([value for color in palette for value in color])  # Flatten the palette
    return image

def process_image(input_image_path, output_image_path, palette, size_threshold_kb=60, resize_dimensions=(336, 192)):
    # Open the image
    image = Image.open(input_image_path)
    
    # Halve the brightness
    enhancer = ImageEnhance.Brightness(image)
    image = enhancer.enhance(0.44)
    
    # Resize the image proportionally
    image.thumbnail(resize_dimensions, Image.LANCZOS)
    
    # Create a white background
    background = Image.new('RGB', resize_dimensions, (255, 255, 255))
    
    # Calculate position to center the image on the white background
    image_width, image_height = image.size
    background_width, background_height = resize_dimensions
    paste_position = ((background_width - image_width) // 2, (background_height - image_height) // 2)
    
    # Paste the image onto the white background
    background.paste(image, paste_position)
    
    # Convert the image to the custom palette
    background = convert_image_to_custom_palette(background, palette)
    
    # Invert the colors
    background = ImageOps.invert(background.convert('RGB')).convert('P')
   
    
    # Save the image
    background.save(output_image_path)
    
    # Check actual file size
    actual_size_kb = check_actual_file_size(output_image_path)
    print(f"Actual file size: {actual_size_kb:.2f} KB")
    
    # If the file size is larger than the threshold, resize the image
    if actual_size_kb > size_threshold_kb:
        print("File size exceeds threshold. Resizing the image...")
        # Open the image again
        image = Image.open(output_image_path)
        image = image.resize(resize_dimensions)
        image.save(output_image_path)
        actual_size_kb = check_actual_file_size(output_image_path)
        print(f"Resized image file size: {actual_size_kb:.2f} KB")

def check_actual_file_size(filename):
    file_size = os.path.getsize(filename)
    file_size_kb = file_size / 1024
    return file_size_kb

def generate_random_hex_filename(length=12):
    # Generate a random hexadecimal string
    return uuid.uuid4().hex[:length]

def main():
    # Create Tkinter root window (hidden)
    root = tk.Tk()
    root.withdraw()  # Hide the root window

    # File picker dialog
    input_image_path = filedialog.askopenfilename(
        title="Select an Image",
        filetypes=[("Image Files", "*.png;*.jpg;*.jpeg;*.bmp;*.gif")]
    )

    # If a file was selected, proceed
    if input_image_path:
        # Generate a random 12-digit hexadecimal filename
        random_hex_filename = generate_random_hex_filename()
        output_image_path = f'{random_hex_filename}.png'
        
        # Process the image with the custom 57-color palette, resizing, and potential resizing
        process_image(input_image_path, output_image_path, custom_palette)
    else:
        print("No file selected.")

if __name__ == "__main__":
    main()
