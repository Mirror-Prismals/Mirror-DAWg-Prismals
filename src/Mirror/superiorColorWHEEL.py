import matplotlib.pyplot as plt
import numpy as np

# Define the color sequence (using your transitions)
colors = [
    "#ff00ff",  # Magenta (start)
    "#8000ff",  # Violet (ff00ff minus 0x80 red → 8000ff)
    "#8080ff",  # Indigo (8000ff + 0x80 green → 8080ff)
    "#0080ff",  # Cerulean/Blue (8080ff minus 0x80 red → 0080ff)
    "#008080",  # Teal (0080ff minus 0x80 blue → 008080)
    "#008000",  # Green (008080 minus 0x80 blue → 008000)
    "#808000",  # Olive (008000 + 0x80 red → 808000)
    "#ff8000",  # Orange (808000 + 0x80 red → ff8000)
    "#ff0000",  # Red (ff8000 minus 0x80 green → ff0000)
    "#ff0080",  # Pink (ff0000 + 0x80 blue → ff0080)
    "#ff00ff"   # Magenta (ff0080 + 0x80 blue → ff00ff, full circle)
]

# We can drop the duplicate final magenta for visualization purposes.
unique_colors = colors[:-1]
N = len(unique_colors)

# Create a polar plot for the color wheel.
fig, ax = plt.subplots(subplot_kw={'projection': 'polar'}, figsize=(8, 8))

# Generate equally spaced angles around the circle.
theta = np.linspace(0.0, 2 * np.pi, N + 1)
width = theta[1] - theta[0]

# Create wedges for each color slice.
bars = ax.bar(theta[:-1], np.ones(N), width=width, bottom=0.0,
              color=unique_colors, edgecolor='k', linewidth=1)

# Clean up the plot for a minimal look.
ax.set_axis_off()
plt.title("Superior Color Order", fontsize=16)
plt.show()
