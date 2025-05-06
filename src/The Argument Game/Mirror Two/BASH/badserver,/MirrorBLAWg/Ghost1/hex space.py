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
        hex_colors = {
            (0, 0): "#666666", (0, 1): "#666667", (0, 2): "#666677", (0, 3): "#666678",
            (0, 4): "#666777", (0, 5): "#666778", (0, 6): "#666788", (0, 7): "#666789",
            (1, 0): "#667777", (1, 1): "#667778", (1, 2): "#667788", (1, 3): "#667789",
            (1, 4): "#667888", (1, 5): "#667889", (1, 6): "#667899", (1, 7): "#677777",
            (2, 0): "#677778", (2, 1): "#677788", (2, 2): "#677789", (2, 3): "#677888",
            (2, 4): "#677889", (2, 5): "#677899", (2, 6): "#678888", (2, 7): "#678889",
            (3, 0): "#678899", (3, 1): "#678999", (3, 2): "#777777", (3, 3): "#777778",
            (3, 4): "#777788", (3, 5): "#777789", (3, 6): "#777888", (3, 7): "#777889",
        }
        return hex_colors.get((self.width, self.depth), "#000000")

    def update_label(self):
        self.label.config(text=self.get_label_text())
        self.root.configure(bg=self.get_hex_color())

    def increase_depth(self, event):
        if self.depth < 7:  # Adjusted to match available hex colors
            self.depth += 1
            self.update_label()

    def decrease_depth(self, event):
        if self.depth > 0:
            self.depth -= 1
            self.update_label()

    def increase_width(self, event):
        if self.width < 3:  # Adjusted to match available hex colors
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
