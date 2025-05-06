import tkinter as tk
from tkinter import filedialog, messagebox
import socket
import threading
import time

def open_file():
    filepath = filedialog.askopenfilename(filetypes=[("Text Files", "*.txt")])
    if filepath:
        with open(filepath, 'r') as file:
            text_editor.delete('1.0', tk.END)
            text_editor.insert(tk.END, file.read())

def save_file():
    filepath = filedialog.asksaveasfilename(defaultextension=".txt", filetypes=[("Text Files", "*.txt")])
    if filepath:
        with open(filepath, 'w') as file:
            file.write(text_editor.get('1.0', tk.END))

def on_key_press(event):
    global client_socket
    char = event.char
    print("Key pressed:", char)

    # Send the pressed key to the server
    client_socket.sendall(char.encode())

def start_server():
    server_window = tk.Toplevel(root)
    server_window.title("Server")
    server_window.configure(bg="#184f51")  # Set background color
    
    server_label = tk.Label(server_window, text="Server is listening on port 5555.", bg="#184f51", fg="#a54234")  # Set text and background color
    server_label.pack()

    while True:
        server_label.config(text="Server is listening on port 5555.")
        time.sleep(0.5)
        server_label.config(text="Server is listening on port 5555. ")
        time.sleep(0.5)

root = tk.Tk()
root.title("how do you feel, tangerine?")
root.attributes('-fullscreen', True)  # Set window to fullscreen
root.configure(bg="#184f51")  # Set main window background color

text_editor = tk.Text(root)
text_editor.pack(expand=True, fill='both')

menu_bar = tk.Menu(root)
file_menu = tk.Menu(menu_bar, tearoff=0)
file_menu.add_command(label="Open", command=open_file)
file_menu.add_command(label="Save", command=save_file)
file_menu.add_separator()
file_menu.add_command(label="Exit", command=root.quit)
menu_bar.add_cascade(label="File", menu=file_menu)

root.config(menu=menu_bar)

# Start the server in a separate thread
server_thread = threading.Thread(target=start_server)
server_thread.start()

root.mainloop()
