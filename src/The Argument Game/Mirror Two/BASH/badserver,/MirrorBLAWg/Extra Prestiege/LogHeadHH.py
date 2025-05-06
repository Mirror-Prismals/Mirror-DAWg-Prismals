import tkinter as tk
import random
import string

class LogHead:
    def __init__(self, root):
        self.root = root
        self.root.title("Log Head - ASCII Waterfall")
        self.root.attributes('-fullscreen', True)  # Set window to fullscreen
        self.root.configure(bg="black")

        self.text_area = tk.Text(self.root, bg="black", fg="green", insertbackground="white", font=("Courier", 12))
        self.text_area.pack(expand=1, fill='both')

        self.exit_button = tk.Button(self.root, text="Exit", command=self.exit_fullscreen, bg="red", fg="white")
        self.exit_button.pack(anchor="ne", padx=10, pady=10)

        self.update_text_area()

    def update_text_area(self):
        max_lines = 30
        new_line = ''.join(random.choices(string.ascii_letters + string.digits + string.punctuation, k=80))
        self.text_area.insert(tk.END, new_line + "\n")
        if int(self.text_area.index('end-1c').split('.')[0]) > max_lines:
            self.text_area.delete("1.0", "2.0")
        self.text_area.see(tk.END)
        self.root.after(100, self.update_text_area)

    def exit_fullscreen(self):
        self.root.attributes('-fullscreen', False)
        self.root.quit()

if __name__ == "__main__":
    root = tk.Tk()
    app = LogHead(root)
    root.mainloop()
