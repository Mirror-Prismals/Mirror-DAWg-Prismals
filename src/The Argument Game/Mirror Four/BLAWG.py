import tkinter as tk

def create_polygon(canvas, points, fill_color):
    canvas.create_polygon(points, fill=fill_color, outline="black")

def create_popup_with_polygons():
    popup = tk.Toplevel()
    popup.title("MirrorDaw Popup")
    
    canvas = tk.Canvas(popup, bg="black")
    canvas.pack(fill="both", expand=True)
    
    # Define polygons and their colors for the popup
    polygons_popup = [
        # Add 12 polygons here based on the new image
        ([100, 100, 200, 100, 250, 200, 150, 200], "white"),
        ([300, 100, 400, 100, 450, 200, 350, 200], "white"),
        ([500, 100, 600, 100, 650, 200, 550, 200], "white"),
        ([700, 100, 800, 100, 850, 200, 750, 200], "white"),
        ([100, 300, 200, 300, 250, 400, 150, 400], "white"),
        ([300, 300, 400, 300, 450, 400, 350, 400], "white"),
        ([500, 300, 600, 300, 650, 400, 550, 400], "white"),
        ([700, 300, 800, 300, 850, 400, 750, 400], "white"),
        ([100, 500, 200, 500, 250, 600, 150, 600], "white"),
        ([300, 500, 400, 500, 450, 600, 350, 600], "white"),
        ([500, 500, 600, 500, 650, 600, 550, 600], "white"),
        ([700, 500, 800, 500, 850, 600, 750, 600], "white")
    ]
    
    for points, color in polygons_popup:
        create_polygon(canvas, points, color)
    
    popup.geometry("1024x768")  # Set the popup window size
    
    popup.mainloop()

def create_mirror_window():
    root = tk.Tk()
    root.attributes('-fullscreen', True)
    
    canvas = tk.Canvas(root, bg="white")
    canvas.pack(fill="both", expand=True)
    
    # Define polygons and their colors for the main window
    polygons_main = [
        # Add your 48 polygons here
        ([50, 50, 150, 50, 200, 150, 100, 150], "lime"),
        ([200, 50, 300, 50, 350, 150, 250, 150], "purple"),
        ([350, 50, 450, 50, 500, 150, 400, 150], "yellow"),
        ([800, 300, 900, 300, 950, 400, 850, 400], "black"),
        ([1200, 50, 1300, 50, 1350, 150, 1250, 150], "blue"),
        ([1350, 50, 1450, 50, 1500, 150, 1400, 150], "red"),
        ([900, 350, 1100, 350, 1100, 450, 900, 450], "black"),
        ([1200, 400, 1250, 400, 1250, 450, 1200, 450], "white"),
        ([1300, 400, 1350, 400, 1350, 450, 1300, 450], "white"),
        ([1400, 400, 1450, 400, 1450, 450, 1400, 450], "white"),
        ([50, 600, 150, 600, 200, 700, 100, 700], "cyan"),
        ([200, 600, 300, 600, 350, 700, 250, 700], "lightblue"),
        ([350, 600, 450, 600, 500, 700, 400, 700], "blue"),
        ([1200, 600, 1250, 600, 1250, 700, 1200, 700], "green"),
        ([1300, 600, 1350, 600, 1350, 700, 1300, 700], "lime"),
        ([1400, 600, 1450, 600, 1450, 700, 1400, 700], "darkgreen")
    ]
    
    # Create the polygons on the canvas
    for points, color in polygons_main:
        create_polygon(canvas, points, color)
    
    # Show popup window with polygons
    create_popup_with_polygons()
    
    # Exit fullscreen on Esc key press
    def exit_fullscreen(event):
        root.attributes('-fullscreen', False)
        root.destroy()

    root.bind("<Escape>", exit_fullscreen)

    root.mainloop()

if __name__ == "__main__":
    create_mirror_window()
