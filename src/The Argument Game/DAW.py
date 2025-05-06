import tkinter as tk

def start_server():
    global server_root
    server_root = tk.Toplevel()
    server_root.title("Mock Server")
    server_root.attributes('-fullscreen', True)  # Set window to nearly fullscreen

    def unlock_server(event):
        password = password_entry.get()
        if password.lower() == "mira":
            server_root.configure(bg="#184f51")  # Change background to cyan
            password_entry.delete(0, tk.END)  # Clear the password entry
            password_entry.unbind("<Return>")  # Unbind the Return key
            password_entry.config(state=tk.DISABLED)  # Disable the password entry
            password_label.config(text="Server Unlocked", fg="green")  # Update the password label

    bad_server_label = tk.Label(server_root, text="Bad Server", bg="#a54234", fg="red", font=("Courier", 24))
    bad_server_label.pack(expand=True, fill="both")

    password_label = tk.Label(server_root, text="Enter Password:", bg="#184f51", fg="white")
    password_label.pack()
    password_entry = tk.Entry(server_root, show="*")
    password_entry.pack()
    password_entry.focus_set()  # Set focus to password entry widget
    password_entry.bind("<Return>", unlock_server)  # Bind Return key to unlock_server function

def main():
    start_server()

if __name__ == "__main__":
    main()
    tk.mainloop()
