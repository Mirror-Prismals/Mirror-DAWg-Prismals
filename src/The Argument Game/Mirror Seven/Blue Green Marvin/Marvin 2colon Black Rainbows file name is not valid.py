import tkinter as tk
import random
import string
from collections import defaultdict
#scumbag start
def display_text():
    green_window = tk.Toplevel()
    green_window.title("Green Window")
    green_window.configure(bg="green")
    text_label = tk.Label(green_window, text="(__.'''dChatGPTAh, got it! 😄'''.__)", fg="white", bg="green")
    text_label.pack(padx=20, pady=20)

# Create main Tkinter window
window = tk.Tk()
window.title("Main Window")

# Create button to display text in green window
display_button = tk.Button(window, text="Display Text in Green Window", command=display_text)
display_button.pack(pady=20)
#scum Lord.
def update_output():
    initial_text = "!"
    input_entry.insert(tk.END, initial_text)
    processed_text = markov_chain.generate_text(length=50)
    output_label1.config(text=processed_text)
    output_label2.config(text=processed_text)
    window2.after(1000, update_output)

class MarkovChain:
    def __init__(self, order=1):
        self.order = order
        self.chain = defaultdict(list)

    def train(self, text):
        for i in range(len(text) - self.order):
            key = text[i:i + self.order]
            value = text[i + self.order]
            self.chain[key].append(value)

    def generate_text(self, length=100):
        current_state = random.choice(list(self.chain.keys()))
        output = current_state
        for _ in range(length):
            next_state = random.choice(self.chain[current_state])
            output += next_state
            current_state = output[-self.order:]
        return output

def update_output():
    input_text = input_entry.get()
    processed_text = markov_chain.generate_text(length=50)
    output_label1.config(text=processed_text)
    output_label2.config(text=processed_text)
    window2.after(1000, update_output)

def hex_to_rgb(hex_color):
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

# Create MarkovChain instance
markov_chain = MarkovChain(order=3)
# Example training data
training_text = "This is a sample text for training the Markov chain model. It will generate text based on the patterns observed in the training data."
markov_chain.train(training_text)

# Create Tkinter windows
window1 = tk.Tk()
window1.title("Output Window 1")
window1.attributes("-fullscreen", True)
output_label1 = tk.Label(window1, text="", fg="white", bg="#0f4400")
output_label1.pack(fill="both", expand=True)

window2 = tk.Tk()
window2.title("Output Window 2")
window2.geometry("400x300")
output_label2 = tk.Label(window2, text="", fg="white", bg="#0f3240")
output_label2.pack(fill="both", expand=True)

# Create input entry
input_entry = tk.Entry(window1)
input_entry.pack()

# Create update button
update_button = tk.Button(window1, text="Update Output", command=update_output)
update_button.pack()

# Run Tkinter main loop for window1
window1.mainloop()

# Start recursive updating for window2
update_output()
window2.mainloop()
