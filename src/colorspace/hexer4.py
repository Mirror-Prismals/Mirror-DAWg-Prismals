import tkinter as tk

def get_hex_colors(hex_value):
    # Extract RGB values from hex
    red = int(hex_value[1:3], 16)
    green = int(hex_value[3:5], 16)
    blue = int(hex_value[5:], 16)

    # Calculate "accent" colors
    accent_red = f'{hex_value[2]}0800' if red != 0 else '000000'
    accent_green = f'00{hex_value[4]}080' if green != 0 else '000000'
    accent_blue = f'0000{hex_value[6]}08' if blue != 0 else '000000'

    # Truncate accent blue to ensure it's a valid hex color
    accent_blue = accent_blue[:6]

    # Generate hex colors
    red_hex = f'#{red:02X}0000'
    green_hex = f'#00{green:02X}00'
    blue_hex = f'#0000{blue:02X}'
    accent_red_hex = f'#{accent_red}0'
    accent_green_hex = f'#{accent_green}'
    accent_blue_hex = f'#{accent_blue}'

    return (hex_value, red_hex, green_hex, blue_hex, accent_red_hex, accent_green_hex, accent_blue_hex)

def display_hex_colors(hex_value):
    colors = get_hex_colors(hex_value)
    labels = ["Hex", "Red", "Green", "Blue", "Red Accent", "Green Accent", "Blue Accent"]

    # Clear any previous color displays
    for widget in root.winfo_children():
        if not isinstance(widget, (tk.Entry, tk.Button)):
            widget.destroy()

    # Display the main hex value
    color_frame = tk.Frame(root, width=100, height=100, bg=colors[0])
    color_frame.grid(row=1, column=1, padx=10, pady=10)
    color_label = tk.Label(root, text=labels[0], font=("Helvetica", 10))
    color_label.grid(row=2, column=1, padx=10, pady=5)
    hex_label = tk.Label(root, text=colors[0], font=("Helvetica", 10))
    hex_label.grid(row=3, column=1, padx=10, pady=5)

    # Display the primary colors (Red, Green, Blue)
    for i in range(1, 4):
        color_frame = tk.Frame(root, width=100, height=100, bg=colors[i])
        color_frame.grid(row=4, column=i-1, padx=10, pady=10)
        color_label = tk.Label(root, text=labels[i], font=("Helvetica", 10))
        color_label.grid(row=5, column=i-1, padx=10, pady=5)
        hex_label = tk.Label(root, text=colors[i], font=("Helvetica", 10))
        hex_label.grid(row=6, column=i-1, padx=10, pady=5)

    # Display the accent colors (Red Accent, Green Accent, Blue Accent)
    for i in range(4, 7):
        color_frame = tk.Frame(root, width=100, height=100, bg=colors[i])
        color_frame.grid(row=7, column=i-4, padx=10, pady=10)
        color_label = tk.Label(root, text=labels[i], font=("Helvetica", 10))
        color_label.grid(row=8, column=i-4, padx=10, pady=5)
        hex_label = tk.Label(root, text=colors[i], font=("Helvetica", 10))
        hex_label.grid(row=9, column=i-4, padx=10, pady=5)

# Create GUI
root = tk.Tk()
root.title("Hex Color Display")
root.geometry("800x800")

# Entry for hex value
hex_entry = tk.Entry(root, font=("Helvetica", 16))
hex_entry.grid(row=0, column=0, padx=10, pady=10)

# Button to display colors
display_button = tk.Button(root, text="Display Colors", font=("Helvetica", 16), command=lambda: display_hex_colors(hex_entry.get()))
display_button.grid(row=0, column=2, padx=10, pady=10)

root.mainloop()
