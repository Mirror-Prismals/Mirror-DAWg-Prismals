import tkinter as tk

def on_return(event):
    print("Return")

root = tk.Tk()
root.title("Text Editor")

text_editor = tk.Text(root, wrap="word", width=80, height=24)  # Adjust width and height as needed
text_editor.pack(expand=True, fill='both')

text_editor.bind("<Return>", on_return)

root.mainloop()
