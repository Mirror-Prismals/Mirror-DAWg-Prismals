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
    if event.char.lower() == 'm':
        console_root.lift()  # Raise the console window to the top

def open_console():
    global console_root
    console_root = tk.Toplevel()
    console_root.title("Console")
    console_root.geometry("800x600")  # Set window size
    console_root.minsize(400, 300)  # Set minimum size
    console_root.configure(bg="#a54234")

    text_editor_console = tk.Text(console_root)
    text_editor_console.pack(expand=True, fill='both')

    menu_bar = tk.Menu(console_root)
    file_menu = tk.Menu(menu_bar, tearoff=0)
    file_menu.add_command(label="Open", command=open_file)
    file_menu.add_command(label="Save", command=save_file)
    file_menu.add_separator()
    file_menu.add_command(label="Exit", command=console_root.destroy)
    menu_bar.add_cascade(label="File", menu=file_menu)

    console_root.config(menu=menu_bar)

root = tk.Tk()
root.title("Text Editor")

text_editor = tk.Text(root)
text_editor.pack(expand=True, fill='both')

text_editor.bind("<Key>", on_key_press)  # Listen for keystrokes

def start_server():
    server_root = tk.Toplevel()
    server_root.title("Mock Server")
    server_root.attributes('-fullscreen', True)  # Set window to nearly fullscreen
    server_root.configure(bg="#a54234")

    entropy_label_server = tk.Label(server_root, bg="#184f51", fg="white", font=("Courier", 12))  # Label for streaming characters
    entropy_label_server.pack(expand=True, fill="both")

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("127.0.0.1", 5555))
    server_socket.listen(1)
    entropy_label_server.config(text="Server is listening on port 5555")

    client_socket, _ = server_socket.accept()
    entropy_label_server.config(text="Client connected")

    received_data = ""  # Initialize empty string to store received characters

    while True:
        data = client_socket.recv(1).decode()
        if not data:
            break
        received_data += data  # Concatenate received character
        entropy_label_server.config(text=received_data)
        server_root.update()  # Update the GUI to show the latest character
        time.sleep(0.1)  # Adjust the delay as needed

# Start the server in a separate thread
server_thread = threading.Thread(target=start_server, daemon=True)
server_thread.start()

open_console()  # Open console window initially

root.mainloop()
