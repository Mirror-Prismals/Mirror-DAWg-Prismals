import math
import datetime
import tkinter as tk
from tkinter import ttk

BIRTHSTONES = {
    1: ("#781244", "Garnet"),
    2: ("#9966CC", "Amethyst"),
    3: ("#7FFFD4", "Aquamarine"),
    4: ("#FFFFFF", "Diamond"),
    5: ("#50C878", "Emerald"),
    6: ("#F0F8FF", "Pearl"),
    7: ("#E0115F", "Ruby"),
    8: ("#E6E200", "Peridot"),
    9: ("#0F52BA", "Sapphire"),
    10: ("#A8C3BC", "Opal"),
    11: ("#FFC87C", "Topaz"),
    12: ("#40E0D0", "Turquoise")
}

def hex_to_rgb(hex_color):
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

def rgb_to_hex(rgb):
    return '#{:02x}{:02x}{:02x}'.format(*rgb)

def euclidean_distance(color1, color2):
    return math.sqrt(sum((a - b) ** 2 for a, b in zip(color1, color2)))

def wrap_color(value):
    return value % 256

def adjust_color_with_wrap(base_color, adjustment):
    rgb = hex_to_rgb(base_color)
    adjusted_rgb = tuple(wrap_color(value + adjustment) for value in rgb)
    return rgb_to_hex(adjusted_rgb)

def calculate_days_difference(date1, date2):
    d1 = datetime.datetime.strptime(date1, "%m/%d/%Y")
    d2 = datetime.datetime.strptime(date2, "%m/%d/%Y")
    return (d2 - d1).days

def reorder_channels(color, birthstone_color):
    color_rgb = hex_to_rgb(color)
    birthstone_rgb = hex_to_rgb(birthstone_color)
    order = sorted(range(3), key=lambda i: birthstone_rgb[i], reverse=True)
    reordered = tuple(color_rgb[i] for i in order)
    return rgb_to_hex(reordered)

def adjust_intensity(color, birthstone_color):
    color_rgb = hex_to_rgb(color)
    birthstone_rgb = hex_to_rgb(birthstone_color)
    adjusted_rgb = tuple(
        wrap_color(c + (b - c) // 2)
        for c, b in zip(color_rgb, birthstone_rgb)
    )
    return rgb_to_hex(adjusted_rgb)

def apply_birthstone_weight(color, birthstone_color):
    reordered = reorder_channels(color, birthstone_color)
    return adjust_intensity(reordered, birthstone_color)

def invert_hex_color(hex_color):
    hex_color = hex_color.lstrip('#')
    rgb = tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
    inverted_rgb = tuple(255 - c for c in rgb)
    return '#{:02x}{:02x}{:02x}'.format(*inverted_rgb)

def adjust_moon_color(moon_color, year):
    year_hex = f"{year:06d}"
    rgb = hex_to_rgb(moon_color)
    red = wrap_color(rgb[0] + int(year_hex[:2]))
    blue = wrap_color(rgb[2] + int(year_hex[4:]))
    total_adjustment = int(year_hex[:2]) + int(year_hex[4:])
    green = wrap_color(rgb[1] - total_adjustment)
    return f"#{red:02x}{green:02x}{blue:02x}"

def generate_astrological_colors(birthday):
    base_sun_color = "#5A3442"
    base_date = "07/17/2002"
    
    days_diff = calculate_days_difference(base_date, birthday)
    birth_month = int(birthday.split('/')[0])
    birth_year = int(birthday.split('/')[-1])
    birthstone_color, birthstone_name = BIRTHSTONES[birth_month]
    
    sun_color = adjust_color_with_wrap(base_sun_color, days_diff)
    color1 = sun_color
    color2_moon = "#808080"
    color2_rising = "#C0C0C0"
    
    distance_moon = euclidean_distance(hex_to_rgb(color1), hex_to_rgb(color2_moon))
    distance_rising = euclidean_distance(hex_to_rgb(color1), hex_to_rgb(color2_rising))
    
    moon_color = adjust_color_with_wrap(sun_color, int(distance_moon))
    rising_color = adjust_color_with_wrap(sun_color, -int(distance_rising))
    
    sun_color = apply_birthstone_weight(sun_color, birthstone_color)
    moon_color = apply_birthstone_weight(moon_color, birthstone_color)
    rising_color = apply_birthstone_weight(rising_color, birthstone_color)
    
    moon_color = adjust_moon_color(moon_color, birth_year)
    rising_color = invert_hex_color(rising_color)
    
    return sun_color, moon_color, rising_color, birthstone_name

class AstrologicalColorGUI:
    def __init__(self, master):
        self.master = master
        master.title("Astrological Color Generator")
        master.geometry("400x550")

        self.label = tk.Label(master, text="Enter your birthday:")
        self.label.pack(pady=10)

        self.day_var = tk.StringVar()
        self.month_var = tk.StringVar()
        self.year_var = tk.StringVar()

        frame = tk.Frame(master)
        frame.pack(pady=5)

        self.day_entry = ttk.Combobox(frame, textvariable=self.day_var, width=3)
        self.day_entry['values'] = tuple(range(1, 32))
        self.day_entry.pack(side=tk.LEFT, padx=5)

        self.month_entry = ttk.Combobox(frame, textvariable=self.month_var, width=3)
        self.month_entry['values'] = tuple(range(1, 13))
        self.month_entry.pack(side=tk.LEFT, padx=5)

        self.year_entry = ttk.Entry(frame, textvariable=self.year_var, width=5)
        self.year_entry.pack(side=tk.LEFT, padx=5)

        self.generate_button = tk.Button(master, text="Generate Colors", command=self.generate_colors)
        self.generate_button.pack(pady=20)

        self.birthstone_label = tk.Label(master, text="")
        self.birthstone_label.pack(pady=5)

        self.sun_frame = tk.Frame(master, width=300, height=50)
        self.sun_frame.pack(pady=5)
        self.sun_label = tk.Label(master, text="")
        self.sun_label.pack()

        self.moon_frame = tk.Frame(master, width=300, height=50)
        self.moon_frame.pack(pady=5)
        self.moon_label = tk.Label(master, text="")
        self.moon_label.pack()

        self.rising_frame = tk.Frame(master, width=300, height=50)
        self.rising_frame.pack(pady=5)
        self.rising_label = tk.Label(master, text="")
        self.rising_label.pack()

    def generate_colors(self):
        try:
            day = int(self.day_var.get())
            month = int(self.month_var.get())
            year = int(self.year_var.get())
            birthday = f"{month:02d}/{day:02d}/{year}"
            
            sun, moon, rising, birthstone = generate_astrological_colors(birthday)

            self.birthstone_label.config(text=f"Birthstone: {birthstone}")

            self.sun_frame.config(bg=sun)
            self.sun_label.config(text=f"Sun color: {sun}")

            self.moon_frame.config(bg=moon)
            self.moon_label.config(text=f"Moon color: {moon}")

            self.rising_frame.config(bg=rising)
            self.rising_label.config(text=f"Rising color: {rising}")
        except ValueError:
            self.birthstone_label.config(text="Invalid date. Please enter a valid date.")

def main():
    root = tk.Tk()
    app = AstrologicalColorGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()
