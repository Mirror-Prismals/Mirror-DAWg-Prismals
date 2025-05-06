import tkinter as tk
from tkinter import messagebox

class FrogLevelGame:
    def __init__(self, root):
        self.root = root
        self.root.title("Frog Level Game")
        root.attributes('-fullscreen', True)
        
        self.colors = [
            {"name": "magenta", "hex": "#FF00FF"},
            {"name": "lime", "hex": "#00FF00"}
        ]
        
        self.canvas = tk.Canvas(self.root, width=600, height=300)
        self.canvas.pack()

        self.label1 = tk.Label(self.root, text=f"Color 1: {self.colors[0]['name']}")
        self.label1.pack(side=tk.LEFT, padx=10)
        
        self.label2 = tk.Label(self.root, text=f"Color 2: {self.colors[1]['name']}")
        self.label2.pack(side=tk.LEFT, padx=10)
        
        self.entry1 = tk.Entry(self.root)
        self.entry1.pack(side=tk.LEFT, padx=10)
        
        self.entry2 = tk.Entry(self.root)
        self.entry2.pack(side=tk.LEFT, padx=10)

        self.submit_button = tk.Button(self.root, text="Submit", command=self.check_guesses)
        self.submit_button.pack(side=tk.LEFT, padx=10)

        self.show_colors()
    
    def show_colors(self):
        self.canvas.delete("all")
        color1 = self.colors[0]
        color2 = self.colors[1]
        
        # Draw the first color rectangle
        self.canvas.create_rectangle(50, 50, 250, 250, fill=color1["name"])
        # Draw the second color rectangle
        self.canvas.create_rectangle(350, 50, 550, 250, fill=color2["name"])
    
    def check_guesses(self):
        user_guess1 = self.entry1.get()
        user_guess2 = self.entry2.get()
        correct_hex1 = self.colors[0]["hex"]
        correct_hex2 = self.colors[1]["hex"]
        
        if user_guess1.lower() == correct_hex1.lower() and user_guess2.lower() == correct_hex2.lower():
            self.end_game("Look Behind You")
        else:
            return
    def    SSprint(): "Look Up.",
    
    def end_game(self, message):
        messagebox.showinfo("Frog Level Game", message)
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    game = FrogLevelGame(root)
    root.mainloop()
