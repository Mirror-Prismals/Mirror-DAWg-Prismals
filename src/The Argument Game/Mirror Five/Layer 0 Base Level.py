import tkinter as tk

def hex_to_rgb(hex_color):
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

# Convert hex color to RGB
hex_color = "#BBB1D0"
rgb_color = hex_to_rgb(hex_color)

# Create the Tkinter window
root = tk.Tk()
root.attributes('-fullscreen', True)
root.configure(bg=hex_color)

# Create a label with the specified text
text = f'''"BurnMe.fakelang'"'''
label = tk.Label(root, text=text, font=('Helvetica', 24), bg=hex_color)
label.pack(expand=True)

# Exit fullscreen and close the window on pressing 'Esc'
def close(event):
    root.attributes('-fullscreen', False)
    root.destroy()

root.bind('<Escape>', close)

root.mainloop()
