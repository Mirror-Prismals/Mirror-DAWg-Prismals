import tkinter as tk

class HexSpace:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("Hex Space")
        self.root.attributes('-fullscreen', True)  # Set window to fullscreen
        self.root.configure(bg="black")  # Set background to black

        self.width = 0
        self.depth = 0

        self.label = tk.Label(self.root, text=self.get_label_text(), bg="black", fg="white")
        self.label.pack(anchor="ne", padx=10, pady=10)

        self.root.bind("<Up>", self.increase_depth)
        self.root.bind("<Down>", self.decrease_depth)
        self.root.bind("<Left>", self.decrease_width)
        self.root.bind("<Right>", self.increase_width)
        self.root.bind("<Button-1>", self.change_color)

        self.root.mainloop()

    def get_label_text(self):
        hex_color = self.get_hex_color()
        return f"Plug Depth, ({self.width}, {self.depth}) {hex_color}"

    def get_hex_color(self):
        # Algorithm to generate hex color based on width and depth
        red = int((self.width / 3) * 255)
        green = int((self.depth / 7) * 255)
        blue = 255 - max(red, green)

        return "#{:02X}{:02X}{:02X}".format(red, green, blue)

    def update_label(self):
        self.label.config(text=self.get_label_text())
        self.root.configure(bg=self.get_hex_color())

    def increase_depth(self, event):
        if self.depth < 7:
            self.depth += 1
            self.update_label()

    def decrease_depth(self, event):
        if self.depth > 0:
            self.depth -= 1
            self.update_label()

    def increase_width(self, event):
        if self.width < 3:
            self.width += 1
            self.update_label()

    def decrease_width(self, event):
        if self.width > 0:
            self.width -= 1
            self.update_label()

    def change_color(self, event):
        self.update_label()

if __name__ == "__main__":
    HexSpace()
