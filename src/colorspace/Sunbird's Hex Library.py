import tkinter as tk
from tkinter import simpledialog
from PIL import Image, ImageTk
import json
def save_catalog(catalog, filename):
    #Funtion to save the catalog to a JSON file
    with open(filename,"w") as f:
        json.dump(catalog, f)



def load_catalog(filename):
    # Function to load the catalog from a JSON file
    try:
        with open(filename,"r") as f:
            catalog_data = json.load(f)
            print("Catalog data loaded:"), catalog_data
            return catalog_data
    except FileNotFoundError:
        return []
    
# Load the catalog data
hex_catalog = load_catalog("hex_catalog.json")
print("Loaded catalog data:", hex_catalog)

# Load the archive catalog data
archive_catalog = load_catalog("archive_catalog.json")
print("Loaded archive catalog data:", archive_catalog)

save_catalog(hex_catalog, "hex_catalog.json")
save_catalog(archive_catalog, "archive_catalog.json")

def calculate_sunbird_colors(hex_catalog, archive_catalog):
    # Function to calculate the sunbird color for each hex code in the catalog
    for color_data in hex_catalog + archive_catalog:
        if "hex_code" in color_data:
            regular_hex = color_data["hex_code"]
            sunbird_hex = calculate_sunbird(regular_hex)
            color_data["sunbird_hex"] = sunbird_hex
        else:
            print("Error: 'hex_code' key not found in color_data.")

def calculate_sunbird(hex_code):
    # Convert hex code to regular and inverted RGB
    regular_rgb = hex_to_rgb(hex_code)
    inverted_rgb = hex_to_rgb(invert_hex(hex_code))
    
    # Apply Deuteranopia simulation to the inverted color
    simulated_inverted_rgb = simulate_deuteranopia(inverted_rgb)
    
    # Add regular and simulated inverted RGB tuples together
    sunbird_rgb = tuple((reg + sim_inv) // 2 for reg, sim_inv in zip(regular_rgb, simulated_inverted_rgb))
    
    # Convert sunbird RGB to hex
    sunbird_hex = rgb_to_hex(sunbird_rgb)
    
    return sunbird_hex

def simulate_deuteranopia(rgb):
    # Function to simulate Deuteranopia by adjusting RGB components
    # Decrease green component
    adjusted_rgb = (rgb[0], rgb[1] * 0.5, rgb[2])
    return adjusted_rgb



def delete_hex(index):
    # Function to delete a hex code from the main view or archive view
    if 0 <= index < len(hex_catalog):
        hex_code_to_delete = hex_catalog.pop(index)
        hex_code_to_delete["smart"] = True
        archive_catalog.append(hex_code_to_delete)
        save_catalog(hex_catalog, "hex_catalog.json")
        save_catalog(archive_catalog, "archive_catalog.json")
        print("Archive catalog after deletion:", archive_catalog)
        view_hexes()  # Update the view to reflect the deletion
    elif 0 <= index < len(archive_catalog):
        hex_code_to_delete = archive_catalog.pop(index)
        save_catalog(archive_catalog, "archive_catalog.json")
        print("Hex code deleted from the archive catalog:", hex_code_to_delete)
        view_archive()  # Update the archive view to reflect the deletion
    else:
        print("Index out of range for both hex_catalog and archive_catalog")


def restore_hex(index):
    # Function to restore a hex color from the archive view to the main view
    if 0 <= index < len(archive_catalog):
        hex_code_to_restore = archive_catalog.pop(index)
        hex_catalog.append(hex_code_to_restore)
        save_catalog(hex_catalog, "hex_catalog.json")
        save_catalog(archive_catalog, "archive_catalog.json")
        print("Hex catalog after restoration:", hex_catalog)
        print("Archive catalog after restoration:", archive_catalog)
        view_archive()  # Update the archive view to reflect the restoration
    else:
        print("Index out of range for archive_catalog")




def edit_hex(index, catalog):
    # Function to edit a hex color
    print("Editing hex at index:", index)
    print("Length of catalog:", len(catalog))
    
    if 0 <= index < len(catalog):
        hex_code_to_edit = catalog[index]

        # Create a context menu
        context_menu = tk.Menu(root, tearoff=0)

        # Add options to the context menu
        context_menu.add_command(label="Edit Name", command=lambda: edit_name(hex_code_to_edit))
        context_menu.add_command(label="Edit Hex Code", command=lambda: edit_hex_code(hex_code_to_edit))
        context_menu.add_separator()
        context_menu.add_command(label="Delete", command=lambda: delete_hex(index))

        # If it's an archive item, add the "Restore" option
        if catalog == archive_catalog:
            context_menu.add_command(label="Restore", command=lambda: restore_hex(index))

        # Display the context menu at the current mouse position
        context_menu.post(root.winfo_pointerx(), root.winfo_pointery())
    else:
        print("Index out of range for catalog")

   


def edit_name(hex_code_to_edit):
    # Function to edit the name of a hex color
    new_name = simpledialog.askstring("Edit Name", "Enter the new name for this color:", initialvalue=hex_code_to_edit["name"])
    if new_name:
        hex_code_to_edit["name"] = new_name
        save_catalog(hex_catalog, "hex_catalog.json")
        view_hexes()

def edit_hex_code(hex_code_to_edit):
    # Function to edit the hex code of a color
    new_hex_code = simpledialog.askstring("Edit Hex Code", "Enter the new hex code (e.g., #RRGGBB):", initialvalue=hex_code_to_edit["hex_code"])
    if new_hex_code:
        hex_code_to_edit["hex_code"] = new_hex_code
        save_catalog(hex_catalog, "hex_catalog.json")
        view_hexes()

# Define num_columns as a global variable
num_columns = 0


def add_hex():
    # Function to add a new hex color code
    # Open a diolog to get the hex code and name from the user
    hex_code = simpledialog.askstring("Input", "Enter the hex code (e.g., #RRGGBB):")
    if hex_code:
        name = simpledialog.askstring("Input", "Enter the name for this color:")
        if name:
            # Determine if the color is "smart"
            smart = is_smart(hex_code)
            # Add the hex code and name to the catalog
            hex_catalog.append({"name": name, "hex_code": hex_code, "smart": smart})
            # Update the view
            
            view_hexes()
            # Save the catalog to a file
            save_catalog(hex_catalog, "hex_catalog.json")
            # Print data types for debugging
            print(type(hex_code))
            

def view_hexes():
    # Function to view all sotred hex color codes
    # Clear the previous view
    for widget in frame.winfo_children():
        widget.destroy()

    # Deternime the number of colums based on the window width
    num_columns = max(1, root.winfo_width() // 200) # Adjust the division factor as needed

    # Display each hex code and its name
    for i, color_data in enumerate(hex_catalog):
        # Calculate row and column indices
        row = i // num_columns
        column = i % num_columns

        # Create a frame for each color square
        color_frame = tk.Frame(frame, width=101, height=101)
        color_frame.grid(row=row, column=column, padx=5, pady=5, sticky="w")

        #Create a canvas for drawing the colors
        canvas = tk.Canvas(color_frame, width=101, height=101, bg="white", highlightthickness=0)
        canvas.pack(side="left")


        # Draw the rectangle with the hex color
        canvas.create_rectangle(0, 0, 101, 101, fill=color_data["hex_code"], outline="")
        
        # Add an "Edit" button
        edit_button = tk.Button(color_frame, text="Edit", command=lambda idx=i, catalog=hex_catalog: edit_hex(idx, catalog))

        edit_button.pack(side="left", padx=5, pady=5)

        

        # If smart, draw the inverted rectangle diagonally
        if color_data["smart"]:
            invert_color = invert_hex(color_data["hex_code"])
            canvas.create_polygon(0, 0, 101, 0, 101, 101, fill=invert_color, outline="")
       

        # Add a label for the color name
        color_label = tk.Label(color_frame, text=color_data["name"], bg=color_data["hex_code"], padx=0, pady=5)
        color_label.place(relx=0.26, rely=0.4, anchor="center")

        # Add a label for the hex code
        hex_label = tk.Label(color_frame, text=color_data["hex_code"], bg=color_data["hex_code"], padx=0, pady=5)
        hex_label.place(relx=0.26, rely=0.6, anchor="center")
        # Calculate sunbird hex and display it to the left
        sunbird_hex = color_data.get("sunbird_hex")
        if sunbird_hex:
            sunbird_frame = tk.Frame(frame, width=101, height=101)
            sunbird_frame.grid(row=row, column=column - 1, padx=5, pady=5, sticky="e")

            sunbird_canvas = tk.Canvas(sunbird_frame, width=101, height=101, bg="white", highlightthickness=0)
            sunbird_canvas.pack(side="left")

            sunbird_canvas.create_rectangle(0, 0, 101, 101, fill=sunbird_hex, outline="")

            sunbird_label = tk.Label(sunbird_frame, text="Sunbird", bg=color_data["sunbird_hex"], padx=-150, pady=5)

            sunbird_label.place(relx=0.5, rely=0.5, anchor="center")



def view_archive():
    # Function to view the archived hex color codes
    for widget in frame.winfo_children():
        widget.destroy()

    # Determine the number of columns based on the window width
    num_columns = max(1, root.winfo_width() // 200) # Adjust the division factor as needed

    # Display each hex code and its name
    for i, color_data in enumerate(archive_catalog):
        # Calculate row and column indices
        row = i // num_columns
        column = i % num_columns

        # Create a frame for each color square
        color_frame = tk.Frame(frame, width=101, height=101)
        color_frame.grid(row=row, column=column, padx=5, pady=5, sticky="w")

        # Create a canvas for drawing colors
        canvas = tk.Canvas(color_frame, width=101, height=101, bg="white", highlightthickness=0)
        canvas.pack(side="left")
        
        # Draw the rectangle with the hex color
        canvas.create_rectangle(0, 0, 101, 101, fill=color_data["hex_code"], outline="")

        # Add an "Edit" button
        edit_button = tk.Button(color_frame, text="Edit", command=lambda idx=i, catalog=archive_catalog: edit_hex(idx, catalog))
        edit_button.pack(side="left", padx=5, pady=5)

        # If smart, draw the inverted rectangle diagonally
        if color_data["smart"]:
            invert_color = invert_hex(color_data["hex_code"])
            canvas.create_polygon(0, 0, 101, 0, 101, 101, fill=invert_color, outline="")

        # Add a label for the color name
        color_label = tk.Label(color_frame, text=color_data["name"], bg=color_data["hex_code"], padx=0, pady=5)
        color_label.place(relx=0.26, rely=0.4, anchor="center")

        # Add a label for the hex code
        hex_label = tk.Label(color_frame, text=color_data["hex_code"], bg=color_data["hex_code"], padx=0, pady=5)
        hex_label.place(relx=0.26, rely=0.6, anchor="center")



def is_smart(hex_code):
    # Funtion to check if a hex code is "smart" (display both color and invert)
    # For example, we can check if the brightness is above a certain threshold
    # Here, we'll just assume all hex codes are "smart" for demonstration purposes
    return True

def invert_hex(hex_code):
    # Function to calculate the invert of a hex color code
    # we can simply invert each RGB component
    hex_code = hex_code.lstrip("#")
    rgb = tuple(int(hex_code[i:i+2], 16) for i in (0, 2, 4))
    invert_rgb = tuple(255 - value for value in rgb)
    invert_hex = '#{:02x}{:02x}{:02x}'.format(*invert_rgb)
    return invert_hex

def on_closing():
    # Funtion to handle application closing
    save_catalog(hex_catalog, "hex_catalog.json")
    save_catalog(archive_catalog, "archive_catalog.json")
    print("Hex catalog before closing:", hex_catalog)
    print("Archive catalog before closing:", archive_catalog)
    root.destroy()


# Create the main application window
root = tk.Tk()
root.title("Hex Color Manager")

# Create the "View Archive" button
view_archive_button = tk.Button(root, text="View Archive", command=view_archive)
view_archive_button.pack()

# Create and place the necessary GUI components
label = tk.Label(root, text="Hex Color Manager")
label.pack()

add_button = tk.Button(root, text="Add Hex", command=add_hex)
add_button.pack()

view_button = tk.Button(root, text="View Hexes", command=view_hexes)
view_button.pack()



frame = tk.Frame(root)
frame.pack()
# Start the main event loop

# Bind the closing event to the on_closing funtion
root.protocol("WM_DELETE_WINDOW", on_closing)

root.mainloop()

