import tkinter as tk
from tkinter import messagebox

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
    elif password.lower() == "mmiiaa ##!!444444666666":
        server_root.destroy()  # Destroy the entire window

def switch_to_single_space_window():
    global is_single_space_window_open
    if not is_single_space_window_open:
        is_single_space_window_open = True
        single_space_window = tk.Toplevel()
        single_space_window.title("Single Space Window")
        single_space_window.geometry("400x300")
        single_space_window.mainloop()

def on_key_press(event):
    char = event.char
    if char and char.isprintable():  # Check if the character is printable
        text = event.widget.get("1.0", "end-1c")  # Get all text from the widget
        if text.strip() == "..;;[[//'']]":  # Check if the text is "..;;[[//'']]"
            switch_to_single_space_window()

def open_password_entry():
    global password_root, password_entry, password_label
    password_root = tk.Toplevel()  # Create a separate window for the password
    password_root.title("Password")
    password_root.geometry("200x100")
    password_root.configure(bg="blue")

    password_label = tk.Label(password_root, text="Enter Password:", bg="blue", fg="white")
    password_label.pack()
    password_entry = tk.Entry(password_root, show="*")
    password_entry.pack()
    password_entry.focus_set()  # Set focus to password entry widget
    password_entry.bind("<Return>", unlock_server)  # Bind Return key to unlock_server function

def process_console_input(event):
    input_text = console_text.get("end-2l", "end-1c")  # Get the last line of input
    console_text.insert(tk.END, f"\n{input_text}")
    console_text.see(tk.END)  # Scroll to the end
    return "break"  # Prevent the default newline behavior

def animate_dots():
    current_text = console_text.get("1.0", "end-1c")  # Get all text
    if current_text.endswith("..."):
        new_text = current_text[:-3]
    else:
        new_text = current_text + "."
    console_text.delete("1.0", tk.END)  # Clear all text
    console_text.insert(tk.END, new_text)  # Insert the new text
    console_text.see(tk.END)  # Scroll to the end
    console_text.after(500, animate_dots)  # Repeat after 500ms

def open_console_window():
    console_window = tk.Toplevel()
    console_window.title("Console")
    console_window.geometry("600x400")
    console_window.configure(bg="black")
    
    global console_text
    console_text = tk.Text(console_window, bg="black", fg="white", insertbackground="white")
    console_text.pack(expand=True, fill="both")
    
    console_text.bind("<Return>", process_console_input)  # Bind the Return key to process input
    console_text.insert(tk.END, "Server is listening on port 5555")

    console_window.lift()  # Lift the console window on top
    console_window.attributes('-topmost', True)  # Make the console window always on top

    animate_dots()  # Start animating the dots
    console_window.mainloop()

def open_red_console_window():
    red_console_window = tk.Toplevel()
    red_console_window.title("Password Entry")
    red_console_window.geometry("600x400")
    red_console_window.configure(bg="#FF0000")  # Red background
    
    global red_console_text
    red_console_text = tk.Text(red_console_window, bg="#FF0000", fg="white", insertbackground="white")
    red_console_text.pack(expand=True, fill="both")
    
    red_console_text.bind("<Return>", unlock_server)  # Bind the Return key to unlock_server function
    red_console_text.insert(tk.END, "Enter password 'mira' to unlock the server.")

def main():
    global server_root, is_unlocked, is_single_space_window_open

    server_root = tk.Tk()
    server_root.title("Mock Server")
    server_root.attributes('-fullscreen', True)  # Set window to nearly fullscreen

    is_unlocked = False
    is_single_space_window_open = False

    bad_server_label = tk.Label(server_root, text="Bad Server", bg="#a54234", fg="red", font=("Courier", 24))
    bad_server_label.pack(expand=True, fill="both")

    open_password_entry()  # Open the password entry popup window
    open_console_window()  # Open the console window
    open_red_console_window()  # Open the red console window for password entry

    server_root.bind("<Key>", on_key_press)  # Listen for keystrokes

if __name__ == "__main__":
    main()
    tk.mainloop()
