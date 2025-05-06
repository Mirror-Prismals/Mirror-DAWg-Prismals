import tkinter as tk

# Hex to RGB conversion
colors = {
    "#308000": (48, 128, 0),
    "#330000": (51, 0, 0)
}

# Create a tkinter window
window = tk.Tk()
window.title("MiriDAW Mockup")
window.geometry("400x300")

# Set background color
window.configure(background='#00C080')

# Create labels with different background colors
label1 = tk.Label(window, text="Mockup", bg="#308000", fg="white")
label1.pack(pady=10)

label2 = tk.Label(window, text="Shell", bg="#330000", fg="white")
label2.pack(pady=10)

window.mainloop()
