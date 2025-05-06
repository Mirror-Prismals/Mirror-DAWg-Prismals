#Wrapper Idea
import tkinter as tk

# Hex to RGB conversion
colors = {
    "#308000": (48, 128, 0),
    "#330000": (51, 0, 0)
}

def move_window(event):
    root.geometry(f"+{event.x_root}+{event.y_root}")

# Create a tkinter window
root = tk.Tk()
root.title("MiriDAW Mockup")
root.geometry("400x300")
root.configure(bg='#00C080')

# Create a frame to hold the shell
shell_frame = tk.Frame(root, bg='#00C080')
shell_frame.place(relx=0.05, rely=0.05, relwidth=0.9, relheight=0.9)

# Create labels with different background colors (simulating text input)
label1 = tk.Label(shell_frame, text="Mockup", bg="#308000", fg="white")
label1.pack(pady=10)

label2 = tk.Label(shell_frame, text="Shell", bg="#330000", fg="white")
label2.pack(pady=10)

# Make the window movable
shell_frame.bind("<B1-Motion>", move_window)

root.mainloop()
'''In this code:

I've added a shell_frame to hold the shell-like interface.
The shell frame is placed within the root window and is draggable.
The move_window function allows the user to drag the shell frame around the screen by clicking and holding the left mouse button.
This should give you a basic shell-like interface that you can move around within the tkinter window. Adjustments can be made to suit your specific requirements!'''
