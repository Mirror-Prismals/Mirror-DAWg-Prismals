import tkinter as tk

def unlock_server(event=None):
    global is_unlocked
    password = password_entry.get()
    if password.lower() == "mira":
        is_unlocked = True
        server_root.configure(bg="#184f51")  # Change background to cyan
        password_entry.delete(0, tk.END)  # Clear the password entry
        password_entry.config(state=tk.DISABLED)  # Disable the password entry
        password_label.config(text="Server Unlocked", fg="green")  # Update the password label
    elif password.lower() == "mia" and is_unlocked:
        server_root.configure(bg="green")  # Change background to green
        password_entry.delete(0, tk.END)  # Clear the password entry
        password_label.config(text="Background Changed", fg="green")  # Update the password label

def on_key_press(event):
    char = event.char
    if char and char.isprintable():  # Check if the character is printable
        text = text_editor.get("1.0", "end-1c")  # Get all text from the text editor
        if text.strip() == "..;;[[//'']]":  # Check if the text is "..;;[[//'']]"
            ask_for_clarification()
        else:
            text_editor.insert(tk.END, char)  # Insert the printable character into the text editor
            if is_unlocked:
                console_text.insert(tk.END, char)  # Insert the printable character into the fake console
                console_text.see(tk.END)  # Scroll to the end of the fake console

def ask_for_clarification():
    # Pop up a message box to ask for clarification
    clarification = tk.messagebox.askyesno("Clarification", "Do you need clarification?")

    if clarification:
        # Handle the player's clarification here
        pass
    else:
        text_editor.insert(tk.END, "\n")  # Add a newline character to move to the next line

def main():
    global server_root, password_entry, password_label, is_unlocked, console_text

    server_root = tk.Tk()
    server_root.title("Mock Server")
    server_root.attributes('-fullscreen', True)  # Set window to nearly fullscreen

    is_unlocked = False

    bad_server_label = tk.Label(server_root, text="Bad Server", bg="#a54234", fg="red", font=("Courier", 24))
    bad_server_label.pack(expand=True, fill="both")

    password_label = tk.Label(server_root, text="Enter Password:", bg="#184f51", fg="white")
    password_label.pack()
    password_entry = tk.Entry(server_root, show="*")
    password_entry.pack()
    password_entry.focus_set()  # Set focus to password entry widget
    password_entry.bind("<Return>", unlock_server)  # Bind Return key to unlock_server function

    console_root = tk.Toplevel()
    console_root.title("Console")
    console_root.geometry("800x600")  # Set window size
    console_root.minsize(400, 300)  # Set minimum size
    console_root.configure(bg="#a54234")

    console_text = tk.Text(console_root)
    console_text.pack(expand=True, fill='both')

    global text_editor
    text_editor = tk.Text(server_root)
    text_editor.pack(expand=True, fill='both')
    text_editor.bind("<Key>", on_key_press)  # Listen for keystrokes

if __name__ == "__main__":
    main()
    tk.mainloop()
