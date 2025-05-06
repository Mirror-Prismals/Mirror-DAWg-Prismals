import tkinter as tk

class DawgServer:
    def __init__(self):
        pass

    def display_error(self):
        # Display an error message
        error_root = tk.Tk()
        error_root.attributes('-fullscreen', True)
        
        # Background widget
        self.bg_label = tk.Label(error_root, text="Bugged Server", fg="lime", font=("Arial", 16), justify="center", bg="green")
        self.bg_label.place(relx=0.5, rely=0.5, anchor=tk.CENTER)
        
        # Non-background widget
        self.non_bg_label = tk.Label(error_root, text="Lorem ipsum", fg="black", font=("Arial", 16), justify="center")
        self.non_bg_label.place(relx=0.5, rely=0.2, anchor=tk.CENTER)

        error_root.mainloop()

    def fullscreen(self):
        # Set fullscreen attribute only for the background widget
        self.error_root.configure(bg="blue")  # Set background color to blue
        self.bg_label.attributes('-fullscreen', True)
        
    def variable(self):
        # Store variables from one to 100 in the non-background widget
        self.non_bg_label.config(text='\n'.join([str(i) for i in range(1, 101)]))



class FullscreenLoop:
    def __init__(self):
        self.root = tk.Tk()
        self.root.attributes('-fullscreen', True)
        self.root.configure(bg="blue")  # Set initial background color
        self.root.bind("<Escape>", self.close_fullscreen)  # Exit fullscreen on Esc key press
        
        # Create example labels
        self.bg_label = tk.Label(self.root, text="Bugged Server", fg="lime", font=("Arial", 16), justify="center", bg="green")
        self.bg_label.place(relx=0.5, rely=0.5, anchor=tk.CENTER)
        self.non_bg_label = tk.Label(self.root, text="Lorem ipsum", fg="black", font=("Arial", 16), justify="center")
        self.non_bg_label.place(relx=0.5, rely=0.2, anchor=tk.CENTER)
        
        self.change_background_color()  # Start changing background color

    def change_background_color(self):
        # Change the background color to green
        self.root.configure(bg="green")
        # Lower the background below the labels
        self.lower_background()
        # Schedule the next change after 1000 milliseconds (1 second)
        self.root.after(1000, self.change_background_color)

    def lower_background(self):
        # Lower the background color to ensure it's below the labels
        self.root.lower()

    def close_fullscreen(self, event):
        # Close fullscreen on Esc key press
        self.root.destroy()

if __name__ == "__main__":
    fullscreen_loop = FullscreenLoop()
    fullscreen_loop.root.mainloop()

dawg_server = DawgServer()
dawg_server.display_error()
dawg_server.variable()
