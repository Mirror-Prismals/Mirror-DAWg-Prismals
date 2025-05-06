import tkinter as tk
from tkinter import filedialog, messagebox
import socket
import threading

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
    global server_socket, client_socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("127.0.0.1", 5555))  # Bind to localhost on port 5555
    server_socket.listen(1)  # Listen for one connection
    print("Server is listening on port 5555")

    client_socket, _ = server_socket.accept()
    print("Client connected")

def receive_from_server():
    global client_socket
    while True:
        try:
            data = client_socket.recv(1024).decode()
            if data:
                print("Server:", data)
        except Exception as e:
            print("Error receiving from server:", e)
            break

root = tk.Tk()
root.title("Text Editor")

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

text_editor.bind("<Key>", on_key_press)

start_server()

# Start a thread to receive messages from the server
receive_thread = threading.Thread(target=receive_from_server)
receive_thread.daemon = True
receive_thread.start()

root.mainloop()
