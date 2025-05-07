import tkinter as tk

# Mapping of letters A-P (case-insensitive) to hex digits 0-F
letter_to_hex = {
    'a': '0', 'b': '1', 'c': '2', 'd': '3', 'e': '4', 'f': '5',
    'g': '6', 'h': '7', 'i': '8', 'j': '9', 'k': 'a', 'l': 'b',
    'm': 'c', 'n': 'd', 'o': 'e', 'p': 'f'
}

def chunk_text_by_color(text):
    chunks = []
    hex_digits = []
    current_chunk = ''
    mapped_count = 0

    for char in text:
        current_chunk += char
        if char.lower() in letter_to_hex:
            hex_digits.append(letter_to_hex[char.lower()])
            mapped_count += 1
            if mapped_count == 6:
                # Pad hex_digits if needed (should always be 6 here)
                color = '#' + ''.join(hex_digits)
                chunks.append((color, current_chunk))
                # Reset for next chunk
                hex_digits = []
                current_chunk = ''
                mapped_count = 0

    # Handle any remainder
    if mapped_count > 0:
        color = '#' + ''.join(hex_digits + ['0'] * (6 - mapped_count))
        chunks.append((color, current_chunk))

    return chunks

# Example longer corpus
corpus = """
Not bad! Pretty good. Yes! A baded example. Another bAdEdE found here. 
Nothing here. abcdefghijklmnop! This is a test of the color tokenizer.
"""

chunks = chunk_text_by_color(corpus)

root = tk.Tk()
root.title("Color Tokenizer Chunks")

for color, chunk in chunks:
    row = tk.Frame(root)
    row.pack(fill=tk.X, padx=5, pady=2)
    color_box = tk.Label(row, width=10, bg=color, text=color, fg='white' if int(color[1:3],16)<128 else 'black')
    color_box.pack(side=tk.LEFT, padx=2)
    text_label = tk.Label(row, text=chunk, anchor='w', justify='left')
    text_label.pack(side=tk.LEFT, fill=tk.X, expand=True)

root.mainloop()
