import math

def hex_to_rgb(hex_color):
    """Convert hex color to RGB tuple."""
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

def euclidean_distance(color1, color2):
    """Calculate the Euclidean distance between two RGB colors with detailed steps."""
    differences = [(a - b) for a, b in zip(color1, color2)]
    squares = [(a - b) ** 2 for a, b in zip(color1, color2)]
    distance = math.sqrt(sum(squares))
    
    print("Calculating Euclidean Distance:")
    print(f"RGB1: {color1}, RGB2: {color2}")
    print(f"Differences: {differences}")
    print(f"Squares of Differences: {squares}")
    print(f"Sum of Squares: {sum(squares)}")
    print(f"Distance: sqrt({sum(squares)}) = {distance:.4f}\n")
    
    return distance

def find_similar_color(base_color, distance, lighter=True):
    """Find a color that is at a similar distance from the base color."""
    base_rgb = hex_to_rgb(base_color)
    adjustment = int(distance / math.sqrt(3))
    if not lighter:
        adjustment = -adjustment
    new_rgb = tuple(min(255, max(0, base_rgb[i] + adjustment)) for i in range(3))
    
    print(f"Finding a similar color to {base_color} (RGB: {base_rgb}):")
    print(f"Distance per channel (approx): {adjustment}")
    print(f"New RGB: {new_rgb}")
    
    return '#{:02x}{:02x}{:02x}'.format(*new_rgb)

# Input colors
color1 = "#161616"
color2 = "#1c1c1c"
base_color = "#194127"
lighter = False  # Set this to True for a lighter color, False for a darker color

# Convert hex to RGB
rgb1 = hex_to_rgb(color1)
rgb2 = hex_to_rgb(color2)

# Calculate Euclidean distance with detailed steps
distance = euclidean_distance(rgb1, rgb2)

# Find a similar color from the base color
similar_color = find_similar_color(base_color, distance, lighter)

# Output results
print(f"Color 1 (RGB): {rgb1}")
print(f"Color 2 (RGB): {rgb2}")
print(f"Distance between Color 1 and Color 2: {distance:.4f}")
print(f"Base color (RGB): {hex_to_rgb(base_color)}")
print(f"Similar color: {similar_color}")

# Calculate the Euclidean distance between base color and the similar color
similar_rgb = hex_to_rgb(similar_color)
similar_distance = euclidean_distance(hex_to_rgb(base_color), similar_rgb)
print(f"Distance between base color and similar color: {similar_distance:.4f}")
