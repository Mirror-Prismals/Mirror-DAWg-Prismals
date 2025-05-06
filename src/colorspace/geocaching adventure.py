import random
import tkinter as tk

# Define rarity categories and possible treasures
rarities = {
    "Common": ["a rusty nail", "a broken twig", "an old coin", "a piece of string", "a faded map"],
    "Uncommon": ["a shiny pebble", "a small key", "a mysterious note", "a gemstone", "an ancient artifact"],
    "Rare": ["a golden ring", "a silver dagger", "a magical scroll", "a rare flower", "an old book"],
    "Epic": ["a legendary sword", "a mystical amulet", "a dragon egg", "a wizard's staff", "a treasure chest"],
    "Legendary": ["the Holy Grail", "Excalibur", "a Phoenix feather", "a Dragon's heart", "an Orb of Power"]
}

# Define the rarities and their probabilities
rarity_levels = ["Common", "Uncommon", "Rare", "Epic", "Legendary"]
rarity_probabilities = [0.6, 0.25, 0.1, 0.04, 0.01]  # Probabilities should sum to 1

def determine_rarity():
    return random.choices(rarity_levels, rarity_probabilities)[0]

def get_treasure():
    rarity = determine_rarity()
    treasure = random.choice(rarities[rarity])
    return rarity, treasure

def on_find_treasure():
    x = x_entry.get()
    y = y_entry.get()
    z = z_entry.get()
    ascii_symbol = ascii_entry.get()

    if not x.isdigit() or not y.isdigit() or not z.isdigit():
        result_text.insert(tk.END, "Please enter valid numerical coordinates.\n")
        result_text.see(tk.END)
        return

    rarity, treasure = get_treasure()
    result_message = f"Coordinates ({x}, {y}, {z}) with marker '{ascii_symbol}'\nYou found a {rarity} treasure: {treasure}!\n\n"
    result_text.insert(tk.END, result_message)
    result_text.see(tk.END)

# Set up the tkinter window
root = tk.Tk()
root.title("Geocaching Adventure")
root.geometry("600x400")

# Coordinate input labels and entries
x_label = tk.Label(root, text="Enter X coordinate:")
x_label.pack()
x_entry = tk.Entry(root)
x_entry.pack()

y_label = tk.Label(root, text="Enter Y coordinate:")
y_label.pack()
y_entry = tk.Entry(root)
y_entry.pack()

z_label = tk.Label(root, text="Enter Z coordinate:")
z_label.pack()
z_entry = tk.Entry(root)
z_entry.pack()

# ASCII symbol input
ascii_label = tk.Label(root, text="Enter ASCII symbol:")
ascii_label.pack()
ascii_entry = tk.Entry(root)
ascii_entry.pack()

# Result text widget
result_text = tk.Text(root, wrap=tk.WORD, height=15, width=70)
result_text.pack(pady=20)

# Find treasure button
find_button = tk.Button(root, text="Find Treasure", command=on_find_treasure)
find_button.pack()

# Quit button
quit_button = tk.Button(root, text="Quit", command=root.quit)
quit_button.pack(pady=10)

# Start the tkinter event loop
root.mainloop()
