from PIL import Image, ImageDraw
import math

def convert_to_rgb(hex_code):
    # Convert hexadecimal color code to RGB tuple
    hex_code = hex_code.lstrip("#")
    return tuple(int(hex_code[i:i+2], 16) for i in (0, 2, 4))

def create_color_wheel(values, format, size=300):
    # Define color codes dictionary
    color_codes = {
        'p': '#680164',  # Purple
        'b': '#0000FF',  # Blue
        'g': '#216216',  # Green
        'r': '#FF0000',  # Red
        'y': '#FFFF00',  # Yellow
        'c': '#00FFFF',  # Cyan
        'o': '#F5A000',  # Orange
        'l': '#8BB58F',  # Lime
        'e': '#808080',   # Extra (Grey)
        'd': '#808080',   # Grey
        'f': '#808080',   # Grey
        'h': '#808080',   # Grey
        'i': '#808080',   # Grey
        'j': '#808080',   # Grey
        'k': '#808080',   # Grey
        'm': '#808080',   # Grey
        'n': '#808080',   # Grey
        'q': '#808080',   # Grey
        's': '#808080',   # Grey
        't': '#808080',   # Grey
        'u': '#808080',   # Grey
        'v': '#808080',   # Grey
        'w': '#808080',   # Grey
        'x': '#808080',   # Grey
        'z': '#808080',   # Grey
        'a': '#808080',   # Grey
        'P': '#680164',  # Purple
        'B': '#0000FF',  # Blue
        'G': '#216216',  # Green
        'R': '#FF0000',  # Red
        'Y': '#FFFF00',  # Yellow
        'C': '#00FFFF',  # Cyan
        'O': '#F5A000',  # Orange
        'L': '#8BB58F',  # Lime
        'E': '#808080',  # Extra (Grey)
        'D': '#808080',  # Grey
        'F': '#808080',  # Grey
        'H': '#808080',  # Grey
        'I': '#808080',  # Grey
        'J': '#808080',  # Grey
        'K': '#808080',  # Grey
        'M': '#808080',  # Grey
        'N': '#808080',  # Grey
        'Q': '#808080',  # Grey
        'S': '#808080',  # Grey
        'T': '#808080',  # Grey
        'U': '#808080',  # Grey
        'V': '#808080',  # Grey
        'W': '#808080',  # Grey
        'X': '#808080',  # Grey
        'Z': '#808080',  # Grey
        'A': '#808080',  # Grey
    }
    
    # Calculate the angle step between colors
    angle_step = 360 / len(format)
    
    # Create a blank image with white background
    image = Image.new("RGB", (size, size), "black")
    draw = ImageDraw.Draw(image)
    
    # Calculate the radius of the color wheel
    radius = size // 2
    
    # Calculate the center of the color wheel
    center = (size // 2, size // 2)
    
    # Iterate over the characters in the format
    for i, channel in enumerate(format):
        # Calculate the angle for this color
        angle = math.radians(i * angle_step)
        
        # Calculate the position of the color on the wheel
        x = int(center[0] + radius * math.cos(angle))
        y = int(center[1] + radius * math.sin(angle))
        
        # Get the hex code for the channel
        hex_code = color_codes[channel]
        
        # Convert hex code to RGB
        rgb = convert_to_rgb(hex_code)
        
        # Get the intensity modifier for this channel and round it to the nearest integer
        intensity_modifier = round(values[i] * 255)
        
        # Multiply each RGB component by the intensity modifier
        rgb = tuple(int(rgb_component * intensity_modifier / 255) for rgb_component in rgb)
        
        # Draw a circle with the color at the calculated position
        draw.ellipse((x - 20, y - 20, x + 20, y + 20), fill=rgb)
    
    # Save the image
    image.save("color_wheel.png")

# Test the function
input_values = [.61, .61, .61, .61, .61, .61, .61]
format_string = "rgbcmyk"
create_color_wheel(input_values, format_string)
