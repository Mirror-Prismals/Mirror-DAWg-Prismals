import tkinter as tk
import random

class LogHead:
    def __init__(self, root):
        self.root = root
        self.root.title("Log Head - ASCII Waterfall")
        self.root.attributes('-fullscreen', True)  # Set window to fullscreen
        self.root.configure(bg="black")

        # Calculate font size based on screen resolution
        width = self.root.winfo_screenwidth()
        height = self.root.winfo_screenheight()
        
        # Assuming a monospaced font where each character is roughly square
        font_size = int(min(width / 2, height / 80))  # Adjust these values as needed

        def hex_to_rgb(hex_code):
            hex_code = hex_code.lstrip('#')
            return tuple(int(hex_code[i:i+2], 16) for i in (0, 2, 4))

        # Example of how to use the function
        hex_color = '#7A89C9'
        rgb_color = hex_to_rgb(hex_color)
        print(rgb_color)
        self.text_area = tk.Text(self.root, bg="black", fg="#7A89C9", insertbackground="white", font=("Courier", font_size))
        self.text_area.pack(expand=1, fill='both')

        self.exit_button = tk.Button(self.root, text="Exit", command=self.exit_fullscreen, bg="red", fg="white")
        self.exit_button.pack(anchor="ne", padx=10, pady=10)

        self.update_text_area()

    def update_text_area(self):
        # Calculate number of lines and characters based on the window size and font size
        width = self.root.winfo_screenwidth()
        height = self.root.winfo_screenheight()
        font_size = int(min(width / 624, height / 624))  # Adjust these values as needed

        char_width = int(width / (font_size * 0.6))  # Width of each character
        max_lines = int(height / font_size)  # Number of lines that fit in the height

        new_line = ''.join(random.choices("qwertyuiopasdfghjklzxcvbnmabcdefghijklmnopqrstuvqxyz", k=char_width))
        self.text_area.insert(tk.END, new_line + "\n")
        if int(self.text_area.index('end-1c').split('.')[0]) > max_lines:
            self.text_area.delete("1.0", "2.0")
        self.text_area.see(tk.END)
        self.root.after(50, self.update_text_area)  # Faster update rate

    def exit_fullscreen(self, event=None):
        self.root.attributes('-fullscreen', False)
        self.root.quit()

if __name__ == "__main__":
    root = tk.Tk()
    app = LogHead(root)
    root.bind("<Escape>", app.exit_fullscreen)
    root.mainloop()
