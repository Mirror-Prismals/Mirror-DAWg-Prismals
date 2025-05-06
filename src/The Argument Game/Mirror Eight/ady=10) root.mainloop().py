#ady=10)     root.mainloop()
#who are yoU?
import tkinter as tk
from tkinter import Canvas, NW, PhotoImage, filedialog

class FakeOS:
    def __init__(self, root):
        self.root = root
        self.canvas = Canvas(root, bg='white', highlightthickness=0)
        self.canvas.pack(fill='both', expand=True)
        self.image_obj = None  # Initialize image object to None
        self.canvas.bind("<ButtonPress-1>", self.start_move)
        self.canvas.bind("<B1-Motion>", self.move_image)

        self.start_x, self.start_y = None, None

    def open_image(self):
        file_path = filedialog.askopenfilename(title="Select an image file", filetypes=[("Image files", "*.jpg;*.jpeg;*.png;*.gif")])
        if file_path:
            self.img = PhotoImage(file=file_path)
            if self.image_obj:  # If image object already exists, delete it
                self.canvas.delete(self.image_obj)
            self.image_obj = self.canvas.create_image(0, 0, anchor=NW, image=self.img)

    def start_move(self, event):
        self.start_x = event.x
        self.start_y = event.y

    def move_image(self, event):
        if self.start_x is not None and self.start_y is not None:
            dx = event.x - self.start_x
            dy = event.y - self.start_y
            if self.image_obj:  # Check if image object exists
                self.canvas.move(self.image_obj, dx, dy)
            self.start_x = event.x
            self.start_y = event.y

if __name__ == "__main__":
    root = tk.Tk()
    root.title("Fake OS")
    app = FakeOS(root)
    open_button = tk.Button(root, text="Open Image", command=app.open_image)
    open_button.pack(pady=10)
    root.mainloop()
