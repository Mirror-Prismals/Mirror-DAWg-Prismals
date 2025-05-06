import tkinter as tk

def create_polygon(canvas, points, fill_color):
    canvas.create_polygon(points, fill=fill_color, outline="black")

def create_machine_window():
    root = tk.Tk("Sunbird Precis Func")
    root.attributes('-fullscreen', True)
    
    canvas = tk.Canvas(root, bg="red")
    canvas.pack(fill="both", expand=True)
    
    # Define the 48 + 48 polygons and their colors
    polygons = [
        # First set of 48 polygons
        ([50, 50, 150, 50, 200, 150, 100, 150], "darkred"),
        ([150, 50, 250, 50, 300, 150, 200, 150], "red"),
        ([250, 50, 350, 50, 400, 150, 300, 150], "orange"),
        ([50, 150, 150, 150, 200, 250, 100, 250], "darkred"),
        ([150, 150, 250, 150, 300, 250, 200, 250], "red"),
        ([250, 150, 350, 150, 400, 250, 300, 250], "orange"),
        ([50, 250, 150, 250, 200, 350, 100, 350], "darkred"),
        ([150, 250, 250, 250, 300, 350, 200, 350], "red"),
        ([250, 250, 350, 250, 400, 350, 300, 350], "orange"),
        ([50, 350, 150, 350, 200, 450, 100, 450], "darkred"),
        ([150, 350, 250, 350, 300, 450, 200, 450], "red"),
        ([250, 350, 350, 350, 400, 450, 300, 450], "orange"),
        ([50, 450, 150, 450, 200, 550, 100, 550], "darkred"),
        ([150, 450, 250, 450, 300, 550, 200, 550], "red"),
        ([250, 450, 350, 450, 400, 550, 300, 550], "orange"),
        ([50, 550, 150, 550, 200, 650, 100, 650], "darkred"),
        ([150, 550, 250, 550, 300, 650, 200, 650], "red"),
        ([250, 550, 350, 550, 400, 650, 300, 650], "orange"),
        # Remaining polygons to complete the first set of 48
        ([300, 300, 350, 300, 350, 350, 300, 350], "orange"),
        ([350, 300, 400, 300, 400, 350, 350, 350], "orange"),
        ([400, 300, 450, 300, 450, 350, 400, 350], "orange"),
        ([450, 300, 500, 300, 500, 350, 450, 350], "orange"),
        ([475, 275, 525, 275, 525, 325, 475, 325], "darkorange"),
        # Second set of 48 polygons
        ([50, 400, 150, 400, 200, 500, 100, 500], "darkred"),
        ([150, 400, 250, 400, 300, 500, 200, 500], "red"),
        ([250, 400, 350, 400, 400, 500, 300, 500], "orange"),
        ([50, 500, 150, 500, 200, 600, 100, 600], "darkred"),
        ([150, 500, 250, 500, 300, 600, 200, 600], "red"),
        ([250, 500, 350, 500, 400, 600, 300, 600], "orange"),
        ([50, 600, 150, 600, 200, 700, 100, 700], "darkred"),
        ([150, 600, 250, 600, 300, 700, 200, 700], "red"),
        ([250, 600, 350, 600, 400, 700, 300, 700], "orange"),
        ([50, 700, 150, 700, 200, 800, 100, 800], "darkred"),
        ([150, 700, 250, 700, 300, 800, 200, 800], "red"),
        ([250, 700, 350, 700, 400, 800, 300, 800], "orange"),
        ([50, 800, 150, 800, 200, 900, 100, 900], "darkred"),
        ([150, 800, 250, 800, 300, 900, 200, 900], "red"),
        ([250, 800, 350, 800, 400, 900, 300, 900], "orange"),
        ([50, 900, 150, 900, 200, 1000, 100, 1000], "darkred"),
        ([150, 900, 250, 900, 300, 1000, 200, 1000], "red"),
        ([250, 900, 350, 900, 400, 1000, 300, 1000], "orange"),
        # Remaining polygons to complete the second set of 48
        ([300, 600, 350, 600, 350, 650, 300, 650], "orange"),
        ([350, 600, 400, 600, 400, 650, 350, 650], "orange"),
        ([400, 600, 450, 600, 450, 650, 400, 650], "orange"),
        ([450, 600, 500, 600, 500, 650, 450, 650], "orange"),
        ([475, 575, 525, 575, 525, 625, 475, 625], "darkorange")
    ]
    
    # Create the polygons on the canvas
    for points, color in polygons:
        create_polygon(canvas, points, color)
    
    # Exit fullscreen on Esc key press
    def exit_fullscreen(event):
        root.attributes('-fullscreen', False)
        root.destroy()

    root.bind("<Escape>", exit_fullscreen)

    root.mainloop()

if __name__ == "__main__":
    create_machine_window()
