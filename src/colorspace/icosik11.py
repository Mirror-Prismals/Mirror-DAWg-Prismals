import tkinter as tk
from tkinter import messagebox
from colorthief import ColorThief
from PIL import Image, ImageDraw
import uuid

def convert_and_display():
    icosi_hex = icosikaitesimal_entry.get().strip()  # Get the input hex code
    if len(icosi_hex) != 12:  # Check for 12 characters No Hash
        messagebox.showerror("Error", "Invalid Icosikaitesimal color code length! Hashtag Not Requested")
        return
    
    try:
        # Split the icosi_hex into two individual color codes
        color1 = '#' + icosi_hex[:6]
        color2 = '#' + icosi_hex[6:]
        
        # Display the extracted colors
        color1_entry.delete(0, tk.END)
        color1_entry.insert(0, color1)
        color2_entry.delete(0, tk.END)
        color2_entry.insert(0, color2)
        
        display_image()
        get_top_colors()
        
    except ValueError:
        messagebox.showerror("Error", "Invalid Icosikaitesimal color code format!")

def create_image(hex1, hex2):
    # Create a new image with size 400x200
    img = Image.new("RGB", (400, 200), color=hex1)
    img2 = Image.new("RGB", (400, 200), color=hex2)
    
    # Save the images to temporary files with unique names
    global img_name1, img_name2
    img_name1 = str(uuid.uuid4()) + ".png"
    img_name2 = str(uuid.uuid4()) + ".png"
    img.save(img_name1)
    img2.save(img_name2)

def display_image():
    try:
        color1 = color1_entry.get().strip()
        color2 = color2_entry.get().strip()
        create_image(color1, color2)
        img1 = tk.PhotoImage(file=img_name1)
        img2 = tk.PhotoImage(file=img_name2)
        canvas.create_image(0, 0, anchor=tk.NW, image=img1)
        canvas.create_image(400, 0, anchor=tk.NW, image=img2)
        canvas.image1 = img1
        canvas.image2 = img2
    except Exception as e:
        messagebox.showerror("Error", f"Could not render image: {e}")

def get_top_colors():
    try:
        color1 = color1_entry.get().strip()
        color2 = color2_entry.get().strip()
        color_thief1 = ColorThief(img_name1)
        color_thief2 = ColorThief(img_name2)
        palette1 = color_thief1.get_palette(color_count=6, quality=9)
        palette2 = color_thief2.get_palette(color_count=6, quality=9)
        palette = palette1 + palette2
        
        # Clear previous colors
        for widget in colors_frame.winfo_children():
            widget.destroy()  

        # Filters for each color
        filters = [
            (76, 76, 165),   # Red
            (76, 0, 76),      # Orange
            (76, 0, 0),       # Yellow
            (0, 76, 0),       # Green
            (0, 0, 76),       # Blue
            (0, 76, 76)       # Purple
        ]

        for i, color in enumerate(palette):
            # Limit to 12 colors
            if i >= 12:
                break
            
            hex_color = '#{0:02x}{1:02x}{2:02x}'.format(*[min(max(c, 0), 255) for c in color[:3]])  # Truncate blue channel to two digits
            opposite_color = tuple(255 - c for c in color)
            
            # Apply filter based on index
            filter_values = filters[i % len(filters)]
            red, green, blue = opposite_color
            red += filter_values[0]
            green += filter_values[1]
            blue -= filter_values[2]
            red = min(max(red, 0), 255)
            green = min(max(green, 0), 255)
            blue = min(max(blue, 0), 255)
            hex_filtered = '#{0:02x}{1:02x}{2:02x}'.format(red, green, blue)
            
            # Create a frame to hold each color square and label
            frame = tk.Frame(colors_frame)
            frame.pack(pady=5)
            
            # Create a colored square
            color_label = tk.Label(frame, text="", bg=hex_color, width=10, height=2)
            color_label.grid(row=0, column=0, padx=5)
            
            # Create a label for the color hex code
            hex_label = tk.Label(frame, text=hex_color)
            hex_label.grid(row=0, column=1, padx=5)
            
            # Create a colored square for the filtered color
            filtered_label = tk.Label(frame, text="", bg=hex_filtered, width=10, height=2)
            filtered_label.grid(row=0, column=2, padx=5)
            
            # Create a label for the filtered color hex code
            hex_filtered_label = tk.Label(frame, text=hex_filtered)
            hex_filtered_label.grid(row=0, column=3, padx=5)
            
    except Exception as e:
        messagebox.showerror("Error", f"Could not process image: {e}")

# Create the GUI window
window = tk.Tk()
window.title("Icosikaitesimal Color Converter and Image Color Extractor")

# Icosikaitesimal converter
icosikaitesimal_label = tk.Label(window, text="Enter Icosikaitesimal Color Code (12 hex digits):")
icosikaitesimal_label.pack()

icosikaitesimal_entry = tk.Entry(window)
icosikaitesimal_entry.pack()

convert_button = tk.Button(window, text="Convert", command=convert_and_display)
convert_button.pack()

# Image color extractor
image_label = tk.Label(window, text="Extracted Colors:")
image_label.pack()

color1_entry = tk.Entry(window)
color1_entry.pack()

color2_entry = tk.Entry(window)
color2_entry.pack()

render_button = tk.Button(window, text="Render Image", command=display_image)
render_button.pack()

get_colors_button = tk.Button(window, text="Get Top Colors", command=get_top_colors)
get_colors_button.pack()

colors_frame = tk.Frame(window)
colors_frame.pack()

# Canvas to display the image
canvas = tk.Canvas(window, width=800, height=200)
canvas.pack()

# Run the GUI
window.mainloop()
