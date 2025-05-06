import random
import string
import torch
import tkinter as tk
from transformers import GPT2LMHeadModel, GPT2Tokenizer

# Define the virtual world size
WORLD_WIDTH = 100
WORLD_HEIGHT = 100
WORLD_DEPTH = 50  # Adding depth for the z coordinate

# Load GPT-2 model and tokenizer
model_name = 'gpt2'
tokenizer = GPT2Tokenizer.from_pretrained(model_name)
model = GPT2LMHeadModel.from_pretrained(model_name)

# Ensure the model runs on CPU
device = torch.device("cpu")
model.to(device)

def generate_adventure_message():
    # Generate a random letter
    random_letter = random.choice(string.ascii_lowercase)
    # Create the prompt with the random letter
    prompt = f"You go on an adventure and {random_letter}"
    # Encode the input prompt
    inputs = tokenizer.encode(prompt, return_tensors="pt").to(device)
    # Generate text
    outputs = model.generate(inputs, max_length=50, num_return_sequences=1, no_repeat_ngram_size=2, early_stopping=True)
    # Decode the output
    text = tokenizer.decode(outputs[0], skip_special_tokens=True)
    return text

def generate_random_coordinates():
    x = random.randint(0, WORLD_WIDTH)
    y = random.randint(0, WORLD_HEIGHT)
    z = random.randint(0, WORLD_DEPTH)
    return x, y, z

def get_random_ascii_art():
    ascii_art_list = [
        r"""
        /\_/\
       ( o.o )
        > ^ <
        """,
        r"""
         __
        /  \
        \__/
        """,
        r"""
        \(^_^)/
        """,
        r"""
        (o_o)
        """,
        r"""
        ( >_<)
        """
    ]
    return random.choice(ascii_art_list)

def start_adventure():
    x, y, z = generate_random_coordinates()
    adventure = generate_adventure_message()
    ascii_art = get_random_ascii_art()

    adventure_message = f"\nYour random coordinates are: ({x}, {y}, {z})\nAdventure: {adventure}\n{ascii_art}\n"
    adventure_text.insert(tk.END, adventure_message)
    adventure_text.see(tk.END)

def on_start_adventure():
    start_adventure()

# Set up the tkinter window
root = tk.Tk()
root.title("Virtual Randonauting Adventure")
root.geometry("600x600")

# Create a text widget to display the adventure messages
adventure_text = tk.Text(root, wrap=tk.WORD, height=30, width=70)
adventure_text.pack(pady=20)

# Create a button to start a new adventure
start_button = tk.Button(root, text="Start Adventure", command=on_start_adventure)
start_button.pack()

# Create a button to quit the game
quit_button = tk.Button(root, text="Quit", command=root.quit)
quit_button.pack(pady=10)

# Start the tkinter event loop
root.mainloop()
