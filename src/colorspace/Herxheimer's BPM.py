import math
import colorsys

def hex_to_rgb(hex_color):
    """Convert hex color to RGB tuple."""
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

def rgb_to_hex(rgb):
    """Convert RGB tuple to hex color."""
    return '#{:02x}{:02x}{:02x}'.format(*rgb)

def euclidean_distance(color1, color2):
    """Calculate the Euclidean distance between two RGB colors."""
    return math.sqrt(sum((a - b) ** 2 for a, b in zip(color1, color2)))

def find_similar_color(base_color, distance, lighter=True):
    """Find a color that is at a similar distance from the base color."""
    base_rgb = hex_to_rgb(base_color)
    adjustment = int(distance / math.sqrt(3))
    if not lighter:
        adjustment = -adjustment
    new_rgb = tuple(min(255, max(0, base_rgb[i] + adjustment)) for i in range(3))
    return rgb_to_hex(new_rgb)

def calculate_tr(herx):
    """Calculate TR based on the octave of Herx."""
    octave = math.floor(math.log2(herx / 440) + 4)
    return octave * 12 * 3  # Octave * ChromaticScale * HexChannels

def calculate_herxheimer_bpm(herx, color1, color2, base_color):
    """Calculate BPM based on the conceptual Herxheimer model."""
    rgb1, rgb2, base_rgb = map(hex_to_rgb, (color1, color2, base_color))
    
    pdo = euclidean_distance(rgb1, rgb2)
    tr = calculate_tr(herx)
    ir = euclidean_distance(base_rgb, hex_to_rgb(find_similar_color(base_color, pdo, lighter=False)))
    
    k = herx / (pdo + tr + ir)
    bpm = k * 100
    
    return round(bpm)

# Example usage
herx_severity = 440  # Hz
color1 = "#F2000F"
color2 = "#C0C0C0"
base_color = "#5A3442"

bpm = calculate_herxheimer_bpm(herx_severity, color1, color2, base_color)
print(f"Calculated BPM: {bpm}")
