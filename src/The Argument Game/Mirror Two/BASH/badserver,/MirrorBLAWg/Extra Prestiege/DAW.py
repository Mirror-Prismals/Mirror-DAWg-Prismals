import tkinter as tk
import socket

def on_return(event):
    print("Return")

    # Send "Return" message back to the client
    client_socket.sendall(b"Return\n")

def start_server():
    global server_socket, client_socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("127.0.0.1", 5555))  # Bind to localhost on port 5555
    server_socket.listen(1)  # Listen for one connection
    print("Server is listening on port 5555")

    client_socket, _ = server_socket.accept()
    print("Client connected")

root = tk.Tk()
root.title("Text Editor")

text_editor = tk.Text(root, wrap="word", width=80, height=24)
text_editor.pack(expand=True, fill='both')

text_editor.bind("<Return>", on_return)

start_server()

root.mainloop()
