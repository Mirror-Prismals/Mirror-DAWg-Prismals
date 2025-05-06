import numpy as np
import matplotlib.pyplot as plt

# Convert RGB color to Lab color space
def rgb_to_lab(rgb):
    # Convert RGB to XYZ
    def rgb_to_xyz(rgb):
        rgb = rgb / 255.0
        mask = rgb <= 0.04045
        rgb[mask] = rgb[mask] / 12.92
        rgb[~mask] = ((rgb[~mask] + 0.055) / 1.055) ** 2.4
        xyz = np.dot(rgb, np.array([[0.4124564, 0.3575761, 0.1804375],
                                     [0.2126729, 0.7151522, 0.0721750],
                                     [0.0193339, 0.1191920, 0.9503041]]).T)
        return xyz

    # Convert XYZ to Lab
    def xyz_to_lab(xyz):
        xyz_ref = np.array([0.95047, 1.0, 1.08883])
        xyz_normalized = xyz / xyz_ref
        mask = xyz_normalized > 0.008856
        xyz_normalized[mask] = xyz_normalized[mask] ** (1/3)
        xyz_normalized[~mask] = (7.787 * xyz_normalized[~mask]) + (16 / 116)
        lab = np.zeros_like(xyz)
        lab[..., 0] = (116 * xyz_normalized[..., 1]) - 16
        lab[..., 1] = 500 * (xyz_normalized[..., 0] - xyz_normalized[..., 1])
        lab[..., 2] = 200 * (xyz_normalized[..., 1] - xyz_normalized[..., 2])
        return lab

    xyz = rgb_to_xyz(rgb)
    lab = xyz_to_lab(xyz)
    return lab

# Convert Lab color to RGB color space
def lab_to_rgb(lab):
    # Convert Lab to XYZ
    def lab_to_xyz(lab):
        xyz_ref = np.array([0.95047, 1.0, 1.08883])
        xyz = np.zeros_like(lab)
        xyz[..., 1] = (lab[..., 0] + 16) / 116
        xyz[..., 0] = lab[..., 1] / 500 + xyz[..., 1]
        xyz[..., 2] = xyz[..., 1] - lab[..., 2] / 200
        xyz = xyz ** 3 * xyz_ref
        return xyz

    # Convert XYZ to RGB
    def xyz_to_rgb(xyz):
        rgb = np.dot(xyz, np.array([[ 3.2404542, -1.5371385, -0.4985314],
                                     [-0.9692660,  1.8760108,  0.0415560],
                                     [ 0.0556434, -0.2040259,  1.0572252]]).T)
        mask = rgb > 0.0031308
        rgb[mask] = 1.055 * (rgb[mask] ** (1 / 2.4)) - 0.055
        rgb[~mask] = 12.92 * rgb[~mask]
        rgb = np.clip(rgb, 0, 1)
        return rgb * 255

    xyz = lab_to_xyz(lab)
    rgb = xyz_to_rgb(xyz)
    return rgb.astype(np.uint8)

# Function to generate analogous colors
def generate_analogous_colors(lab_color):
    # Keep L* constant
    L = lab_color[0]
    # Adjust a* and b* to create analogous colors
    analogous_colors = []
    for da, db in [(-20, 20), (20, -20)]:
        new_lab_color = lab_color.copy()
        new_lab_color[1] += da
        new_lab_color[2] += db
        analogous_colors.append(new_lab_color)
    return analogous_colors

# Function to generate complementary color
def generate_complementary_color(lab_color):
    # Invert a* and b* channels to find complementary color
    complementary_color = lab_color.copy()
    complementary_color[1] = -complementary_color[1]
    complementary_color[2] = -complementary_color[2]
    return complementary_color

# Function to invert RGB color
def invert_rgb_color(rgb):
    return 255 - rgb

# Function to display colors
def display_colors(colors):
    num_colors = len(colors)
    fig, axes = plt.subplots(1, num_colors, figsize=(num_colors * 3, 3))
    for i, color in enumerate(colors):
        rgb_color = lab_to_rgb(color)
        axes[i].imshow([[rgb_color]])
        axes[i].axis('off')
    plt.show()

# Main function
def main():
    hex_color = input("Enter a hexadecimal color code (e.g., #RRGGBB): ")
    # Convert hex to RGB
    rgb_color = np.array([int(hex_color[i:i+2], 16) for i in (1, 3, 5)])
    lab_color = rgb_to_lab(rgb_color)
    
    # Generate analogous and complementary colors
    analogous_colors = generate_analogous_colors(lab_color)
    complementary_color = generate_complementary_color(lab_color)
    
    # Invert RGB color and convert to Lab
    inverted_rgb = invert_rgb_color(rgb_color)
    inverted_lab = rgb_to_lab(inverted_rgb)
    
    # Display colors
    display_colors([lab_color, complementary_color, inverted_lab] + analogous_colors)

if __name__ == "__main__":
    main()
